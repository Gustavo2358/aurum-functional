#!/usr/bin/env python3
"""Byte-exact public contract comparison. Golden generation is an explicit separate command.

No production imports; candidates see only seed/commands. Every run records exit,
stdout/stderr bytes and the entire sandbox file contents after execution.
"""
import argparse,base64,hashlib,json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORPUS=ROOT/'tests/corpus/cases.json'
GOLDEN=ROOT/'tests/corpus/golden.json'
def b64(b):return base64.b64encode(b).decode('ascii')
def unb64(s):return base64.b64decode(s)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def observe(binary,case):
 with tempfile.TemporaryDirectory(prefix='aurum-differential-') as directory:
  wd=Path(directory)
  for name,data in case['files'].items():
   p=wd/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(unb64(data))
  before={str(p.relative_to(wd)):b64(p.read_bytes()) for p in sorted(wd.rglob('*')) if p.is_file()}
  env=dict(os.environ,LC_ALL='C',TZ='UTC')
  env.update(case.get('env',{}))
  try:
   p=subprocess.run([str(binary),*case['args']],input=unb64(case.get('stdin','')),capture_output=True,cwd=wd,env=env,timeout=15)
   result=dict(exit_code=p.returncode,stdout=b64(p.stdout),stderr=b64(p.stderr))
  except subprocess.TimeoutExpired as e:
   result=dict(timeout=True,stdout=b64(e.stdout or b''),stderr=b64(e.stderr or b''))
  after={str(p.relative_to(wd)):b64(p.read_bytes()) for p in sorted(wd.rglob('*')) if p.is_file()}
  result['external_files']={k:v for k,v in after.items() if before.get(k)!=v}
  result['deleted_files']=sorted(set(before)-set(after))
  return result

def compare(binary,cases,golden):
 failures=[];observations={}
 for case in cases:
  actual=observe(binary,case);observations[case['name']]=actual
  want=golden['observations'].get(case['name'])
  if actual!=want:
   fields=sorted(k for k in set(actual)|set(want or {}) if actual.get(k)!=(want or {}).get(k))
   failures.append(dict(case=case['name'],fields=fields))
 return failures,observations

def main():
 p=argparse.ArgumentParser();p.add_argument('--candidate',type=Path);p.add_argument('--freeze-original',type=Path);p.add_argument('--report',type=Path,default=ROOT/'evidence/harness-differential.json');p.add_argument('--self-test',action='store_true');p.add_argument('--corpus',type=Path,default=CORPUS);p.add_argument('--golden',type=Path,default=GOLDEN);a=p.parse_args()
 global_corpus=a.corpus;global_golden=a.golden
 cases=json.loads(global_corpus.read_text())['cases'];binary=(a.freeze_original or a.candidate)
 if not binary:p.error('--candidate or --freeze-original required')
 binary=binary.resolve()
 if a.freeze_original:
  if global_golden.exists():p.error('Golden exists; refusing to silently replace frozen expectations')
  golden=dict(source_revision='a9f3b7e8ec7f09e2ccf9f514be635950226ad513',binary_sha256=sha(binary),corpus_sha256=sha(global_corpus),observations={c['name']:observe(binary,c) for c in cases})
  global_golden.write_text(json.dumps(golden,indent=2)+'\n');print('FROZEN',len(cases),'cases');return 0
 golden=json.loads(global_golden.read_text())
 if golden['corpus_sha256']!=sha(global_corpus):raise SystemExit('FAIL: corpus changed after freeze')
 failures,observations=compare(binary,cases,golden)
 report=dict(status='FAIL' if failures else 'PASS',cases=len(cases),binary=str(binary),binary_sha256=sha(binary),corpus_sha256=sha(global_corpus),golden_sha256=sha(global_golden),failures=failures,observations=observations)
 if a.self_test:
  import sys
  control=subprocess.run([sys.executable,str(ROOT/'tests/negative_controls.py'),'--original',str(binary)],capture_output=True,text=True)
  report['negative_controls']={'exit_code':control.returncode,'stdout':control.stdout,'stderr':control.stderr,'evidence':'evidence/harness-negative-controls.json'}
  if control.returncode:report['status']='FAIL'
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:report[k] for k in ('status','cases','failures')},indent=2))
 return int(report['status']!='PASS')
if __name__=='__main__':raise SystemExit(main())
