#!/usr/bin/env python3
"""Integridade editorial do pacote. --gherkin exige parser oficial.
Não executa a aplicação C, não mede cobertura semântica e não faz slicing.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path
from urllib.parse import unquote
from typing import Any

ROOT = Path(__file__).resolve().parents[1]

def read_json(path: str) -> Any:
    return json.loads((ROOT/path).read_text(encoding='utf-8'))

def escape(text: str) -> str:
    return text.replace('\\','\\\\').replace('|','\\|').replace('\n','\\n')

def block(case: dict[str, Any], op: str, n: int) -> str:
    """Renderiza dados para reconciliação textual; NÃO é parser de Gherkin."""
    pad=' '*n
    out=[f'{pad}Cenário: {case["name"]}', f'{pad}  Dado o perfil "base" e os seguintes dados',f'{pad}    | campo | valor |']
    def rows(values: dict[str, Any]) -> list[str]:
        return [f'{pad}    | {escape(k)} | {escape(json.dumps(v,ensure_ascii=False,separators=(",",":")))} |' for k,v in values.items()]
    out+=rows(case['given'] or {'profile':'base'})
    out += [f'{pad}  Quando avalio "{op}"',f'{pad}  Então observo os seguintes resultados',f'{pad}    | campo | valor |']
    out+=rows(case['expected'])
    return '\n'.join(out)

def scenarios(node: Any):
    if isinstance(node,dict):
        if 'scenario' in node:
            yield node['scenario']
        for key,val in node.items():
            if key!='scenario':
                yield from scenarios(val)
    elif isinstance(node,list):
        for val in node:
            yield from scenarios(val)

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--gherkin',action='store_true')
    args=ap.parse_args()
    errors=[]
    try:
        manifest=read_json('PACKAGE_MANIFEST.json')
        rules=read_json('docs/oracle/rules.json')['rules']
        flows=read_json('docs/oracle/flows.json')['flows']
    except (OSError,ValueError,KeyError) as exc:
        print(f'FAIL: arquivo ilegível: {exc}',file=sys.stderr)
        return 1
    ids=[r['id'] for r in rules]
    all_cases=[c for r in rules for c in r['cases']]+flows
    case_ids=[c['id'] for c in all_cases]
    for kind,values in [('regra',ids),('cenário',case_ids)]:
        for key,count in Counter(values).items():
            if count!=1: errors.append(f'{kind} duplicado: {key}')
    br=sum(i.startswith('BR-') for i in ids)
    fr=sum(i.startswith('FR-') for i in ids)
    if manifest['rule_counts']!={'BR':br,'FR':fr}: errors.append('Contagem de regras divergente')
    counts={'contract':len(all_cases)-len(flows),'full_flows':len(flows),'total':len(all_cases)}
    if manifest['scenario_counts']!=counts: errors.append('Contagem de cenários divergente')
    features=sorted((ROOT/'docs/oracle/features').glob('*.feature'))
    if len(features)!=manifest['feature_files']: errors.append('Contagem de features divergente')
    for r in rules:
        cap=r['id'].split('-')[1].lower()
        fp=ROOT/f'docs/oracle/features/{cap}.feature'
        cp=ROOT/f'docs/oracle/catalog/{cap}.md'
        if not fp.exists() or not cp.exists():
            errors.append(f'Arquivos ausentes: {cap}')
            continue
        text=fp.read_text(encoding='utf-8')
        if f'Regra: {r["id"]} — {r["title"]}' not in text: errors.append(f'Título divergente: {r["id"]}')
        if r['norm'] not in text or r['norm'] not in cp.read_text(encoding='utf-8'): errors.append(f'Enunciado divergente: {r["id"]}')
        if not r['cases']: errors.append(f'Sem exemplos: {r["id"]}')
        for c in r['cases']:
            if not c['expected'] or block(c,r['operation'],4) not in text: errors.append(f'Dados divergentes: {c["id"]}')
    text=(ROOT/'docs/oracle/features/flows.feature').read_text(encoding='utf-8')
    for c in flows:
        if block(c,'flow.run',2) not in text: errors.append(f'Dados divergentes: {c["id"]}')
        for ref in c['covers']:
            if ref not in ids: errors.append(f'Regra inexistente no fluxo: {ref}')
    # Apenas inventário lexical, não interpretação da gramática.
    feature_text='\n'.join(p.read_text(encoding='utf-8') for p in features)
    tags=re.findall(r'@(SC-[A-Za-z0-9_-]+)(?=\s|$)',feature_text)
    if Counter(tags)!=Counter(case_ids): errors.append('Tags não reconciliam com o JSON')
    links=0
    for p in ROOT.rglob('*.md'):
        if any(part in {'.venv', 'dist', 'build', '.git'} for part in p.relative_to(ROOT).parts): continue
        text=p.read_text(encoding='utf-8')
        if text.count('```')%2: errors.append(f'Cerca Markdown desbalanceada: {p.relative_to(ROOT)}')
        for target in re.findall(r'\[[^\]]+\]\(([^)]+)\)',text):
            if '://' in target or target.startswith('#'): continue
            rel=unquote(target.split('#',1)[0])
            if not rel: continue
            links+=1
            dest=(p.parent/rel).resolve()
            if not dest.is_relative_to(ROOT) or not dest.exists(): errors.append(f'Link inválido: {p.relative_to(ROOT)} → {target}')
    editorial_errors=list(errors)
    gherkin='NOT_REQUESTED'
    parsed=0
    if args.gherkin:
        try:
            from gherkin.parser import Parser
        except ImportError:
            gherkin='NOT_RUN'
            errors.append('Parser oficial ausente. Instale gherkin-official. Nenhuma validação de sintaxe executada.')
        else:
            for p in features:
                try:
                    document=Parser().parse(p.read_text(encoding='utf-8'))
                    for s in scenarios(document):
                        parsed+=1
                        if len(s.get('steps',[]))!=3: errors.append(f'Steps inesperados: {p.name}/{s.get("name")}')
                except Exception as exc:
                    errors.append(f'Gherkin inválido: {p.name}: {exc}')
            if parsed!=len(all_cases): errors.append(f'Parser encontrou {parsed}; esperados {len(all_cases)} cenários')
            gherkin='PASS' if not errors else 'FAIL'
    result={'editorial_status':'FAIL' if editorial_errors else 'PASS','BR':br,'FR':fr,
            'scenarios':len(all_cases),'features':len(features),'internal_links_checked':links,
            'gherkin_parser':gherkin,'C_execution':'NOT_EVALUATED_BY_EDITORIAL_CHECK','Joern':'NOT_EVALUATED_BY_EDITORIAL_CHECK','errors':errors}
    print(json.dumps(result,ensure_ascii=False,indent=2))
    return 2 if gherkin=='NOT_RUN' else (1 if errors else 0)

if __name__=='__main__':
    raise SystemExit(main())
