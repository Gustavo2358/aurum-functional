#!/usr/bin/env python3
"""Conservative libclang restrictions; no memory/ownership inference engine."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
from clang import cindex as C

ROOT = Path(__file__).resolve().parents[1]
PURE_LIBC = {'strcmp', 'strncmp', 'strlen', 'memcmp', 'strchr'}
LOCAL_WRITES = {'strcpy', 'memcpy', 'memset'}
BRANCHES = {'IF_STMT', 'SWITCH_STMT', 'CASE_STMT', 'DEFAULT_STMT',
            'FOR_STMT', 'WHILE_STMT', 'DO_STMT', 'CONDITIONAL_OPERATOR'}
NO_ESCAPE = {'Continuation', 'Edit', 'EditValue', 'Step', 'Result'}

def descendants(node):
    for child in node.get_children():
        yield child
        yield from descendants(child)

def at(node):
    return f'{node.location.file}:{node.location.line}:{node.location.column}'

def pointer_issues(typ, trail='', seen=None, ban_all=False):
    seen = set() if seen is None else seen
    typ = typ.get_canonical()
    key = (typ.kind.name, typ.spelling)
    if key in seen:
        return []
    seen = seen | {key}
    if typ.kind == C.TypeKind.POINTER:
        p = typ.get_pointee()
        issues = []
        if ban_all or not p.is_const_qualified():
            issues.append(trail + ': ' + typ.spelling)
        return issues + pointer_issues(p, trail + '*', seen, ban_all)
    if typ.kind in (C.TypeKind.CONSTANTARRAY, C.TypeKind.INCOMPLETEARRAY):
        return pointer_issues(typ.element_type, trail + '[]', seen, ban_all)
    if typ.kind == C.TypeKind.RECORD:
        return [issue for field in typ.get_fields()
                for issue in pointer_issues(field.type, trail + '.' + field.spelling, seen, ban_all)]
    return []

def local_destination(expr):
    """Accept direct fields/arrays of local value objects, never pointer aliases."""
    nodes = [expr, *descendants(expr)]
    refs = [n.referenced for n in nodes if n.kind == C.CursorKind.DECL_REF_EXPR]
    if not refs:
        return False
    # Other references may index a local array; each must be a local value too.
    for ref in refs:
        if ref is None or ref.kind != C.CursorKind.VAR_DECL:
            return False
        if ref.semantic_parent.kind != C.CursorKind.FUNCTION_DECL:
            return False
        if ref.storage_class in (C.StorageClass.STATIC, C.StorageClass.EXTERN):
            return False
        if ref.type.get_canonical().kind == C.TypeKind.POINTER or pointer_issues(ref.type, ban_all=True):
            return False
    # Function calls can synthesize destinations, so never admit those.
    return not any(n.kind in (C.CursorKind.CALL_EXPR, C.CursorKind.CSTYLE_CAST_EXPR) for n in nodes)

def parse(path, include):
    # clang's package ships a matching parser; system compiler supplies libc headers.
    resource = subprocess.run(['cc', '-print-file-name=include'], capture_output=True,
                              text=True, check=True).stdout.strip()
    tu = C.Index.create().parse(str(path), args=['-std=c17', '-I' + str(include),
                                               '-isystem', resource])
    errors = [str(d) for d in tu.diagnostics if d.severity >= C.Diagnostic.Error]
    return tu, errors

def inspect(core_files, shell_files, include):
    violations, inventory, hashes = [], [], {}
    core_names = set()
    parsed = []
    for path, layer in [(p, 'core') for p in core_files] + [(p, 'shell') for p in shell_files]:
        path = path.resolve()
        hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        tu, errors = parse(path, include)
        violations.extend(f'parse: {e}' for e in errors)
        parsed.append((path, layer, tu))
        if layer == 'core':
            for node in descendants(tu.cursor):
                if node.kind == C.CursorKind.FUNCTION_DECL and node.is_definition() and node.location.file and Path(str(node.location.file)).resolve() == path:
                    core_names.add(node.spelling)
    seen_types = set()
    for path, layer, tu in parsed:
        text = path.read_text()
        if layer == 'core' and re.search(r'#\s*include\s*[<"]aurum\.h[>"]', text):
            violations.append(f'{path}: core imports mutable shell ownership header')
        if re.search(r'\b(?:BR|FR|SC)-[A-Z0-9]', text):
            violations.append(f'{path}: oracle/rule identifier embedded in production')
        for node in descendants(tu.cursor):
            if not node.location.file:
                continue
            locpath = Path(str(node.location.file)).resolve()
            own = locpath == path
            if layer == 'core' and node.kind == C.CursorKind.TYPEDEF_DECL and node.spelling in NO_ESCAPE | {'StateView'}:
                if node.spelling not in seen_types:
                    seen_types.add(node.spelling)
                    for problem in pointer_issues(node.underlying_typedef_type, node.spelling, ban_all=node.spelling in NO_ESCAPE):
                        violations.append(f'{at(node)}: API pointer graph {problem}')
            if not own:
                continue
            if layer == 'core':
                if node.kind == C.CursorKind.VAR_DECL:
                    global_storage = node.semantic_parent.kind == C.CursorKind.TRANSLATION_UNIT or node.storage_class in (C.StorageClass.STATIC, C.StorageClass.EXTERN)
                    if global_storage and not node.type.is_const_qualified():
                        violations.append(f'{at(node)}: mutable global/static {node.spelling}')
                    if global_storage:
                        for issue in pointer_issues(node.type, node.spelling):
                            violations.append(f'{at(node)}: mutable global pointer graph {issue}')
                if node.kind == C.CursorKind.DECL_REF_EXPR and node.referenced and node.referenced.kind == C.CursorKind.VAR_DECL:
                    ref = node.referenced
                    if ref.semantic_parent.kind == C.CursorKind.TRANSLATION_UNIT and not ref.type.is_const_qualified():
                        violations.append(f'{at(node)}: mutable external/global read {ref.spelling}')
                if node.kind.name in {'ASM_STMT', 'GCC_ASM_STMT', 'MS_ASM_STMT'}:
                    violations.append(f'{at(node)}: inline assembly')
                if node.kind == C.CursorKind.CSTYLE_CAST_EXPR and node.type.get_canonical().kind == C.TypeKind.POINTER and not node.type.get_pointee().is_const_qualified():
                    violations.append(f'{at(node)}: cast to mutable pointer')
                if node.kind == C.CursorKind.CALL_EXPR:
                    ref = node.referenced
                    if not ref or ref.kind != C.CursorKind.FUNCTION_DECL:
                        violations.append(f'{at(node)}: indirect/unresolved call')
                    elif ref.spelling in LOCAL_WRITES:
                        args = list(node.get_arguments())
                        if not args or not local_destination(args[0]):
                            violations.append(f'{at(node)}: libc write destination not a direct local value')
                    elif ref.spelling not in core_names | PURE_LIBC:
                        violations.append(f'{at(node)}: effect/unapproved dependency {ref.spelling}')
            if node.kind != C.CursorKind.FUNCTION_DECL or not node.is_definition():
                continue
            if layer == 'core':
                for arg in node.get_arguments():
                    for issue in pointer_issues(arg.type, arg.spelling):
                        violations.append(f'{at(arg)}: mutable parameter graph {issue}')
                if node.type.get_result().get_canonical().kind == C.TypeKind.POINTER:
                    for issue in pointer_issues(node.type.get_result(), 'return'):
                        violations.append(f'{at(node)}: mutable return {issue}')
                if node.storage_class != C.StorageClass.STATIC and not node.spelling.startswith('core_'):
                    violations.append(f'{at(node)}: public core symbol missing core_ prefix')
            elif node.spelling.startswith('core_'):
                violations.append(f'{at(node)}: shell defines domain symbol')
            body = list(descendants(node))
            support = node.storage_class == C.StorageClass.STATIC
            inventory.append({'name': node.spelling, 'file': str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
                'line': node.location.line, 'layer': layer,
                'classification': 'SUPPORT' if support else ('DOMAIN' if layer == 'core' else 'TECHNICAL'),
                'contract': 'docs/ARCHITECTURE-DESIGN.md' if layer == 'core' else 'tests/original/subject/CLI.md',
                'review': 'structural classification; semantic traceability requires source review',
                'calls': sorted({n.spelling for n in body if n.kind == C.CursorKind.CALL_EXPR}),
                'decisions': [{'kind': n.kind.name, 'line': n.location.line,
                               'expression': ' '.join(t.spelling for t in n.get_tokens())[:180]}
                              for n in body if n.kind.name in BRANCHES]})
    if not core_files:
        violations.append('no production core source files found')
    return {'status': 'FAIL' if violations else 'PASS', 'violations': sorted(set(violations)),
            'functions': inventory, 'source_sha256': hashes, 'pure_libc_allowlist': sorted(PURE_LIBC),
            'limitations': ['Conservative type/effect restrictions, not a whole-program purity proof.',
                            'Domain placement and contract association require human/source review.']}

def self_test():
    cases = {
        'local_memset': ('extern void *memset(void *,int,unsigned long); int core_ok(void){int a;memset(&a,0,sizeof a);return a;}', True),
        'aliased_memset': ('extern void *memset(void *,int,unsigned long); int core_bad(const int *p){void *q=(void *)p;memset(q,0,4);return 0;}', False),
        'good': ('typedef struct {const int *p;} V; int core_ok(V v){return *v.p;}', True),
        'out_parameter': ('int core_bad(int *p){*p=1;return 1;}', False),
        'nested_pointer': ('typedef struct {int *p;} V; int core_bad(V v){return *v.p;}', False),
        'global': ('int state; int core_bad(void){return ++state;}', False),
        'static_local': ('int core_bad(void){static int state;return ++state;}', False),
        'effect': ('extern void *malloc(unsigned long); int core_bad(void){return malloc(8)!=0;}', False),
        'escape': ('typedef struct {const int *p;} Continuation; int core_ok(void){return 0;}', False),
        'cast': ('int core_bad(const int *p){return *(int *)p;}', False),
        'internal_out': ('static int helper(int *p){return *p;} int core_bad(void){int a=0;return helper(&a);}', False),
        'function_pointer': ('int core_bad(int (*f)(void)){return f();}', False),
    }
    outcomes = []
    with tempfile.TemporaryDirectory(prefix='aurum-architecture-') as d:
        for name, (source, expected) in cases.items():
            p = Path(d) / (name + '.c'); p.write_text(source)
            report = inspect([p], [], Path(d))
            outcomes.append({'name': name, 'expected_accept': expected, 'accepted': report['status'] == 'PASS',
                             'pass': (report['status'] == 'PASS') == expected})
    return outcomes

def main():
    p = argparse.ArgumentParser(); p.add_argument('--self-test', action='store_true')
    p.add_argument('--report', type=Path, default=ROOT/'evidence/purity-architecture.json')
    p.add_argument('--inventory', type=Path, default=ROOT/'evidence/architecture-inventory.json')
    p.add_argument('--core-dir', type=Path, default=ROOT/'subject/src')
    p.add_argument('--shell-dir', type=Path, default=ROOT/'subject/src')
    a = p.parse_args()
    controls = self_test() if a.self_test else []
    core_files = sorted(a.core_dir.glob('core_*.c'))
    shell_files = sorted(p for p in a.shell_dir.glob('*.c') if not p.name.startswith('core_'))
    report = inspect(core_files, shell_files, ROOT/'subject/include')
    report['negative_controls'] = controls
    if any(not c['pass'] for c in controls): report['status'] = 'FAIL'
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(report, indent=2)+'\n')
    a.inventory.write_text(json.dumps({'status': report['status'], 'functions': report['functions'],
                                      'source_sha256': report['source_sha256']}, indent=2)+'\n')
    print(json.dumps({'status': report['status'], 'functions': len(report['functions']),
                      'violations': report['violations'], 'negative_controls': controls}, indent=2))
    return int(report['status'] != 'PASS')
if __name__ == '__main__': raise SystemExit(main())
