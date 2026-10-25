#!/usr/bin/env python3
"""Instrument the new implementation; never compile the oracle as candidate."""
import argparse, gzip, json, os, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def main():
 p=argparse.ArgumentParser();p.add_argument('--mode',choices=['sanitize','coverage'],required=True);a=p.parse_args()
 folder=ROOT/'build'/a.mode;folder.mkdir(parents=True,exist_ok=True)
 out=ROOT/'evidence'/a.mode;out.mkdir(parents=True,exist_ok=True)
 runs=[]
 def run(name,cmd,env=None,cwd=ROOT):
  r=subprocess.run(list(map(str,cmd)),cwd=cwd,env=env,capture_output=True)
  (out/(name+'.stdout')).write_bytes(r.stdout);(out/(name+'.stderr')).write_bytes(r.stderr)
  runs.append(dict(name=name,command=list(map(str,cmd)),exit_code=r.returncode))
  (out/'summary.json').write_text(json.dumps(dict(status='RUNNING' if r.returncode==0 else 'FAIL',mode=a.mode,runs=runs),indent=2)+'\n')
  print(name, 'PASS' if r.returncode==0 else 'FAIL',flush=True)
  if r.returncode:
   print((r.stdout+r.stderr).decode(errors='replace')[-5000:]);raise SystemExit(1)
 sources=sorted((ROOT/'subject/src').glob('*.c'));library=[x for x in sources if x.name!='main.c']+[ROOT/'tests/legacy_adapter.c']
 flags=['cc','-std=c17','-g','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(ROOT/'subject/include')]
 flags+=['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.mode=='sanitize' else ['-O0','--coverage']
 for pattern in ('*.gcda','*.gcno','*.gcov.json.gz'):
  for old in folder.glob(pattern):old.unlink()
 run('build-cli',flags+sources+(['-no-pie'] if a.mode=='sanitize' else [])+['-o',folder/'aurum'],cwd=folder)
 run('build-library',flags+['-fPIC','-shared']+library+['-o',folder/'libaurum.so'],cwd=folder)
 env=dict(os.environ)
 if a.mode=='sanitize':
  asan=subprocess.check_output(['cc','-print-file-name=libasan.so'],text=True).strip()
  env.update(LD_PRELOAD=asan,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 run('acceptance',[sys.executable,ROOT/'tests/acceptance.py','--binary',folder/'aurum','--library',folder/'libaurum.so','--report-dir',out/'acceptance'],env)
 cli_env=env.copy();cli_env.pop('LD_PRELOAD',None)
 if a.mode=='sanitize':cli_env['ASAN_OPTIONS']='detect_leaks=1:halt_on_error=1'
 for name,corpus,golden in [('fixed','cases.json','golden.json'),('generated','generated.json','golden-generated.json')]:
  run(name,[sys.executable,ROOT/'tests/differential.py','--candidate',folder/'aurum','--corpus',ROOT/'tests/corpus'/corpus,'--golden',ROOT/'tests/corpus'/golden,'--report',out/(name+'.json')],cli_env)
 coverage={}
 if a.mode=='coverage':
  run('gcov',['gcov','--json-format','--branch-probabilities',*sorted(folder.glob('*.gcda'))],cwd=folder)
  merged={}
  for path in folder.glob('*.gcov.json.gz'):
   for f in json.loads(gzip.decompress(path.read_bytes()))['files']:
    name=Path(f['file'])
    if name.parent!=ROOT/'subject/src':continue
    target=merged.setdefault(str(name.relative_to(ROOT)),{})
    for row in f['lines']:
     record=target.setdefault(row['line_number'],dict(count=0,branches={}))
     record['count']+=row['count']
     for i,branch in enumerate(row.get('branches',[])):record['branches'][i]=record['branches'].get(i,0)+branch['count']
  for name,lines in merged.items():
   coverage[name]=dict(lines=len(lines),lines_hit=sum(x['count']>0 for x in lines.values()),branches=sum(len(x['branches']) for x in lines.values()),branches_hit=sum(sum(v>0 for v in x['branches'].values()) for x in lines.values()))
  if not coverage:raise SystemExit('FAIL: no production coverage data')
 report=dict(status='PASS',mode=a.mode,runs=runs,coverage=coverage,scope='Full candidate acceptance and fixed/generated differential; ASan+UBSan for C shared and CLI; CLI differential leak detection enabled. Python host leak detection disabled. Coverage is runtime evidence, not proof of equivalence.')
 (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status='PASS',mode=a.mode,coverage=coverage)))
if __name__=='__main__':main()
