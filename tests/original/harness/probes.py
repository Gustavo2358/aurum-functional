"""Acceptance adapters: explicit input preparation, production calls, observations.

All expected answers stay in feature DataTables. This module never receives them.
"""
import ctypes as C
import json,os,subprocess,tempfile
from pathlib import Path
import bindings as B
from bindings import ROOT
class UnknownProbe(Exception):pass
ENUM=lambda name,value:B.ENUMS[name].index(value)
def enc(v):return v.encode('ascii')
def text(v):return v.decode('ascii')
def named(fn,value):return text(getattr(B,fn)(value))
def reason(value):return named('reason_name',value)
def decision(value):return named('decision_name',value)
def state(value):return named('state_name',value)
def output(r):
    d=B.decode(r);d.update(op=named('operation_name',r.op),decision=decision(r.decision),reason=reason(r.reason));return d
COLLECTIONS=['accounts','cards','merchants','auths','captures','lots','invoices','history','ledger','keys','payments','refunds','rewards']
FINANCIAL=['auths','captures','lots','invoices','ledger','payments','refunds','rewards']
def snapshot(e,financial=False):
    names=FINANCIAL if financial else COLLECTIONS
    values={n:B.entries(e,n) for n in names}
    if financial:values['account_totals']=[(a['cached_debt'],a['cached_held'],a['cash_refund_total']) for a in B.entries(e,'accounts')]
    else:values['now']=e.now;values['config']=B.decode(e.config)
    return values

def projection(e,index=0):return B.decode(B.project_account(C.byref(e),C.byref(e.accounts[index])))
def balanced(e):
    totals={}
    for x in B.entries(e,'ledger'):totals[x['event_id']]=totals.get(x['event_id'],0)+x['amount']
    actual=all(v==0 for v in totals.values())
    assert bool(B.ledger_balanced(C.byref(e)))==actual,'ledger balance disagrees with journal'
    return actual

def events(e):return len({x['event_id'] for x in B.entries(e,'ledger')})
def ledger_totals(e):
    out={n:0 for n in B.ENUMS['LedgerAccount']}
    for x in B.entries(e,'ledger'):out[B.ENUMS['LedgerAccount'][x['account']]]+=x['amount']
    return out

def call_bool_value(fn,*args):
    out=C.c_int64();ok=bool(getattr(B,fn)(*args,C.byref(out)))
    return {'ok':ok,'value':out.value,'error':'NONE' if ok else 'NUMERIC_RANGE'}

def command(op='AUTHORIZE',values=None):
    c=B.Command();B.command_default(C.byref(c),getattr(B,op))
    # Base profile is explicit test input, not a default inferred from the oracle.
    base={'request_id':'R1','account_id':'A1','card_id':'C1','merchant_id':'M1','amount':10000}
    if op not in ('AUTHORIZE','QUOTE'):base={}
    base.update(values or {})
    types=dict(B.Command._fields_)
    for key,value in base.items():
        if key not in types or key=='op':continue
        if key in ('channel','tier') and isinstance(value,str):value=ENUM('Channel' if key=='channel' else 'Tier',value)
        setattr(c,key,enc(value) if isinstance(value,str) else value)
    return c

def execute_checked(e,c):
    initial=snapshot(e);financial=snapshot(e,True);projection_before=projection(e)
    r=B.execute(C.byref(e),C.byref(c))
    if r.decision in (B.ERROR,B.DECLINED,B.REVIEW):
        assert snapshot(e,True)==financial,'rejected/reviewed operation changed financial state'
    if r.decision==B.ERROR:assert snapshot(e)==initial,'ERROR left partial effects'
    if r.decision==B.DECLINED:
        after=projection(e)
        assert (after['daily_count'],after['daily_gross_principal'])==(projection_before['daily_count'],projection_before['daily_gross_principal']),'DECLINED changed daily consumption'
    return r

def perform(e,op='AUTHORIZE',**values):return execute_checked(e,command(op,values))
def fixture_parents(e):
    """Supply structural parents of explicit historical receivables.

    Sums here describe input records, not answers. Formula probes remain
    component tests; only flow.run claims reachability through public commands.
    """
    for lot in list(B.entries(e,'lots')):
        if not lot['capture_id']:raise AssertionError('lot needs explicit capture identity')
        if not B.find_capture(C.byref(e),enc(lot['capture_id'])):
            peers=[x for x in B.entries(e,'lots') if x['capture_id']==lot['capture_id']]
            principal=sum(x['principal'] for x in peers);fee=sum(x['fee'] for x in peers)
            aid='PA_'+lot['capture_id']
            B.append(e,'auths',B.record(B.Authorization,id=aid,account_id=lot['account_id'],state=B.CAPTURED,principal=principal,fee=fee,captured_principal=principal,captured_fee=fee,approved_at=max(0,e.now-2880),expires_at=max(0,e.now-1440),installments=len(peers),tier=e.accounts[0].tier,category=B.NORMAL))
            B.append(e,'captures',B.record(B.Capture,id=lot['capture_id'],auth_id=aid,account_id=lot['account_id'],principal=principal,fee=fee,refunded_principal=sum(x['cancelled_principal'] for x in peers),refunded_fee=sum(x['cancelled_fee'] for x in peers),cycle=peers[0]['cycle']))

