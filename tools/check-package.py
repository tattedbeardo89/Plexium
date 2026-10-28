#!/usr/bin/env python3
"""Inspect a real native package; never substitute metadata for executable validation."""
from pathlib import Path
import json,sys,struct,hashlib
root=Path(sys.argv[1] if len(sys.argv)>1 else 'dist/PPSA85421')
required=['eboot.bin','sce_sys/param.json','sce_sys/icon0.png','sce_sys/pic0.dds','sce_sys/pic1.dds','sce_module/libc.prx','fonts/Inter-Regular.ttf','certs/cacert.pem','LICENSE','THIRD_PARTY.md','imports.txt']
for name in required:
 p=root/name
 if not p.is_file() or p.stat().st_size==0:sys.exit('Missing native package file: '+name)
for p in root.rglob('*'):
 if p.is_symlink():sys.exit('Package contains a symlink')
 if p.name in ['account.json','device.json','credentials.json'] or p.suffix=='.log':sys.exit('Private runtime data in package')
m=json.loads((root/'sce_sys/param.json').read_text())
assert m['titleId']=='PPSA85421' and m['localizedParameters']['en-US']['titleName']=='Plexium'
assert m['contentId'].split('-')[1].split('_')[0]==m['titleId']
eboot=(root/'eboot.bin').read_bytes()
assert len(eboot)>1024*1024 and struct.unpack('<I',eboot[:4])[0]==0x1D3D154F, 'Missing/invalid signed SELF'
icon=(root/'sce_sys/icon0.png').read_bytes();assert icon[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack_from('>II',icon,16)==(512,512)
assert hashlib.sha256(icon).digest()==hashlib.sha256(Path('ps5/sce_sys/icon0.png').read_bytes()).digest()
for name in ['pic0.dds','pic1.dds']:
 b=(root/'sce_sys'/name).read_bytes();assert b[:4]==b'DDS ' and b[84:88]==b'DXT1' and struct.unpack_from('<II',b,12)==(1080,1920)
print('PASS: native package identity, executable envelope, isolation and artwork; hardware untested')
