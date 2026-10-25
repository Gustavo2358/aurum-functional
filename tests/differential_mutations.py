#!/usr/bin/env python3
"""Real source mutants must fail the frozen byte comparator; equivalent rename must pass."""
import importlib.util,json,shutil,subprocess,tempfile,hashlib
from pathlib import Path
from differential import ROOT,CORPUS,GOLDEN,compare

def main():
 spec=importlib.util.spec_from_file_location('frozen_mutations',ROOT/'tests/original/tools/mutations.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 cases=json.loads(CORPUS.read_text())['cases'];golden=json.loads(GOLDEN.read_text());records=[]
 variants=module.MUTANTS+[dict(name='equivalent_local_rename',file='lifecycle.c',before='principal_cash',after='principal_remainder',equivalent=True)]
 for variant in variants:
  with tempfile.TemporaryDirectory(prefix='aurum-differential-mutant-') as d:
   root=Path(d);shutil.copytree(ROOT/'tests/original/subject',root/'subject')
   path=root/'subject/src'/variant['file'];source=path.read_text();count=source.count(variant['before'])
   assert count and count==(count if variant.get('equivalent') else variant.get('occurrences',1))
   path.write_text(source.replace(variant['before'],variant['after']));binary=root/'aurum'
   command=['cc','-std=c17','-O0','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(root/'subject/include'),*[str(x) for x in sorted((root/'subject/src').glob('*.c'))],'-o',str(binary)]
   build=subprocess.run(command,capture_output=True,timeout=90)
   record=dict(name=variant['name'],source_before_sha256=hashlib.sha256(source.encode()).hexdigest(),source_after_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),build_status=build.returncode,build_stdout=build.stdout.decode(),build_stderr=build.stderr.decode())
   if build.returncode:record['status']='BUILD_ERROR'
   else:
    failures,observed=compare(binary,cases,golden)
    record['status']=('EQUIVALENT_PASS' if not failures else 'EQUIVALENT_FAIL') if variant.get('equivalent') else ('KILLED' if failures else 'SURVIVED')
    record['failures']=failures
    if failures:
     name=failures[0]['case'];record['counterexample']={'case':name,'expected':golden['observations'][name],'actual':observed[name]}
   records.append(record);print(variant['name'],record['status'],flush=True)
 report=dict(status='PASS' if all(r['status'] in ('KILLED','EQUIVALENT_PASS') for r in records) else 'FAIL',source='frozen original source mutations; candidate is never mutated by this script',mutants=records)
 (ROOT/'evidence/harness-differential-mutations.json').write_text(json.dumps(report,indent=2)+'\n')
 return int(report['status']!='PASS')
if __name__=='__main__':raise SystemExit(main())
