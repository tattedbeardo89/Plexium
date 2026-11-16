#!/usr/bin/env python3
"""Create an uploadable failure record even when a check never started."""
import json,os,pathlib
root=pathlib.Path('artifacts/logs');root.mkdir(parents=True,exist_ok=True)
checks=[]
for path in sorted(root.glob('*.json')):
    if path.name=='failure-summary.json':continue
    try:checks.append(json.loads(path.read_text()))
    except (OSError,ValueError):continue
(root/'failure-summary.json').write_text(json.dumps({'commit':os.environ.get('GITHUB_SHA','local'),'run_id':os.environ.get('GITHUB_RUN_ID','local'),'checks':checks,'note':'First failing check remains the primary error. Missing check records indicate an earlier workflow/prerequisite failure; inspect the original step.'},indent=2)+'\n')
