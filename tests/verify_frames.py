from pathlib import Path
from PIL import Image
import hashlib,sys
frames=[]
for name in ['welcome','linking','home','library','details','profiles']:
 p=Path(sys.argv[1] if len(sys.argv)>1 else 'build')/(name+'.ppm')
 with Image.open(p) as im:
  assert im.size==(1280,720)
  assert len(im.getcolors(im.width*im.height))>80, name+' lacks rendered content'
  im.save(p.with_suffix('.png'))
 frames.append(hashlib.sha256(p.read_bytes()).hexdigest())
assert len(set(frames))==6, 'Screens rendered identically'
print('PASS: six distinct, nonblank 1280x720 frames, PNG artifacts written')
