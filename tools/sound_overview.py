#!/usr/bin/env python3
"""Consolidate styled SOUND panels behind one explicit native layer selector.
Run after skin_design.py. Detail pages, physical parameter IDs and CC seed stay
unchanged; Q-Links use UI-only context proxies, not guessed page-format events.
"""
import json,os,sys
from copy import deepcopy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MPC=Path(os.environ.get('MPC_VST',ROOT.parent/'mpc-vst-plugins'))
sys.path.insert(0,str(MPC/'tools'))
import shadow_skin
from skin_design import segment_image
params=json.loads((ROOT/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
p=next((ROOT/'build/skin').glob('*/Plugin Skins/TUI.json'));doc=json.loads(p.read_text());page=doc['pageData']
defs={x['key']:x['value'] for x in page['componentDefinitions']['localComponentDefinitions']}
# The three original panels are already styled with their proper layer labels.
# Flatten their controls into one page; native IndexedEnabling owns visibility.
combined=[]
for layer in range(3):
 children=deepcopy(defs[f'SYNTH|LAYER {layer+1}']['componentsData'])
 for c in children:
  if c['componentData']['type']=='gkViewMarker':continue
  mapping=c.get('handle remapping',{}).get('map',[])
  if any(m['key']=='Data' and m['value']==f'Parameter {ids[f"sound_l{layer+1}_focus"]}' for m in mapping):
   c['bounds']['bounds']='805 90 150 28'
  for m in mapping:
   if m['key']!='Data':continue
   for slot in range(3):
    for suffix in ('freq_fine','level'):
     if m['value']==f'Parameter {ids[f"o{layer*3+slot+1}_{suffix}"]}':
      m['value']=f'Parameter {ids[f"sound_o{slot+1}_{suffix}"]}'
  c['bounds'].setdefault('additionalInvalidatingHandles',[]).append(f'IndexedEnabling/{layer}/3/Parameter {ids["sound_layer"]}')
  combined.append(c)
# Native touch buttons explicitly write the selected layer. No Q-Link handler.
parts=[]
for layer in range(3):
 base=f'gk_sound_layer_{layer+1}'
 for state in ('on','off'):segment_image(150,32,f'LAYER {layer+1}',state=='on').save(p.parent/f'{base}_{state}.png')
 parts.append(shadow_skin._button(base+'_on.png',base+'_off.png',layer,3,150,32,x=layer*160))
item=shadow_skin._local('gkSoundLayerSelect',[],parts);item['value']['hideQLinkBounds']=True
page['componentDefinitions']['localComponentDefinitions'].append(item)
combined.append(shadow_skin._placed('gkSoundLayerSelect','Layer',ids['sound_layer'],286,174,470,32,focus='No'))
defs['SYNTH|LAYER 1']['componentsData']=combined
for definition in page['componentDefinitions']['localComponentDefinitions']:
 definition['value']['componentsData']=[c for c in definition['value'].get('componentsData',[]) if c['componentData']['type']!='gkViewMarker']
page['tabs']=[t for t in page['tabs'] if not(t['fnKeyIndex']==0 and t['fnKeySubIndex']>0) and t['componentName']!='PRESETS|PRESETS']
sound=next(t for t in page['tabs'] if t['fnKeyIndex']==0);sound['tabName']='SOUND'
qpath=p.parent/'Q-Links.json';q=json.loads(qpath.read_text());maps=q['Screen Mode Q-Links']['map']
q['Screen Mode Q-Links']['map']=[m for m in maps if not(m['Tab']==1 and m['SubTab']>1) and not(m['Tab']==6 and m['SubTab']==5)]
replacements={ids[f'o{slot}_{suffix}']:ids[f'sound_o{slot}_{suffix}'] for slot in range(1,4) for suffix in ('freq_fine','level')}
for mapping in q['Screen Mode Q-Links']['map']:
 if mapping['Tab']==1:
  mapping['Q-Links']={key:replacements.get(value,value) for key,value in mapping['Q-Links'].items()}
program=q.get('Program Mode Q-Links',{});program=program.get('map',program)
if isinstance(program,dict):
 for key,value in program.items():
  if key.startswith('Q-Link '):program[key]=replacements.get(value,value)
p.write_text(json.dumps(doc,indent=2)+'\n');qpath.write_text(json.dumps(q,indent=2)+'\n')
print('One SOUND overview, explicit three-way native layer selector, shared rotary proxies, detail pages unchanged')
