#!/usr/bin/env python3
"""Materialize deterministic requests from frozen specs/tests, validating them on original.
No candidate code is imported or examined. Run BEFORE candidate implementation.
"""
import argparse,sys,json,importlib.util,os
from pathlib import Path
from differential import b64,ROOT,CORPUS
sys.path.insert(0,str(ROOT/'tests/original/harness'))
sys.dont_write_bytecode=True
import adversarial as adv
BASE=b'CLOCK now=144600\nACCOUNT id=A1\nCARD id=C1 account_id=A1\nMERCHANT id=M1\n'
cases=[]
def add(name,seed,commands,args=None,env=None):
 if isinstance(seed,str):seed=seed.encode('ascii')
 if isinstance(commands,str):commands=commands.encode('ascii')
 rec=dict(name=name,files={'seed.txt':b64(seed)},args=args or ['--seed','seed.txt','--commands','-'],stdin=b64(commands))
 if env:rec['env']=env
 cases.append(rec)

def main():
 p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);a=p.parse_args()
 if CORPUS.exists():p.error('Corpus exists; refusing to overwrite preimplementation vectors')
 class Collector(adv.CLI):
  group=''
  def run(self,commands,setup=None,reconcile=True):
   raw=commands if isinstance(commands,str) else '\n'.join(commands)+'\n'
   if reconcile:raw+='RECONCILE\n'
   add('adversarial-'+self.group+'-'+str(len(cases)),setup or adv.seed(),raw)
   return super().run(commands,setup,reconcile)
 cli=Collector(a.original)
 for name,fn in adv.GROUPS.items():cli.group=name;fn(cli)
 # Record protocol scenarios without altering their frozen independent assertions.
 import protocol_adversarial as proto
 proto.BINARY=a.original.resolve();original_run=proto.run
 def collect(commands,seed=proto.BASE,env=None):
  selected={k:env[k] for k in ('LC_ALL','TZ') if env and k in env}
  add('protocol-'+str(len(cases)),seed,commands,env=selected)
  return original_run(commands,seed,env)
 proto.run=collect;proto.main()
 # All twelve documented full lifecycles; append observable queries and reconcile.
 flows=json.loads((ROOT/'tests/original/docs/oracle/flows.json').read_text())['flows']
 for flow in flows:
  d=flow['given'];seed=['CLOCK now='+str(d.get('now',d.get('day',100)*1440+600))]
  if 'capacity.idempotency' in d:seed+=['CONFIG keys_capacity='+str(d['capacity.idempotency'])]
  account={'id':'A1','active':d.get('account.active',True),'tier':d.get('tier','REGULAR'),'credit_limit':d.get('credit_limit',1000000),'daily_limit':d.get('daily_limit',300000),'per_operation_limit':d.get('per_operation_limit',200000),'base_score':d.get('base_score',10)}
  seed+=['ACCOUNT '+' '.join(k+'='+(str(v).lower() if isinstance(v,bool) else str(v)) for k,v in account.items()),'CARD id=C1 account_id=A1','MERCHANT id=M1']
  commands=d['commands']+['GET_ACCOUNT account_id=A1','GET_AUTH auth_id=R1','GET_INVOICE invoice_id=I1','RECONCILE']
  add('oracle-'+flow['id'],'\n'.join(seed)+'\n','\n'.join(commands)+'\n')
 # Invocation and file transport, plus missing-file failures, are part of CLI compatibility.
 for i,args in enumerate([[],['--seed','seed.txt'],['--commands','-'],['--seed','seed.txt','--seed','seed.txt','--commands','-'],['--seed','missing.txt','--commands','-'],['--seed','seed.txt','--commands','missing.txt'],['--unknown'],['--seed','-','--commands','-']]):
  add('invocation-'+str(i),BASE,b'QUOTE amount=100\n',args=args if args else ['--invalid-empty-invocation'])
 cases[-8]['args']=[]
 add('commands-file',BASE,b'',args=['--commands','commands.txt','--seed','seed.txt'])
 cases[-1]['files']['commands.txt']=b64(b'QUOTE amount=100\nGET_ACCOUNT account_id=A1\nRECONCILE\n')
 # Every public operation appears in a complete stream; boundaries already generated above.
 public={'QUOTE','AUTHORIZE','CAPTURE','CANCEL','REFUND','CLOSE','PAY','ASSESS_LATE_FEE','TICK','GET_ACCOUNT','GET_AUTH','GET_INVOICE','RECONCILE'}
 import base64
 covered=set()
 for c in cases:
  raw=base64.b64decode(c['stdin'])
  for line in raw.splitlines():
   fields=line.split()
   if fields:covered.add(fields[0].decode('ascii',errors='replace'))
 assert public<=covered,public-covered
 CORPUS.parent.mkdir(parents=True,exist_ok=True)
 CORPUS.write_text(json.dumps(dict(schema=1,source='frozen independent adversarial, protocol and normative flow inputs',public_operations=sorted(public),cases=cases),indent=2)+'\n')
 print('CORPUS',len(cases),'cases',cli.responses,'adversarial responses')
if __name__=='__main__':main()
