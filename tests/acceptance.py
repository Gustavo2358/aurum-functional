#!/usr/bin/env python3
"""Run frozen independent acceptance against a specified CLI and original-ABI library."""
import argparse, os, json, subprocess, sys, hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ORIGINAL=ROOT/'tests/original'

def main():
 p=argparse.ArgumentParser();p.add_argument('--binary',type=Path);p.add_argument('--library',type=Path);p.add_argument('--build-original',action='store_true');p.add_argument('--report-dir',type=Path,default=ROOT/'evidence/harness-acceptance');p.add_argument('--mutations',action='store_true');a=p.parse_args()
 out=a.report_dir.resolve();out.mkdir(parents=True,exist_ok=True)
 records=[]
 def run(name,cmd,env=None):
  r=subprocess.run([str(x) for x in cmd],capture_output=True,env=env,cwd=ORIGINAL,timeout=180)
  (out/(name+'.stdout')).write_bytes(r.stdout);(out/(name+'.stderr')).write_bytes(r.stderr)
  records.append(dict(gate=name,command=[str(x) for x in cmd],exit_code=r.returncode,status='PASS' if r.returncode==0 else 'FAIL'))
  print(name,records[-1]['status'],flush=True)
  return r.returncode==0
 if a.build_original:
  binary=out/'aurum';library=out/'libaurum.so'
  src=sorted((ORIGINAL/'subject/src').glob('*.c'));flags=['cc','-std=c17','-O2','-g','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(ORIGINAL/'subject/include')]
  ok=run('build-cli',flags+src+['-o',binary])
  ok=run('build-library',flags+['-fPIC','-shared']+[x for x in src if x.name!='main.c']+['-o',library]) and ok
  if not ok: return 1
 else:
  if not a.binary or not a.library:p.error('--binary and --library required unless --build-original')
  binary=a.binary.resolve();library=a.library.resolve()
 env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',PYTHONPATH=str(ROOT/'tests/vendor'),AURUM_LIB=str(library),AURUM_BIN=str(binary),AURUM_BINARY=str(binary))
 py=sys.executable
 commands=[('oracle-math',[py,ORIGINAL/'tools/check_oracle_math.py']),('gherkin',[py,ORIGINAL/'harness/run.py','--self-test','--report',out/'gherkin.json']),('protocol',[py,ORIGINAL/'harness/protocol_adversarial.py']),('adversarial',[py,ORIGINAL/'harness/adversarial.py','--binary',binary,'--report',out/'adversarial.json']),('numeric',[py,ORIGINAL/'harness/numeric_properties.py','--report',out/'numeric.json']),('invariants',[py,ORIGINAL/'harness/invariants.py','--report',out/'invariants.json']),('regression',[py,ORIGINAL/'harness/regression_audit.py','--binary',binary,'--report',out/'regression.json'])]
 if a.mutations:commands.append(('mutations',[py,ORIGINAL/'tools/mutations.py','--binary',binary,'--report',out/'mutations.json']))
 for name,cmd in commands:run(name,cmd,env)
 report=dict(status='PASS' if all(x['status']=='PASS' for x in records) else 'FAIL',binary=str(binary),binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),library=str(library),library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),python=sys.version,gates=records)
 (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
 return int(report['status']!='PASS')
if __name__=='__main__':raise SystemExit(main())
