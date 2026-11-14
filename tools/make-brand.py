#!/usr/bin/env python3
"""Deterministic original SVG geometry -> PNG icon, splash, DXT1 DDS."""
from pathlib import Path
import xml.etree.ElementTree as ET
from PIL import Image, ImageDraw, ImageFont
import hashlib,json
root=Path(__file__).resolve().parents[1]
master=root/'assets/brand/plexium.svg'
def logo(size):
 im=Image.new('RGB',(512,512));d=ImageDraw.Draw(im)
 for e in ET.parse(master).getroot():
  tag=e.tag.rsplit('}',1)[-1]
  if tag=='rect':d.rectangle((int(e.attrib['x']),int(e.attrib['y']),int(e.attrib['x'])+int(e.attrib['width']),int(e.attrib['y'])+int(e.attrib['height'])),fill=e.attrib['fill'])
  if tag=='polygon':d.polygon([tuple(map(int,p.split(','))) for p in e.attrib['points'].split()],fill=e.attrib['fill'])
 return im.resize((size,size),Image.Resampling.LANCZOS)
out=root/'ps5/sce_sys';out.mkdir(parents=True,exist_ok=True)
logo(512).save(out/'icon0.png')
brand=root/'assets/brand'
logo(128).save(brand/'logo.png')
font=ImageFont.truetype(str(root/'assets/fonts/Inter-SemiBold.ttf'),80)
small=ImageFont.truetype(str(root/'assets/fonts/Inter-Regular.ttf'),30)
bg=Image.new('RGB',(1920,1080));pixels=bg.load()
for y in range(1080):
 for x in range(1920):
  glow=max(0,1-((x-1200)**2/(1400**2)+(y-300)**2/(1000**2)))
  pixels[x,y]=(int(10+20*glow),int(11+13*glow),int(18+42*glow))
bg.paste(logo(220),(850,280));d=ImageDraw.Draw(bg)
d.text((960,555),'plexium',font=font,fill='#efeffa',anchor='mm')
d.text((960,665),'A cinematic Plex experience for PlayStation 5.',font=small,fill='#b6b4cc',anchor='mm')
d.text((960,995),'Unofficial Plex client; not affiliated with or endorsed by Plex',font=small,fill='#9da1bb',anchor='mm')
for name in ['splash','signin']:bg.save(brand/(name+'.png'))
for name in ['pic0','pic1']:bg.save(out/(name+'.dds'),pixel_format='DXT1')
# Text-only category masters, no pictograms or copied brand artwork.
for label in ['Movies','TV Shows','Music','Audiobooks','Collections']:
 (brand/(label.lower().replace(' ','-')+'.svg')).write_text(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 360"><rect width="640" height="360" rx="24" fill="#171522"/><text x="320" y="195" font-family="Inter" font-size="48" text-anchor="middle" fill="#eae6ff">{label}</text></svg>\n')
(brand/'wordmark.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 800 180"><text x="20" y="130" font-family="Inter" font-size="130" fill="#d7d2ff">plexium</text></svg>\n')
(brand/'loading.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 80 80"><circle cx="40" cy="40" r="28" fill="none" stroke="#8b76ff" stroke-width="6" stroke-dasharray="110 66"/></svg>\n')
files=[out/'icon0.png',out/'pic0.dds',out/'pic1.dds',brand/'splash.png']
(brand/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(root))+'\n' for p in files))
print('Generated original Plexium PNG, SVG and DXT1 DDS assets')
