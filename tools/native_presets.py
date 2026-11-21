#!/usr/bin/env python3
"""Build native MPC Sounds-browser presets from authentic current VST states."""
import hashlib,json,os,sys,xml.etree.ElementTree as ET
from pathlib import Path
import build
import factory_bank
import program_bank
sys.path.insert(0,str(build.MPC/'tools'));import xpl
ROOT=build.ROOT
cfg=build.CFG
folder=f"{cfg['vendor']} - VST - {cfg['name']}"
skin=ROOT/'build/skin'/folder
out=ROOT/'build/native-preset-export';out.mkdir(exist_ok=True)
# Normal native helper and library keep sanitizer runtime out of Python.
f=build.flags('native');cc=build.compiler('native')
build.run([cc,*f,'-Itools','-Dmain=plugin_test_main','-c','tools/export_native_presets.c','-o',out/'export.o'])
build.run([cc,*f,out/'export.o','tests/load_plugin.c','-ldl','-lpthread','-lm','-o',out/'export'])
os.environ['GK_NATIVE_PRESETS']=str(out)
build.run([out/'export',ROOT/'build/native'/cfg['so']])
names=[f'{i:04d}-{name}' for i,name in enumerate(json.loads((ROOT/'params.json').read_text())['params'][8]['options'])]+['1000-Starter Kit (16 Pads)']
dest=skin/'Presets';dest.mkdir(exist_ok=True)
uid=int.from_bytes(cfg['uid'].encode(),'big');manifest=[]
for stem in names:
 raw=(out/(stem+'.chunk')).read_bytes();assert raw.startswith(b'GKMP9 ') and raw[-1:]==b'\0'
 name=stem.split('-',1)[1]
 if stem.startswith('1000-'):raw=program_bank.stamp(raw,8);(out/(stem+'.chunk')).write_bytes(raw)
 text=xpl.xpl(cfg['name'],cfg['vendor'],f'%payload-path%/{folder}/{cfg["so"]}',uid,cfg['version'],raw,preset=name)
 tree=ET.fromstring(text);bank=xpl.b64dec(tree.findtext('state'));assert bank[:4]==b'CcnK' and bank[8:12]==b'FBCh'
 assert int.from_bytes(bank[16:20],'big')==uid and int.from_bytes(bank[156:160],'big')==len(raw) and bank[160:]==raw
 assert tree.findtext('preset')==name
 (dest/(stem+'.xpl')).write_text(text)
 manifest.append(dict(file=stem+'.xpl',name=name,state_sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw)))
(root_default:=dest/'0000-Default.xpl').unlink(missing_ok=True)
records,factory_presets,notices=factory_bank.prepare()
factory_manifest=[]
program_names=program_bank.names(factory_presets)
for j,p in enumerate(factory_presets):
 name=f'{p["pack"]} - {p["name"]}'+(' (Kit)' if p['kit'] else '')
 if p['pack']=='AVL-Drumkit':name='MPC AVL-derived Kit'
 stem=f'{2000+j:04d}-'+name
 raw=program_bank.stamp(factory_bank.state(p,records),j+9)
 name=program_names[j+9]
 text=xpl.xpl(cfg['name'],cfg['vendor'],f'%payload-path%/{folder}/{cfg["so"]}',uid,cfg['version'],raw,preset=name)
 (dest/(stem+'.xpl')).write_text(text)
 item=dict(file=stem+'.xpl',name=name,state_sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw),pack=p['pack'],source=p['source'],ids=p['ids'])
 manifest.append(item);factory_manifest.append(item)
 (out/(stem+'.chunk')).write_bytes(raw)
license_text=(ROOT/'factory/NOTICE.txt').read_text()+'\n\n'+'\n\n'.join(f'===== {pack}: {source} =====\n{text}' for pack,source,text in notices)
(skin/'Factory-Licenses.txt').write_text(license_text)
(ROOT/'build/factory-native-manifest.json').write_text(json.dumps(factory_manifest,indent=2)+'\n')
(ROOT/'build/native-preset-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Native MPC browser:9 MPC starting states plus109 original factory presets/kits; source licences included')
