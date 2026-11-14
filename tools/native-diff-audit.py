#!/usr/bin/env python3
"""Read-only ELF/SELF comparison; structural results are not execution results."""
import argparse,bisect,collections,hashlib,json,pathlib,struct,subprocess
class ELF:
 def __init__(self,path):
  self.path=pathlib.Path(path);self.raw=self.path.read_bytes();assert self.raw[:6]==b'\x7fELF\x02\x01'
  self.h=struct.unpack_from('<16sHHIQQQIHHHHHH',self.raw);h=self.h
  self.ph=[struct.unpack_from('<IIQQQQQQ',self.raw,h[5]+i*h[9]) for i in range(h[10])]
  entries=[struct.unpack_from('<IIQQQQIIQQ',self.raw,h[6]+i*h[11]) for i in range(h[12])]
  names=entries[h[13]];strings=self.raw[names[4]:names[4]+names[5]]
  self.sections={self.str(strings,s[0]):s for s in entries};self.symbols=[]
  for s in entries:
   if s[1] not in (2,11):continue
   st=entries[s[6]];st=self.raw[st[4]:st[4]+st[5]]
   for at in range(s[4],s[4]+s[5],s[9]):
    n,info,other,index,value,size=struct.unpack_from('<IBBHQQ',self.raw,at);self.symbols.append((value,size,self.str(st,n),info&15,index))
  self.functions={v:n for v,size,n,kind,index in self.symbols if kind==2 and index and n}
  self.relative={};self.relocations=[];self.import_slots={}
  for s in entries:
   if s[1]!=4:continue
   for at in range(s[4],s[4]+s[5],s[9]):
    where,info,addend=struct.unpack_from('<QQq',self.raw,at);kind=info&0xffffffff
    self.relocations.append((where,kind,addend))
    if kind in (6,7):
     table=entries[s[6]];strings=entries[table[6]];strings=self.raw[strings[4]:strings[4]+strings[5]]
     symbol=struct.unpack_from('<IBBHQQ',self.raw,table[4]+(info>>32)*table[9]);self.import_slots[where]=self.str(strings,symbol[0])
    if kind==8:self.relative[where]=addend
 @staticmethod
 def str(data,at):return data[at:data.find(b'\0',at)].decode(errors='replace')
 def summary(self):
  loads=[p for p in self.ph if p[0]==1]
  def mapped(address,size=1):return any(p[3]<=address and address+size<=p[3]+p[6] for p in loads)
  issues=[]
  for p in loads:
   if p[5]>p[6] or p[2]+p[5]>len(self.raw):issues.append('load bounds')
   if p[7] and (p[2]-p[3])%p[7]:issues.append('load alignment')
  if not any(p[1]&1 and p[3]<=self.h[4]<p[3]+p[6] for p in loads):issues.append('entry not executable')
  constructors=[]
  if '.init_array' in self.sections:
   s=self.sections['.init_array']
   for i in range(0,s[5],8):
    address=s[3]+i;value=self.relative.get(address,struct.unpack_from('<Q',self.raw,s[4]+i)[0]);constructors.append(self.functions.get(value,'unknown:'+hex(value)))
  selected={}
  for n in ['.text','.rodata','.data','.bss','.init_array','.rela.dyn','.got','.got.plt','.tdata','.tbss']:
   if n in self.sections:
    s=self.sections[n];selected[n]={'address':s[3],'size':s[5],'sha256':hashlib.sha256(self.raw[s[4]:s[4]+s[5]]).hexdigest() if s[1]!=8 else None}
  rel=collections.Counter(k for w,k,a in self.relocations)
  missing=[hex(w) for w,k,a in self.relocations if k in (1,6,7,8) and not mapped(w,8)]
  if missing:issues.append('relocation targets outside LOAD memory')
  bad_values=[hex(a) for w,k,a in self.relocations if k==8 and not any(p[3]<=a<=p[3]+p[6] for p in loads)]
  nonwritable=[hex(w) for w,k,a in self.relocations if not any(p[1]&2 and p[3]<=w and w+8<=p[3]+p[6] for p in loads)]
  if bad_values:issues.append('relative relocation values outside image')
  if nonwritable:issues.append('relocation targets not writable')
  return {'relative_values_outside_image':bad_values[:20],'nonwritable_relocations':nonwritable[:20],'imports':sorted(self.import_slots.values()),'entry':self.h[4],'elf_type':self.h[1],'load_segments':[[p[1],p[2],p[3],p[5],p[6],p[7]] for p in loads],'sections':selected,'constructors':constructors,'relocations_by_type':dict(rel),'unmapped_relocation_targets':missing[:20],'layout_issues':issues,'function_count':len(self.functions)}
def main():
 p=argparse.ArgumentParser();p.add_argument('files',nargs='+');p.add_argument('--output',required=True);a=p.parse_args();r={}
 for path in a.files:
  e=ELF(path);r[path]=e.summary()
 pathlib.Path(a.output).write_text(json.dumps(r,indent=2)+'\n')
 for path,s in r.items():print(path, 'constructors='+str(len(s['constructors'])), 'issues='+str(s['layout_issues']))
if __name__=='__main__':main()
