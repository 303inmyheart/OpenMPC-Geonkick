#!/usr/bin/env python3
"""Final native SOUND layout contract; validates the actual deliverable skin."""
import json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
params=json.loads((ROOT/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
p=next((ROOT/'build/skin').glob('*/Plugin Skins/TUI.json'));page=json.loads(p.read_text())['pageData'];defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
assert len(params)==4096
old=json.loads((ROOT/'tests/abi/sound-overview-baseline.json').read_text())['params'];assert params[:3985]==old[:3985]
assert params[3994:]==old[3994:]
assert [p['key'] for p in params[3992:3994]]==['sample_preview','sample_preview_stop']
assert [p['key'] for p in old[3992:3994]]==['viz_l3_e_11','viz_l3_e_12']
assert len(page['tabs'])==24
assert [sum(t['fnKeyIndex']==i for t in page['tabs']) for i in range(6)]==[1,6,6,6,1,4]
q=json.loads((p.parent/'Q-Links.json').read_text());maps=q['Screen Mode Q-Links']['map']
assert {(m['Tab']-1,m['SubTab']-1) for m in maps}=={(t['fnKeyIndex'],t['fnKeySubIndex']) for t in page['tabs']}
children=defs['SYNTH|LAYER 1']['componentsData'];selector=[c for c in children if c['componentData']['type']=='gkSoundLayerSelect'];assert len(selector)==1
assert selector[0]['bounds']['acceptsHWFocus']=='No'
assert not defs['gkSoundLayerSelect']['actions'];assert len(defs['gkSoundLayerSelect']['componentsData'])==3
mapping=next(m for m in maps if m['Tab']==1)
wanted={ids[f'sound_o{s}_{suffix}'] for s in range(1,4) for suffix in ('freq_fine','level')}|{ids['length_ms'],ids['volume']}
assert {i for i in mapping['Q-Links'].values() if i>=0}==wanted
for layer in range(3):
 enabled=f'IndexedEnabling/{layer}/3/Parameter {ids["sound_layer"]}'
 group=[c for c in children if enabled in c['bounds'].get('additionalInvalidatingHandles',[])]
 assert group
 for channel in ('p','n','e'):
  curves=[c for c in group if c['componentData']['type']=='gkPlot_'+channel];assert len(curves)==432
  expected={f'Parameter {ids[f"viz_{channel}_{col}" if channel=="e" else f"viz_sound_{channel}_{col}"]}' for col in range(432)}
  assert {m['value'] for c in curves for m in c['handle remapping']['map'] if m['key']=='Data'}==expected
 for row in range(4):
  members={mapping['Q-Links'][f'Q-Link {13+col-4*row}'] for col in range(4)}-{-1}
  if not members:continue
  x,y,w,h=map(int,next(t for t in page['tabs'] if t['fnKeyIndex']==0)['qlinkBoundsData'][row].split());assert 0<=x<x+w<=1280 and 0<=y<y+h<=628
  for c in group:
   if not any(m['key']=='Data' and m['value'] in {f'Parameter {i}' for i in members} for m in c.get('handle remapping',{}).get('map',[])):continue
   xx,yy,ww,hh=map(int,c['bounds']['bounds'].split());assert x<=xx and y<=yy and xx+ww<=x+w and yy+hh<=y+h
for definition in defs.values():
 for c in definition.get('componentsData',[]):
  handles=[m['value'] for m in c.get('handle remapping',{}).get('map',[])]+c.get('bounds',{}).get('additionalInvalidatingHandles',[])
  for handle in handles:
   for match in re.finditer(r'Parameter (\d+)',handle):assert int(match[1])<4096
for t in page['tabs']:
 assert not any(c['componentData']['type']=='gkViewMarker' for c in defs[t['componentName']]['componentsData'])
assert all(t['componentName']!='PRESETS|PRESETS' for t in page['tabs'])
print('24 native pages, three explicit conditional SOUND panels,432 columns, rotary-only Q-Links/full frames, no caption inference, all handles<=4095 PASS')
