#!/usr/bin/env python3
"""Official Gherkin compilation, strict bindings, and literal oracle comparisons."""
import argparse,json,sys,traceback,os
from pathlib import Path
from gherkin.parser import Parser
from gherkin.pickles.compiler import Compiler
from probes import probe, UnknownProbe
ROOT=Path(__file__).resolve().parents[1]
PROBE_MAP=json.loads((ROOT/'harness/probe-map.json').read_text())
def table(step):
    rows=step['argument']['dataTable']['rows']
    assert [c['value'] for c in rows[0]['cells']]==['campo','valor']
    out={}
    for row in rows[1:]:
        key,value=[c['value'] for c in row['cells']]
        assert key not in out, 'duplicate table key'
        out[key]=json.loads(value)
    return out

def compare(actual,expected):
    failures=[]
    for key,want in expected.items():
        got=actual
        try:
            if key in actual:got=actual[key]
            else:
                for component in key.split('.'):
                    got=got[int(component)] if isinstance(got,list) else got[component]
        except (KeyError,IndexError,TypeError):
            failures.append(f'{key}: missing (expected {want!r})');continue
        if type(got) is not type(want) or got!=want:failures.append(f'{key}: got {got!r}, expected {want!r}')
    if failures:raise AssertionError('; '.join(failures))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--family',action='append');parser.add_argument('--self-test',action='store_true');parser.add_argument('--report',default='build/gherkin-results.json');args=parser.parse_args()
    results=[]
    for path in sorted((ROOT/'docs/oracle/features').glob('*.feature')):
        if args.family and path.stem not in args.family:continue
        document=Parser().parse(path.read_text());document['uri']=str(path.relative_to(ROOT))
        for case in Compiler().compile(document):
            rec={'name':case['name'],'uri':case['uri'],'line':case['location']['line'],'tags':[x['name'] for x in case['tags']]}
            try:
                steps=case['steps'];assert len(steps)==3,'undefined step layout'
                assert steps[0]['text']=='o perfil "base" e os seguintes dados','undefined Given'
                assert steps[2]['text']=='observo os seguintes resultados','undefined Then'
                action=steps[1]['text'];assert action.startswith('avalio "') and action.endswith('"'),'undefined When'
                op=action[len('avalio "'):-1];inputs=table(steps[0]);expected=table(steps[2]);assert expected,'empty oracle'
                rec.update(operation=op,kind=PROBE_MAP[op]['kind'])
                actual=probe(op,inputs);compare(actual,expected);rec['status']='PASS'
            except Exception as error:
                rec.update(status='FAIL',error=f'{type(error).__name__}: {error}')
                print(f"FAIL {rec['uri']}:{rec['line']} {rec['name']}: {rec['error']}",flush=True)
                if os.environ.get('AURUM_TRACEBACK'):traceback.print_exc()
            results.append(rec)
    assert results,'no scenarios selected'
    if args.self_test:
        actual=probe('money.add',{'a':7,'b':3})
        try:compare(actual,{'value':11})
        except AssertionError:results.append({'name':'runner rejects divergent oracle','status':'PASS','kind':'HARNESS'})
        else:results.append({'name':'runner rejects divergent oracle','status':'FAIL','kind':'HARNESS'})
        try:probe('billing.issue',{'cycle':3,'lots':[{'cycle':3,'principal':10,'fee':0,'cancelled_principal':11}]})
        except AssertionError:results.append({'name':'runner rejects invalid financial prestate','status':'PASS','kind':'HARNESS'})
        else:results.append({'name':'runner rejects invalid financial prestate','status':'FAIL','kind':'HARNESS'})
        try:probe('undefined.operation',{})
        except UnknownProbe:results.append({'name':'runner rejects undefined operation','status':'PASS','kind':'HARNESS'})
        else:results.append({'name':'runner rejects undefined operation','status':'FAIL','kind':'HARNESS'})
    report=ROOT/args.report;report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(results,indent=2,ensure_ascii=False)+'\n')
    passed=sum(r['status']=='PASS' for r in results);print(f'Gherkin: {passed}/{len(results)} PASS; {len(results)-passed} FAIL; zero skips')
    return int(passed!=len(results))
if __name__=='__main__':sys.exit(main())