def validate_fixture(e):
    accounts={x['id'] for x in B.entries(e,'accounts')}
    auths={x['id']:x for x in B.entries(e,'auths')}
    captures={x['id']:x for x in B.entries(e,'captures')}
    invoices={x['id']:x for x in B.entries(e,'invoices')}
    for name,values in (('auths',auths),('captures',captures),('invoices',invoices)):
        assert len(values)==getattr(e,name+'_len'),'duplicate fixture identities'
        assert all(x['account_id'] in accounts for x in values.values()),'missing account parent'
    for cap in captures.values():
        assert cap['auth_id'] in auths,'missing authorization parent'
        assert auths[cap['auth_id']]['account_id']==cap['account_id'],'capture ownership'
        assert 0<=cap['refunded_principal']<=cap['principal'] and 0<=cap['refunded_fee']<=cap['fee'],'invalid capture components'
    for lot in B.entries(e,'lots'):
        assert lot['capture_id'] in captures,'missing capture parent'
        assert lot['account_id']==captures[lot['capture_id']]['account_id'],'lot ownership'
        if lot['invoice_id']:assert lot['invoice_id'] in invoices,'missing invoice parent'
        for component in ('principal','fee'):
            assert all(lot[k]>=0 for k in (component,'paid_'+component,'cancelled_'+component))
            assert lot['paid_'+component]+lot['cancelled_'+component]<=lot[component],'overconsumed lot'
    for reward in B.entries(e,'rewards'):
        assert reward['capture_id'] in captures,'missing reward parent'
        assert reward['account_id']==captures[reward['capture_id']]['account_id'],'reward ownership'

def opening(e):
    fixture_parents(e)
    validate_fixture(e)
    # These entries describe explicit fixture inputs. Amounts are observed by the
    # production projection helpers; no expected output is used to build state.
    for i in range(e.lots_len):
        lot=e.lots[i]
        for account,value in ((B.RECEIVABLE_PRINCIPAL,B.lot_principal(C.byref(lot))),(B.RECEIVABLE_FEE,B.lot_fee(C.byref(lot)))):
            if value:assert B.ledger_pair(C.byref(e),enc('OPENING_LOT_'+str(i)+'_'+str(account)),lot.account_id,account,B.OPENING_OFFSET,value)
    for i in range(e.invoices_len):
        inv=e.invoices[i];value=inv.late-inv.paid_late
        if value:assert B.ledger_pair(C.byref(e),enc('OPENING_LATE_'+str(i)),inv.account_id,B.RECEIVABLE_LATE,B.OPENING_OFFSET,value)
    for i in range(e.auths_len):
        auth=e.auths[i];value=B.authorization_hold(C.byref(auth))
        if value:assert B.ledger_pair(C.byref(e),enc('OPENING_HOLD_'+str(i)),auth.account_id,B.HOLD_ASSET,B.HOLD_OFFSET,value)
    assert B.refresh_projections(C.byref(e))
    debt=C.c_int64();held=C.c_int64()
    assert B.reconcile(C.byref(e),b'A1',C.byref(debt),C.byref(held)),'unbalanced or inconsistent fixture opening state'

