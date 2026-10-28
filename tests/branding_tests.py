from pathlib import Path
from PIL import Image
import json,hashlib,subprocess
root=Path(__file__).resolve().parents[1]
param=json.loads((root/'ps5/sce_sys/param.json').read_text())
assert param['titleId']=='PPSA85421'
assert 'PPSA85421' in param['contentId'] and 'PPSA85301' not in param['contentId']
assert param['localizedParameters']['en-US']['titleName']=='Plexium'
paths=[root/'ps5/sce_sys'/n for n in ['icon0.png','pic0.dds','pic1.dds']]
before=[hashlib.sha256(p.read_bytes()).hexdigest() for p in paths]
subprocess.run(['python3','tools/make-brand.py'],cwd=root,check=True,stdout=subprocess.DEVNULL)
assert before==[hashlib.sha256(p.read_bytes()).hexdigest() for p in paths]
for p,size in zip(paths,[(512,512),(1920,1080),(1920,1080)]):
 with Image.open(p) as im: assert im.size==size
for p in [root/'src/platform.h',root/'src/app/session.h']:
 assert 'PPSA85421' in p.read_text()
print('PASS: title/data identity, icon and DDS dimensions, deterministic regeneration')
