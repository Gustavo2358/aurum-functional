#!/usr/bin/env python3
"""Relational invariants over production state reached through real commands.

The independent equations below belong to the evaluator and never enter subject/.
All financial records originate in execute; only initial entities/config are set.
"""
import argparse
from collections import defaultdict
import ctypes as C
import json
from pathlib import Path
import random
import bindings as B
from probes import new_engine,snapshot,execute_checked,output

checks=0
commands=0
streams=0

def check(condition,label):
    global checks
    checks+=1
    if not condition:raise AssertionError(label)

def group(rows,key):
    result=defaultdict(list)
    for row in rows:result[row[key]].append(row)
    return result

def residual(lot,component):return lot[component]-lot['paid_'+component]-lot['cancelled_'+component]

def verify(e,issued):
    rows={name:B.entries(e,name) for name in ('accounts','auths','captures','lots','invoices','history','ledger','keys','payments','refunds','rewards')}
    accounts={x['id']:x for x in rows['accounts']};auths={x['id']:x for x in rows['auths']};captures={x['id']:x for x in rows['captures']};invoices={x['id']:x for x in rows['invoices']}
    for name in ('accounts','auths','captures','invoices','payments','refunds'):
        check(len({x['id'] for x in rows[name]})==len(rows[name]),('unique ids',name))
    by_auth=group(rows['captures'],'auth_id');by_capture=group(rows['lots'],'capture_id');refunds=group(rows['refunds'],'capture_id');rewards=group(rows['rewards'],'capture_id')
    holds=defaultdict(int);debt=defaultdict(int);points=defaultdict(int);gross=defaultdict(int);paid=defaultdict(int);cash_refunds=defaultdict(int)
    for auth in rows['auths']:
        aid=auth['id'];check(auth['account_id'] in accounts,('authorization owner',aid))
        attached=by_auth[aid];cp=sum(x['principal'] for x in attached);cf=sum(x['fee'] for x in attached)
        check((cp,cf)==(auth['captured_principal'],auth['captured_fee']),('capture authorization conservation',aid))
        check(0<=cp<=auth['principal'] and cf==auth['fee']*cp//auth['principal'],('capture cumulative tariff',aid))
        if auth['state']==B.ACTIVE:check(cp==0,('active principal',aid))
        if auth['state']==B.PARTIAL:check(0<cp<auth['principal'],('partial principal',aid))
        if auth['state']==B.CAPTURED:check(cp==auth['principal'],('captured principal',aid))
        if auth['state'] in (B.ACTIVE,B.PARTIAL):holds[auth['account_id']]+=auth['principal']+auth['fee']-cp-cf
    for cap in rows['captures']:
        cid=cap['id'];check(cap['auth_id'] in auths,('capture parent',cid));a=auths[cap['auth_id']]
        check(cap['account_id']==a['account_id'],('capture ownership',cid));lots=by_capture[cid]
        check(len(lots)==a['installments'] and {l['index'] for l in lots}==set(range(a['installments'])),('installment indices',cid))
        check(sum(l['principal'] for l in lots)==cap['principal'] and sum(l['fee'] for l in lots)==cap['fee'],('lot conservation',cid))
        for lot in lots:
            check(lot['cycle']==cap['cycle']+lot['index'],('scheduled cycle',cid,lot['index']))
            for component in ('principal','fee'):
                expected=cap[component]//a['installments']+(lot['index']<cap[component]%a['installments'])
                check(lot[component]==expected,('installment remainder',cid,lot['index'],component))
        rr=refunds[cid]
        check(sum(r['principal'] for r in rr)==cap['refunded_principal'] and sum(r['fee'] for r in rr)==cap['refunded_fee'] and sum(r['points'] for r in rr)==cap['reversed'],('refund records',cid))
        check(0<=cap['refunded_principal']<=cap['principal'],('refund principal bound',cid))
        check(cap['refunded_fee']==cap['fee']*cap['refunded_principal']//cap['principal'],('refund cumulative fee',cid))
        check(cap['reversed']==cap['granted']*cap['refunded_principal']//cap['principal'],('refund cumulative points',cid))
        cancelled=sum(l['cancelled_principal']+l['cancelled_fee'] for l in lots);cash=sum(r['cash'] for r in rr)
        check(cap['refunded_principal']+cap['refunded_fee']==cancelled+cash,('refund cancellation cash partition',cid))
        cash_refunds[cap['account_id']]+=cash
        reward=rewards[cid];check(len(reward)==1,('one reward record per capture',cid))
        check((reward[0]['account_id'],reward[0]['cycle'],reward[0]['granted'])==(cap['account_id'],cap['cycle'],cap['granted']),('reward record association',cid))
        raw=(cap['principal']//e.config.reward_unit)*(2 if a['tier']==B.PREMIUM else 1) if a['category']==B.NORMAL else 0
        reward_key=(cap['account_id'],cap['cycle']);expected=min(raw,max(0,e.config.reward_month_cap-gross[reward_key]))
        check(cap['granted']==expected,('gross cap cannot be replenished by refunds',cid,cap['granted'],expected))
        gross[reward_key]+=cap['granted'];points[cap['account_id']]+=cap['granted']-cap['reversed']
    for lot in rows['lots']:
        check(lot['capture_id'] in captures and lot['account_id']==captures[lot['capture_id']]['account_id'],('lot parent owner',lot['capture_id']))
        for component in ('principal','fee'):
            check(lot[component]>=0 and lot['paid_'+component]>=0 and lot['cancelled_'+component]>=0 and residual(lot,component)>=0,('nonnegative lot components',lot['capture_id'],lot['index'],component))
            debt[lot['account_id']]+=residual(lot,component);paid[lot['account_id']]+=lot['paid_'+component]
        if lot['invoice_id']:check(lot['invoice_id'] in invoices and invoices[lot['invoice_id']]['account_id']==lot['account_id'] and invoices[lot['invoice_id']]['cycle']==lot['cycle'],('lot invoice parent',lot['capture_id']))
        elif lot['paid_principal'] or lot['paid_fee']:check(False,('future lot was paid',lot['capture_id']))
    for inv in rows['invoices']:
        iid=inv['id'];fields=tuple(inv[key] for key in ('account_id','cycle','due_day','issued_principal','issued_fee','issued_total','minimum_due'))
        if iid in issued:check(issued[iid]==fields,('issued snapshot changed',iid))
        else:issued[iid]=fields
        check(inv['issued_total']==inv['issued_principal']+inv['issued_fee'],('issued components',iid))
        check(0<=inv['paid_late']<=inv['late'] and inv['late']==(e.config.late_fee if inv['late_fee_assessed'] else 0),('late fee assessment state',iid))
        debt[inv['account_id']]+=inv['late']-inv['paid_late'];paid[inv['account_id']]+=inv['paid_late']
    ledger=defaultdict(int);events=defaultdict(int)
    for entry in rows['ledger']:events[entry['event_id']]+=entry['amount'];ledger[(entry['account_id'],entry['account'])]+=entry['amount']
    check(all(total==0 for total in events.values()),'each journal event balances')
    for i,account in enumerate(rows['accounts']):
        aid=account['id'];expected_debt=debt[aid];expected_hold=holds[aid]
        check((account['cached_debt'],account['cached_held'],account['cash_refund_total'])==(expected_debt,expected_hold,cash_refunds[aid]),('cached balances',aid))
        check(ledger[(aid,B.HOLD_ASSET)]==expected_hold and ledger[(aid,B.HOLD_OFFSET)]==-expected_hold,('journal holds',aid))
        check(sum(ledger[(aid,kind)] for kind in (B.RECEIVABLE_PRINCIPAL,B.RECEIVABLE_FEE,B.RECEIVABLE_LATE))==expected_debt,('journal receivables',aid))
        pay_total=sum(x['amount'] for x in rows['payments'] if x['account_id']==aid)
        check(pay_total==paid[aid],('payment allocation conservation',aid))
        check(ledger[(aid,B.CASH)]==pay_total-cash_refunds[aid],('cash payment refund conservation',aid))
        cap_rows=[x for x in rows['captures'] if x['account_id']==aid]
        check(ledger[(aid,B.MERCHANT_CLEARING)]==-sum(x['principal']-x['refunded_principal'] for x in cap_rows),('merchant clearing conservation',aid))
        check(ledger[(aid,B.FEE_REVENUE)]==-sum(x['fee']-x['refunded_fee'] for x in cap_rows),('fee revenue conservation',aid))
        check(ledger[(aid,B.LATE_REVENUE)]==-sum(x['late'] for x in rows['invoices'] if x['account_id']==aid),('late revenue conservation',aid))
        observed=B.decode(B.project_account(C.byref(e),C.byref(e.accounts[i])))
        day=[h for h in rows['history'] if h['account_id']==aid and h['minute']//1440==e.now//1440]
        expected={'debt':expected_debt,'held':expected_hold,'points':points[aid],'available_credit':max(0,account['credit_limit']-expected_debt-expected_hold),'cash_refund_total':cash_refunds[aid],'daily_gross_principal':sum(h['principal'] for h in day if h['decision']==B.APPROVED),'daily_count':sum(h['decision'] in (B.APPROVED,B.REVIEW) for h in day)}
        check(all(observed[key]==value for key,value in expected.items()),('projection versus independent rows',aid,observed,expected))
    key_pairs=[(x['command']['op'],x['command']['request_id']) for x in rows['keys']]
    check(len(set(key_pairs))==len(key_pairs),'unique idempotency namespace')
    d=C.c_int64();h=C.c_int64();check(bool(B.reconcile(C.byref(e),b'',C.byref(d),C.byref(h))) and d.value==0 and h.value==0,'production reconcile agrees with independent invariants')

class Stream:
    def __init__(self,seed,custom=True):
        global streams
        streams+=1;self.rng=random.Random(seed);self.e=new_engine({'credit_limit':10**9,'daily_limit':10**9,'per_operation_limit':10**7,'base_score':0});self.issued={}
        if custom:
            c=self.e.config;c.minimum_installment=17;c.international_bps=317;c.installment_bps_per_extra=73;c.tariff_cap=1234;c.reward_month_cap=17;c.reward_unit=137;c.late_fee=37;c.daily_count_limit=100000
        check(bool(B.config_valid(C.byref(self.e.config))),'stream config valid')
        second=B.Account.from_buffer_copy(self.e.accounts[0]);second.id=b'A2';second.tier=B.PREMIUM;B.append(self.e,'accounts',second)
        card=B.Card.from_buffer_copy(self.e.cards[0]);card.id=b'C2';card.account_id=b'A2';B.append(self.e,'cards',card)
        verify(self.e,self.issued)
    def close(self):B.engine_free(C.byref(self.e))
    def run(self,line,replay=True):
        global commands
        commands+=1;q=B.Command();check(B.parse_command(line.encode(),C.byref(q))==1,('command parses',line))
        r=execute_checked(self.e,q);verify(self.e,self.issued)
        if replay and q.request_id and r.decision!=B.ERROR:
            previous=snapshot(self.e);again=execute_checked(self.e,q)
            check(B.decode(again)==B.decode(r),('cached response unchanged',line));check(snapshot(self.e)==previous,('cache replay has no effects',line))
            conflict=B.Command.from_buffer_copy(q)
            if q.op in (B.AUTHORIZE,B.PAY):conflict.amount+=(-1 if conflict.amount==10**12 else 1)
            elif q.op in (B.CAPTURE,B.REFUND):conflict.principal+=(-1 if conflict.principal==10**12 else 1)
            elif q.op==B.CANCEL:conflict.auth_id=b'MISSING'
            elif q.op==B.CLOSE:conflict.cycle+=1
            elif q.op==B.ASSESS_LATE_FEE:conflict.invoice_id=b'MISSING'
            bad=execute_checked(self.e,conflict)
            check((bad.decision,bad.reason)==(B.ERROR,B.IDEMPOTENCY_CONFLICT),('same key different payload',line,output(bad)))
            check(snapshot(self.e)==previous,('idempotency conflict has no effects',line))
        return r

def generated_stream(seed):
    s=Stream(seed)
    try:
        prefix=['AUTHORIZE request_id=A account_id=A1 card_id=C1 merchant_id=M1 amount=10001 installments=3',
            'CAPTURE request_id=X auth_id=A principal=3333','CAPTURE request_id=Y auth_id=A principal=3333','CAPTURE request_id=Z auth_id=A principal=3335',
            'AUTHORIZE request_id=B account_id=A1 card_id=C1 merchant_id=M1 amount=50000 currency=USD',
            'CAPTURE request_id=U auth_id=B principal=100000','CANCEL request_id=CB auth_id=B',
            'REFUND request_id=RU capture_id=U principal=30000',
            'AUTHORIZE request_id=C account_id=A2 card_id=C2 merchant_id=M1 amount=20000 installments=2',
            'CAPTURE request_id=V auth_id=C principal=10000']
        for line in prefix:check(s.run(line).decision in (B.OK,B.APPROVED),('fixed stream prefix',line))
        for i in range(8):
            account='A1' if i%2==0 else 'A2';card='C1' if i%2==0 else 'C2';amount=s.rng.randrange(1000,15000);n=s.rng.randrange(1,4)
            a=s.run(f'AUTHORIZE request_id=G{i} account_id={account} card_id={card} merchant_id=M1 amount={amount} installments={n}')
            check(a.decision==B.APPROVED,('generated approval',seed,i,output(a)))
            first=s.rng.randrange(100,amount-100)
            check(s.run(f'CAPTURE request_id=K{i} auth_id=G{i} principal={first}').decision==B.OK,('generated capture',seed,i))
            if i%3==0:s.run(f'CANCEL request_id=N{i} auth_id=G{i}')
            else:check(s.run(f'CAPTURE request_id=L{i} auth_id=G{i} principal={amount-first}').decision==B.OK,('generated remainder',seed,i))
            if i%2==0:s.run(f'REFUND request_id=Q{i} capture_id=K{i} principal={first//2}')
        s.run('TICK now=172800')
        for account in ('A1','A2'):s.run(f'CLOSE request_id=F{account} account_id={account} cycle=3')
        s.run('TICK now=188640')
        for invoice in ('I1','I2'):s.run(f'ASSESS_LATE_FEE request_id=J{invoice} invoice_id={invoice}')
        for account in ('A1','A2'):
            s.run(f'PAY request_id=P{account} account_id={account} amount=1000')
            declined=s.run(f'PAY request_id=O{account} account_id={account} amount=1000000000000');check(declined.decision==B.DECLINED,'overpayment decline')
        s.run('REFUND request_id=RX capture_id=X principal=3333')
        s.run('REFUND request_id=RV capture_id=V principal=10000')
        s.run('REFUND request_id=RU2 capture_id=U principal=70000')
        s.run('REFUND request_id=INVALIDR capture_id=Y principal=3334')
        s.run('CAPTURE request_id=INVALIDC auth_id=C principal=1')
        s.run('CAPTURE request_id=MISSING auth_id=ABSENT principal=1')
        for cycle in range(4,7):
            s.run(f'TICK now={(cycle+1)*30*1440}')
            for account in ('A1','A2'):s.run(f'CLOSE request_id=F{cycle}{account} account_id={account} cycle={cycle}')
            for account in ('A1','A2'):
                lots=[x for x in B.entries(s.e,'lots') if x['account_id']==account and x['invoice_id']]
                invoices=[x for x in B.entries(s.e,'invoices') if x['account_id']==account]
                due=sum(residual(x,'principal')+residual(x,'fee') for x in lots)+sum(x['late']-x['paid_late'] for x in invoices)
                if due:s.run(f'PAY request_id=P{cycle}{account} account_id={account} amount={due}')
        check(s.e.captures_len>=17 and s.e.refunds_len>=8 and s.e.payments_len>=5,('stream exercises captures, refunds and payments',s.e.captures_len,s.e.refunds_len,s.e.payments_len))
        check(all(a['cached_debt']==0 and a['cached_held']==0 for a in B.entries(s.e,'accounts')),'final settlement')
    finally:s.close()

def tick_atomicity():
    s=Stream(0,False)
    try:
        for name,amount in [('Z',100),('A',200)]:s.run(f'AUTHORIZE request_id={name} account_id=A1 card_id=C1 merchant_id=M1 amount={amount}')
        s.e.config.ledger_capacity=s.e.ledger_len+3;prior=snapshot(s.e)
        r=s.run('TICK now=146040');check((r.decision,r.reason)==(B.ERROR,B.CAPACITY),'whole TICK fails capacity');check(snapshot(s.e)==prior,'whole TICK state and clock rollback')
        s.e.config.ledger_capacity=10000;start=s.e.ledger_len;r=s.run('TICK now=146040');check(r.released==300,'all holds expired')
        event_ids=[]
        for entry in B.entries(s.e,'ledger')[start:]:
            if not event_ids or event_ids[-1]!=entry['event_id']:event_ids.append(entry['event_id'])
        check(event_ids==['EXPIRE:A','EXPIRE:Z'],('expiry ASCII order',event_ids))
        prior=snapshot(s.e);r=s.run('TICK now=146040');check(r.released==0 and snapshot(s.e)==prior,'same-time TICK has no effects')
    finally:s.close()

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--seed',type=int,default=20260930);parser.add_argument('--streams',type=int,default=8);parser.add_argument('--report',type=Path,default=Path('build/invariants.json'));args=parser.parse_args();failure=None
    try:
        for i in range(args.streams):generated_stream(args.seed+i)
        tick_atomicity()
    except Exception as exc:failure=repr(exc)
    report={'status':'FAIL' if failure else 'PASS','seed':args.seed,'streams':streams,'commands':commands,'assertions':checks,'oracle':'Independent relational sums and arbitrary-precision equations over records produced by real C commands'}
    if failure:report['error']=failure
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(failure is not None)
if __name__=='__main__':raise SystemExit(main())
