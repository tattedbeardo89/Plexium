#!/usr/bin/env python3
"""Dependency identity excludes only the always-rebuilt application title script."""
import argparse,hashlib,pathlib,subprocess

def fingerprint(root,legacy_title=None):
    root=pathlib.Path(root)
    paths=[root/'ci/build-lock.json',root/'ci/source-checksums.txt']
    for folder in ['ps5','patches']:
        paths.extend(p for p in (root/folder).rglob('*') if p.is_file() and not p.is_symlink())
    h=hashlib.sha256()
    for p in sorted(paths,key=lambda p:p.relative_to(root).as_posix().encode()):
        name=p.relative_to(root).as_posix()
        if name=='ps5/build-title.sh':
            if legacy_title is None:continue
            data=legacy_title
        else:data=p.read_bytes()
        h.update((hashlib.sha256(data).hexdigest()+'  '+name+'\n').encode())
    return h.hexdigest()

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--legacy-reference');args=parser.parse_args()
    root=pathlib.Path(__file__).resolve().parents[1]
    title=None
    if args.legacy_reference:
        title=subprocess.check_output(['git','-C',str(root),'show',args.legacy_reference+':ps5/build-title.sh'])
    print(fingerprint(root,title))
