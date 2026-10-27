#!/usr/bin/env python3
"""Record and verify the actual native artifact's source/debug identity."""
from pathlib import Path
import subprocess,hashlib,json,re,zipfile
repo=Path(__file__).resolve().parents[1];root=repo/'artifacts'
commit=subprocess.check_output(['git','-c','safe.directory='+str(repo),'-C',str(repo),'rev-parse','HEAD'],text=True).strip()
elf=root/'Plexium-native-debug.elf';raw=elf.read_bytes();assert commit.encode() in raw,'Build source stamp mismatch'
subprocess.run(['python3',str(repo/'ci/check-native-stack.py'),str(elf),str(root/'native-stack.json')],check=True)
notes=subprocess.check_output(['readelf','-n',str(elf)],text=True);build_id=re.search(r'Build ID: ([0-9a-f]+)',notes).group(1)
with zipfile.ZipFile(root/'Plexium-PPSA85421.zip') as z:
 assert z.testzip() is None
 names=z.namelist();assert len(names)==len(set(names)) and all(n.startswith('PPSA85421/') for n in names)
 for info in z.infolist():
  expected=0o755 if info.filename.endswith(('eboot.bin','.prx')) else 0o644
  assert ((info.external_attr>>16)&0o777)==expected,info.filename
 assert z.read('PPSA85421/eboot.bin')==(repo/'dist/PPSA85421/eboot.bin').read_bytes()
notices=len(json.loads((repo/'dist/PPSA85421/licenses/INDEX.json').read_text()));assert notices>=100
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
(root/'native-validation.json').write_text(json.dumps({'native_build_commit':commit,'title_id':'PPSA85421','elf_build_id':build_id,'self_sha256':h(repo/'dist/PPSA85421/eboot.bin'),'zip_sha256':h(root/'Plexium-PPSA85421.zip'),'debug_elf_sha256':h(elf),'archive_permissions':'passed','archive_crc':'passed','notice_files':notices,'startup_stack':json.loads((root/'native-stack.json').read_text()),'delivery_status':'held after user-reported reboot' if (repo/'ci/HARDWARE-HOLD').exists() else 'development candidate','hardware_verification':'not run','live_plex_verification':'not run'},indent=2)+'\n')
print('PASS: actual native source stamp, ELF build ID, matching ZIP payload, CRC and executable permissions')
