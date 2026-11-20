#!/usr/bin/env python3
"""Build native MPC Sounds-browser presets from authentic current VST states."""
import hashlib,json,os,sys,xml.etree.ElementTree as ET
from pathlib import Path
import build
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
 name=stem.split('-',1)[1];text=xpl.xpl(cfg['name'],cfg['vendor'],f'%payload-path%/{folder}/{cfg["so"]}',uid,cfg['version'],raw,preset=name)
 tree=ET.fromstring(text);bank=xpl.b64dec(tree.findtext('state'));assert bank[:4]==b'CcnK' and bank[8:12]==b'FBCh'
 assert int.from_bytes(bank[16:20],'big')==uid and int.from_bytes(bank[156:160],'big')==len(raw) and bank[160:]==raw
 assert tree.findtext('preset')==name
 (dest/(stem+'.xpl')).write_text(text)
 manifest.append(dict(file=stem+'.xpl',name=name,state_sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw)))
(root_default:=dest/'0000-Default.xpl').unlink(missing_ok=True)
(ROOT/'build/native-preset-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Native MPC browser: Init,7 MPC starter sounds and16-pad starter kit; all9 JUCE/XPL bank payloads verified')
