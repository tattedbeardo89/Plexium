#!/usr/bin/env python3
"""Guard the two startup frame reservations against measured working 0.1.
This is not a bound on full call-chain/driver stack use or a hardware launch test.
"""
import json,pathlib,re,subprocess,sys
elf=pathlib.Path(sys.argv[1]);out=pathlib.Path(sys.argv[2])
def reservation(symbol):
 text=subprocess.check_output(['objdump','-d','--disassemble='+symbol,str(elf)],text=True)
 assert '<'+symbol+'>:' in text,'Missing native function '+symbol
 values=re.findall(r'sub\s+\$0x([0-9a-f]+),%rsp',text)
 assert values,'Unrecognized native frame prologue: '+symbol
 return int(values[0],16)
main=reservation('main');view=reservation('_ZN7plexium4View5frameERKNS_5ModelEjf')
r={'main_reservation_bytes':main,'shared_view_reservation_bytes':view,'combined_reservations_bytes':main+view,
   'working_01_main_bytes':3744,'working_01_shared_view_bytes':1480,'working_01_combined_bytes':5224,
   'scope':'immediate stack reservations; excludes callee stacks and saved registers; no hardware acceptance'}
assert main+view<=5224, 'Startup frame reservations exceed the working 0.1 comparison: '+str(r)
r['comparison']='passed';out.write_text(json.dumps(r,indent=2)+'\n');print('PASS: native startup frame reservations',main,view,'combined',main+view,'<= working 0.1',5224)