def new_engine(d):
    e=B.Engine();B.engine_init(C.byref(e));e.now=d.get('now',d.get('day',100)*1440+600)
    account=B.record(B.Account,id='A1',active=d.get('account.active',True),tier=ENUM('Tier',d.get('tier','REGULAR')),credit_limit=d.get('credit_limit',1000000),daily_limit=d.get('daily_limit',300000),per_operation_limit=d.get('per_operation_limit',200000),base_score=d.get('base_score',10),next_cycle_to_close=e.now//1440//e.config.cycle_days)
    B.append(e,'accounts',account)
    B.append(e,'cards',B.record(B.Card,id='C1',account_id='A1',active=d.get('card.status','ACTIVE')=='ACTIVE',expiry_day=d.get('card.expiry_day',500),allow_international=d.get('card.allow_international',True),allow_contactless=d.get('card.allow_contactless',True)))
    B.append(e,'merchants',B.record(B.Merchant,id='M1',active=d.get('merchant.status','ACTIVE')=='ACTIVE',category=ENUM('Category',d.get('merchant.category',d.get('category','NORMAL'))),country='BR'))
    for item in d.get('history',[]):
        B.append(e,'history',B.record(B.History,account_id=item.get('account_id','A1'),minute=item.get('minute',item.get('day',e.now//1440)*1440+600),decision=ENUM('Decision',item['decision']),principal=item.get('principal',0)))
    for i,item in enumerate(d.get('invoices',[])):
        iid='I'+str(i+1);p=item.get('outstanding',0)
        B.append(e,'invoices',B.record(B.Invoice,id=iid,account_id='A1',cycle=i,due_day=item['due_day'],issued_principal=p,issued_total=p))
        B.append(e,'lots',B.record(B.Lot,account_id='A1',capture_id='OPEN',invoice_id=iid,index=i,cycle=i,principal=p))
    if e.invoices_len:opening(e)
    return e

def config(d):
    c=B.Config();B.config_default(C.byref(c));return c

def admit(r,field='allowed'):return {field:r==B.NONE,'reason':reason(r)}

def seed_auth(e,d):
    p=d.get('principal',10000);fee=d.get('fee',0)
    a=B.record(B.Authorization,id='R1',account_id='A1',state=B.ACTIVE,principal=p,fee=fee,approved_at=e.now,expires_at=d.get('expires_at',e.now+e.config.hold_ttl_minutes),installments=d.get('n',d.get('installments',1)),tier=e.accounts[0].tier,category=B.NORMAL)
    B.append(e,'auths',a);opening(e)
    if d.get('captured',0):
        r=perform(e,'CAPTURE',request_id='PRECAP',auth_id='R1',principal=d['captured']);assert r.decision==B.OK,output(r)
    return e.auths[0]

def fault(e,label):
    mapping={'AUTH_STORAGE':'auths','IDEMPOTENCY_STORAGE':'keys','LOT_STORAGE':'lots','LEDGER_STORAGE':'ledger','HISTORY':'history','LEDGER':'ledger'}
    name=mapping[label];setattr(e.config,name+'_capacity',getattr(e,name+'_len'))

def summary(e,r=None,before=None):
    p=projection(e);out=dict(p);out.update(available=p['available_credit'],auth_count=e.auths_len,capture_count=e.captures_len,balanced=balanced(e),events_added=events(e))
    if r is not None:out.update(output(r))
    if e.auths_len:
        a=e.auths[0];out.update(state=state(a.state),auth_state=state(a.state),approved_at=a.approved_at,expires_at=a.expires_at)
    if before:
        out.update(unchanged=snapshot(e)==before['all'],financial_unchanged=snapshot(e,True)==before['financial'],count_delta=p['daily_count']-before['projection']['daily_count'],available_delta=p['available_credit']-before['projection']['available_credit'],events_added=events(e)-before['events'])
    return out

def before(e):return {'all':snapshot(e),'financial':snapshot(e,True),'projection':projection(e),'events':events(e),'ledger':ledger_totals(e)}

def probe(op,d):
    c=config(d);p=d.get('principal',10000);fee=d.get('fee',0);n=d.get('n',d.get('installments',1));tier=ENUM('Tier',d.get('tier','REGULAR'))
    if op=='money.validate':return {'ok':bool(B.money_valid(d['value'])),'error':'NONE' if B.money_valid(d['value']) else 'NUMERIC_RANGE'}
    if op in ('money.add','money.subtract','money.checked_mul'):
        return call_bool_value({'money.add':'money_add','money.subtract':'money_sub','money.checked_mul':'checked_mul'}[op],d['a'],d['b'])
    if op in ('money.floor_rate','money.ceil_rate','money.fx'):return call_bool_value({'money.floor_rate':'floor_rate','money.ceil_rate':'ceil_rate','money.fx':'money_fx'}[op],d['value'],d.get('bps',d.get('rate')))
    if op=='money.validate_rates':c.international_bps=d['bps'];c.fx_usd=d['fx_rate'];return {'ok':bool(B.config_valid(C.byref(c)))}
    if op in ('pricing.components','pricing.quote'):
        q=B.price_components(C.byref(c),p,n,enc(d.get('currency','BRL')),tier) if op.endswith('components') else B.quote(C.byref(c),C.byref(command('QUOTE',d)))
        out=B.decode(q);out.update(ok=q.reason==B.NONE,reason=reason(q.reason));return out
    if op=='limits.available':return {'available':B.available_credit(d['credit_limit'],d['debt'],d['held'])}
    if op=='limits.credit':return admit(B.credit_admission(p,fee,d['available']))
    if op=='limits.operation':return admit(B.operation_admission(p,d.get('per_operation_limit',200000)))
    if op=='limits.daily_amount':return admit(B.daily_admission(p,d.get('used',0),d.get('daily_limit',300000)))
    if op=='risk.score':return {'score':B.risk_score(d.get('base_score',10),enc(d.get('country','BR')),d.get('card_present',True),p,d.get('recent_count',0))}
    if op=='risk.classify':return {'decision':decision(B.risk_classify(C.byref(c),d['score']))}
    if op=='installment.admission':return {'allowed':bool(B.installment_allowed(C.byref(c),p,n,enc(d.get('country','BR')))),'reason':'INSTALLMENTS'}
    if op=='installment.capture_admission':return {'allowed':bool(B.capture_installment_allowed(C.byref(c),d['capture_principal'],n)),'reason':'INSTALLMENTS'}
    if op=='installment.split':return {'parts':[B.split_part(d['total'],n,i) for i in range(n)]}
    if op in ('installment.split_components','installment.conservation'):
        ps=[B.split_part(p,n,i) for i in range(n)];fs=[B.split_part(fee,n,i) for i in range(n)]
        return {'principal_parts':ps,'fee_parts':fs,'sum_principal':sum(ps),'sum_fee':sum(fs)}
    if op in ('capture.admission','capture.amount'):
        a=B.record(B.Authorization,state=ENUM('AuthState',d.get('state','ACTIVE')),principal=p,captured_principal=d.get('captured',0),expires_at=d.get('expires_at',146040))
        return admit(B.capture_admission(C.byref(a),d.get('now',144600),d.get('amount',1)))
    if op=='capture.next_state':return {'state':state(B.capture_state(p,d['captured_after']))}
    if op in ('capture.fee_delta','reversal.refund_fee','rewards.refund'):
        whole=d.get('capture_principal',p);total=d.get('capture_fee',d.get('granted',fee));prior=d.get('captured',d.get('refunded',0))
        out=call_bool_value('cumulative_delta',total,prior,d['amount'],whole)
        out[{'capture.fee_delta':'fee_delta','reversal.refund_fee':'fee_refund','rewards.refund':'reversed'}[op]]=out['value'];return out
    if op=='reversal.refund_amount':return admit(B.refund_admission(C.byref(B.record(B.Capture,principal=d['capture_principal'],refunded_principal=d['refunded'])),d['amount']))
    if op=='billing.due_day':return {'due_day':B.invoice_due_day(C.byref(c),d['cycle'])}
    if op=='billing.minimum_due':return {'minimum_due':B.invoice_minimum(C.byref(c),d['total'])}
    if op=='billing.close_time':return admit(B.close_admission(C.byref(c),d['day'],d['cycle'],d['cycle']))
    if op=='billing.payment_admission':return admit(B.payment_admission(d['payment'],d['invoiced']))
    if op=='rewards.raw':return {'raw_points':B.reward_raw(C.byref(c),p,tier,ENUM('Category',d.get('category','NORMAL')))}
    if op=='rewards.cap':return {'granted':B.reward_grant(C.byref(c),d['raw_points'],d['gross_granted'])}
    if op=='io.identifier':return {'valid':bool(B.identifier_valid(enc(d['id'])))}
    if op=='io.money_parse':return call_bool_value('decimal_parse',enc(d['text']),1000000000000)
    if op=='idempotency.error_policy':
        r=B.record(B.Result,decision=B.ERROR,reason=ENUM('Reason',d['reason']));return {'cache_response':bool(B.cacheable(C.byref(r)))}
    if op=='idempotency.canonical':
        commands=[]
        for payload in (d['first'],d['second']):
            q=B.Command();line='AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 '+' '.join(k+'='+str(v) for k,v in payload.items());assert B.parse_command(enc(line),C.byref(q))==1;commands.append(q)
        return {'same_payload':bool(B.command_equal(C.byref(commands[0]),C.byref(commands[1])))}
    if op=='flow.run' or op.startswith('io.') and op!='io.capacity':return cli_probe(op,d)
    e=new_engine(d)
    try:return engine_probe(op,d,e)
    finally:B.engine_free(C.byref(e))
def reward_history(e,identifier,cycle,granted,reversed=0):
    # COMPONENT input: gross grant/reversal scalars do not claim a publicly
    # reachable authorization. Structural parents preserve ownership and bounds.
    B.append(e,'auths',B.record(B.Authorization,id='P_'+identifier,account_id='A1',state=B.CAPTURED,principal=1000000000000,captured_principal=1000000000000,installments=1))
    B.append(e,'captures',B.record(B.Capture,id=identifier,auth_id='P_'+identifier,account_id='A1',principal=1000000000000,cycle=cycle,granted=granted,reversed=reversed))
    B.append(e,'rewards',B.record(B.Reward,account_id='A1',capture_id=identifier,cycle=cycle,granted=granted))
    validate_fixture(e)

def engine_probe(op,d,e):
    p=d.get('principal',10000);fee=d.get('fee',0);n=d.get('n',d.get('installments',1));c=e.config
    if op=='eligibility.check':return admit(B.eligibility(C.byref(e),C.byref(e.accounts[0]),C.byref(e.cards[0]),C.byref(e.merchants[0]),C.byref(command('AUTHORIZE',d))),'eligible')
    if op in ('risk.velocity','limits.day_projection','limits.daily_count','limits.record_decision'):
        if op=='limits.daily_count':
            for dec,count in [('APPROVED',d['approved']),('REVIEW',d['review']),('DECLINED',d['declined'])]:
                for i in range(count):B.append(e,'history',B.record(B.History,account_id='A1',minute=e.now,decision=ENUM('Decision',dec),principal=0))
        if op=='limits.record_decision':assert B.record_decision(C.byref(e),b'A1',ENUM('Decision',d['decision']),p)
        gross=C.c_int64();count=C.c_int64();B.daily_totals(C.byref(e),b'A1',e.now//1440,C.byref(gross),C.byref(count))
        if op=='limits.daily_count':return admit(B.count_admission(count.value,c.daily_count_limit))
        if op=='risk.velocity':
            count=B.recent_count(C.byref(e),b'A1');return {'recent_count':count,'component':B.risk_score(0,b'BR',True,0,count)}
        return {'gross_principal':gross.value,'count':count.value,'gross_delta':gross.value,'count_delta':count.value}
    if op in ('auth.decide','auth.approve','auth.quote_consistency','auth.capacity','io.capacity','pricing.purity','idempotency.query'):
        if 'fail_at' in d:fault(e,d['fail_at'])
        if op=='io.capacity':fault(e,d['resource'])
        snap=before(e);q=command('AUTHORIZE',d)
        if op in ('pricing.purity','idempotency.query'):q=command(d.get('op','QUOTE'),d)
        quote_result=perform(e,'QUOTE',**d) if op=='auth.quote_consistency' else None
        responses=[execute_checked(e,q) for _ in range(d.get('repeat',1))]
        out=summary(e,responses[-1],snap);out.update(auth_created=e.auths_len>0,same_response=all(B.decode(r)==B.decode(responses[0]) for r in responses),idempotency_delta=e.keys_len-len(snap['all']['keys']))
        if quote_result is not None:out['same_quote']=(quote_result.principal,quote_result.fee)==(responses[0].principal,responses[0].fee)
        return out
    if op=='idempotency.namespace':
        a=command(d['first_op'],{'request_id':d['request_id']});b=command(d['second_op'],{'request_id':d['request_id']})
        return {'same_key':bool(B.command_same_key(C.byref(a),C.byref(b)))}
    if op in ('idempotency.replay','idempotency.conflict','idempotency.declined_replay','idempotency.capture_replay'):
        if op=='idempotency.declined_replay':
            e.accounts[0].credit_limit=d.get('first_credit_limit',1000000);e.accounts[0].base_score=d.get('first_base_score',10)
        q=command('AUTHORIZE',dict(d,amount=d.get('first_amount',d.get('amount',p)),currency=d.get('first_currency',d.get('currency','BRL'))))
        first=execute_checked(e,q);responses=[first]
        if op=='idempotency.conflict':q.amount=d.get('second_amount',q.amount);q.currency=enc(d.get('second_currency',text(q.currency)))
        if op=='idempotency.declined_replay':e.accounts[0].credit_limit=d.get('later_credit_limit',e.accounts[0].credit_limit);e.accounts[0].base_score=d.get('later_base_score',e.accounts[0].base_score)
        if op=='idempotency.capture_replay':q=command('CAPTURE',{'request_id':'CAP1','auth_id':'R1','principal':p});responses=[]
        for i in range(d.get('repeat',2)-(0 if op=='idempotency.capture_replay' else 1)):responses.append(execute_checked(e,q))
        out=summary(e,responses[-1]);out.update(same_response=all(B.decode(r)==B.decode(responses[0]) for r in responses),first_decision=decision(first.decision),second_decision=decision(responses[-1].decision));return out
    if op=='limits.after_reversal':
        perform(e,'AUTHORIZE',amount=d['approved_principal'])
        if d['action']=='REFUND':perform(e,'CAPTURE',request_id='CAP1',auth_id='R1',principal=d['approved_principal']);perform(e,'REFUND',request_id='REF1',capture_id='CAP1',principal=d['approved_principal'])
        else:perform(e,'CANCEL',request_id='CAN1',auth_id='R1')
        return summary(e)
    if op.startswith('capture.') or op=='installment.schedule':
        if op=='installment.schedule':e.now=d['capture_day']*1440+600;e.accounts[0].next_cycle_to_close=d['capture_day']//c.cycle_days
        seed_auth(e,d)
        if 'fail_at' in d:fault(e,d['fail_at'])
        if 'current_base_score' in d:e.accounts[0].base_score=d['current_base_score']
        snap=before(e);r=perform(e,'CAPTURE',request_id='CAP1',auth_id='R1',principal=d.get('amount',p));out=summary(e,r,snap)
        after_ledger=ledger_totals(e)
        for key in ('RECEIVABLE_PRINCIPAL','RECEIVABLE_FEE','MERCHANT_CLEARING','FEE_REVENUE'):out[key.lower()+'_delta']=after_ledger[key]-snap['ledger'][key]
        if op=='installment.schedule':out.update(cycles=[e.lots[i].cycle for i in range(e.lots_len)],due_days=[B.invoice_due_day(C.byref(e.config),e.lots[i].cycle) for i in range(e.lots_len)])
        return out
    if op in ('reversal.cancel','reversal.cancel_terminal','reversal.expire'):
        setup=dict(d)
        if op=='reversal.expire':setup.update(principal=d.get('remaining_hold',0)+d.get('debt',0),captured=d.get('debt',0));e.now=min(d['now'],d['expires_at']-1)
        if d.get('state')!='MISSING':
            seed_auth(e,setup)
            if d.get('state')=='CAPTURED' and not d.get('debt',0):
                perform(e,'CAPTURE',request_id='COMPLETE',auth_id='R1',principal=p)
            if d.get('state')=='CANCELLED':perform(e,'CANCEL',request_id='PRECAN',auth_id='R1')
        snap=before(e)
        r=perform(e,'TICK',now=d['now']) if op=='reversal.expire' else perform(e,'CANCEL',request_id='CAN1',auth_id='R1')
        out=summary(e,r,snap);out['released']=snap['projection']['held']-projection(e)['held'];return out
    if op.startswith('reversal.refund_'):return refund_probe(op,d,e)
    if op.startswith('billing.'):return billing_probe(op,d,e)
    if op=='rewards.cycle_cap':
        for i,x in enumerate(d['grants']):reward_history(e,'OLD'+str(i),x['cycle'],x['points'])
        gross=B.reward_gross(C.byref(e),b'A1',d['capture_day']//c.cycle_days);return {'granted':B.reward_grant(C.byref(e.config),d['raw_points'],gross)}
    if op=='rewards.after_refund':
        reward_history(e,'OLD',3,d['gross_granted'],d['points_reversed'])
        gross=B.reward_gross(C.byref(e),b'A1',3);return {'gross_granted':gross,'new_granted':B.reward_grant(C.byref(e.config),d['new_raw_points'],gross)}
    if op=='rewards.capture_partition':
        seed_auth(e,{'principal':sum(d['captures'])})
        for i,amount in enumerate(d['captures']):
            r=perform(e,'CAPTURE',request_id='CAP'+str(i),auth_id='R1',principal=amount);assert r.decision==B.OK,output(r)
        return summary(e)
    if op.startswith('batch.'):return batch_probe(op,d,e)
    raise UnknownProbe(op)

def invoice_setup(e,principal=0,fee=0,late=0,due_day=130,assessed=False,cycle=3,minimum_due=None):
    B.append(e,'invoices',B.record(B.Invoice,id='I1',account_id='A1',cycle=cycle,due_day=due_day,issued_principal=principal,issued_fee=fee,issued_total=principal+fee,minimum_due=(minimum_due if minimum_due is not None else B.invoice_minimum(C.byref(e.config),principal+fee)),late=late,late_fee_assessed=assessed))
    B.append(e,'lots',B.record(B.Lot,account_id='A1',capture_id='CAP1',invoice_id='I1',index=0,cycle=cycle,principal=principal,fee=fee))

def billing_probe(op,d,e):
    if op=='billing.close_order':
        e.now=300*1440;e.accounts[0].next_cycle_to_close=d['next_cycle']
        if d.get('already_closed'):B.append(e,'invoices',B.record(B.Invoice,id='I1',account_id='A1',cycle=d['requested'],due_day=B.invoice_due_day(C.byref(e.config),d['requested'])))
        return summary(e,perform(e,'CLOSE',request_id='CLOSE1',account_id='A1',cycle=d['requested']))
    if op in ('billing.issue','billing.empty'):
        cycle=d['cycle'];e.now=(cycle+1)*e.config.cycle_days*1440;e.accounts[0].next_cycle_to_close=cycle
        for i,x in enumerate(d['lots']):B.append(e,'lots',B.record(B.Lot,account_id='A1',capture_id='CAP'+str(i),index=i,**x))
        opening(e);snap=before(e);r=perform(e,'CLOSE',request_id='CLOSE1',account_id='A1',cycle=cycle);out=summary(e,r,snap)
        if e.invoices_len:
            inv=e.invoices[0];out.update(principal=inv.issued_principal,fee=inv.issued_fee,total=inv.issued_total,minimum_due=inv.minimum_due,next_cycle=e.accounts[0].next_cycle_to_close)
        return out
    if op=='billing.allocate_payment':invoice_setup(e,d['principal'],d['fee'],d['late'])
    elif op=='billing.pay_invoiced':
        invoice_setup(e,d['invoiced']);B.append(e,'lots',B.record(B.Lot,account_id='A1',capture_id='CAP2',index=1,cycle=4,principal=d['future']))
    elif op=='billing.partial_payment':
        invoice_setup(e,d['issued_total'],due_day=d['due_day'],minimum_due=d['minimum_due'])
    elif op=='billing.assess_late_fee':invoice_setup(e,d['outstanding'],due_day=d['due_day'],assessed=d['assessed'])
    else:raise UnknownProbe(op)
    opening(e);snap=before(e)
    r=perform(e,'ASSESS_LATE_FEE',request_id='LATE1',invoice_id='I1') if op=='billing.assess_late_fee' else perform(e,'PAY',request_id='PAY1',account_id='A1',amount=d['payment'])
    out=summary(e,r,snap);inv=e.invoices[0];lot=e.lots[0]
    eligibility=B.eligibility(C.byref(e),C.byref(e.accounts[0]),C.byref(e.cards[0]),C.byref(e.merchants[0]),C.byref(command()))
    out.update(late_remaining=inv.late-inv.paid_late,fee_remaining=B.lot_fee(C.byref(lot)),principal_remaining=B.lot_principal(C.byref(lot)),invoiced_remaining=projection(e)['invoiced_outstanding'],future_remaining=projection(e)['future_outstanding'],issued_total=inv.issued_total,minimum_due=inv.minimum_due,outstanding=B.invoice_outstanding(C.byref(e),C.byref(inv)),past_due=eligibility==B.PAST_DUE,late_delta=ledger_totals(e)['RECEIVABLE_LATE']-snap['ledger']['RECEIVABLE_LATE'],assessed=bool(inv.late_fee_assessed));return out

def refund_probe(op,d,e):
    if op=='reversal.refund_closed_auth':
        amount=d['daily_gross_principal'];perform(e,'AUTHORIZE',amount=amount)
        r=perform(e,'CAPTURE',request_id='CAP1',auth_id='R1',principal=d['captured']);assert r.decision==B.OK,output(r)
        if d['auth_state']=='CANCELLED':perform(e,'CANCEL',request_id='CAN1',auth_id='R1')
        else:perform(e,'TICK',now=e.auths[0].expires_at)
        r=perform(e,'REFUND',request_id='REF1',capture_id='CAP1',principal=d['refund_principal']);out=summary(e,r)
        gross=C.c_int64();count=C.c_int64();B.daily_totals(C.byref(e),b'A1',100,C.byref(gross),C.byref(count));out['daily_gross_principal']=gross.value;return out
    if op=='reversal.refund_allocation':
        parts=d['lot_principal'];p=sum(parts);fee=0
        for i,principal in enumerate(parts):B.append(e,'lots',B.record(B.Lot,account_id='A1',capture_id='CAP1',index=i,cycle=3+i,principal=principal))
    elif op=='reversal.refund_paid':
        p=d['capture_principal'];fee=d['capture_fee'];B.append(e,'lots',B.record(B.Lot,account_id='A1',capture_id='CAP1',index=0,cycle=3,principal=p,fee=fee,paid_principal=p-d['unpaid_principal'],paid_fee=fee-d['unpaid_fee']))
    elif op=='reversal.refund_with_late_fee':
        p=d['unpaid_principal'];fee=d['unpaid_fee'];invoice_setup(e,p,fee,d['unpaid_late_fee'])
    else:raise UnknownProbe(op)
    B.append(e,'auths',B.record(B.Authorization,id='R1',account_id='A1',state=B.CAPTURED,principal=p,fee=fee,captured_principal=p,captured_fee=fee,installments=1,tier=B.REGULAR,category=B.NORMAL))
    B.append(e,'captures',B.record(B.Capture,id='CAP1',auth_id='R1',account_id='A1',principal=p,fee=fee,cycle=3));opening(e)
    r=perform(e,'REFUND',request_id='REF1',capture_id='CAP1',principal=d['refund_principal']);out=summary(e,r)
    out.update(debt_after=projection(e)['debt'],remaining_principal=[B.lot_principal(C.byref(e.lots[i])) for i in range(e.lots_len)],unpaid_principal=sum(B.lot_principal(C.byref(e.lots[i])) for i in range(e.lots_len)),unpaid_fee=sum(B.lot_fee(C.byref(e.lots[i])) for i in range(e.lots_len)),unpaid_late_fee=sum(e.invoices[i].late-e.invoices[i].paid_late for i in range(e.invoices_len)));return out
def batch_probe(op,d,e):
    if op in ('batch.order','batch.continue','batch.replay','batch.account_isolation'):
        if op=='batch.account_isolation':
            other=B.Account.from_buffer_copy(e.accounts[0]);other.id=b'A2';B.append(e,'accounts',other)
            card=B.Card.from_buffer_copy(e.cards[0]);card.id=b'C2';card.account_id=b'A2';B.append(e,'cards',card)
            account=d['op_account'];requests=[{'id':'R1','amount':d['amount'],'account_id':account,'card_id':'C1' if account=='A1' else 'C2'}]
        else:requests=d.get('authorizations',d.get('requests',[{'id':'R'+str(i),'amount':v} for i,v in enumerate(d.get('amounts',[]))]))
        responses=[perform(e,'AUTHORIZE',request_id=x['id'],amount=x['amount'],account_id=x.get('account_id','A1'),card_id=x.get('card_id','C1')) for x in requests]
        out=summary(e);out.update(decisions=[decision(r.decision) for r in responses],response_count=len(responses))
        if op=='batch.account_isolation':out.update(A1_held=projection(e,0)['held'],A2_held=projection(e,1)['held'])
        return out
    if op=='batch.reconcile':
        for i,(p,f) in enumerate(zip(d['lot_principal'],d['lot_fee'])):B.append(e,'lots',B.record(B.Lot,account_id='A1',capture_id='CAP'+str(i),index=i,cycle=3,principal=p,fee=f))
        for i,late in enumerate(d['late']):B.append(e,'invoices',B.record(B.Invoice,id='I'+str(i),account_id='A1',cycle=i,due_day=130,late=late))
        for i,held in enumerate(d['holds']):B.append(e,'auths',B.record(B.Authorization,id='R'+str(i),account_id='A1',state=B.ACTIVE,principal=held,expires_at=e.now+1440,installments=1))
        opening(e);e.accounts[0].cached_debt=d['cached_debt'];e.accounts[0].cached_held=d['cached_held']
        debt=C.c_int64();held=C.c_int64();sut=bool(B.reconcile(C.byref(e),b'A1',C.byref(debt),C.byref(held)))
        p=projection(e);ledger=ledger_totals(e);actual=(p['debt']==e.accounts[0].cached_debt==ledger['RECEIVABLE_PRINCIPAL']+ledger['RECEIVABLE_FEE']+ledger['RECEIVABLE_LATE'] and p['held']==e.accounts[0].cached_held==ledger['HOLD_ASSET'] and balanced(e));assert actual==sut
        return dict(p,reconciled=actual,reason='NONE' if actual else 'INVARIANT_VIOLATION')
    if op=='batch.balance_check':
        for i,event in enumerate(d['events']):
            for j,amount in enumerate(event):assert B.ledger_post(C.byref(e),enc('EV'+str(i)),b'A1',B.HOLD_ASSET if j==0 else B.HOLD_OFFSET,amount)
        return {'balanced':balanced(e)}
    if op=='batch.report_order':
        lines=['AUTHORIZE request_id='+x+' account_id=A1 card_id=C1 merchant_id=M1 amount=100' for x in d['ids']]+['GET_ACCOUNT account_id=A1']
        run,rows=run_cli(lines,d);assert run.returncode==0;return {'ids':rows[-1]['auth_ids']}
    if op=='batch.tick':
        e.now=d['current_now']
        for i,amount in enumerate(d.get('expiring_holds',[])):B.append(e,'auths',B.record(B.Authorization,id='R'+str(i),account_id='A1',state=B.ACTIVE,principal=amount,expires_at=d['requested_now'],installments=1))
        opening(e)
        if 'fail_at' in d:fault(e,d['fail_at'])
        snap=before(e);return summary(e,perform(e,'TICK',now=d['requested_now']),snap)
    raise UnknownProbe(op)

def seed_text(d):
    lines=['CLOCK now='+str(d.get('now',d.get('day',100)*1440+600))]
    if 'capacity.idempotency' in d:lines.append('CONFIG keys_capacity='+str(d['capacity.idempotency']))
    account={'id':'A1','active':d.get('account.active',True),'tier':d.get('tier','REGULAR'),'credit_limit':d.get('credit_limit',1000000),'daily_limit':d.get('daily_limit',300000),'per_operation_limit':d.get('per_operation_limit',200000),'base_score':d.get('base_score',10)}
    lines.append('ACCOUNT '+' '.join(k+'='+str(v).lower() if isinstance(v,bool) else k+'='+str(v) for k,v in account.items()))
    lines.extend(['CARD id=C1 account_id=A1','MERCHANT id=M1'])
    return '\n'.join(lines)+'\n'

def run_cli(lines,d=None,seed=None,environment=None):
    d=d or {};payload=lines if isinstance(lines,str) else '\n'.join(lines)+'\n'
    with tempfile.TemporaryDirectory(prefix='aurum-harness-') as directory:
        path=Path(directory)/'seed.txt';path.write_text(seed if seed is not None else seed_text(d))
        result=subprocess.run([os.environ.get('AURUM_BIN',str(ROOT/'build/aurum')),'--seed',str(path),'--commands','-'],input=payload.encode('ascii'),capture_output=True,env=environment)
    rows=[json.loads(line) for line in result.stdout.splitlines()]
    assert all(type(x)==dict for x in rows),'stdout must consist solely of JSON objects'
    return result,rows

def cli_probe(op,d):
    if op=='flow.run':
        # Every provided line executes through the public binary. A second real C
        # engine supplies the journal snapshot missing from the public protocol.
        # Public queries are checked against its projections before assertions.
        e=new_engine(d)
        try:
            if 'capacity.idempotency' in d:e.config.keys_capacity=d['capacity.idempotency']
            internal=[]
            for line in d['commands']:
                q=B.Command();assert B.parse_command(enc(line),C.byref(q))==1
                internal.append(execute_checked(e,q))
            queries=['GET_ACCOUNT account_id=A1']+['GET_AUTH auth_id='+text(e.auths[i].id) for i in range(e.auths_len)]+['GET_INVOICE invoice_id='+text(e.invoices[i].id) for i in range(e.invoices_len)]
            result,rows=run_cli(d['commands']+queries,d);assert result.returncode==0,result.stderr
            assert len(rows)==len(d['commands'])+len(queries)
            responses=rows[:len(d['commands'])]
            for public,private in zip(responses,internal):
                observed=output(private)
                for key in ('decision','reason'):assert public[key]==observed[key],(public,observed)
                for key in ('principal','fee','reserved','released','cash_refund','points','points_reversed'):
                    if key in public:assert public[key]==observed[key],(key,public,observed)
            observed=projection(e);account=rows[len(d['commands'])]
            for key,value in observed.items():assert account[key]==value,(key,account,observed)
            out=dict(account,responses=responses,decision_sequence=[r['decision'] for r in responses],auth_count=e.auths_len,capture_count=e.captures_len,balanced=balanced(e))
            pos=len(d['commands'])+1;out['auths']={}
            for i in range(e.auths_len):
                row=rows[pos+i];assert row['state']==state(e.auths[i].state);out['auths'][text(e.auths[i].id)]=row
            pos+=e.auths_len;out['invoices']={}
            for i in range(e.invoices_len):
                row=rows[pos+i];assert row['outstanding']==B.invoice_outstanding(C.byref(e),C.byref(e.invoices[i]));out['invoices'][text(e.invoices[i].id)]=row
            return out
        finally:B.engine_free(C.byref(e))
    if op=='io.parse':
        result,rows=run_cli([d['raw']],d);assert result.returncode==0;assert len(rows)==1;return rows[0]
    if op=='io.line_limit':
        line='QUOTE amount=100';line+=' '*(d['line_bytes']-len(line)-1);line+='\n'
        if 'next_line' in d:line+=d['next_line']+'\n'
        result,rows=run_cli(line);assert result.returncode==0
        return {'length_allowed':rows[0]['decision']=='OK','next_processed':len(rows)==2 and rows[1]['decision']=='OK' and rows[1]['principal']==100}
    if op=='io.ignored_lines':
        result,rows=run_cli(d['lines']);return {'response_count':len(rows),'decision':rows[0]['decision'] if rows else None}
    if op=='io.output':
        runs=[run_cli([d['raw']]) for _ in range(d['runs'])]
        assert all(r.returncode==0 for r,rows in runs)
        return {'response_count_per_run':len(runs[0][1]),'principal_is_integer':all(type(x['principal']) is int for r,rows in runs for x in rows),'identical_bytes':all(r.stdout==runs[0][0].stdout for r,rows in runs)}
    if op=='io.exit':
        result,rows=run_cli(['AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=200001'],seed='CLOCK now=invalid\n' if d['mode']=='INVALID_SEED' else None)
        if d['mode']=='INVALID_SEED':assert result.stderr and not rows
        else:assert rows[0]['decision']=='DECLINED'
        return {'exit_code':result.returncode}
    if op=='io.seed':
        seed=seed_text({})
        if d['defect']=='DUPLICATE_ACCOUNT':seed=seed.replace('CARD id=C1','ACCOUNT id=A1\nCARD id=C1')
        if d['defect']=='MISSING_CARD_OWNER':seed=seed.replace('CARD id=C1 account_id=A1','CARD id=C1 account_id=MISSING')
        if d['defect']=='INVALID_RATE':seed=seed.replace('ACCOUNT id=A1','CONFIG fx_usd=0\nACCOUNT id=A1')
        result,rows=run_cli(['QUOTE amount=100'],seed=seed);assert result.stderr
        return {'ok':result.returncode==0,'commands_processed':len(rows)}
    if op=='io.environment':
        variants=['UTC','America/Sao_Paulo'] if d['variation']=='TZ' else ['C','C.utf8']
        runs=[run_cli(['AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=101','TICK now=146040','GET_ACCOUNT account_id=A1'],environment=dict(os.environ,**{d['variation']:value})) for value in variants]
        assert all(r.returncode==0 for r,rows in runs)
        return {'identical_results':runs[0][0].stdout==runs[1][0].stdout}
    raise UnknownProbe(op)
