#!/usr/bin/env python3
"""Fixed-RNG public lifecycles. Generates inputs only, never expected answers."""
import json,random
from differential import ROOT,b64
SEED=20261025

def main():
 rng=random.Random(SEED);cases=[]
 for stream in range(24):
  seed=['CLOCK now=144600','CONFIG daily_count_limit=1000 review_score=90 decline_score=100 hold_ttl_minutes=180','ACCOUNT id=A1 tier='+('PREMIUM' if stream%2 else 'REGULAR')+' credit_limit=1000000000 daily_limit=1000000000 per_operation_limit=10000000 base_score=0','ACCOUNT id=A2 credit_limit=1000000000 daily_limit=1000000000 per_operation_limit=10000000 base_score=0','CARD id=C1 account_id=A1','CARD id=C2 account_id=A2','MERCHANT id=M1']
  commands=[];auths=[];invoices=[];now=144600
  for i in range(18):
   account='A'+str(1+(i%2));card='C'+str(1+(i%2));rid=f'R{i}';cid=f'C{i}';auths.append(rid)
   amount=rng.choice([500,999,1000,1001,10000,10001,19999,20000,50001,99999])
   n=rng.choice([1,1,2,3]);cur=rng.choice(['BRL','USD','EUR']);country=rng.choice(['BR','BR','US'])
   auth=f'AUTHORIZE request_id={rid} account_id={account} card_id={card} merchant_id=M1 amount={amount} currency={cur} installments={n} country={country}'
   commands += [f'QUOTE amount={amount} currency={cur} installments={n} country={country}',auth,f'GET_AUTH auth_id={rid}']
   p=amount*{'BRL':1,'USD':5,'EUR':6}[cur];first=rng.choice([1,p//3,p//2,p]);first=max(1,first)
   capture=f'CAPTURE request_id={cid} auth_id={rid} principal={first}'
   commands += [capture,capture,auth]
   branch=i%6
   if branch==0:
    commands += [f'CANCEL request_id=X{i} auth_id={rid}',f'CAPTURE request_id=Z{i} auth_id={rid} principal=1']
   elif branch==1:
    now+=181;commands += [f'TICK now={now}',f'CAPTURE request_id=Z{i} auth_id={rid} principal=1']
   elif first<p:commands += [f'CAPTURE request_id=Z{i} auth_id={rid} principal={p-first}']
   if branch in (2,3,4):
    refund=f'REFUND request_id=F{i} capture_id={cid} principal={max(1,first//2)}'
    commands += [refund,refund,f'REFUND request_id=F{i} capture_id={cid} principal={max(1,first//2)+1}']
   commands += [f'PAY request_id=P{i} account_id={account} amount={rng.choice([1,499,500,1000,5000,99999999])}',f'GET_ACCOUNT account_id={account}','RECONCILE']
  # Advance across successive cycles, close both accounts, assess and replay late fees.
  for cycle in range(3,7):
   now=(cycle+1)*30*1440;commands += [f'TICK now={now}']
   for aid in ('A1','A2'):commands += [f'CLOSE request_id=CL{cycle}{aid} account_id={aid} cycle={cycle}']
   now+=11*1440;commands += [f'TICK now={now}']
   for number in range(1,2*(cycle-2)+1):
    inv=f'I{number}';late=f'ASSESS_LATE_FEE request_id=L{cycle}_{inv} invoice_id={inv}';commands += [late,late,f'GET_INVOICE invoice_id={inv}']
  commands += [f'GET_AUTH auth_id={x}' for x in auths]+[f'GET_INVOICE invoice_id=I{i}' for i in range(1,9)]+['GET_ACCOUNT account_id=A1','GET_ACCOUNT account_id=A2','RECONCILE']
  cases.append(dict(name=f'generated-{stream:02d}',files={'seed.txt':b64(('\n'.join(seed)+'\n').encode())},args=['--seed','seed.txt','--commands','-'],stdin=b64(('\n'.join(commands)+'\n').encode())))
 path=ROOT/'tests/corpus/generated.json'
 if path.exists():raise SystemExit('Refusing to replace frozen generated inputs')
 path.write_text(json.dumps(dict(schema=1,seed=SEED,cases=cases),indent=2)+'\n');print('generated',len(cases),'streams')
if __name__=='__main__':main()
