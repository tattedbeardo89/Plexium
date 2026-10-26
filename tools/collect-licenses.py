#!/usr/bin/env python3
"""Collect public source license notices into the native installation folder."""
from pathlib import Path
import sys,shutil,os,json
repo=Path(__file__).resolve().parents[1]
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
roots=[('application',repo),('third-party',repo/'third_party'),('fonts',repo/'assets/fonts')]
work=Path(os.environ.get('VLC_PS5_WORK','/workspace/plexium-work'))
if (work/'src').is_dir():
 for p in sorted((work/'src').iterdir()):
  if p.is_dir() and not p.name.startswith('build-'):roots.append((p.name,p))
for name in ['PS5_Vulkan','PS5_PayloadSDK','PS5_Mesa']:
 p=Path('/workspace/deps')/name
 if p.is_dir():roots.append((name,p))
index=[]
for label,base in roots:
 candidates=[repo/'LICENSE',repo/'THIRD_PARTY.md'] if label=='application' else base.rglob('*')
 for p in candidates:
  if not p.is_file() or p.is_symlink():continue
  relative=p.relative_to(base)
  if any(part in {'.git','.deps','build','node_modules'} for part in relative.parts):continue
  name=p.name.lower()
  if label!='application' and not (name.startswith(('license','copying','copyright','authors','notice')) or 'ofl' in name or name in {'ftl.txt','gplv2.txt','license.rst'}):continue
  target=out/label/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
  index.append({'component':label,'source_path':str(relative),'notice':str(target.relative_to(out))})
(out/'INDEX.json').write_text(json.dumps(index,indent=2)+'\n')
if len(index)<20:raise SystemExit('License collection unexpectedly incomplete')
print(f'Collected {len(index)} license/attribution notices')
