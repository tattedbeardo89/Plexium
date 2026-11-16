#!/usr/bin/env python3
import json,os,pathlib,subprocess,sys,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Diagnostics(unittest.TestCase):
 def test_first_error_exit_status_and_bounded_tail(self):
  with tempfile.TemporaryDirectory() as directory:
   env=dict(os.environ,PLEXIUM_LOG_DIR=directory)
   code="import sys;print('compiler error: actionable first failure',flush=True);sys.stdout.write('x'*2000000);sys.exit(23)"
   result=subprocess.run([sys.executable,str(ROOT/'ci/run-check.py'),'failed',sys.executable,'-c',code],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
   self.assertEqual(result.returncode,23)
   report=json.loads((pathlib.Path(directory)/'failed.json').read_text())
   self.assertIn('actionable first failure',report['first_actionable_error'])
   self.assertLessEqual((pathlib.Path(directory)/'failed.log').stat().st_size,512*1024)
 def test_missing_command_is_recorded(self):
  with tempfile.TemporaryDirectory() as directory:
   result=subprocess.run([sys.executable,str(ROOT/'ci/run-check.py'),'missing','/missing-plexium-command'],env=dict(os.environ,PLEXIUM_LOG_DIR=directory),stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
   self.assertEqual(result.returncode,127)
   self.assertTrue((pathlib.Path(directory)/'missing.json').is_file())
 def test_no_check_logs_still_produce_diagnostic(self):
  with tempfile.TemporaryDirectory() as directory:
   subprocess.run([sys.executable,str(ROOT/'ci/collect-failure.py')],cwd=directory,check=True)
   report=json.loads((pathlib.Path(directory)/'artifacts/logs/failure-summary.json').read_text())
   self.assertEqual(report['checks'],[])
if __name__=='__main__':unittest.main()
