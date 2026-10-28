#!/usr/bin/env bash
# Never create/publish a public release; only update this existing draft.
set -euo pipefail
if [[ -f ci/HARDWARE-HOLD ]]; then
 echo "Hardware hold: retain CI investigation artifacts; do not update candidate delivery."
 exit 0
fi
repo=${GITHUB_REPOSITORY:?GitHub repository required}
tag=v0.2.0-native-candidate.1
release=$(gh api "repos/$repo/releases" --jq ".[] | select(.tag_name==\"$tag\" and .draft==true) | .id")
if [[ -z $release ]]; then
 echo 'No matching draft: native Actions artifacts are available; no release was changed.'
 exit 0
fi
gh api "repos/$repo/releases/$release" > artifacts/release-current.json
python3 - <<'PY'
from pathlib import Path
import hashlib,json,os
v=json.loads(Path('artifacts/native-validation.json').read_text())
body=f"Native PS5 development candidate PPSA85421, built in GitHub Actions from {v['native_build_commit']}.\n\nELF/SELF integrity, package identity/assets/CRC/modes and matching debug identity passed. Host regressions and ASan/UBSan passed earlier in this workflow.\n\nPhysical firmware 13.40, live Plex playback and console artwork acceptance remain UNTESTED. The feature set is incomplete. Follow INSTALLATION.md and preserve PlexStation PPSA85301. No hardware deployment occurred.\n\nSHA256SUMS records the attached installation, debug and source files. Run: https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}\n"
desired={'body':body,'name':'Plexium native PS5 browsing update 0.2 (hardware acceptance pending)','tag_name':'v0.2.0-native-candidate.1','target_commitish':v['native_build_commit'],'draft':True,'prerelease':True}
current=json.loads(Path('artifacts/release-current.json').read_text())
Path('artifacts/release-update.json').write_text(json.dumps(desired))
# A metadata PATCH can regenerate GitHub's draft URLs. Keep the prior verified
# run link when this exact native candidate already has the desired metadata.
changed=not current['assets'] or any(current.get(k)!=value for k,value in desired.items() if k!='body')
Path('artifacts/release-update-needed').write_text('yes' if changed else 'no')
existing={a['name']:a.get('digest') for a in current['assets']}
uploads=[]
for name in ['Plexium-PPSA85421.zip','Plexium-native-debug.elf','Plexium-native-source.tar.gz',
             'Plexium-source.bundle','SHA256SUMS','native-validation.json','INSTALLATION.md','DELIVERY.md']:
    path=Path('artifacts')/name
    with path.open('rb') as f:
        digest=hashlib.file_digest(f,'sha256').hexdigest()
    if existing.get(name)!='sha256:'+digest:
        uploads.append(str(path))
Path('artifacts/release-upload-files.txt').write_text(''.join(p+'\n' for p in uploads))
PY
if [[ $(cat artifacts/release-update-needed) == yes ]]; then
 gh api --method PATCH "repos/$repo/releases/$release" --input artifacts/release-update.json --silent
fi
(cd artifacts && sha256sum --check SHA256SUMS)
mapfile -t uploads < artifacts/release-upload-files.txt
if (( ${#uploads[@]} )); then
 gh release upload "$tag" --repo "$repo" --clobber "${uploads[@]}"
else
 echo 'Draft metadata and every asset already match the verified native delivery.'
fi
