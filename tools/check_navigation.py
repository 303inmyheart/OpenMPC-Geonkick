#!/usr/bin/env python3
"""Check reachability, exhaustive rotary bindings, native focus and frame geometry."""
import json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(Path(os.environ.get('MPC_VST',ROOT.parent/'mpc-vst-plugins'))/'tools'))
import shadow_skin
layouts,_=shadow_skin.parse_layout(ROOT/'layout.conf');layouts={l['name']:l for l in layouts}
params=json.loads((ROOT/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
p=next((ROOT/'build/skin').glob('*/Plugin Skins/TUI.json'));page=json.loads(p.read_text())['pageData'];defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
q=json.loads((p.parent/'Q-Links.json').read_text());maps=q['Screen Mode Q-Links']['map'];tabs=page['tabs'];assert len(tabs)==25
assert {t['fnKeyIndex'] for t in tabs}==set(range(6))
assert [sum(t['fnKeyIndex']==k for t in tabs) for k in range(6)]==[3,6,6,6,1,3]
assert {(m['Tab']-1,m['SubTab']-1) for m in maps}=={(t['fnKeyIndex'],t['fnKeySubIndex']) for t in tabs}
for layer in range(1,4):
 assert [t['componentName'].split('|')[1] for t in tabs if t['fnKeyIndex']==layer]==[f'L{layer} {kind}{osc}' for osc in range(1,4) for kind in ('OSC','FX')]
assert [t['componentName'] for t in tabs if t['fnKeyIndex']==5]==['SYNTH|MIX','EFFECTS|EFFECTS','PRESETS|PRESETS']
points={'point_select','point_level_fine','point_time_fine','point_copy','free_level','free_time','free_bend'}
for t in tabs:
 source,bank=t['componentName'].split('|');children=defs[t['componentName']]['componentsData']
 wanted={ids[w['key']] for w in layouts[source]['widgets'] if w['kind']=='knob' and w['key'] not in points and (not w.get('banks') or bank in w['banks'].split('|'))}
 m=next(m for m in maps if (m['Tab'],m['SubTab'])==(t['fnKeyIndex']+1,t['fnKeySubIndex']+1));assert m['Bank Direction']=='RowTopDown'
 assigned=[i for i in m['Q-Links'].values() if i>=0];assert set(assigned)==wanted and len(assigned)==len(wanted),(bank,wanted,assigned)
 assert not any(params[i].get('options') or params[i].get('live') or params[i].get('momentary') for i in assigned),(bank,'non-rotary binding')
 for row in range(4):
  members={m['Q-Links'][f'Q-Link {13+col-4*row}'] for col in range(4)}-{-1}
  if not members:continue
  x,y,w,h=map(int,t['qlinkBoundsData'][row].split());assert 0<=x<x+w<=1280 and 0<=y<y+h<=628
  for c in children:
   data=next((v['value'] for v in c.get('handle remapping',{}).get('map',[]) if v['key']=='Data'),None)
   if data not in {f'Parameter {i}' for i in members}:continue
   xx,yy,ww,hh=map(int,c['bounds']['bounds'].split());assert x<=xx and y<=yy and xx+ww<=x+w and yy+hh<=y+h,(bank,'cropped rotary/label/value')
 for c in children:
  data=next((v['value'] for v in c.get('handle remapping',{}).get('map',[]) if v['key']=='Data'),None);d=defs.get(c['componentData']['type'])
  if not data or not d or d.get('ignoreMousePresses'):continue
  if int(data.split()[1]) not in wanted:
   assert c['bounds']['acceptsHWFocus']=='No'
   assert not any(a.get('handler')=='Q-Link' for a in d.get('actions',[])),(bank,'non-rotary hardware focus')
 print(t['tabName'],':',', '.join(params[i]['key'] for i in assigned) or 'touch only')
print('25 pages reachable on six keys; only rotary Q-Links; selectors/sliders/points touch-only; full native bounds PASS')

# The native eight-button grid must be centred inside the actual painted field.
from skin_design import PRESET_FRAME
preset=next(t for t in tabs if t['componentName']=='PRESETS|PRESETS')
cells=[c for c in defs[preset['componentName']]['componentsData'] if any(m['key']=='Data' and m['value']=='Parameter '+str(ids['preset']) for m in c.get('handle remapping',{}).get('map',[]))]
assert len(cells)==8
rects=[tuple(map(int,c['bounds']['bounds'].split())) for c in cells]
l=min(r[0] for r in rects);top=min(r[1] for r in rects);r=max(x+w for x,y,w,h in rects);bottom=max(y+h for x,y,w,h in rects)
fl,ft,fr,fb=PRESET_FRAME
assert fl<=l<r<=fr and ft<=top<bottom<=fb
assert abs((l+r)-(fl+fr))<=1 and abs((top+bottom)-(ft+fb))<=1
print('All eight preset buttons inside and centred in their panel, horizontal/vertical margins balanced PASS')
