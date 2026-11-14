#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify downloaded GitHub artifacts against their source/debug/package identity."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import zipfile

root = Path(sys.argv[1])
expected_commit = sys.argv[2]

def unique(name):
    matches = list(root.rglob(name))
    assert len(matches) == 1, (name, len(matches))
    return matches[0]

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

checks = {}
for line in unique('SHA256SUMS').read_text().splitlines():
    checksum, name = line.split(maxsplit=1)
    name = name.lstrip('*')
    assert name == Path(name).name, name
    assert re.fullmatch('[0-9a-f]{64}', checksum), name
    assert digest(unique(name)) == checksum, name
    checks[name] = checksum
assert set(checks) == {'Plexium-PPSA85421.zip', 'Plexium-native-debug.elf',
                       'Plexium-native-source.tar.gz', 'Plexium-source.bundle'}
validation = json.loads(unique('native-validation.json').read_text())
assert validation['native_build_commit'] == expected_commit
assert validation['title_id'] == 'PPSA85421'
assert validation['zip_sha256'] == checks['Plexium-PPSA85421.zip']
assert validation['debug_elf_sha256'] == checks['Plexium-native-debug.elf']
elf = unique('Plexium-native-debug.elf')
assert expected_commit.encode() in elf.read_bytes(), 'Missing native source stamp'
notes = subprocess.check_output(['readelf', '-n', str(elf)], text=True)
assert re.search(r'Build ID: ([0-9a-f]+)', notes).group(1) == validation['elf_build_id']
sections = subprocess.check_output(['readelf', '-S', str(elf)], text=True)
assert '.debug_info' in sections and '.symtab' in sections, 'ELF lacks debug symbols'
with zipfile.ZipFile(unique('Plexium-PPSA85421.zip')) as archive:
    assert archive.testzip() is None
    names = archive.namelist()
    assert len(names) == len(set(names))
    assert all(n.startswith('PPSA85421/') and '..' not in Path(n).parts for n in names)
    for name in ['eboot.bin', 'sce_module/libc.prx', 'sce_sys/param.json',
                 'sce_sys/icon0.png', 'sce_sys/pic0.dds', 'sce_sys/pic1.dds',
                 'licenses/INDEX.json']:
        assert 'PPSA85421/' + name in names, name
    assert json.loads(archive.read('PPSA85421/sce_sys/param.json'))['titleId'] == 'PPSA85421'
    assert hashlib.sha256(archive.read('PPSA85421/eboot.bin')).hexdigest() == validation['self_sha256']
    assert len(json.loads(archive.read('PPSA85421/licenses/INDEX.json'))) >= 100
    for info in archive.infolist():
        expected = 0o755 if info.filename.endswith(('eboot.bin', '.prx')) else 0o644
        assert (info.external_attr >> 16) & 0o777 == expected, info.filename
with tarfile.open(unique('Plexium-native-source.tar.gz'), 'r:gz') as source:
    prefix = 'Plexium-native-source/'
    source_validation = json.load(source.extractfile(prefix + 'native-validation.json'))
    assert source_validation == validation, 'Source kit belongs to another build'
    manifest = source.extractfile(prefix + 'SHA256SUMS').read().decode()
    for line in manifest.splitlines():
        checksum, name = line.split(maxsplit=1)
        member = source.extractfile(prefix + name)
        h = hashlib.sha256()
        for chunk in iter(lambda: member.read(1024 * 1024), b''):
            h.update(chunk)
        assert h.hexdigest() == checksum, name
bundle = unique('Plexium-source.bundle')
with __import__('tempfile').TemporaryDirectory() as temporary:
    subprocess.run(['git', 'init', '--bare', '--quiet', temporary], check=True)
    subprocess.run(['git', '-C', temporary, 'bundle', 'verify', str(bundle.resolve())], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    subprocess.run(['git', '-C', temporary, 'fetch', '--quiet', str(bundle.resolve()), expected_commit], check=True)
    subprocess.run(['git', '-C', temporary, 'cat-file', '-e', expected_commit + '^{commit}'], check=True)
assert unique('INSTALLATION.md').stat().st_size > 0
assert unique('DELIVERY.md').stat().st_size > 0
report = {'status': 'passed', 'native_commit': expected_commit,
          'elf_build_id': validation['elf_build_id'], 'sha256': checks,
          'checks': ['downloaded checksums', 'ZIP CRC/identity/permissions', 'debug ELF/source stamp',
                     'source kit manifest', 'source bundle commit', 'installation/delivery instructions'],
          'hardware_verification': 'not run', 'live_plex_verification': 'not run'}
(root / 'github-delivery-verification.json').write_text(json.dumps(report, indent=2) + '\n')
print('::notice title=Verified GitHub downloads::' + json.dumps(report))
