#!/usr/bin/env python3
"""Offline native build ladder. No console access, publishing, resets or worktrees."""
import hashlib,json,os,pathlib,re,shutil,subprocess,tarfile,time
ROOT=pathlib.Path(__file__).resolve().parents[1]
BASE='2fbce98d89f830bd0df67f321fa7262296b6f643'
NEW='75d7ecf672ed79ab165a879a9a059b899801ae56'
OUT=pathlib.Path(os.environ.get('PLEXIUM_BISECT_OUT',str(ROOT/'artifacts/native-bisection')));SOURCE=OUT/'source'
LIBASS=os.environ.get('PLEXIUM_BISECT_LIBASS','')
VLC_TIME=os.environ.get('PLEXIUM_BISECT_VLC_TIME','')
REUSE=pathlib.Path(os.environ['PLEXIUM_BISECT_REUSE']) if os.environ.get('PLEXIUM_BISECT_REUSE') else None
GROUPS=[('00-baseline',[]),('01-protocol',['src/plex/client.h','src/plex/client.cc','src/plex/home_feed.h','src/plex/presentation.h']),('02-application',['src/app/browse.h','src/app/capabilities.h','src/app/session.h','src/app/session.cc','src/app/view.h','src/app/view.cc','src/plex/free_tv.h']),('03-entry',['src/main.cc','src/player.cc'])]
def run(args,**kw):return subprocess.run(args,check=True,**kw)
def git(*args):return subprocess.check_output(['git','-C',str(ROOT),*args])
def sha(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 OUT.mkdir(parents=True,exist_ok=True)
 if SOURCE.exists():raise SystemExit('Refuse to overwrite a previous source experiment; preserve it first')
 SOURCE.mkdir()
 archive=OUT/'baseline-source.tar';archive.write_bytes(git('archive',BASE))
 with tarfile.open(archive) as t:t.extractall(SOURCE,filter='data')
 archive.unlink()
 script=SOURCE/'ps5/build-title.sh';text=script.read_text();line=next(l for l in text.splitlines() if l.startswith('build_commit='));text=text.replace(line,'build_commit=${PLEXIUM_BISECT_STAMP:?Explicit experiment source identity required}')
 if LIBASS:
  text=text.replace('lib=$prefix/lib/lib${item#-l}.a','lib=$prefix/lib/lib${item#-l}.a\n        if [[ $item == -lass ]]; then lib=/run/libass-no-iconv.a; fi')
  text=text.replace('    *) vlc_inputs+=("$item") ;;','    */libass.a) vlc_inputs+=(/run/libass-no-iconv.a) ;;\n    *) vlc_inputs+=("$item") ;;')
 if VLC_TIME:
  text=text.replace('    *) vlc_inputs+=("$item") ;;','    */libvlccore.a) vlc_inputs+=(/run/libvlccore-matched.a) ;;\n    *) vlc_inputs+=("$item") ;;')
 if REUSE:
  start=text.index('# 2.');end=text.index('# 3.')
  text=text[:start]+'if [[ -d /run/reuse-$PLEXIUM_BISECT_CASE ]]; then\n cp -a /run/reuse-$PLEXIUM_BISECT_CASE/. "$work/"\nelse\n'+text[start:end]+'fi\n\n'+text[end:]
 script.write_text(text)
 manifest={'base':BASE,'new':NEW,'hardware_execution':'none','delivery':'held; do not install','recipe_adjustment':'Source identity is explicit. Optional libass archive override and verified app-object reuse are recorded separately. Application sources are not patched. Canonical container path remains /workspace/Plexium.','cases':[]}
 image='plexium-ps5:prepared';expected=(ROOT/'ci/native-image-id.txt').read_text().strip();actual=subprocess.check_output(['docker','image','inspect','--format','{{.Id}}',image],text=True).strip();assert actual==expected
 dependencies=[pathlib.Path('/workspace/deps/PS5_Vulkan/build/host/ps5-native-tool'),pathlib.Path('/workspace/deps/PS5_Vulkan/runtime/libc.prx'),pathlib.Path('/workspace/deps/PS5_Vulkan/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a'),pathlib.Path('/workspace/plexium-work/build-vlc-ps5/src/.libs/libvlccore.a')]
 manifest['dependency_sha256']={str(p):sha(p) for p in dependencies}
 name='plexium-bisection-'+str(os.getpid());(OUT/'container-name').write_text(name)
 args=['docker','run','-d','--name',name,'--network','none','--mount','type=bind,src=/workspace,dst=/workspace','--mount',f'type=bind,src={SOURCE},dst=/workspace/Plexium','--mount','type=bind,src=/etc/ssl/certs/ca-certificates.crt,dst=/run/ca-bundle.pem,readonly']
 for key in ['HTTP_PROXY','HTTPS_PROXY','NO_PROXY','http_proxy','https_proxy','no_proxy','ALL_PROXY','all_proxy']:args+=['-e',key]
 if LIBASS:args+=['--mount',f'type=bind,src={pathlib.Path(LIBASS).resolve()},dst=/run/libass-no-iconv.a,readonly']
 if VLC_TIME:
  args+=['--mount',f'type=bind,src={OUT / "libvlccore-matched.a"},dst=/run/libvlccore-matched.a,readonly']
 if REUSE:
  for label,_ in GROUPS:
   folder=REUSE/label/'link-inputs'
   if folder.is_dir():args+=['--mount',f'type=bind,src={folder.resolve()},dst=/run/reuse-{label},readonly']
 args+=['-e','SSL_CERT_FILE=/run/ca-bundle.pem','-e','CURL_CA_BUNDLE=/run/ca-bundle.pem','-e','GIT_SSL_CAINFO=/run/ca-bundle.pem','-w','/workspace/Plexium',image,'sleep','infinity']
 changed=[]
 if VLC_TIME:
  original=pathlib.Path('/workspace/plexium-work/build-vlc-ps5/src/.libs/libvlccore.a');data=original.read_bytes()
  dates=re.findall(rb'[A-Z][a-z]{2} [ 0-9][0-9] 202[0-9] [0-9:]{8}',data)
  assert len(dates)==1 and len(VLC_TIME.encode())==len(dates[0]),'Ambiguous compilation timestamp; stop for review'
  normalized=OUT/'libvlccore-matched.a';normalized.write_bytes(data.replace(dates[0],VLC_TIME.encode()))
  manifest['vlc_timestamp_normalization']={'original_sha256':sha(original),'normalized_sha256':sha(normalized),'original_timestamp':dates[0].decode(),'timestamp':VLC_TIME,'original_cache_modified':False}
 if LIBASS:
  symbols=subprocess.check_output(['nm','--defined-only',LIBASS],text=True)
  assert 'sub_recode' not in symbols,'Expected libass without iconv; verify the actual configure result before building'
  manifest['libass_override_sha256']=sha(pathlib.Path(LIBASS))
 try:
  run(args,stdout=subprocess.DEVNULL)
  for index,(label,files) in enumerate(GROUPS):
   for file in files:
    dest=SOURCE/file;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(git('show',NEW+':'+file));changed.append(file)
   stamp=BASE if index==0 else NEW if index==3 else hashlib.sha256(('\n'.join(changed)).encode()).hexdigest()[:40]
   if index==3:
    all_changed=set(git('diff','--name-only',BASE,NEW,'--','src').decode().splitlines());assert all_changed==set(changed),(all_changed,set(changed))
   result=OUT/label;result.mkdir();case={'label':label,'source_stamp':stamp,'changed_from_base':list(changed),'source_sha256':{p:sha(SOURCE/p) for p in changed},'status':'building'};manifest['cases'].append(case);(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(label+': build started',flush=True)
   if REUSE and (REUSE/label/'link-inputs').is_dir():
    original=json.loads((REUSE/'manifest.json').read_text());prior=next(c for c in original['cases'] if c['label']==label)
    assert prior['source_stamp']==stamp and prior['source_sha256']==case['source_sha256']
    assert stamp.encode() in (REUSE/label/'link-inputs/libprobe.a').read_bytes()
    case['reused_app_objects_from']=str(REUSE/label/'link-inputs')
   started=time.monotonic()
   with (result/'build.log').open('w') as log:run(['docker','exec','-e','PS5_VULKAN=/workspace/deps/PS5_Vulkan','-e','VLC_PS5_WORK=/workspace/plexium-work','-e','PLEXIUM_BISECT_STAMP='+stamp,'-e','PLEXIUM_BISECT_CASE='+label,'-e','SOURCE_DATE_EPOCH='+git('show','-s','--format=%ct',BASE).decode().strip(),name,'bash','ci/build-native.sh'],stdout=log,stderr=subprocess.STDOUT)
   for f in ['Plexium-native-debug.elf','Plexium-PPSA85421.zip','imports.txt','elf-layout.txt','symbols.txt','plexium.map']:
    src=SOURCE/'artifacts'/f
    if f=='imports.txt':src=SOURCE/'dist/PPSA85421/imports.txt'
    shutil.copy2(src,result/f)
   shutil.copy2('/workspace/plexium-work/build-title/eboot.elf',result/'eboot.elf')
   shutil.copy2(SOURCE/'dist/PPSA85421/eboot.bin',result/'eboot.bin')
   case.update(status='built',elapsed_seconds=round(time.monotonic()-started,2),elf_sha256=sha(result/'Plexium-native-debug.elf'),self_sha256=sha(result/'eboot.bin'))
   expected_folder='github-2fbce98' if index==0 else 'github-75d7ecf' if index==3 else None
   if expected_folder:
    import zipfile
    with zipfile.ZipFile(ROOT/'artifacts'/expected_folder/'Plexium-PPSA85421.zip') as z:case['self_matches_published']=hashlib.sha256(z.read('PPSA85421/eboot.bin')).hexdigest()==case['self_sha256']
   (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(label+': built; self='+case['self_sha256'],flush=True)
  assert manifest['dependency_sha256']=={str(p):sha(p) for p in dependencies},'Native dependencies changed during comparison'
 finally:
  subprocess.run(['docker','exec',name,'chown','-R',str(os.getuid())+':'+str(os.getgid()),'/workspace/Plexium'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  subprocess.run(['docker','rm','-f',name],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
if __name__=='__main__':main()
