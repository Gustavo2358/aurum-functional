#!/usr/bin/env python3
"""Executable controls for stdout, stderr, status and filesystem comparisons."""
import argparse,json,tempfile,sys
from pathlib import Path
from differential import ROOT,CORPUS,GOLDEN,compare

def main():
 p=argparse.ArgumentParser();p.add_argument('--original',type=Path,default=ROOT/'evidence/harness-original/aurum');a=p.parse_args();binary=a.original.resolve()
 cases=json.loads(CORPUS.read_text())['cases'][:1];golden=json.loads(GOLDEN.read_text());records=[]
 controls={'stdout':"sys.stdout.buffer.write(b'WRONG\\n'+p.stdout)", 'stderr':"sys.stderr.buffer.write(b'wrong diagnostic\\n')",'exit_code':'sys.exit(99)','external_files':"Path('unexpected.txt').write_bytes(b'unexpected effect')",'deleted_files':"Path('seed.txt').unlink()"}
 with tempfile.TemporaryDirectory(prefix='aurum-controls-') as d:
  for name,action in controls.items():
   path=Path(d)/name
   content=f'#!{sys.executable}\nimport subprocess,sys\nfrom pathlib import Path\np=subprocess.run([{str(binary)!r},*sys.argv[1:]],input=sys.stdin.buffer.read(),capture_output=True)\n'
   if name!='stdout':content+='sys.stdout.buffer.write(p.stdout)\n'
   content+='sys.stderr.buffer.write(p.stderr)\n'+action+'\n'
   if name!='exit_code':content+='sys.exit(p.returncode)\n'
   path.write_text(content);path.chmod(0o755)
   failures,observed=compare(path,cases,golden)
   record=dict(control=name,status='PASS' if failures and name in failures[0]['fields'] else 'FAIL',failures=failures,observed=observed)
   records.append(record);print(name,record['status'])
 report=dict(status='PASS' if all(r['status']=='PASS' for r in records) else 'FAIL',controls=records)
 (ROOT/'evidence/harness-negative-controls.json').write_text(json.dumps(report,indent=2)+'\n')
 return int(report['status']!='PASS')
if __name__=='__main__':raise SystemExit(main())
