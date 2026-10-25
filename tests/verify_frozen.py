#!/usr/bin/env python3
"""Check source/oracle inventory before any candidate acceptance run."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 manifest=json.loads((ROOT/'tests/original-manifest.json').read_text());fail=[]
 for rel,want in manifest['files'].items():
  p=ROOT/'tests/original'/rel
  if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=want:fail.append(rel)
 for rel,want in json.loads((ROOT/'tests/frozen-manifest.json').read_text())['files'].items():
  p=ROOT/rel
  if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=want:fail.append(rel)
 actual={str(p.relative_to(ROOT/'tests/original')) for p in (ROOT/'tests/original').rglob('*') if p.is_file()}
 extra=sorted(actual-set(manifest['files']))
 report=dict(status='FAIL' if fail or extra else 'PASS',source_revision=manifest['revision'],files=len(manifest['files']),modified_or_missing=fail,unexpected=extra)
 print(json.dumps(report,indent=2));return int(bool(fail or extra))
if __name__=='__main__':raise SystemExit(main())
