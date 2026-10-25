#!/usr/bin/env python3
"""Independent mathematical oracles and adversarial tests against the public C CLI."""
from __future__ import annotations
import argparse
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GROUPS = {}

def group(fn):
    GROUPS[fn.__name__] = fn
    return fn

def expect(actual, **expected):
    for key, value in expected.items():
        got = actual.get(key, '<missing>')
        if type(got) is not type(value) or got != value:
            raise AssertionError(f'{key}: expected {value!r}, got {got!r}; response={actual}')

def seed(config='', account='', card='', now=144600, second=False):
    lines = [f'CLOCK now={now}']
    if config:
        lines.append(f'CONFIG {config}')
    lines += [f'ACCOUNT id=A1 {account}']
    if second:
        lines += ['ACCOUNT id=A2']
    lines += [f'CARD id=C1 account_id=A1 {card}']
    if second:
        lines += ['CARD id=C2 account_id=A2']
    lines += ['MERCHANT id=M1']
    return '\n'.join(lines) + '\n'

def auth(id='A', amount=10000, extra='', account='A1', card='C1'):
    return f'AUTHORIZE request_id={id} account_id={account} card_id={card} merchant_id=M1 amount={amount} {extra}'.strip()

def capture(id='X', authorization='A', amount=10000):
    return f'CAPTURE request_id={id} auth_id={authorization} principal={amount}'

def refund(id='F', captured='X', amount=10000):
    return f'REFUND request_id={id} capture_id={captured} principal={amount}'

ACCOUNT = 'GET_ACCOUNT account_id=A1'

class CLI:
    def __init__(self, binary):
        self.binary = str(Path(binary).resolve())
        self.processes = 0
        self.responses = 0

    def run(self, commands, setup=None, reconcile=True):
        if isinstance(commands, str):
            raw = commands
        else:
            raw = '\n'.join(commands) + '\n'
        if reconcile:
            raw += 'RECONCILE\n'
        with tempfile.TemporaryDirectory(prefix='aurum-adversarial-') as directory:
            path = Path(directory) / 'seed.txt'
            path.write_text(setup or seed(), encoding='ascii')
            proc = subprocess.run([self.binary, '--seed', str(path), '--commands', '-'],
                                  input=raw.encode('ascii'), capture_output=True, timeout=30)
        self.processes += 1
        if proc.returncode != 0:
            raise AssertionError(f'CLI exit {proc.returncode}: {proc.stderr.decode(errors="replace")}')
        if proc.stderr:
            raise AssertionError(f'unexpected stderr: {proc.stderr.decode(errors="replace")}')
        rows = [json.loads(line) for line in proc.stdout.splitlines()]
        self.responses += len(rows)
        if reconcile:
            expect(rows.pop(), op='RECONCILE', decision='OK', status='OK', debt_difference=0, held_difference=0)
        return rows


