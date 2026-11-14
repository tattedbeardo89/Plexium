import os,pathlib,subprocess,sys,tempfile
binary=sys.argv[1]
env={k:v for k,v in os.environ.items() if not k.startswith('PLEXIUM_TEST_')}
def run(config=None):
 e=env.copy()
 if config:e['PLEXIUM_TEST_CONFIG']=str(config)
 return subprocess.run([binary],env=e,capture_output=True,text=True)
r=run();assert r.returncode==77 and 'no network requests' in r.stdout
with tempfile.TemporaryDirectory() as folder:
 p=pathlib.Path(folder)/'config.json';p.write_text('{"server_token":"SYNTHETIC_SECRET_VALUE"}');p.chmod(0o644)
 r=run(p);assert r.returncode==2 and 'SYNTHETIC_SECRET_VALUE' not in r.stdout+r.stderr
 p.chmod(0o600);r=run(p);assert r.returncode==2 and 'SYNTHETIC_SECRET_VALUE' not in r.stdout+r.stderr
 p.write_text('{"server_url":"http://fixture.test","server_token":"SYNTHETIC_SECRET_VALUE","machine_id":"fixture"}')
 r=run(p);assert r.returncode==2 and 'fixture.test' not in r.stdout+r.stderr
 p.unlink();p.symlink_to('/dev/null');assert run(p).returncode==2
print('PASS: optional real-PMS runner skips without credentials, rejects unsafe config/symlinks/HTTP, and redacts secrets and addresses')
