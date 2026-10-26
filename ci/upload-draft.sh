#!/usr/bin/env bash
# Never create/publish a public release; only update this existing draft.
set -euo pipefail
repo=${GITHUB_REPOSITORY:?GitHub repository required}
tag=v0.1.0-native-candidate.1
release=$(gh api "repos/$repo/releases" --jq ".[] | select(.tag_name==\"$tag\" and .draft==true) | .id")
if [[ -z $release ]]; then
 echo 'No matching draft: native Actions artifacts are available; no release was changed.'
 exit 0
fi
python3 - <<'PY'
from pathlib import Path
import json,os
v=json.loads(Path('artifacts/native-validation.json').read_text())
body=f"Native PS5 development candidate PPSA85421, built in GitHub Actions from {v['native_build_commit']}.\n\nELF/SELF integrity, package identity/assets/CRC/modes and matching debug identity passed. Host regressions and ASan/UBSan passed earlier in this workflow.\n\nPhysical firmware 13.40, live Plex playback and console artwork acceptance remain UNTESTED. The feature set is incomplete. Follow INSTALLATION.md and preserve PlexStation PPSA85301. No hardware deployment occurred.\n\nSHA256SUMS records the attached installation, debug and source files. Run: https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}\n"
Path('artifacts/release-update.json').write_text(json.dumps({'body':body,'target_commitish':v['native_build_commit'],'draft':True,'prerelease':True}))
PY
gh api --method PATCH "repos/$repo/releases/$release" --input artifacts/release-update.json --silent
(cd artifacts && sha256sum --check SHA256SUMS)
gh release upload "$tag" --repo "$repo" --clobber \
 artifacts/Plexium-PPSA85421.zip artifacts/Plexium-native-debug.elf \
 artifacts/Plexium-native-source.tar.gz artifacts/Plexium-source.bundle \
 artifacts/SHA256SUMS artifacts/native-validation.json artifacts/INSTALLATION.md artifacts/DELIVERY.md
