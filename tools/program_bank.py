#!/usr/bin/env python3
"""Names and immutable complete states for the native MPC VST program menu."""
import json
from pathlib import Path
import factory_bank as bank
ROOT=bank.ROOT
PACK={'AVL-Drumkit':'AVL','DSoS':'DSoS','DigitalSamba':'Samba','GeonkickBells':'Bells','HaraldReveryKicks':'Harald','SPKickPresetsVol1':'SP Kick','TimeKit':'TimeKit','ToniLinkTechnoKicks':'Toni Link','UnfaTutorial':'Unfa'}
def names(presets):
 result=json.loads((ROOT/'params.json').read_text())['params'][8]['options']+['Starter Kit (16 Pads)']
 for p in presets:
  name=('AVL-derived Kit' if p['pack']=='AVL-Drumkit' else PACK[p['pack']]+' '+p['name']+(' Kit' if p['kit'] else ''))
  name=name.encode('ascii',errors='replace').decode()[:23]
  if name in result:name=name[:18]+' %03d'%len(result)
  assert name and name not in result;result.append(name)
 assert len(result)==118 and all(len(s.encode())<=23 for s in result)
 return result

def starter():
 keys,defaults,_=bank.definitions()
 changes=[{},dict(o1_freq=43,o1_decay=1.1,length=1.2,o1_sweep=30,o1_sweep_time=.065),dict(length=.24,o1_decay=.22,o1_freq=65,o1_sweep_time=.018),dict(length=.3,o1_freq=180,o1_level=.45,o1_sweep=4,o1_decay=.15,o2_on=1,o2_wave=4,o2_level=.65,o2_sweep=0,o2_decay=.23),dict(o1_wave=4,o1_sweep=0,length=.1,o1_decay=.075,filter_on=1,filter_type=1,cutoff=6500,cutoff_wide=6500,filter_model=0),dict(o1_wave=4,o1_sweep=0,length=.65,o1_decay=.6,filter_on=1,filter_type=1,cutoff=6500,cutoff_wide=6500,filter_model=0),dict(length=.5,o1_wave=1,o1_freq=370,o1_level=.35,o1_sweep=0,o1_fm=.6,o2_on=1,o2_freq=541,o2_sweep=0,o2_decay=.45),dict(o1_freq=115,length=.6,o1_sweep=7,o1_sweep_time=.12)]
 programs=[1,2,3,4,5,6,7,7,1,3,4,5,6,6,7,0];tunes=[0,0,0,0,0,0,0,7,-5,3,7,-5,-12,7,-5,12];rows=[]
 for p,t in zip(programs,tunes):
  row=defaults.copy();row.update(changes[p]);row['preset']=p;row['tune']=t;rows.append(row)
 tokens=['GKMP9']+[bank.num(rows[0][k]) for k in keys]+['KIT1','1','36','0','1']
 for row in rows[1:]:tokens+=['P']+[bank.num(row[k]) for k in keys]
 tokens+=['MAP1','0']+list(map(str,range(36,52)))
 return (' '.join(tokens)+'\0').encode()
def stamp(raw,index):
 assert raw.endswith(b'\0') and b' PRG1 ' not in raw
 return raw[:-1]+f' PRG1 {index}'.encode()+b'\0'
def generate(records,presets):
 labels=names(presets);entries=[]
 for i,label in enumerate(labels):
  raw=None if i<8 else stamp(starter() if i==8 else bank.state(presets[i-9],records),i)
  if raw:
   # Adjacent literals keep compiler line lengths bounded. No runtime file IO.
   text=raw[:-1].decode();literal='\n'.join(json.dumps(text[j:j+4096]) for j in range(0,len(text),4096))
  else:literal='NULL'
  ids=presets[i-9]['ids'] if i>=9 else []
  entries.append('{'+json.dumps(label)+','+literal+',{'+(','.join(map(str,ids)) if ids else '0')+'}}')
 header='/* Generated immutable program bank. Software GPL; sounds retain pack licences. */\ntypedef struct {const char *name,*state;int ids[16];} BankProgram;\n#define BANK_PROGRAM_COUNT 118\nstatic const BankProgram bank_programs[]={\n'+',\n'.join(entries)+'\n};\n'
 (ROOT/'build/program_bank_data.h').write_text(header)
 (ROOT/'build/program-bank-names.json').write_text(json.dumps(labels,indent=2)+'\n')
