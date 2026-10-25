#!/usr/bin/env python3
"""Functional qualification only. No extraction, Joern, network or installation."""
import argparse, hashlib, json, os, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def ast_python():
 candidate=os.environ.get('AURUM_AST_PYTHON')
 choices=[candidate,sys.executable,'/home/gustavo/workspace/aurum-fixture/.venv/bin/python']
 for choice in choices:
  if choice and subprocess.run([choice,'-c','import clang.cindex'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0:return choice
 raise RuntimeError('NOT_RUN: libclang Python binding unavailable; set AURUM_AST_PYTHON to an existing environment. No automatic install.')

def main():
 p=argparse.ArgumentParser();p.add_argument('--fast',action='store_true');a=p.parse_args()
 out=ROOT/'evidence'/('fast' if a.fast else 'full');out.mkdir(parents=True,exist_ok=True)
 py=sys.executable;apy=ast_python();runs=[]
 commands=[('frozen',[py,'tests/verify_frozen.py']),('build',['make','all']),
  ('acceptance',[py,'tests/acceptance.py','--binary','build/aurum','--library','build/libaurum.so','--report-dir',str(out/'acceptance')]),
  ('differential',[py,'tests/differential.py','--candidate','build/aurum','--report',str(out/'differential.json')]),
  ('architecture',[apy,'tests/architecture_gate.py','--self-test']),
  ('purity',[apy,'tests/purity_gate.py'])]
 if not a.fast:
  commands += [('generated',[py,'tests/differential.py','--candidate','build/aurum','--corpus','tests/corpus/generated.json','--golden','tests/corpus/golden-generated.json','--report',str(out/'generated.json')]),
   ('full-state',[py,'tests/state_differential.py','--library','build/libaurum.so','--report',str(out/'state.json')]),
   ('purity-sanitize',[apy,'tests/purity_gate.py','--sanitizers','--report-dir',str(out/'purity-sanitize')]),
   ('sanitize',[py,'tests/instrument_candidate.py','--mode','sanitize']),
   ('coverage',[py,'tests/instrument_candidate.py','--mode','coverage'])]
 for name,command in commands:
  print('RUN',name,flush=True)
  r=subprocess.run(command,cwd=ROOT,capture_output=True)
  (out/(name+'.stdout')).write_bytes(r.stdout);(out/(name+'.stderr')).write_bytes(r.stderr)
  runs.append(dict(gate=name,command=command,exit_code=r.returncode,status='PASS' if r.returncode==0 else 'FAIL'))
  report=dict(status='RUNNING' if r.returncode==0 else 'FAIL',gates=runs,python=sys.version,ast_python=apy,source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((ROOT/'subject').rglob('*')) if p.is_file()})
  (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
  print(name,runs[-1]['status'],flush=True)
  if r.returncode:
   print((r.stdout+r.stderr).decode(errors='replace')[-6000:]);return 1
 report['status']='PASS';report['excluded']='Joern, slicing and extraction methodologies intentionally NOT_RUN per user request.'
 (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');return 0
if __name__=='__main__':raise SystemExit(main())
