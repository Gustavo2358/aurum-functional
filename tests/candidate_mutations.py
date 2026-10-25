#!/usr/bin/env python3
"""Mutation sensitivity of the replacement against frozen independent goldens.

Only isolated copies of current production sources are edited. A pristine build
must first pass both corpora; a compile error is never counted as a killed mutant.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import sys
import os
from pathlib import Path

from differential import ROOT, compare

VARIANTS = [
    dict(name='credit_equality', file='core_policy.c',
         before='total.value <= available', after='total.value < available'),
    dict(name='credit_omits_fee', file='core_policy.c',
         before='core_money_add(principal,fee)', after='core_money_add(principal,fee-fee)'),
    dict(name='fx_half_down', file='core_money.c',
         before='product.value % 10000 >= 5000', after='product.value % 10000 > 5000'),
    dict(name='capture_state_guard_removed', file='core_policy.c',
         before='return AUTH_STATE;', after='return NONE;'),
    dict(name='review_boundary_exclusive', file='core_policy.c',
         before='score >= config->review_score', after='score > config->review_score'),
    dict(name='velocity_excludes_boundary', file='core_state.c',
         before='history->minute >= state.now-60', after='history->minute > state.now-60'),
    dict(name='accept_truncated_line', file='shell_protocol.c',
         before='return invalid ? -1 : 1;', after='return invalid ? 1 : 1;'),
    dict(name='capture_independent_rounding', file='core_transition.c',
         before='core_cumulative_delta(auth->fee,auth->captured_principal,command.principal,auth->principal)',
         after='core_proportional(auth->fee,command.principal,auth->principal)'),
    dict(name='refund_entire_fee_first', file='core_transition.c',
         before='core_cumulative_delta(capture->fee,capture->refunded_principal,command.principal,capture->principal)',
         after='((Number){true,capture->fee})'),
    dict(name='payment_principal_before_fee', file='core_transition.c',
         before='component == 0', after='component != 0', occurrences=4),
    dict(name='expire_one_minute_early', file='core_transition.c',
         before='auth->expires_at > now', after='auth->expires_at > now+1'),
    dict(name='expiration_descending_ids', file='core_transition.c',
         before='strcmp(auth->id,state.auths[next].id) < 0',
         after='strcmp(auth->id,state.auths[next].id) > 0'),
    dict(name='refund_oldest_first', file='core_transition.c',
         before='a->cycle > b->cycle ? -1 : 1', after='a->cycle < b->cycle ? -1 : 1'),
    dict(name='replay_executes_again', file='core_transition.c',
         before='if (core_command_equal(&key->command,&command)) next.result=key->result;',
         after='if (core_command_equal(&key->command,&command)) break;'),
    dict(name='zero_ledger_entries', file='core_transition.c',
         before='if (!amount) return continuation;', after='if (amount < 0) return continuation;'),
    dict(name='abort_publishes_private_state', file='shell_storage.c',
         before='case CORE_ABORT:\n                if (staged) engine_free(&scratch);',
         after='case CORE_ABORT:\n                if (staged && step.status == CORE_ABORT) { engine_free(e); *e=scratch; staged=false; }\n                if (staged) engine_free(&scratch);'),
    dict(name='equivalent_local_rename', file='core_money.c',
         before='quotient', after='division_quotient', equivalent=True),
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_manifest(root):
    return {str(p.relative_to(root)): digest(p)
            for p in sorted((root/'subject').rglob('*')) if p.is_file()}


def build(root, binary):
    command = ['cc', '-std=c17', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic',
               '-Werror', '-I'+str(root/'subject/include')]
    command += [str(p) for p in sorted((root/'subject/src').glob('*.c'))]
    command += ['-o', str(binary)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=90)
    return dict(command=command, exit_code=result.returncode,
                stdout=result.stdout, stderr=result.stderr)


def check(binary, corpora):
    failures = []
    counterexample = None
    for name, cases, golden in corpora:
        differences, observations = compare(binary, cases, golden)
        failures.extend(dict(corpus=name, **d) for d in differences)
        if differences and counterexample is None:
            case = differences[0]['case']
            counterexample = dict(corpus=name, case=case,
                                  expected=golden['observations'][case],
                                  actual=observations[case])
    return failures, counterexample


def state_check(root):
    library = root/'libaurum.so'
    command = ['cc', '-std=c17', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic',
               '-Werror', '-fPIC', '-shared', '-I'+str(root/'subject/include')]
    command += [str(p) for p in sorted((root/'subject/src').glob('*.c')) if p.name != 'main.c']
    command += [str(ROOT/'tests/legacy_adapter.c'), '-o', str(library)]
    build_result = subprocess.run(command, capture_output=True, text=True, timeout=90)
    if build_result.returncode:
        return dict(status='BUILD_ERROR', stderr=build_result.stderr)
    target = root/'state.json'
    result = subprocess.run([sys.executable, str(ROOT/'tests/state_differential.py'),
                             '--library', str(library), '--report', str(target)],
                            capture_output=True, text=True, timeout=180)
    if result.returncode not in (0, 1) or not target.exists():
        return dict(status='RUNNER_ERROR', exit_code=result.returncode,
                    stdout=result.stdout, stderr=result.stderr)
    details = json.loads(target.read_text())
    return {k: details[k] for k in ('status', 'commands', 'failures', 'first_counterexample') if k in details}


def numeric_check(root):
    target = root/'numeric.json'
    result = subprocess.run([sys.executable, str(ROOT/'tests/original/harness/numeric_properties.py'),
                             '--report', str(target)], capture_output=True, text=True, timeout=90,
                            env=dict(os.environ, AURUM_LIB=str(root/'libaurum.so'), PYTHONDONTWRITEBYTECODE='1'))
    if result.returncode not in (0, 1) or not target.exists():
        return dict(status='RUNNER_ERROR', exit_code=result.returncode,
                    stdout=result.stdout, stderr=result.stderr)
    return json.loads(target.read_text())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--report', type=Path, default=ROOT/'evidence/review-mutations.json')
    args = parser.parse_args()
    corpora = []
    frozen = {}
    for name, corpus_file, golden_file in [
        ('base', 'cases.json', 'golden.json'),
        ('generated', 'generated.json', 'golden-generated.json'),
    ]:
        corpus_path = ROOT/'tests/corpus'/corpus_file
        golden_path = ROOT/'tests/corpus'/golden_file
        corpus = json.loads(corpus_path.read_text())
        golden = json.loads(golden_path.read_text())
        if golden['corpus_sha256'] != digest(corpus_path):
            raise RuntimeError('Frozen corpus hash mismatch: '+corpus_file)
        frozen[corpus_file], frozen[golden_file] = digest(corpus_path), digest(golden_path)
        corpora.append((name, corpus['cases'], golden))
    manifest = source_manifest(ROOT)
    report = dict(status='FAIL', source_sha256=manifest, frozen_sha256=frozen,
                  corpora={name: len(cases) for name, cases, _ in corpora}, mutants=[])
    with tempfile.TemporaryDirectory(prefix='aurum-candidate-mutations-') as directory:
        root = Path(directory)
        pristine = root/'pristine'
        shutil.copytree(ROOT/'subject', pristine/'subject')
        baseline = build(pristine, pristine/'aurum')
        report['baseline'] = baseline
        if baseline['exit_code'] == 0:
            failures, example = check(pristine/'aurum', corpora)
            baseline.update(failures=failures, status='FAIL' if failures else 'PASS')
            if example:
                baseline['counterexample'] = example
            if not failures:
                baseline['state'] = state_check(pristine)
                if baseline['state']['status'] != 'PASS':
                    baseline['status'] = 'FAIL'
                else:
                    baseline['numeric'] = numeric_check(pristine)
                    if baseline['numeric']['status'] != 'PASS':
                        baseline['status'] = 'FAIL'
            if baseline.get('status') == 'PASS':
                for index, variant in enumerate(VARIANTS):
                    mutant = root/str(index)
                    shutil.copytree(pristine/'subject', mutant/'subject')
                    source = mutant/'subject/src'/variant['file']
                    before = source.read_text()
                    count = before.count(variant['before'])
                    record = dict(name=variant['name'], file='subject/src/'+variant['file'],
                                  replacement_count=count, source_before_sha256=digest(source))
                    expected = count if variant.get('equivalent') else variant.get('occurrences', 1)
                    if not count or count != expected:
                        record.update(status='ANCHOR_ERROR', expected_count=expected)
                    else:
                        source.write_text(before.replace(variant['before'], variant['after']))
                        record['source_after_sha256'] = digest(source)
                        record['build'] = build(mutant, mutant/'aurum')
                        if record['build']['exit_code']:
                            record['status'] = 'BUILD_ERROR'
                        else:
                            failures, example = check(mutant/'aurum', corpora)
                            record['failures'] = failures
                            if variant.get('equivalent'):
                                record['status'] = 'EQUIVALENT_FAIL' if failures else 'EQUIVALENT_PASS'
                            else:
                                record['status'] = 'KILLED' if failures else 'SURVIVED'
                            if not failures:
                                record['state'] = state_check(mutant)
                                state_status = record['state']['status']
                                if state_status == 'FAIL':
                                    record['status'] = 'EQUIVALENT_FAIL' if variant.get('equivalent') else 'KILLED'
                                elif state_status != 'PASS':
                                    record['status'] = state_status
                                else:
                                    record['numeric'] = numeric_check(mutant)
                                    numeric_status = record['numeric']['status']
                                    if numeric_status == 'FAIL':
                                        record['status'] = 'EQUIVALENT_FAIL' if variant.get('equivalent') else 'KILLED'
                                    elif numeric_status != 'PASS':
                                        record['status'] = numeric_status
                            if example:
                                record['counterexample'] = example
                    report['mutants'].append(record)
                    print(record['name'], record['status'], flush=True)
        report['source_unchanged'] = source_manifest(ROOT) == manifest
        report['frozen_unchanged'] = all(digest(ROOT/'tests/corpus'/name) == value
                                       for name, value in frozen.items())
        if (baseline.get('status') == 'PASS' and report['mutants']
                and all(r['status'] in ('KILLED', 'EQUIVALENT_PASS') for r in report['mutants'])
                and report['source_unchanged'] and report['frozen_unchanged']):
            report['status'] = 'PASS'
    report['killed'] = sum(r['status'] == 'KILLED' for r in report['mutants'])
    report['survived'] = sum(r['status'] == 'SURVIVED' for r in report['mutants'])
    report['equivalent'] = sum(r['status'] == 'EQUIVALENT_PASS' for r in report['mutants'])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key: report[key] for key in ('status', 'killed', 'survived', 'equivalent')}))
    return int(report['status'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
