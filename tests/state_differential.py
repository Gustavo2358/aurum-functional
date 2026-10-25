#!/usr/bin/env python3
"""Canonical full-record state comparison after each generated public command.

Uses the frozen ABI reader. Records hashes of every public record field in all
collections, configuration, logical time and result; ignores allocator capacities,
addresses and padding. Golden derived from ORIGINAL is compatibility evidence,
not a replacement for independently authored normative expected values.
"""
import argparse,base64,ctypes as C,hashlib,json,os,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def digest(value):return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--library',type=Path,required=True);p.add_argument('--freeze-original',action='store_true');p.add_argument('--corpus',type=Path,default=ROOT/'tests/corpus/generated.json');p.add_argument('--golden',type=Path,default=ROOT/'tests/corpus/golden-state.json');p.add_argument('--report',type=Path,default=ROOT/'evidence/harness-state-differential.json');a=p.parse_args()
 if a.freeze_original and a.golden.exists():p.error('Refusing to overwrite frozen state golden')
 os.environ['AURUM_LIB']=str(a.library.resolve());sys.path.insert(0,str(ROOT/'tests/original/harness'));sys.dont_write_bytecode=True
 import bindings as B
 from probes import snapshot,output
 libc=C.CDLL(None);libc.fopen.argtypes=[C.c_char_p,C.c_char_p];libc.fopen.restype=C.c_void_p;libc.fclose.argtypes=[C.c_void_p];libc.fclose.restype=C.c_int
 B.lib.seed_load.argtypes=[C.c_void_p,C.POINTER(B.Engine)];B.lib.seed_load.restype=C.c_bool
 cases=json.loads(a.corpus.read_text())['cases'];states={};failures=[];counterexample=None
 golden=None if a.freeze_original else json.loads(a.golden.read_text())
 if golden and golden['corpus_sha256']!=hashlib.sha256(a.corpus.read_bytes()).hexdigest():raise SystemExit('Corpus changed after freeze')
 for case in cases:
  records=[];e=B.Engine();B.engine_init(C.byref(e))
  with tempfile.TemporaryDirectory(prefix='aurum-state-') as d:
   path=Path(d)/'seed.txt';path.write_bytes(base64.b64decode(case['files']['seed.txt']));f=libc.fopen(os.fsencode(path),b'r')
   assert f,'could not open seed'
   try:assert B.lib.seed_load(f,C.byref(e)),case['name']
   finally:libc.fclose(f)
  try:
   for index,line in enumerate(base64.b64decode(case['stdin']).decode().splitlines()):
    command=B.Command();assert B.parse_command(line.encode(),C.byref(command))==1
    result=B.execute(C.byref(e),C.byref(command));state=snapshot(e);response=output(result)
    record={'state_sha256':digest(state),'result_sha256':digest(response)};records.append(record)
    if golden and record!=golden['states'][case['name']][index]:
     failures.append(dict(case=case['name'],index=index,command=line,expected=golden['states'][case['name']][index],actual=record))
     if counterexample is None:counterexample=dict(case=case['name'],index=index,command=line,state=state,result=response)
  finally:B.engine_free(C.byref(e))
  states[case['name']]=records
 report=dict(status='FAIL' if failures else 'PASS',library_sha256=hashlib.sha256(a.library.read_bytes()).hexdigest(),corpus_sha256=hashlib.sha256(a.corpus.read_bytes()).hexdigest(),commands=sum(map(len,states.values())),states=states,failures=failures)
 if counterexample:report['first_counterexample']=counterexample
 target=a.golden if a.freeze_original else a.report;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:report[k] for k in ('status','commands')}));return int(bool(failures))
if __name__=='__main__':raise SystemExit(main())
