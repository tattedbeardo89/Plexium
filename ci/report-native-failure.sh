#!/usr/bin/env bash
# Keep bounded build diagnostics with the existing draft when artifact access fails.
set -euo pipefail
repo=${GITHUB_REPOSITORY:?}
release=$(gh api "repos/$repo/releases" --jq '.[] | select(.tag_name=="v0.1.0-native-candidate.1" and .draft==true) | .id')
[[ -n $release ]] || exit 0
gh api "repos/$repo/releases/$release" > artifacts/draft-before-failure.json
python3 - <<'PY'
import json, os
from pathlib import Path
p=Path('artifacts/draft-before-failure.json')
r=json.loads(p.read_text())
marker='\n\n<!-- native-build-diagnostics -->'
body=(r.get('body') or '').split(marker)[0]
log=Path('artifacts/logs/hosted-native.log')
tail='\n'.join(log.read_text(errors='replace').splitlines()[-100:])[-20000:] if log.exists() else 'Native build did not start.'
body+=marker+f"\nLatest native build failed; no new binary delivery is claimed. Run: https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}\n\n```text\n{tail.replace('```','~~~')}\n```\n"
Path('artifacts/draft-failure-update.json').write_text(json.dumps({'body':body,'draft':True,'prerelease':True}))
PY
gh api --method PATCH "repos/$repo/releases/$release" --input artifacts/draft-failure-update.json --silent
