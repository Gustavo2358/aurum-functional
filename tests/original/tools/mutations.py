#!/usr/bin/env python3
"""Compile semantic mutants in temporary copies; fixed external tests classify each."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

# The original source and expectations are never modified by this driver.
MUTANTS = [
    dict(name='credit_equality', file='policy.c',
         before='return total <= available ? NONE : CREDIT_LIMIT;',
         after='return total < available ? NONE : CREDIT_LIMIT;', group='credit_boundaries'),
    dict(name='credit_omits_fee', file='policy.c',
         before='money_add(p,f,&total)', after='money_add(p,f-f,&total)', group='credit_boundaries'),
    dict(name='capture_state_guard_removed', file='lifecycle.c',
         before='return AUTH_STATE;', after='return NONE;', group='expiration'),
    dict(name='capture_independent_rounding', file='lifecycle.c',
         before='cumulative_delta(auth->fee, auth->captured_principal, c->principal,\n                          auth->principal, &fee)',
         after='proportional(auth->fee, c->principal, auth->principal, &fee)', group='capture_fragments'),
    dict(name='refund_entire_fee_first', file='lifecycle.c',
         before='cumulative_delta(capture->fee, capture->refunded_principal, c->principal,\n                          capture->principal, &fee)',
         after='(fee = capture->fee, true)', group='refund_fragments'),
    dict(name='refund_restores_reward_cap', file='lifecycle.c',
         before='    Refund refund = {0};',
         after='    for (size_t i = 0; i < e->rewards_len; ++i)\n'
               '        if (strcmp(e->rewards[i].capture_id, capture->id) == 0) e->rewards[i].granted -= points;\n'
               '    Refund refund = {0};', group='reward_cap'),
    dict(name='payment_late_last', file='billing.c',
         before='!allocate_late(e,event,a->id,&remaining) || !allocate_lots(e,event,a->id,&remaining)',
         after='!allocate_lots(e,event,a->id,&remaining) || !allocate_late(e,event,a->id,&remaining)', group='payment_priority'),
    dict(name='payment_principal_before_fee', file='billing.c',
         before='component==0', after='component!=0', occurrences=3, group='payment_priority'),
    dict(name='replay_executes_again', file='engine.c',
         before='if (command_equal(&key->command,c)) return key->result;',
         after='if (command_equal(&key->command,c)) break;', group='replay'),
    dict(name='expire_one_minute_early', file='lifecycle.c',
         before='a->expires_at <= c->now', after='a->expires_at <= c->now + 1',
         occurrences=2, group='expiration'),
    dict(name='accept_truncated_line', file='protocol.c',
         before='return invalid ? -1 : 1;', after='return invalid ? 1 : 1;', group='line_boundary'),
]


def run_tests(binary, group=None):
    command=[sys.executable,str(ROOT/'harness/adversarial.py'),'--binary',str(binary)]
    if group:
        command += ['--group',group]
    process=subprocess.run(command,capture_output=True,text=True,timeout=90)
    try:
        report=json.loads(process.stdout)
    except json.JSONDecodeError:
        report={'status':'RUNNER_ERROR','output':process.stdout[-1200:],'error':process.stderr[-1200:]}
    return process.returncode,report


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--binary',type=Path,default=ROOT/'build/aurum')
    parser.add_argument('--report',type=Path,default=ROOT/'artifacts/mutations.json')
    parser.add_argument('--only',action='append',choices=[m['name'] for m in MUTANTS])
    args=parser.parse_args()
    compiler=shlex.split(os.environ.get('CC','cc'))
    flags=['-std=c17','-O0','-g','-Wall','-Wextra','-Wpedantic','-Werror']
    version=subprocess.run(compiler+['--version'],capture_output=True,text=True,check=True).stdout.splitlines()[0]
    baseline_code,baseline=run_tests(args.binary)
    results=[]
    if baseline_code == 0:
        selected=[m for m in MUTANTS if not args.only or m['name'] in args.only]
        variants=selected+[dict(name='equivalent_local_rename',file='lifecycle.c',before='principal_cash',after='principal_remainder',equivalent=True)]
        for mutant in variants:
            record={'name':mutant['name'],'file':'subject/src/'+mutant['file']}
            with tempfile.TemporaryDirectory(prefix='aurum-mutant-') as directory:
                root=Path(directory)
                shutil.copytree(ROOT/'subject/src',root/'src')
                shutil.copytree(ROOT/'subject/include',root/'include')
                source=root/'src'/mutant['file']
                original=source.read_text()
                count=original.count(mutant['before'])
                required=mutant.get('occurrences',1)
                if mutant.get('equivalent'):
                    required=count
                record['source_sha256']=hashlib.sha256(original.encode()).hexdigest()
                record['replacement_count']=count
                if count == 0 or count != required:
                    record.update(status='ANCHOR_ERROR',expected_count=required)
                    results.append(record)
                    continue
                source.write_text(original.replace(mutant['before'],mutant['after']))
                binary=root/'aurum'
                build=subprocess.run(compiler+flags+['-I'+str(root/'include')]+[str(p) for p in sorted((root/'src').glob('*.c'))]+['-o',str(binary)],
                                     capture_output=True,text=True,timeout=90)
                if build.returncode:
                    record.update(status='BUILD_ERROR',error=build.stderr[-1600:])
                else:
                    code,test=run_tests(binary,mutant.get('group'))
                    failed=[r for r in test.get('groups',[]) if r['status']=='FAIL']
                    if mutant.get('equivalent'):
                        record['status']='EQUIVALENT_PASS' if code==0 else 'EQUIVALENT_FAIL'
                        record['equivalence']='Consistent renaming of a function-local variable; no type, control or data changes.'
                    elif test.get('status')=='RUNNER_ERROR':
                        record['status']='RUNNER_ERROR'
                    else:
                        record['status']='KILLED' if code!=0 else 'SURVIVED'
                    record['tests']=test.get('groups',[])
                    record['responses']=test.get('responses',0)
                    if failed:
                        record['counterexample']=failed[0]['error']
                results.append(record)
                print(f'{record["name"]}: {record["status"]}',flush=True)
    status='PASS' if baseline_code==0 and results and all(r['status'] in ('KILLED','EQUIVALENT_PASS') for r in results) else 'FAIL'
    report={'status':status,'compiler':version,'flags':flags,'baseline':baseline,
            'mutants':results,'killed':sum(r['status']=='KILLED' for r in results),
            'survived':sum(r['status']=='SURVIVED' for r in results),
            'equivalent':sum(r['status']=='EQUIVALENT_PASS' for r in results)}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('status','killed','survived','equivalent')},indent=2))
    return 0 if status=='PASS' else 1

if __name__=='__main__':
    raise SystemExit(main())
