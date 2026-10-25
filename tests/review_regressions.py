#!/usr/bin/env python3
"""Review counterexamples: late failures preserve state and legacy Result fields.

All prestates are reached through the public parser and command executor. The
fixed expected errors are authored from contracts, before observing candidate
responses. Original execution additionally checks complete record equivalence.
"""
import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
AUTH = 'AUTHORIZE request_id=A account_id=A1 card_id=C1 merchant_id=M1 amount=10000'
CAPTURE = 'CAPTURE request_id=X auth_id=A principal=10000'
CLOSE = ['TICK now=172800', 'CLOSE request_id=C account_id=A1 cycle=3']
CASES = [
    dict(name='assess_second_ledger_entry_capacity', config='ledger_capacity=7',
         setup=[AUTH, CAPTURE, *CLOSE, 'TICK now=188640'],
         command='ASSESS_LATE_FEE request_id=L invoice_id=I1', invoice_id='I1', principal=0),
    dict(name='payment_record_capacity', config='payments_capacity=1',
         setup=[AUTH, CAPTURE, *CLOSE, 'PAY request_id=P1 account_id=A1 amount=1'],
         command='PAY request_id=P2 account_id=A1 amount=1', invoice_id='', principal=1),
    dict(name='assess_cache_capacity', config='keys_capacity=3',
         setup=[AUTH, CAPTURE, *CLOSE, 'TICK now=188640'],
         command='ASSESS_LATE_FEE request_id=L invoice_id=I1', invoice_id='', principal=0),
    dict(name='payment_cache_capacity', config='keys_capacity=3',
         setup=[AUTH, CAPTURE, *CLOSE],
         command='PAY request_id=P account_id=A1 amount=1', invoice_id='', principal=0),
    dict(name='cancel_second_ledger_entry_capacity', config='ledger_capacity=3',
         setup=[AUTH], command='CANCEL request_id=C auth_id=A', invoice_id='', principal=0),
    dict(name='tick_second_authorization_capacity', config='ledger_capacity=7',
         setup=[AUTH.replace('request_id=A ', 'request_id=Z '), AUTH],
         command='TICK now=146040', invoice_id='', principal=0),
    dict(name='refund_second_ledger_entry_capacity', config='ledger_capacity=7',
         setup=[AUTH, CAPTURE], command='REFUND request_id=F capture_id=X principal=1',
         invoice_id='', principal=0),
]


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def worker(library):
    os.environ['AURUM_LIB'] = str(library.resolve())
    sys.path.insert(0, str(ROOT/'tests/original/harness'))
    sys.dont_write_bytecode = True
    import bindings as B
    from probes import snapshot
    libc = C.CDLL(None)
    libc.fopen.argtypes = [C.c_char_p, C.c_char_p]
    libc.fopen.restype = C.c_void_p
    libc.fclose.argtypes = [C.c_void_p]
    B.lib.seed_load.argtypes = [C.c_void_p, C.POINTER(B.Engine)]
    B.lib.seed_load.restype = C.c_bool
    results = []
    for case in CASES:
        e = B.Engine()
        B.engine_init(C.byref(e))
        try:
            with tempfile.TemporaryDirectory(prefix='aurum-review-seed-') as directory:
                path = Path(directory)/'seed.txt'
                path.write_text('CLOCK now=144600\nCONFIG '+case['config']+
                                '\nACCOUNT id=A1\nCARD id=C1 account_id=A1\nMERCHANT id=M1\n')
                stream = libc.fopen(os.fsencode(path), b'r')
                assert stream
                try:
                    assert B.lib.seed_load(stream, C.byref(e)), case['name']
                finally:
                    libc.fclose(stream)
            for line in case['setup']:
                command = B.Command()
                assert B.parse_command(line.encode(), C.byref(command)) == 1
                result = B.execute(C.byref(e), C.byref(command))
                assert result.decision in (B.OK, B.APPROVED), (case['name'], line, B.decode(result))
            before = snapshot(e)
            command = B.Command()
            assert B.parse_command(case['command'].encode(), C.byref(command)) == 1
            result = B.execute(C.byref(e), C.byref(command))
            response = B.decode(result)
            after = snapshot(e)
            failures = []
            if result.decision != B.ERROR or result.reason != B.CAPACITY:
                failures.append('expected ERROR/CAPACITY')
            if before != after:
                failures.append('full published snapshot changed')
            if response['invoice_id'] != case['invoice_id'] or result.principal != case['principal']:
                failures.append('legacy error Result fields differ')
            results.append(dict(name=case['name'], status='FAIL' if failures else 'PASS',
                                failures=failures, result=response, before_sha256=digest(before),
                                after_sha256=digest(after)))
        finally:
            B.engine_free(C.byref(e))
    return results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--worker', type=Path)
    parser.add_argument('--candidate', type=Path, default=ROOT/'build/libaurum.so')
    parser.add_argument('--original', type=Path)
    parser.add_argument('--report', type=Path, default=ROOT/'evidence/review-regressions.json')
    args = parser.parse_args()
    if args.worker:
        print(json.dumps(worker(args.worker)))
        return 0
    oracle_build = None
    if args.original is None:
        verified = subprocess.run([sys.executable, str(ROOT/'tests/verify_frozen.py')],
                                  capture_output=True, text=True)
        if verified.returncode:
            raise RuntimeError('Frozen source verification failed: '+verified.stdout+verified.stderr)
        args.original = ROOT/'build/oracle/libaurum.so'
        args.original.parent.mkdir(parents=True, exist_ok=True)
        sources = [p for p in sorted((ROOT/'tests/original/subject/src').glob('*.c')) if p.name != 'main.c']
        command = ['cc', '-std=c17', '-O0', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                   '-fPIC', '-shared', '-I'+str(ROOT/'tests/original/subject/include')]
        command += [str(p) for p in sources]+['-o', str(args.original)]
        built = subprocess.run(command, capture_output=True, text=True, timeout=90)
        oracle_build = dict(command=command, exit_code=built.returncode,
                            stdout=built.stdout, stderr=built.stderr,
                            source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                           for p in sources})
        if built.returncode:
            raise RuntimeError('Isolated original build failed: '+built.stderr)
    records = {}
    for name, library in [('original', args.original), ('candidate', args.candidate)]:
        process = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--worker', str(library)],
                                 capture_output=True, text=True, timeout=60)
        if process.returncode:
            records[name] = dict(status='RUNNER_ERROR', stderr=process.stderr)
        else:
            cases = json.loads(process.stdout)
            records[name] = dict(status='PASS' if all(r['status'] == 'PASS' for r in cases) else 'FAIL',
                                 library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(), cases=cases)
    equivalent = (records.get('original', {}).get('cases') == records.get('candidate', {}).get('cases')
                  and records.get('original', {}).get('cases') is not None)
    report = dict(status='PASS' if equivalent and all(r['status'] == 'PASS' for r in records.values()) else 'FAIL',
                  cases=CASES, complete_results_and_snapshots_equal=equivalent, runs=records)
    if oracle_build:
        report['oracle_build'] = oracle_build
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'status': report['status'], 'cases': len(CASES), 'equivalent': equivalent}))
    return int(report['status'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
