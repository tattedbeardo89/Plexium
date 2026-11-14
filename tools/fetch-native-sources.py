#!/usr/bin/env python3
"""Fetch/recheck pinned public source archives using published upstream digests."""
from pathlib import Path
import sys,re,json,subprocess,hashlib
root=Path(__file__).resolve().parents[1]
destination=Path(sys.argv[1] if len(sys.argv)>1 else '/workspace/plexium-work/dl');destination.mkdir(parents=True,exist_ok=True)
checks={}
for line in (root/'ci/source-checksums.txt').read_text().splitlines():
 digest,name=line.split();assert name not in checks;assert len(digest) in (64,128);checks[name]=digest
version=None;urls={}
for line in (root/'ps5/deps/build-deps.sh').read_text().splitlines():
 match=re.search(r'local v=([0-9.]+)',line)
 if match:version=match[1]
 match=re.search(r'fetch "(https://[^"]+)"(?: "([^"]+)")?',line)
 if match:
  url=match[1].replace('$v',version);name=(match[2] or url.rsplit('/',1)[-1]).replace('$v',version);urls[name]=url
assert set(checks)==set(urls),'Recipe and published checksum manifest differ'
failures=[]
for name,url in urls.items():
 target=destination/name;digest=checks[name]
 def valid(p):
  if not p.is_file():return False
  h=hashlib.sha256() if len(digest)==64 else hashlib.sha512()
  with p.open('rb') as f:
   while data:=f.read(1024*1024):h.update(data)
  return h.hexdigest()==digest
 if target.exists():
  if not valid(target):raise SystemExit('Cached source checksum mismatch: '+name)
  print('Verified cached',name,flush=True);continue
 temp=destination/(name+'.part')
 result=subprocess.run(['curl','--fail','--location','--silent','--show-error',
                        '--retry','4','--retry-all-errors','--retry-delay','3',
                        '--connect-timeout','30','--max-time','300',
                        '--proto','=https','--proto-redir','=https',url,'-o',str(temp)])
 if result.returncode:
  failures.append(name);print('BLOCKED:',name,flush=True);continue
 if not valid(temp):raise SystemExit('Downloaded source checksum mismatch: '+name)
 temp.replace(target);print('Verified',name,flush=True)
if failures:raise SystemExit('Source verification incomplete: '+', '.join(failures))
print('PASS: all 22 pinned source archives verified against published digests')
