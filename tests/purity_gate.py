#!/usr/bin/env python3
"""Build and run direct core-only immutability and deterministic replay checks."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from clang import cindex as C
from architecture_gate import parse, descendants
ROOT = Path(__file__).resolve().parents[1]
TYPES = ['Config', 'Account', 'Card', 'Merchant', 'Command', 'Result', 'Authorization',
         'Capture', 'Lot', 'Invoice', 'History', 'Entry', 'Cached', 'Payment', 'Refund',
         'Reward', 'Price', 'Projection']

def fingerprint_header(path):
    tu, errors = parse(ROOT/'subject/include/core.h', ROOT/'subject/include')
    if errors: raise RuntimeError('\n'.join(errors))
    typedefs = {n.spelling: n.underlying_typedef_type for n in descendants(tu.cursor)
                if n.kind == C.CursorKind.TYPEDEF_DECL and n.spelling in TYPES}
    lines = ['/* Generated field-wise fingerprints. They are not behavioral expectations. */']
    def emit(typ, expr):
        typ = typ.get_canonical()
        if typ.kind == C.TypeKind.RECORD:
            if typ.get_declaration().kind == C.CursorKind.UNION_DECL:
                raise RuntimeError('Fingerprint requires an explicit active union discriminator')
            for f in typ.get_fields(): emit(f.type, expr + '.' + f.spelling)
        elif typ.kind == C.TypeKind.CONSTANTARRAY:
            if typ.element_type.get_canonical().kind in (C.TypeKind.CHAR_S, C.TypeKind.CHAR_U):
                lines.append(f' h=hash_string(h, {expr}, sizeof({expr}));')
            else:
                raise RuntimeError('Non-string array needs element-wise fingerprint')
        elif typ.kind == C.TypeKind.POINTER:
            raise RuntimeError('Fingerprint cannot treat pointers as semantic values')
        else:
            lines.append(f' h=hash_bytes(h, &({expr}), sizeof({expr}));')
    for name in TYPES:
        lines.append(f'static uint64_t hash_{name}(uint64_t h, const {name} *v) {{')
        emit(typedefs[name], '(*v)')
        lines.append(' return h;\n}')
    path.write_text('\n'.join(lines)+'\n')

def main():
    p=argparse.ArgumentParser();p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--report-dir',type=Path,default=ROOT/'evidence/purity-runtime');a=p.parse_args()
    out=a.report_dir.resolve();out.mkdir(parents=True,exist_ok=True)
    fingerprint_header(out/'purity_semantic.h')
    sources=sorted((ROOT/'subject/src').glob('core_*.c'))
    command=['cc','-std=c17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
             '-I'+str(ROOT/'subject/include'),'-I'+str(out)]
    if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    command += [str(ROOT/'tests/purity_runtime.c')]+[str(p) for p in sources]+['-o',str(out/'purity-runtime')]
    build=subprocess.run(command,capture_output=True,text=True)
    (out/'build.stdout').write_text(build.stdout);(out/'build.stderr').write_text(build.stderr)
    report={'status':'FAIL','build_exit':build.returncode,'command':command,
            'source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                             for p in sources+[ROOT/'subject/include/core.h',ROOT/'tests/purity_runtime.c']}}
    if build.returncode == 0:
        run=subprocess.run([str(out/'purity-runtime')],capture_output=True,text=True,timeout=60)
        (out/'run.stdout').write_text(run.stdout);(out/'run.stderr').write_text(run.stderr)
        report.update(exit_code=run.returncode, status='PASS' if run.returncode==0 else 'FAIL')
        if run.returncode==0:report['checks']=json.loads(run.stdout)
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));return int(report['status']!='PASS')
if __name__=='__main__':raise SystemExit(main())
