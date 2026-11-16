#!/usr/bin/env python3
"""Stream complete CI output; retain a bounded tail and the first useful error."""
import collections,json,os,pathlib,re,subprocess,sys
MAX_BYTES=512*1024

def main():
    if len(sys.argv)<3 or not re.fullmatch(r'[a-z0-9-]+',sys.argv[1]):
        print('usage: run-check.py phase command [args...]',file=sys.stderr);return 2
    phase=sys.argv[1];root=pathlib.Path(os.environ.get('PLEXIUM_LOG_DIR','artifacts/logs'));root.mkdir(parents=True,exist_ok=True)
    chunks=collections.deque();size=0;first=None
    try:
        process=subprocess.Popen(sys.argv[2:],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        # read1 bounds memory even for a subprocess emitting a huge unterminated line.
        partial=b''
        while chunk:=process.stdout.read1(8192):
            sys.stdout.buffer.write(chunk);sys.stdout.buffer.flush()
            chunks.append(chunk);size+=len(chunk)
            while size>MAX_BYTES:
                excess=size-MAX_BYTES;head=chunks.popleft()
                if len(head)>excess:chunks.appendleft(head[excess:]);size-=excess
                else:size-=len(head)
            if first is None:
                partial+=chunk
                lines=partial.split(b'\n');partial=lines.pop()[-4096:]
                for line in lines:
                    if re.search(rb'(?i)(fatal error:|error:|FAIL\b|Traceback|missing |not found)',line):
                        first=line[:4096].decode('utf-8','replace');break
        code=process.wait()
    except OSError as exc:
        code=127;first=f'Cannot launch check: {exc}';chunks.append(first.encode()[:4096])
    (root/(phase+'.log')).write_bytes(b''.join(chunks))
    result={'phase':phase,'exit_code':code,'first_actionable_error':first,'tail_limit_bytes':MAX_BYTES}
    (root/(phase+'.json')).write_text(json.dumps(result,indent=2)+'\n')
    if code:print(f'CHECK FAILED [{phase}] exit={code}: {first or "inspect bounded log tail and workflow output"}',file=sys.stderr)
    return code if code>=0 else 128-code
if __name__=='__main__':sys.exit(main())
