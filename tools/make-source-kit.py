#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package application history, pinned toolchain source and verified archives."""
from pathlib import Path
import subprocess,hashlib,json,shutil,tempfile,tarfile
repo=Path(__file__).resolve().parents[1]
art=repo/'artifacts';work=Path('/workspace/plexium-work');deps=Path('/workspace/deps')
def run(*args):return subprocess.check_output(args,text=True).strip()
def digest(p,algorithm='sha256'):
 h=hashlib.new(algorithm)
 with p.open('rb') as f:
  while data:=f.read(1024*1024):h.update(data)
 return h.hexdigest()
validation=json.loads((art/'native-validation.json').read_text())
native=validation['native_build_commit'];head=run('git','-C',str(repo),'rev-parse','HEAD')
assert not run('git','-C',str(repo),'diff',native,head,'--','src'), 'Application source differs from built commit'
run('git','-C',str(repo),'bundle','create',str(art/'Plexium-source.bundle'),'--all')
run('git','-C',str(repo),'bundle','verify',str(art/'Plexium-source.bundle'))
extra=work/'dl/zlib-1.3.1.tar.gz'
if not extra.exists():
 subprocess.run(['curl','--fail','--location','--silent','--show-error','--proto','=https','--proto-redir','=https','https://zlib.net/fossils/zlib-1.3.1.tar.gz','-o',str(extra)],check=True)
checks={line.split()[1]:line.split()[0] for line in (repo/'ci/source-checksums.txt').read_text().splitlines()}
checks['vlc-3.0.24.tar.xz']='e7cab503d1d7d5849b89d2cf0e1ee60d0ef6d012407791b644b9cfc0cc225fdf'
checks['zlib-1.3.1.tar.gz']='9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23'
with tempfile.TemporaryDirectory(prefix='plexium-source-kit-',dir=art) as temporary:
 root=Path(temporary)/'Plexium-native-source';root.mkdir();(root/'sources').mkdir();(root/'toolchains').mkdir()
 shutil.copyfile(art/'Plexium-source.bundle',root/'Plexium-source.bundle')
 for name,expected in checks.items():
  source=work/'dl'/name;assert digest(source,'sha256' if len(expected)==64 else 'sha512')==expected,name
  shutil.copyfile(source,root/'sources'/name)
 pins=json.loads((repo/'ci/build-lock.json').read_text())
 for name in ['PS5_Vulkan','PS5_PayloadSDK','PS5_Mesa']:
  source=deps/name;sha=pins[name];bare=Path(temporary)/(name+'.git')
  subprocess.run(['git','init','--bare','--quiet',str(bare)],check=True)
  subprocess.run(['git','-C',str(bare),'fetch','--quiet','--no-tags','--depth=1','file://'+str(source),sha],check=True)
  subprocess.run(['git','-C',str(bare),'update-ref','refs/heads/pinned',sha],check=True)
  subprocess.run(['git','-C',str(bare),'symbolic-ref','HEAD','refs/heads/pinned'],check=True)
  (bare/'FETCH_HEAD').unlink(missing_ok=True)
  with tarfile.open(root/'toolchains'/(name+'.git.tar.gz'),'w:gz') as t:t.add(bare,arcname=name+'.git')
  print('Archived pinned source metadata:',name,flush=True)
 extras=[
  (deps/'PS5_Vulkan/.deps/native/ps5-payload-sdk.zip','8cfbc7cd5811e719eb4f0c47eea668d3dc7b40bc8ab11c4a5031d40c23ec02da'),
  (deps/'PS5_Vulkan/.deps/native/zlib/zlib-1.3.2.tar.gz','bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16'),
  (deps/'PS5_Vulkan/.deps/work/radv-src/subprojects/packagecache/zlib_1.3.1-1_patch.zip','e79b98eb24a75392009cec6f99ca5cdca9881ff20bfa174e8b8926d5c7a47095')]
 for source,expected in extras:
  assert digest(source)==expected,source.name;shutil.copyfile(source,root/'sources'/source.name)
 shutil.copyfile(art/'native-validation.json',root/'native-validation.json')
 (root/'README.md').write_text(f'''# Plexium native source kit\n\nNative application commit: {native}\nSource/history snapshot: {head}\n\nClone Plexium-source.bundle, then check out the native application commit above to reproduce that exact source stamp. Application patches, build scripts, checksum provenance and licenses are in that history. The complete verified dependency archives and SDK bootstrap archive are in sources/. Pinned shallow bare source repositories (with no credentials/remotes) are in toolchains/. Extract them and use git archive at their pinned commit to inspect/export their source; the SDK and Mesa bootstrap accept bare repository paths as their fork inputs. Build prerequisites and source pins are documented in docs/BUILD.md and ci/. The Arch compiler/tool packages are normal external build tools, pinned by the recorded signed snapshot; the large prepared container image is an optional local environment cache and is not included here.\n\nThe installation ZIP is a homebrew title folder, not a retail PKG. Hardware/firmware 13.40 and live Plex acceptance remain untested. No public repository/release publication occurred. See THIRD_PARTY.md and the installation's 100 collected notices for licensing.\n''')
 manifest={str(p.relative_to(root)):digest(p) for p in sorted(root.rglob('*')) if p.is_file()}
 (root/'SHA256SUMS').write_text(''.join(f'{h}  {n}\n' for n,h in manifest.items()))
 output=art/'Plexium-native-source.tar.gz'
 with tarfile.open(output,'w:gz') as t:t.add(root,arcname=root.name)
print('Created',output,flush=True)