def reference_quote(amount, currency, tier, installments, country, *, usd=50000, eur=60000,
                    international=200, installment=50, cap=5000, minimum=500):
    """Specification math in arbitrary precision, never calling or reading the SUT."""
    rates = {'BRL':10000, 'USD':usd, 'EUR':eur}
    if currency not in rates:
        return {'decision':'DECLINED', 'reason':'UNSUPPORTED_CURRENCY'}
    principal = (amount * rates[currency] + 5000) // 10000
    if principal > 10**12:
        return {'decision':'ERROR', 'reason':'NUMERIC_RANGE'}
    allowed = 1 <= installments <= 12 and (country == 'BR' or installments <= 3)
    allowed = allowed and principal > 0 and (installments == 1 or principal // installments >= minimum)
    if not allowed:
        return {'decision':'DECLINED', 'reason':'INSTALLMENTS'}
    foreign = 0 if currency == 'BRL' or tier == 'PREMIUM' else principal * international // 10000
    parcel = principal * installment * (installments - 1) // 10000
    return {'decision':'OK', 'reason':'NONE', 'principal':principal, 'fee':min(cap, foreign + parcel)}

@group
def quote_reference(cli):
    profiles = [('', {}), ('fx_usd=12345 fx_eur=54321 international_bps=317 installment_bps_per_extra=73 tariff_cap=1234 minimum_installment=17',
                dict(usd=12345, eur=54321, international=317, installment=73, cap=1234, minimum=17))]
    for config, params in profiles:
        commands, expected = [], []
        for amount in (1, 1499, 1500, 10001, 200000, 10**12):
            for currency in ('BRL', 'USD', 'EUR', 'ZZZ'):
                for tier in ('REGULAR', 'PREMIUM'):
                    for n, country in ((0,'BR'),(1,'US'),(3,'US'),(4,'US'),(12,'BR'),(13,'BR')):
                        commands.append(f'QUOTE amount={amount} currency={currency} tier={tier} installments={n} country={country}')
                        expected.append(reference_quote(amount,currency,tier,n,country,**params))
        rows = cli.run(commands, seed(config))
        if len(rows) != len(expected):
            raise AssertionError('quote response count differs')
        for row, oracle in zip(rows, expected):
            expect(row, **oracle)

@group
def credit_boundaries(cli):
    for credit, decision in ((10199,'DECLINED'),(10200,'APPROVED'),(10201,'APPROVED')):
        rows = cli.run([auth(amount=2000,extra='currency=USD'), ACCOUNT], seed(account=f'credit_limit={credit}'))
        expect(rows[0], decision=decision, reason='NONE' if decision=='APPROVED' else 'CREDIT_LIMIT')
        expect(rows[1], held=0 if decision=='DECLINED' else 10200, debt=0)
    rows = cli.run([auth('A',40000,'currency=USD'),auth('B',200001),ACCOUNT])
    expect(rows[0], decision='APPROVED', principal=200000, fee=4000)
    expect(rows[1], decision='DECLINED', reason='OPERATION_LIMIT')
    expect(rows[2], held=204000, daily_gross_principal=200000, daily_count=1)

@group
def daily_gross(cli):
    rows = cli.run([auth('A',6000),'CANCEL request_id=C auth_id=A',auth('B',4000),auth('C',1),ACCOUNT], seed(account='daily_limit=10000'))
    expect(rows[2],decision='APPROVED')
    expect(rows[3],decision='DECLINED',reason='DAILY_AMOUNT')
    expect(rows[4],held=4000,daily_gross_principal=10000,daily_count=2)
    rows = cli.run([auth(str(i),1) for i in range(4)] + [ACCOUNT], seed(config='daily_count_limit=3'))
    assert [r['decision'] for r in rows[:4]] == ['APPROVED']*3 + ['DECLINED']
    expect(rows[3], reason='DAILY_COUNT')
    expect(rows[4], daily_count=3, daily_gross_principal=3)

@group
def eligibility(cli):
    rows = cli.run([ACCOUNT,auth(),ACCOUNT], seed(account='active=false credit_limit=0 base_score=99'))
    expect(rows[1],decision='DECLINED',reason='ACCOUNT_INACTIVE',principal=0,fee=0)
    assert rows[0] == rows[2]
    rows = cli.run([auth('A',10000,'currency=USD pin_ok=false'),auth('B',10001,'currency=USD pin_ok=false')])
    expect(rows[0], decision='APPROVED', principal=50000)
    expect(rows[1], decision='DECLINED', reason='PIN_REQUIRED', principal=0)
    rows = cli.run([auth('A',10000,'country=US'),auth('B',2000,'currency=USD')], seed(card='allow_international=false'))
    expect(rows[0], decision='DECLINED', reason='INTERNATIONAL_DISABLED')
    expect(rows[1], decision='APPROVED')

@group
def risk_window(cli):
    for score, decision in ((49,'APPROVED'),(50,'REVIEW'),(79,'REVIEW'),(80,'DECLINED')):
        rows = cli.run([auth(amount=10),ACCOUNT],seed(account=f'base_score={score}'))
        expect(rows[0],decision=decision)
        expect(rows[1],held=10 if decision=='APPROVED' else 0,daily_count=0 if decision=='DECLINED' else 1)
    commands = [auth(str(i),10) for i in range(4)]
    commands += ['TICK now=144660',auth('D',10),'TICK now=144661',auth('E',10),ACCOUNT]
    rows = cli.run(commands,seed(account='base_score=30'))
    assert [r['decision'] for r in rows[:4]] == ['APPROVED']*3+['REVIEW']
    expect(rows[5],decision='REVIEW')
    expect(rows[7],decision='APPROVED')
    expect(rows[8],held=40,daily_gross_principal=40,daily_count=6)

@group
def capture_fragments(cli):
    rows = cli.run([auth(extra='installments=2'),ACCOUNT,capture('X','A',3333),capture('Y','A',3333),capture('Z','A',3334),'GET_AUTH auth_id=A',ACCOUNT],
                   seed(config='installment_bps_per_extra=3'))
    expect(rows[1],held=10003,debt=0,points=0)
    assert [row['fee'] for row in rows[2:5]] == [0,1,2]
    expect(rows[5],state='CAPTURED',captured_principal=10000,captured_fee=3,remaining_hold=0)
    expect(rows[6],held=0,debt=10003,points=0,available_credit=989997)

@group
def refund_fragments(cli):
    commands=[auth(amount=2000,extra='currency=USD'),capture()]
    commands += [refund('F1','X',3333),refund('F2','X',3333),refund('F3','X',3334),ACCOUNT]
    rows=cli.run(commands)
    assert [r['fee'] for r in rows[2:5]] == [66,67,67]
    assert [r['points_reversed'] for r in rows[2:5]] == [0,0,1]
    expect(rows[-1],debt=0,points=0,held=0,cash_refund_total=0,daily_gross_principal=10000)
    commands=[auth(extra='installments=2'),capture(),refund('F1','X',3333),refund('F2','X',3333),refund('F3','X',3334),ACCOUNT]
    rows=cli.run(commands,seed(config='installment_bps_per_extra=3'))
    assert [r['fee'] for r in rows[2:5]] == [0,1,2]
    expect(rows[-1],debt=0,points=0)

@group
def future_refund(cli):
    rows=cli.run([auth(amount=10001,extra='installments=3'),capture(amount=10001),'TICK now=172800',
        'CLOSE request_id=CL account_id=A1 cycle=3','PAY request_id=P account_id=A1 amount=3368',
        refund(amount=5000),'GET_INVOICE invoice_id=I1',ACCOUNT,refund('F2','X',5001),ACCOUNT])
    expect(rows[3],issued_total=3368,minimum_due=1000)
    expect(rows[5],fee=49,cash_refund=0)
    expect(rows[6],issued_total=3368,outstanding=0)
    expect(rows[7],debt=1684,invoiced_outstanding=0,future_outstanding=1684,cash_refund_total=0)
    expect(rows[8],fee=51,cash_refund=3368)
    expect(rows[9],debt=0,cash_refund_total=3368)

@group
def payment_priority(cli):
    prefix=[auth(amount=2000,extra='currency=USD'),capture(),'TICK now=172800','CLOSE request_id=CL account_id=A1 cycle=3']
    rows=cli.run(prefix+['PAY request_id=P account_id=A1 amount=10000',refund(amount=100),ACCOUNT])
    expect(rows[5],fee=2,cash_refund=2)
    expect(rows[6],debt=100,cash_refund_total=2)
    rows=cli.run(prefix+['TICK now=188640','ASSESS_LATE_FEE request_id=L invoice_id=I1',
        'PAY request_id=P account_id=A1 amount=1000',refund(),ACCOUNT])
    expect(rows[5],decision='OK',fee=1000)
    expect(rows[7],cash_refund=0)
    expect(rows[8],debt=0,cash_refund_total=0)

@group
def reward_cap(cli):
    commands=[auth('A',20000),capture('X','A',20000),refund('F1','X',10000),
              auth('B',20000),capture('Y','B',20000),refund('F2','X',10000),
              auth('C',10000),capture('Z','C',10000),ACCOUNT,'TICK now=173400',
              auth('D',10000),capture('W','D',10000),ACCOUNT]
    rows=cli.run(commands,seed(config='reward_month_cap=3'))
    assert [rows[i]['points'] for i in (1,4,7,11)] == [2,1,0,1]
    expect(rows[8],points=1,debt=30000)
    expect(rows[12],points=2,debt=40000,daily_count=1)

@group
def capture_partition(cli):
    setup=seed(config='installment_bps_per_extra=3')
    single=cli.run([auth(extra='installments=2'),capture(),ACCOUNT],setup)[-1]
    split=cli.run([auth(extra='installments=2'),capture('X','A',3333),capture('Y','A',3333),capture('Z','A',3334),ACCOUNT],setup)[-1]
    expect(single,debt=10003,held=0,points=1)
    expect(split,debt=10003,held=0,points=0)
    for key in ('debt','held','available_credit','daily_gross_principal','daily_count'):
        assert single[key] == split[key]

@group
def replay(cli):
    commands=[auth(),auth(),capture(amount=6000),capture(amount=6000),
       'CANCEL request_id=C auth_id=A','CANCEL request_id=C auth_id=A',
       refund(amount=1000),refund(amount=1000),'TICK now=172800',
       'CLOSE request_id=CL account_id=A1 cycle=3','CLOSE request_id=CL account_id=A1 cycle=3',
       'PAY request_id=P account_id=A1 amount=1000','PAY request_id=P account_id=A1 amount=1000',
       'TICK now=188640','ASSESS_LATE_FEE request_id=L invoice_id=I1','ASSESS_LATE_FEE request_id=L invoice_id=I1',
       'PAY request_id=P account_id=A1 amount=1001',ACCOUNT]
    rows=cli.run(commands)
    for a,b in ((0,1),(2,3),(4,5),(6,7),(9,10),(11,12),(14,15)):
        assert rows[a] == rows[b], (a,b,rows[a],rows[b])
    expect(rows[16],decision='ERROR',reason='IDEMPOTENCY_CONFLICT')
    expect(rows[17],debt=5000,held=0,daily_count=0)
    rows=cli.run([auth('Q'),capture('Q','Q'),ACCOUNT])
    expect(rows[1],decision='OK',capture_id='Q')
    expect(rows[2],debt=10000,points=1)
    normal=auth()
    canonical='AUTHORIZE amount=010000 merchant_id=M1 card_id=C1 request_id=A account_id=A1 installments=01 currency=BRL'
    rows=cli.run([normal,ACCOUNT,canonical,ACCOUNT])
    assert rows[0]==rows[2] and rows[1]==rows[3]

@group
def declined_replay(cli):
    rows=cli.run([auth('A'),auth('B',1),'CANCEL request_id=C auth_id=A',auth('B',1),auth('D',1),ACCOUNT],seed(account='credit_limit=10000'))
    expect(rows[1],decision='DECLINED',reason='CREDIT_LIMIT')
    assert rows[1] == rows[3]
    expect(rows[4],decision='APPROVED')
    expect(rows[5],held=1,daily_count=2,daily_gross_principal=10001)

@group
def capacity(cli):
    scenarios=[('keys_capacity=1',auth(),capture()),('lots_capacity=1',auth(extra='installments=2'),capture()),
               ('ledger_capacity=2',auth(),capture()),('history_capacity=1',auth(),auth('B')),
               ('auths_capacity=1',auth(),auth('B'))]
    for config,first,failing in scenarios:
        rows=cli.run([first,ACCOUNT,failing,ACCOUNT,failing,ACCOUNT],seed(config=config))
        expect(rows[2],decision='ERROR',reason='CAPACITY')
        expect(rows[4],decision='ERROR',reason='CAPACITY')
        assert rows[1] == rows[3] == rows[5], config
    rows=cli.run([auth(amount=20000),capture(amount=10000),ACCOUNT,capture('Y','A',10000),ACCOUNT],seed(config='rewards_capacity=1'))
    expect(rows[3],decision='ERROR',reason='CAPACITY')
    assert rows[2] == rows[4]
    rows=cli.run([auth('Z'),auth('A'),ACCOUNT,'TICK now=146040',ACCOUNT,'GET_AUTH auth_id=Z','GET_AUTH auth_id=A','TICK now=144600'],seed(config='ledger_capacity=7'))
    expect(rows[3],decision='ERROR',reason='CAPACITY')
    assert rows[2] == rows[4]
    expect(rows[5],state='ACTIVE'); expect(rows[6],state='ACTIVE'); expect(rows[7],decision='OK')
    rows=cli.run([auth(),capture(),refund(amount=1),ACCOUNT,refund('F2','X',1),ACCOUNT],seed(config='refunds_capacity=1'))
    expect(rows[4],decision='ERROR',reason='CAPACITY'); assert rows[3] == rows[5]
    prefix=[auth(),capture(),'TICK now=172800','CLOSE request_id=C account_id=A1 cycle=3','PAY request_id=P account_id=A1 amount=1']
    rows=cli.run(prefix+[ACCOUNT,'PAY request_id=P2 account_id=A1 amount=1',ACCOUNT],seed(config='payments_capacity=1'))
    expect(rows[6],decision='ERROR',reason='CAPACITY'); assert rows[5] == rows[7]

@group
def expiration(cli):
    setup=seed(config='hold_ttl_minutes=7')
    rows=cli.run([auth(),'TICK now=144606','GET_AUTH auth_id=A',capture(amount=4000),'TICK now=144607',
         'GET_AUTH auth_id=A',capture('Y','A',1),ACCOUNT,'TICK now=144607'],setup)
    expect(rows[1],released=0); expect(rows[2],state='ACTIVE')
    expect(rows[4],released=6000); expect(rows[5],state='EXPIRED',captured_principal=4000)
    expect(rows[6],decision='DECLINED',reason='AUTH_STATE'); expect(rows[7],held=0,debt=4000)
    expect(rows[8],released=0)
    direct=cli.run([auth(),'TICK now=144607',ACCOUNT],setup)[-1]
    stepped=cli.run([auth(),'TICK now=144601','TICK now=144605','TICK now=144607',ACCOUNT],setup)[-1]
    assert direct == stepped

@group
def independent_accounts(cli):
    setup=seed(second=True)
    a=[auth('A',12000),capture('X','A',5000)]
    b=[auth('B',7000,account='A2',card='C2'),capture('Y','B',3000)]
    queries=[ACCOUNT,'GET_ACCOUNT account_id=A2']
    first=cli.run(a+b+queries,setup)[-2:]
    swapped=cli.run(b+a+queries,setup)[-2:]
    assert first == swapped
    expect(first[0],debt=5000,held=7000,daily_count=1)
    expect(first[1],debt=3000,held=4000,daily_count=1)
    interleaved=cli.run([a[0],ACCOUNT,'QUOTE amount=1',a[1]]+b+queries,setup)[-2:]
    assert first == interleaved

@group
def line_boundary(cli):
    prefix=auth()
    valid=prefix+' '*(8192-len(prefix)-1)+'\n'
    invalid=prefix+' '*(8193-len(prefix)-1)+'\n'
    rows=cli.run(valid+ACCOUNT+'\n')
    expect(rows[0],decision='APPROVED'); expect(rows[1],held=10000)
    rows=cli.run(invalid+ACCOUNT+'\n'+auth()+'\n')
    assert len(rows)==3
    expect(rows[0],decision='ERROR',reason='INVALID_INPUT'); expect(rows[1],held=0,daily_count=0)
    expect(rows[2],decision='APPROVED')
    rows=cli.run('QUOTE amount=1\r\n\t # ignored\r\n\r\nQUOTE amount=2\n')
    assert [r['principal'] for r in rows]==[1,2]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--binary',default=str(ROOT/'build/aurum'))
    parser.add_argument('--group',choices=sorted(GROUPS),action='append')
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    cli=CLI(args.binary)
    selected=args.group or list(GROUPS)
    results=[]
    for name in selected:
        try:
            GROUPS[name](cli)
            results.append({'group':name,'status':'PASS'})
        except Exception as exc:
            results.append({'group':name,'status':'FAIL','error':str(exc)})
    report={'status':'PASS' if all(r['status']=='PASS' for r in results) else 'FAIL',
            'groups':results,'processes':cli.processes,'responses':cli.responses}
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    return 0 if report['status']=='PASS' else 1

if __name__=='__main__':
    raise SystemExit(main())
