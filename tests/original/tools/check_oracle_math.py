#!/usr/bin/env python3
"""Audita vetores numéricos selecionados do oráculo autoral.
Não implementa a aplicação, não gera expectativas e não executa o C.
Cobre somente as operações explicitamente selecionadas e o perfil de configuração base.
"""
import json
from pathlib import Path
from math import ceil
ROOT=Path(__file__).resolve().parents[1]
rs=json.loads((ROOT/'docs/oracle/rules.json').read_text(encoding='utf-8'))['rules']
checks=0;cases=0;errors=[]
def split(t,n):return [t//n+(i<t%n) for i in range(n)]
for r in rs:
 for c in r['cases']:
  a=c['given'];e=c['expected'];o=r['operation'];v={}
  if o=='money.add':
   z=a['a']+a['b'];v={'ok':z<=10**12};v.update({'value':z} if v['ok'] else {'error':'NUMERIC_RANGE'})
  elif o=='money.subtract':
   z=a['a']-a['b'];v={'ok':z>=0};v.update({'value':z} if v['ok'] else {'error':'NUMERIC_RANGE'})
  elif o=='money.floor_rate':v={'value':a['value']*a['bps']//10000}
  elif o=='money.ceil_rate':v={'value':(a['value']*a['bps']+9999)//10000}
  elif o=='money.fx':v={'value':(a['value']*a['rate']+5000)//10000}
  elif o=='money.checked_mul':
   z=a['a']*a['b'];v={'ok':z<2**63};v.update({'value':z} if v['ok'] else {'error':'NUMERIC_RANGE'})
  elif o=='pricing.components':
   p=a.get('principal',10000);n=a.get('installments',1)
   f1=p*200//10000 if a.get('currency','BRL')!='BRL' and a.get('tier','REGULAR')!='PREMIUM' else 0
   f2=p*50*(n-1)//10000
   v={'international_fee':f1,'installment_fee':f2,'fee':min(5000,f1+f2)}
  elif o=='pricing.quote':
   rate={'BRL':10000,'USD':50000,'EUR':60000}.get(a.get('currency','BRL'))
   v={'ok':True,'principal':(a['amount']*rate+5000)//10000} if rate else {'ok':False,'reason':'UNSUPPORTED_CURRENCY'}
  elif o=='limits.available':v={'available':max(0,a['credit_limit']-a['debt']-a['held'])}
  elif o=='risk.score':
   z=a.get('base_score',10)+15*(a.get('country','BR')!='BR')+20*(not a.get('card_present',True))+15*(a.get('principal',10000)>=100000)+20*(a.get('recent_count',0)>=3)
   v={'score':min(100,z)}
  elif o=='risk.classify':v={'decision':'DECLINED' if a['score']>=80 else 'REVIEW' if a['score']>=50 else 'APPROVED'}
  elif o=='risk.velocity':
   n=sum(x['decision'] in ['APPROVED','REVIEW'] and a['now']-60<=x['minute']<=a['now'] for x in a['history'])
   v={'recent_count':n,'component':20*(n>=3)}
  elif o=='capture.fee_delta':
   v={'fee_delta':a['fee']*(a['captured']+a['amount'])//a['principal']-a['fee']*a['captured']//a['principal']}
  elif o=='reversal.refund_fee':
   v={'fee_refund':a['capture_fee']*(a['refunded']+a['amount'])//a['capture_principal']-a['capture_fee']*a['refunded']//a['capture_principal']}
  elif o=='installment.split':v={'parts':split(a['total'],a['n'])}
  elif o=='installment.split_components':v={'principal_parts':split(a['principal'],a['n']),'fee_parts':split(a['fee'],a['n'])}
  elif o=='installment.conservation':v={'sum_principal':sum(split(a['principal'],a['n'])),'sum_fee':sum(split(a['fee'],a['n']))}
  elif o=='installment.schedule':
   cs=[a['capture_day']//30+i for i in range(a['n'])];v={'cycles':cs,'due_days':[(x+1)*30+10 for x in cs]}
  elif o=='billing.minimum_due':v={'minimum_due':min(a['total'],max(1000,(a['total']*1000+9999)//10000))}
  elif o=='billing.due_day':v={'due_day':(a['cycle']+1)*30+10}
  elif o=='rewards.raw':v={'raw_points':0 if a.get('category','NORMAL')=='RESTRICTED' else (a['principal']//10000)*(2 if a.get('tier','REGULAR')=='PREMIUM' else 1)}
  elif o=='rewards.cap':v={'granted':min(a['raw_points'],max(0,1000-a['gross_granted']))}
  elif o=='rewards.refund':v={'reversed':a['granted']*(a['refunded']+a['amount'])//a['principal']-a['granted']*a['refunded']//a['principal']}
  elif o=='rewards.capture_partition':v={'points':sum(x//10000 for x in a['captures'])*(2 if a['tier']=='PREMIUM' else 1)}
  if not v:continue
  cases+=1
  for k,val in e.items():
   if k not in v:continue
   checks+=1
   if v[k]!=val: errors.append((c['id'],k,val,v[k]))
print(json.dumps({'arithmetic_cases_checked':cases,'assertions_checked':checks,'errors':errors},indent=2))
raise SystemExit(1 if errors else 0)
