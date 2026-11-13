#!/usr/bin/env python3
"""Test finalization guards with synthetic files, not native build acceptance."""
import hashlib,json,os,pathlib,runpy,subprocess,tempfile,unittest
from unittest import mock
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Stage1(unittest.TestCase):
 def run_fixture(self,startup=True,mismatch=False,full=False):
  with tempfile.TemporaryDirectory() as temp:
   root=pathlib.Path(temp);(root/'ci').mkdir();(root/'artifacts').mkdir();(root/'src').mkdir();(root/'docs').mkdir()
   script=root/'ci/finalize-stage1.py';script.write_text((ROOT/'ci/finalize-stage1.py').read_text())
   (root/'src/platform.h').write_text('#define VLC_PS5_VERSION "0.5.0"\n');(root/'docs/STAGE1-INSTALLATION.md').write_text('fixture: not installable\n')
   (root/'docs/FULL-CLIENT-INSTALLATION.md').write_text('held full client fixture\n')
   if full:(root/'ci/HARDWARE-HOLD').write_text('held\n')
   art=root/'artifacts';(art/'Plexium-PPSA85421.zip').write_bytes(b'fixture, not native');(art/'Plexium-native-debug.elf').write_bytes(b'fixture, not ELF')
   commit='a'*40;digest=hashlib.sha256((art/'Plexium-PPSA85421.zip').read_bytes()).hexdigest()
   manifest={'source_commit':commit,'startup_only':startup}
   (art/'native-validation.json').write_text(json.dumps({'build_manifest':manifest,'native_build_commit':commit,'zip_sha256':'0'*64 if mismatch else digest,'debug_elf_sha256':hashlib.sha256((art/'Plexium-native-debug.elf').read_bytes()).hexdigest(),'elf_build_id':'fixture'}))
   with mock.patch.dict(os.environ,{'PLEX_STARTUP_ONLY':'0' if full else '1'}),mock.patch.object(subprocess,'check_output',return_value=commit+'\n'):runpy.run_path(str(script),run_name='__main__')
   result=json.loads((art/'build-manifest.json').read_text());self.assertIn('-full-client.' if full else '-stage1.',result['archive']);self.assertEqual(result['startup_only'],not full);self.assertTrue(result['build_timestamp']);self.assertEqual(result['archive_sha256'],digest)
   self.assertIn('build-manifest.json',(art/'SHA256SUMS').read_text())
 def test_validated_fixture_finalization(self):self.run_fixture()
 def test_full_client_rejected(self):
  with self.assertRaises(SystemExit):self.run_fixture(startup=False)
 def test_full_client_explicit_held(self):self.run_fixture(startup=False,full=True)
 def test_changed_archive_rejected(self):
  with self.assertRaises(SystemExit):self.run_fixture(mismatch=True)
if __name__=='__main__':unittest.main()
