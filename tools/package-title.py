#!/usr/bin/env python3
from pathlib import Path
import sys,os,zipfile
root=Path(sys.argv[1]);output=Path(sys.argv[2]) if len(sys.argv)>2 else None
for p in [root,*root.rglob('*')]:
 if p.is_symlink():raise SystemExit('Refuse package symlinks')
 os.chmod(p,0o755 if p.is_dir() or p.name=='eboot.bin' or p.suffix=='.prx' else 0o644)
if output:
 output.parent.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
  for p in sorted(root.rglob('*')):
   if p.is_file():z.write(p,root.name+'/'+str(p.relative_to(root)))
 print('Wrote',output)
