#!/usr/bin/env python3
"""Independent arbitrary-precision numeric references, outside Gherkin adapters."""
import argparse
import ctypes as C
import json
import random
from pathlib import Path
import bindings as B
from probes import new_engine, command, execute_checked

MAX=10**12
I64=2**63-1
SEED=20260930
checks=0
cases=0

def check(condition,label):
    global checks
    checks+=1
    if not condition:raise AssertionError(label)

def returned(name,args,valid,expected=None):
    global cases
    cases+=1
    out=C.c_int64(-777)
    actual=bool(getattr(B,name)(*args,C.byref(out)))
    check(actual is bool(valid),(name,args,'validity',actual,valid))
    if valid:check(out.value==expected,(name,args,'value',out.value,expected))

def config(**overrides):
    c=B.Config();B.config_default(C.byref(c))
    for key,value in overrides.items():setattr(c,key,value)
    return c

def numbers(rng):
    values=[-1,0,1,2,49,50,99,100,4999,5000,9999,10000,10001,MAX//2,MAX-1,MAX,MAX+1,I64]
    pairs=[(a,b) for a in values for b in values]
    pairs += [(rng.randrange(MAX+2),rng.randrange(MAX+2)) for _ in range(1500)]
    for a,b in pairs:
        valid=0<=a<=MAX and 0<=b<=MAX
        returned('money_add',(a,b),valid and a+b<=MAX,a+b)
        returned('money_sub',(a,b),valid and a>=b,a-b)
    mult_values=values+[I64//2,I64//2+1,I64//100000,I64//100000+1]
    for a,b in [(a,b) for a in mult_values for b in mult_values]+[(rng.randrange(I64+1),rng.randrange(100001)) for _ in range(1500)]:
        returned('checked_mul',(a,b),a>=0 and b>=0 and a*b<=I64,a*b)
    rates=[-1,0,1,2,49,199,200,9999,10000,10001]
    rate_cases=[(v,bps) for v in values for bps in rates]+[(rng.randrange(MAX+1),rng.randrange(10001)) for _ in range(1800)]
    for v,bps in rate_cases:
        valid=0<=v<=MAX and 0<=bps<=10000
        returned('floor_rate',(v,bps),valid,v*bps//10000)
        returned('ceil_rate',(v,bps),valid,(v*bps+9999)//10000)
    fxrates=[-1,0,1,2,4999,5000,9999,10000,10001,14999,15000,15001,99999,100000,100001]
    fx_cases=[(v,r) for v in values for r in fxrates]+[(rng.randrange(MAX+1),rng.randrange(1,100001)) for _ in range(1800)]
    for rate in fxrates:
        if rate>0:
            boundary=MAX*10000//rate
            fx_cases += [(min(I64,max(0,boundary+i)),rate) for i in (-2,-1,0,1,2)]
    for v,rate in fx_cases:
        expected=(v*rate+5000)//10000
        returned('money_fx',(v,rate),0<=v<=MAX and 1<=rate<=100000 and expected<=MAX,expected)
    triples=[(t,p,w) for t in (0,1,MAX-1,MAX,MAX+1) for w in (0,1,2,MAX-1,MAX,MAX+1) for p in (-1,0,1,max(0,w-1),w,min(I64,w+1))]
    triples += [(rng.randrange(MAX+1),rng.randrange(MAX+1),rng.randrange(1,MAX+1)) for _ in range(2400)]
    triples += [(MAX,MAX-i,MAX) for i in range(100)]
    for total,part,whole in triples:
        valid=0<=total<=MAX and 0<=part<=whole<=MAX and whole>0
        returned('proportional',(total,part,whole),valid,total*part//whole if whole else None)
    quads=[(MAX,before,amount,whole) for whole in (0,1,2,MAX-1,MAX) for before in (0,1,max(0,whole-1),whole) for amount in (0,1,max(0,whole-before),MAX)]
    for _ in range(2400):
        whole=rng.randrange(1,MAX+1);prior=rng.randrange(whole+1);amount=rng.randrange(whole-prior+2)
        quads.append((rng.randrange(MAX+1),prior,amount,whole))
    for total,prior,amount,whole in quads:
        valid=0<=total<=MAX and 0<=prior<=MAX and 0<=amount<=MAX and 0<whole<=MAX and prior+amount<=whole
        expected=(total*(prior+amount)//whole-total*prior//whole) if whole else None
        returned('cumulative_delta',(total,prior,amount,whole),valid,expected)
    for total in (0,1,2,11,12,13,10001,MAX-1,MAX):
        for n in range(1,13):
            actual=[B.split_part(total,n,i) for i in range(n)]
            expected=[total//n+(i<total%n) for i in range(n)]
            check(actual==expected,('split',total,n,actual,expected));check(sum(actual)==total,('split conservation',total,n))

def configuration(rng):
    intervals={'fx_usd':(1,100000),'fx_eur':(1,100000),'international_bps':(0,10000),'installment_bps_per_extra':(0,10000),'minimum_due_bps':(0,10000),'tariff_cap':(0,MAX),'pin_threshold':(0,MAX),'minimum_installment':(1,MAX),'minimum_due_floor':(0,MAX),'late_fee':(0,MAX),'hold_ttl_minutes':(1,10080),'daily_count_limit':(1,100000),'reward_month_cap':(0,MAX),'reward_unit':(1,MAX)}
    intervals.update({key:(1,1000000) for key,typ in B.Config._fields_ if key.endswith('_capacity')})
    for field,(low,high) in intervals.items():
        for value in (low-1,low,low+1,(low+high)//2,high-1,high,high+1):
            c=config(**{field:value});check(bool(B.config_valid(C.byref(c)))==(low<=value<=high),('config range',field,value))
    for field,constant in [('fx_brl',10000),('cycle_days',30),('due_grace_days',10)]:
        for value in (constant-1,constant,constant+1):check(bool(B.config_valid(C.byref(config(**{field:value}))))==(value==constant),('fixed config',field,value))
    for review,decline in [(0,1),(0,100),(17,63),(49,50),(99,100)]:
        c=config(review_score=review,decline_score=decline);check(B.config_valid(C.byref(c)),('risk config',review,decline))
        for score in range(101):
            expected=B.DECLINED if score>=decline else B.REVIEW if score>=review else B.APPROVED
            check(B.risk_classify(C.byref(c),score)==expected,('risk override',review,decline,score))
    for review,decline in [(-1,50),(50,50),(51,50),(0,101),(100,101)]:check(not B.config_valid(C.byref(config(review_score=review,decline_score=decline))),('risk invalid',review,decline))
    for limit in (1,2,10,100000):
        for count in (0,limit-1,limit,limit+1):check((B.count_admission(count,limit)==B.NONE)==(count<limit),('count limit',limit,count))
    for minimum in (1,17,500,MAX//12,MAX):
        c=config(minimum_installment=minimum)
        for n in (1,2,3,12):
            for principal in (1,min(MAX,max(1,n*minimum-1)),min(MAX,n*minimum),MAX):
                allowed=n==1 or principal//n>=minimum
                check(bool(B.capture_installment_allowed(C.byref(c),principal,n))==allowed,('capture installment override',minimum,n,principal))
                for country in (b'BR',b'US'):check(bool(B.installment_allowed(C.byref(c),principal,n,country))==(allowed and (country==b'BR' or n<=3)),('installment override',minimum,n,principal,country))
    for unit in (1,17,10000,MAX):
        for cap in (0,1,19,MAX):
            c=config(reward_unit=unit,reward_month_cap=cap)
            for principal in (0,1,min(MAX,unit-1),unit,min(MAX,unit+1),MAX):
                for tier in (B.REGULAR,B.PREMIUM):
                    raw=(principal//unit)*(2 if tier==B.PREMIUM else 1)
                    check(B.reward_raw(C.byref(c),principal,tier,B.NORMAL)==raw,('reward unit',unit,principal,tier))
                    check(B.reward_raw(C.byref(c),principal,tier,B.RESTRICTED)==0,('restricted rewards',unit,principal,tier))
                    for gross in (0,max(0,cap-1),cap,cap+1):check(B.reward_grant(C.byref(c),raw,gross)==min(raw,max(0,cap-gross)),('reward cap',cap,gross,raw))
    for _ in range(1200):
        total=rng.choice((0,1,MAX,rng.randrange(MAX+1)));bps=rng.choice((0,1,10000,rng.randrange(10001)));floor=rng.choice((0,1,MAX,rng.randrange(MAX+1)))
        c=config(minimum_due_bps=bps,minimum_due_floor=floor)
        expected=min(total,max(floor,(total*bps+9999)//10000))
        check(B.invoice_minimum(C.byref(c),total)==expected,('minimum override',total,bps,floor))
    for _ in range(1500):
        principal=rng.choice((1,9999,MAX,rng.randrange(MAX+1)));n=rng.randrange(1,13)
        ibps=rng.choice((0,1,10000,rng.randrange(10001)));pbps=rng.choice((0,1,10000,rng.randrange(10001)));cap=rng.choice((0,1,MAX,rng.randrange(MAX+1)))
        c=config(international_bps=ibps,installment_bps_per_extra=pbps,tariff_cap=cap)
        tier=rng.choice((B.REGULAR,B.PREMIUM));currency=rng.choice((b'BRL',b'USD'))
        expected_i=0 if tier==B.PREMIUM or currency==b'BRL' else principal*ibps//10000
        expected_p=principal*pbps*(n-1)//10000
        actual=B.price_components(C.byref(c),principal,n,currency,tier)
        check(actual.reason==B.NONE and (actual.international_fee,actual.installment_fee,actual.fee)==(expected_i,expected_p,min(cap,expected_i+expected_p)),('fee override',principal,n,ibps,pbps,cap,tier,currency,B.decode(actual)))
    for pin in (0,1,17,10000,MAX):
        e=new_engine({})
        try:
            e.config.pin_threshold=pin
            for nominal in {1,max(1,pin-1),max(1,pin),min(MAX,pin+1)}:
                for present in (False,True):
                    q=command('AUTHORIZE',{'amount':nominal,'currency':'USD','card_present':present,'pin_ok':False})
                    actual=B.eligibility(C.byref(e),C.byref(e.accounts[0]),C.byref(e.cards[0]),C.byref(e.merchants[0]),C.byref(q))
                    check(actual==(B.PIN_REQUIRED if present and nominal>pin else B.NONE),('nominal pin override',pin,nominal,present))
        finally:B.engine_free(C.byref(e))
    for ttl in (1,2,7,1440,10080):
        e=new_engine({})
        try:
            e.config.hold_ttl_minutes=ttl
            r=execute_checked(e,command('AUTHORIZE',{'amount':1}));check(r.decision==B.APPROVED and r.expires_at==144600+ttl,('ttl override',ttl,B.decode(r)))
        finally:B.engine_free(C.byref(e))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--seed',type=int,default=SEED);parser.add_argument('--report',type=Path,default=Path('build/numeric-properties.json'));args=parser.parse_args()
    rng=random.Random(args.seed);failure=None
    try:numbers(rng);configuration(rng)
    except Exception as exc:failure=repr(exc)
    report={'status':'FAIL' if failure else 'PASS','seed':args.seed,'numeric_cases':cases,'assertions':checks,'reference':'Python arbitrary-precision integers; no expected values from the C implementation'}
    if failure:report['error']=failure
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(failure is not None)
if __name__=='__main__':raise SystemExit(main())
