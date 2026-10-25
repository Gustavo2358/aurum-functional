#!/usr/bin/env python3
"""Focused regressions from the final API and numeric-bound audit."""
from __future__ import annotations
import argparse
import ctypes as C
import json
from pathlib import Path
import bindings as b
from adversarial import CLI, ACCOUNT, auth, capture, refund, expect, seed

ROOT=Path(__file__).resolve().parents[1]
MAXIMUM=10**12


def serialized(e,command,result):
    """Observe the production serializer through an actual C FILE stream."""
    libc=C.CDLL(None)
    libc.tmpfile.argtypes=[]; libc.tmpfile.restype=C.c_void_p
    libc.fflush.argtypes=[C.c_void_p]; libc.fflush.restype=C.c_int
    libc.fseek.argtypes=[C.c_void_p,C.c_long,C.c_int]; libc.fseek.restype=C.c_int
    libc.fread.argtypes=[C.c_void_p,C.c_size_t,C.c_size_t,C.c_void_p]; libc.fread.restype=C.c_size_t
    libc.fclose.argtypes=[C.c_void_p]; libc.fclose.restype=C.c_int
    b.lib.write_result.argtypes=[C.c_void_p,C.POINTER(b.Engine),C.POINTER(b.Command),C.POINTER(b.Result)]
    b.lib.write_result.restype=None
    stream=libc.tmpfile()
    if not stream:
        raise AssertionError('tmpfile failed')
    try:
        b.lib.write_result(stream,C.byref(e),C.byref(command) if command is not None else None,C.byref(result))
        assert libc.fflush(stream)==0 and libc.fseek(stream,0,0)==0
        buf=C.create_string_buffer(2048)
        count=libc.fread(buf,1,len(buf),stream)
        assert 0<count<len(buf)
        return json.loads(buf.raw[:count])
    finally:
        assert libc.fclose(stream)==0


def malformed_api():
    e=b.Engine(); b.engine_init(C.byref(e))
    before=C.string_at(C.byref(e),C.sizeof(e))
    cases=0
    try:
        required=('request_id','account_id','card_id','merchant_id','currency','country')
        for field in required:
            command=b.Command(); b.command_default(C.byref(command),b.AUTHORIZE)
            command.request_id=b'R'; command.account_id=b'A1'; command.card_id=b'C1'; command.merchant_id=b'M1'; command.amount=10000
            size=4 if field=='currency' else 3 if field=='country' else 49
            C.memset(C.addressof(command)+getattr(b.Command,field).offset,ord('X'),size)
            result=b.execute(C.byref(e),C.byref(command))
            assert result.decision==b.ERROR and result.reason==b.INVALID_INPUT
            assert len(result.request_id)<49, 'error Result must retain a terminated public identifier'
            row=serialized(e,command,result)
            expect(row,decision='ERROR',reason='INVALID_INPUT')
            if field=='request_id':
                assert 'request_id' not in row, 'malformed ID must be omitted, never truncated or overread'
            else:
                expect(row,request_id='R')
            assert C.string_at(C.byref(e),C.sizeof(e))==before
            cases+=1
        for operation,field in ((b.CAPTURE,'auth_id'),(b.CANCEL,'auth_id'),(b.REFUND,'capture_id'),(b.ASSESS_LATE_FEE,'invoice_id')):
            command=b.Command(); b.command_default(C.byref(command),operation)
            command.request_id=b'R'; command.principal=1
            C.memset(C.addressof(command)+getattr(b.Command,field).offset,ord('X'),49)
            result=b.execute(C.byref(e),C.byref(command))
            assert result.decision==b.ERROR and result.reason==b.INVALID_INPUT
            expect(serialized(e,command,result),decision='ERROR',reason='INVALID_INPUT',request_id='R')
            assert C.string_at(C.byref(e),C.sizeof(e))==before
            cases+=1
        result=b.execute(C.byref(e),None)
        assert result.decision==b.ERROR and result.reason==b.INVALID_INPUT
        expect(serialized(e,None,result),op='INVALID',decision='ERROR',reason='INVALID_INPUT')
        assert C.string_at(C.byref(e),C.sizeof(e))==before
        cases+=1
    finally:
        b.engine_free(C.byref(e))
    return cases


