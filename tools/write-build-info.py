import json,pathlib,re,subprocess,sys
root=pathlib.Path(__file__).resolve().parents[1]
sha=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
date=subprocess.check_output(['git','show','-s','--format=%cI','HEAD'],cwd=root,text=True).strip()
version=re.search(r'#define VLC_PS5_VERSION "([^"]+)"',(root/'src/platform.h').read_text()).group(1)
text=''.join('#define '+name+' '+json.dumps(value)+'\n' for name,value in [('PLEX_BUILD_COMMIT',sha),('PLEX_SOURCE_DATE',date),('PLEX_APP_VERSION',version)])
p=pathlib.Path(sys.argv[1]);p.parent.mkdir(parents=True,exist_ok=True)
if not p.exists() or p.read_text()!=text:p.write_text(text)
