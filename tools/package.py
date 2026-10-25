#!/usr/bin/env python3
"""Create a local reviewable project archive and a SHA-256 manifest. No network."""
import argparse,hashlib,json,subprocess,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--archive',type=Path,required=True);a=p.parse_args()
 names=set(subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0'))-{'' ,'MANIFEST.json'}
 # Keep the exact golden executables used for the preimplementation reference.
 names.update(['build/aurum','build/libaurum.so','evidence/harness-original/aurum','evidence/harness-original/libaurum.so'])
 files={}
 for name in sorted(names):
  path=ROOT/name
  if not path.is_file() or path.is_symlink():raise SystemExit('Missing or nonregular deliverable: '+name)
  files[name]=dict(bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
 manifest=dict(schema=1,project='aurum-functional',golden_revision='a9f3b7e8ec7f09e2ccf9f514be635950226ad513',files=files,exclusions=['.git history (preserved in local PGC checkout separately)','temporary compiler outputs and Python caches','this manifest itself','archive and delivery verification receipt'],claim='File integrity and reproducibility; not a proof of universal behavioral equivalence.')
 (ROOT/'MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 a.archive.parent.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(a.archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for name in sorted(names|{'MANIFEST.json'}):z.write(ROOT/name,'aurum-functional/'+name)
 with zipfile.ZipFile(a.archive) as z:
  for name,value in files.items():
   if hashlib.sha256(z.read('aurum-functional/'+name)).hexdigest()!=value['sha256']:raise SystemExit('Archive hash mismatch: '+name)
 print(json.dumps(dict(status='PASS',archive=str(a.archive.resolve()),files=len(files),bytes=a.archive.stat().st_size,sha256=hashlib.sha256(a.archive.read_bytes()).hexdigest()),indent=2))
if __name__=='__main__':main()