def numeric_edges(cli):
    maximum_account=f'credit_limit={MAXIMUM} daily_limit={MAXIMUM} per_operation_limit={MAXIMUM} base_score=0'
    setup=seed(config='late_fee=1',account=maximum_account)
    commands=[auth(amount=MAXIMUM),capture(amount=MAXIMUM),'TICK now=172800',
        'CLOSE request_id=C account_id=A1 cycle=3','TICK now=188640',ACCOUNT,
        'ASSESS_LATE_FEE request_id=L invoice_id=I1',ACCOUNT,'GET_INVOICE invoice_id=I1',
        refund(amount=1),'ASSESS_LATE_FEE request_id=L invoice_id=I1',ACCOUNT]
    rows=cli.run(commands,setup)
    expect(rows[0],decision='APPROVED',reserved=MAXIMUM)
    expect(rows[6],decision='ERROR',reason='NUMERIC_RANGE')
    assert rows[5]==rows[7], 'numeric aggregate failure must preserve full account projection'
    expect(rows[8],issued_total=MAXIMUM,outstanding=MAXIMUM,late_fee_assessed=False)
    expect(rows[9],decision='OK',principal=1)
    expect(rows[10],decision='OK',fee=1)
    expect(rows[11],debt=MAXIMUM,held=0)

    setup=seed(config=f'international_bps=10000 installment_bps_per_extra=10000 tariff_cap={MAXIMUM}',account=maximum_account)
    commands=[f'QUOTE amount={MAXIMUM} installments=12',auth(amount=MAXIMUM,extra='installments=12'),ACCOUNT,auth(amount=1),ACCOUNT]
    rows=cli.run(commands,setup)
    expect(rows[0],decision='OK',principal=MAXIMUM,fee=MAXIMUM)
    expect(rows[1],decision='ERROR',reason='NUMERIC_RANGE')
    expect(rows[2],held=0,debt=0,daily_count=0,daily_gross_principal=0)
    expect(rows[3],decision='APPROVED',principal=1,fee=0)
    expect(rows[4],held=1,daily_count=1)

    setup=seed(account=maximum_account,second=True).replace('ACCOUNT id=A2\n',f'ACCOUNT id=A2 {maximum_account}\n')
    rows=cli.run([auth('Z',MAXIMUM),auth('A',MAXIMUM,account='A2',card='C2'),
                  'TICK now=146040',ACCOUNT,'GET_ACCOUNT account_id=A2'],setup)
    expect(rows[2],decision='OK',released=2*MAXIMUM)
    expect(rows[3],held=0,debt=0); expect(rows[4],held=0,debt=0)

    rows=cli.run(['CLOSE request_id=C account_id=A1 cycle=9223372036854775807',ACCOUNT])
    expect(rows[0],decision='ERROR',reason='CYCLE_ORDER')
    expect(rows[1],next_cycle_to_close=3,debt=0,held=0)

    setup=seed(config='hold_ttl_minutes=10080',now=525600000,card='expiry_day=9223372036854775807')
    rows=cli.run([auth(),'GET_AUTH auth_id=A','TICK now=525600000',capture(),ACCOUNT],setup)
    expect(rows[1],approved_at=525600000,expires_at=525610080,state='ACTIVE')
    expect(rows[2],decision='OK',released=0)
    expect(rows[3],decision='OK'); expect(rows[4],debt=10000,held=0)
    return 5


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--binary',default=str(ROOT/'build/aurum'))
    parser.add_argument('--report',type=Path,default=ROOT/'artifacts/regression-audit.json')
    args=parser.parse_args()
    cli=CLI(args.binary)
    results=[]
    for name,fn in (('malformed_c_api',malformed_api),('numeric_transaction_edges',lambda:numeric_edges(cli))):
        try:
            cases=fn()
            results.append({'group':name,'status':'PASS','cases':cases})
        except Exception as exc:
            results.append({'group':name,'status':'FAIL','error':str(exc)})
    report={'status':'PASS' if all(r['status']=='PASS' for r in results) else 'FAIL',
            'groups':results,'cli_processes':cli.processes,'cli_responses':cli.responses}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    return 0 if report['status']=='PASS' else 1

if __name__=='__main__':
    raise SystemExit(main())
