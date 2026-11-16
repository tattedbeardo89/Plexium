import importlib.util,pathlib,tempfile
source=pathlib.Path(__file__).resolve().parents[1]/'ci/dependency-fingerprint.py'
spec=importlib.util.spec_from_file_location('dependency_fingerprint',source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as folder:
 root=pathlib.Path(folder)
 for name,data in {'ci/build-lock.json':'pinned SDKs','ci/source-checksums.txt':'verified archives','ps5/build-title.sh':'original title','ps5/deps/recipe.sh':'dependency recipe','ps5/modules/decoder.c':'module source','patches/vlc.patch':'patch'}.items():
  p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(data)
 current=module.fingerprint(root);legacy=module.fingerprint(root,b'original title')
 (root/'ps5/build-title.sh').write_text('application source list changed')
 assert module.fingerprint(root)==current
 assert module.fingerprint(root,b'original title')==legacy
 for name in ['ci/build-lock.json','ci/source-checksums.txt','ps5/deps/recipe.sh','ps5/modules/decoder.c','patches/vlc.patch']:
  p=root/name;before=p.read_bytes();p.write_bytes(before+b'changed')
  assert module.fingerprint(root)!=current
  assert module.fingerprint(root,b'original title')!=legacy
  p.write_bytes(before)
print('PASS: dependency recipes/pins/digests/modules/patches invalidate cache; title-only edits preserve dependency identity; legacy migration rejects every other changed input')
