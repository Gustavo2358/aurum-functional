#!/usr/bin/env python3
"""Prove frozen ctypes record layouts match candidate before loading its shared library."""
import argparse,json,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--report',type=Path,default=ROOT/'evidence/harness-abi-layout.json');a=p.parse_args()
 header=(ROOT/'tests/original/subject/include/aurum.h').read_text()
 source=['#include "aurum.h"','#include <stdio.h>','#include <stddef.h>','int main(void) {']
 fields=0
 for body,name in re.findall(r'typedef struct\s*\{([^}]+)\}\s*(\w+);',header):
  source.append(f'printf("{name}=%zu\\n",sizeof({name}));')
  for declaration in body.split(';'):
   if not declaration.strip():continue
   typ,variables=declaration.strip().split(None,1)
   for variable in variables.split(','):
    field=re.fullmatch(r'\s*\**(\w+)(?:\[\w+\])?\s*',variable).group(1)
    source.append(f'printf("{name}.{field}=%zu:%zu\\n",offsetof({name},{field}),sizeof((({name} *)0)->{field}));');fields+=1
 source+=['return 0;','}'];observations={}
 with tempfile.TemporaryDirectory(prefix='aurum-abi-layout-') as d:
  path=Path(d)/'abi.c';path.write_text('\n'.join(source)+'\n')
  for name,include in [('original',ROOT/'tests/original/subject/include'),('candidate',ROOT/'subject/include')]:
   binary=Path(d)/name
   result=subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(include),str(path),'-o',str(binary)],capture_output=True,text=True)
   if result.returncode:observations[name]={'build_exit':result.returncode,'stderr':result.stderr}
   else:
    run=subprocess.run([str(binary)],capture_output=True,text=True);observations[name]={'build_exit':0,'exit_code':run.returncode,'stdout':run.stdout,'stderr':run.stderr}
 report=dict(status='PASS' if observations['original']==observations['candidate'] and observations['original'].get('exit_code')==0 else 'FAIL',fields=fields,observations=observations)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('ABI',report['status'],fields,'fields');return int(report['status']!='PASS')
if __name__=='__main__':raise SystemExit(main())
