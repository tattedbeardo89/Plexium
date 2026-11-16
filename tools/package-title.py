#!/usr/bin/env python3
"""Normalize public package modes and create a deterministic installation ZIP."""
from pathlib import Path
from datetime import datetime, timezone
import sys,os,zipfile,subprocess
root=Path(sys.argv[1]);output=Path(sys.argv[2]) if len(sys.argv)>2 else None
for p in [root,*root.rglob('*')]:
 if p.is_symlink():raise SystemExit('Refuse package symlinks')
 os.chmod(p,0o755 if p.is_dir() or p.name=='eboot.bin' or p.suffix=='.prx' else 0o644)
if output:
 epoch=os.environ.get('SOURCE_DATE_EPOCH')
 if epoch is None:
  repo=Path(__file__).resolve().parents[1]
  try:epoch=subprocess.check_output(['git','-c','safe.directory='+str(repo),'-C',str(repo),'show','-s','--format=%ct','HEAD'],text=True,stderr=subprocess.DEVNULL).strip()
  except subprocess.CalledProcessError:epoch='315532800'
 stamp=datetime.fromtimestamp(max(315532800,int(epoch)),timezone.utc).timetuple()[:6]
 output.parent.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for p in sorted(root.rglob('*')):
   if p.is_file():
    info=zipfile.ZipInfo(root.name+'/'+str(p.relative_to(root)),stamp)
    info.create_system=3;info.compress_type=zipfile.ZIP_DEFLATED
    info.external_attr=(p.stat().st_mode&0xffff)<<16
    z.writestr(info,p.read_bytes(),compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
 print('Wrote',output)
