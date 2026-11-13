#!/usr/bin/env python3
"""Name and describe an already validated Stage 1 artifact; never fabricate one."""
import datetime,hashlib,json,pathlib,re,shutil,subprocess
repo=pathlib.Path(__file__).resolve().parents[1];art=repo/'artifacts'
v=json.loads((art/'native-validation.json').read_text());manifest=v['build_manifest']
commit=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip()
if v['native_build_commit']!=commit or manifest['source_commit']!=commit:raise SystemExit('Refuse mismatched native artifact source')
if manifest['startup_only'] is not True:raise SystemExit('Only isolated Stage 1 artifacts may be finalized by this workflow')
version=re.search(r'#define VLC_PS5_VERSION "([0-9.]+)"',(repo/'src/platform.h').read_text()).group(1)+'-stage1.'+commit[:12]
filename='Plexium-'+version+'.zip';source=art/'Plexium-PPSA85421.zip'
if hashlib.sha256(source.read_bytes()).hexdigest()!=v['zip_sha256']:raise SystemExit('Installation ZIP changed after validation')
if hashlib.sha256((art/"Plexium-native-debug.elf").read_bytes()).hexdigest()!=v["debug_elf_sha256"]:raise SystemExit("Debug ELF changed after validation")
shutil.copyfile(source,art/filename)
release={**manifest,'version':version,'archive':filename,'build_timestamp':datetime.datetime.now(datetime.timezone.utc).isoformat(),'archive_sha256':v['zip_sha256'],'debug_elf_sha256':v['debug_elf_sha256'],'elf_build_id':v['elf_build_id'],'native_validation':'passed','physical_ps5':'not tested','hardware_hold':(repo/'ci/HARDWARE-HOLD').exists()}
(art/'build-manifest.json').write_text(json.dumps(release,indent=2,sort_keys=True)+'\n')
shutil.copyfile(repo/'docs/STAGE1-INSTALLATION.md',art/'INSTALLATION.md')
(art/'DELIVERY.md').write_text(f"# Plexium Stage 1 investigation candidate\n\nSource: `{commit}`\n\nArchive: `{filename}`\n\nSHA-256: `{v['zip_sha256']}`\n\nOnly platform/renderer/controller startup is enabled. No Plex session, decoder or full-client playback is initialized. Native build/package checks passed; physical PS5 safety is unverified. Hardware hold remains. This is not a complete Plexium release and must not be deployed automatically.\n")
files=sorted([p for p in art.iterdir() if p.name in [filename,'Plexium-PPSA85421.zip','Plexium-native-debug.elf','Plexium-native-source.tar.gz','Plexium-source.bundle','build-manifest.json','native-validation.json','INSTALLATION.md','DELIVERY.md']])
(art/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in files))
print('Verified Stage 1 package:',filename)
