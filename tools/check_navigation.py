#!/usr/bin/env python3
"""Check reachability, exhaustive rotary bindings, native focus and frame geometry."""
import json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(Path(os.environ.get('MPC_VST',ROOT.parent/'mpc-vst-plugins'))/'tools'))
import shadow_skin
layouts,_=shadow_skin.parse_layout(ROOT/'layout.conf');layouts={l['name']:l for l in layouts}
params=json.loads((ROOT/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
p=next((ROOT/'build/skin').glob('*/Plugin Skins/TUI.json'));page=json.loads(p.read_text())['pageData'];defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
q=json.loads((p.parent/'Q-Links.json').read_text());maps=q['Screen Mode Q-Links']['map'];tabs=page['tabs'];assert len(tabs)==27
assert {t['fnKeyIndex'] for t in tabs}==set(range(6))
assert [sum(t['fnKeyIndex']==k for t in tabs) for k in range(6)]==[3,6,6,6,1,5]
assert {(m['Tab']-1,m['SubTab']-1) for m in maps}=={(t['fnKeyIndex'],t['fnKeySubIndex']) for t in tabs}
for layer in range(1,4):
 assert [t['componentName'].split('|')[1] for t in tabs if t['fnKeyIndex']==layer]==[f'L{layer} {kind}{osc}' for kind in ('OSC','FX') for osc in range(1,4)]
assert [t['componentName'] for t in tabs if t['fnKeyIndex']==5]==['KIT|KIT','SAMPLES|SAMPLES','SYNTH|MIX','EFFECTS|EFFECTS','PRESETS|PRESETS']
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
print('27 pages reachable on six keys; only rotary Q-Links; selectors/sliders/points touch-only; full native bounds PASS')

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

# SOUND overview has no redundant nine-oscillator selector. The editor and
# MASTER/MIX still retain their original target controls.
focus=[w for w in layouts['SYNTH']['widgets'] if w.get('key')=='plot_osc' and w['kind'].startswith('enum')]
assert len(focus)==1 and focus[0]['banks']=='MIX'
assert any(w.get('key')=='plot_osc' and w['kind'].startswith('enum') for w in layouts['ENVELOPE']['widgets'])
kit=next(t for t in tabs if t['componentName']=='KIT|KIT')
cells=[c for c in defs[kit['componentName']]['componentsData'] if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids['kit_pad']) for m in c.get('handle remapping',{}).get('map',[]))]
assert len(cells)==16
assert all(c['bounds']['acceptsHWFocus']=='No' for c in cells)
print('Sixteen touch-only pad selectors; redundant upper SOUND selector removed; ENVELOPE target retained PASS')

audition=[c for c in defs[kit['componentName']]['componentsData'] if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids['audition']) for m in c.get('handle remapping',{}).get('map',[]))]
for c in audition:
 x,y,w,h=map(int,c['bounds']['bounds'].split())
 for cell in cells:
  xx,yy,ww,hh=map(int,cell['bounds']['bounds'].split())
  assert x+w<=xx or xx+ww<=x or y+h<=yy or yy+hh<=y,'Audition overlaps a pad'
for t in tabs:
 header=[c for c in defs[t['componentName']]['componentsData'] if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids['kit_title']) for m in c.get('handle remapping',{}).get('map',[]))]
 assert header
 x,y,w,h=map(int,header[0]['bounds']['bounds'].split());assert 14<=y and y+h<=56
print('Kit audition/pads do not overlap; selected-pad header fits the banner on every page PASS')

ys=sorted({int(c['bounds']['bounds'].split()[1]) for c in cells})
for c in cells:
 button=next(p for p in defs[c['componentData']['type']]['componentsData'] if p['componentData']['type']=='Button')
 i=button['componentData']['data']['buttonId']
 x,y,w,h=map(int,c['bounds']['bounds'].split());assert y==ys[3-i//4]
print('Native pad values match MPC physical layout:01 bottom-left,16 top-right PASS')

# The learned-map controls remain touch-only and fit inside the kit panel.
for key in ('kit_mapping','kit_learn'):
 controls=[c for c in defs[kit['componentName']]['componentsData'] if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids[key]) for m in c.get('handle remapping',{}).get('map',[]))]
 assert len(controls)==2
 for c in controls:
  assert c['bounds']['acceptsHWFocus']=='No'
  x,y,w,h=map(int,c['bounds']['bounds'].split());assert 70<=x and x+w<=1220 and 72<=y and y+h<=603
  for cell in cells:
   xx,yy,ww,hh=map(int,cell['bounds']['bounds'].split())
   assert x+w<=xx or xx+ww<=x or y+h<=yy or yy+hh<=y,'Learn controls overlap pad buttons'
print('Learn/map selectors touch-only, inside panel, separate from pad buttons PASS')

# Each SOUND layer uses its own cached envelope, never whichever layer was last
# globally edited. Matching sliders/readout share the layer-local focus handle.
for layer in range(1,4):
 name=f'SYNTH|LAYER {layer}';children=defs[name]['componentsData']
 curves=[c for c in children if c['componentData']['type']=='gkPlot_e']
 assert len(curves)==432
 mapped={m['value'] for c in curves for m in c['handle remapping']['map'] if m['key']=='Data'}
 assert mapped=={f'Parameter {ids[f"viz_e_{col}"]}' for col in range(432)}
 for channel in ('p','n'):
  wave={m['value'] for c in children if c['componentData']['type']=='gkPlot_'+channel for m in c.get('handle remapping',{}).get('map',[]) if m['key']=='Data'}
  assert wave=={f'Parameter {ids[f"viz_sound_{channel}_{col}"]}' for col in range(432)}
 assert any(any(m['key']=='Data' and m['value']==f'Parameter {ids[f"sound_l{layer}_focus"]}' for m in c.get('handle remapping',{}).get('map',[])) for c in children)
page_samples=defs['SAMPLES|SAMPLES']['componentsData']
for row in range(8):
 c=next(c for c in page_samples if any(m['key']=='Data' and m['value']==f'Parameter {ids[f"sample_pick{row}"]}' for m in c.get('handle remapping',{}).get('map',[])))
 assert any(m['key']=='Text' and m['value']==f'Parameter {ids[f"sample_row{row}"]}' for m in c['handle remapping']['map'])
 assert c['bounds']['acceptsHWFocus']=='No'
for tabname in ('KIT|KIT','SAMPLES|SAMPLES'):
 for c in defs[tabname]['componentsData']:
  if not any(m['key']=='Data' and m['value']==f'Parameter {ids["audition"]}' for m in c.get('handle remapping',{}).get('map',[])):continue
  x,y,w,h=map(int,c['bounds']['bounds'].split())
  for part in defs[c['componentData']['type']]['componentsData']:
   if part['componentData']['type']=='Button':assert part['bounds']['bounds']==f'0 0 {w} {h}','Aliased button assets/definitions across pages'
print('Three layer-specific SOUND curves/targets; eight live-text touch-only file rows; independent audition button geometry PASS')

for tab in tabs:
 children=defs[tab['componentName']]['componentsData']
 assert any(m['key']=='Data' and m['value']==f'Parameter {ids["kit_title"]}' for m in children[-1].get('handle remapping',{}).get('map',[])),'Header hidden by later bank artwork'
print('Selected-pad header above every bank artwork PASS')

# Model the actual MPC native handle registry, not just the unconstrained VST ABI.
assert len(params)<=4096
import re
for definition in defs.values():
    for component in definition.get('componentsData',[]):
        for remap in component.get('handle remapping',{}).get('map',[]):
            for match in re.finditer(r'Parameter (\d+)',remap['value']):assert int(match[1])<4096
        for handle in component.get('bounds',{}).get('additionalInvalidatingHandles',[]):
            for match in re.finditer(r'Parameter (\d+)',handle):assert int(match[1])<4096
print('All native data, text, visibility and invalidation handles within MPC Parameter0..4095 PASS')
for layer in range(1,4):
    children=defs[f'SYNTH|LAYER {layer}']['componentsData']
    assert any(c['componentData']['type']=='gkViewMarker' and any(m['value']==f'Parameter {ids[f"ui_view_l{layer}"]}' for m in c['handle remapping']['map']) for c in children)
for name in ('SYNTH|MIX','ENVELOPE|ENVELOPE'):
    assert any(c['componentData']['type']=='gkViewMarker' and any(m['value']==f'Parameter {ids["ui_view_mix"]}' for m in c['handle remapping']['map']) for c in defs[name]['componentsData'])

for tab in page['tabs']:
    for c in defs[tab['componentName']]['componentsData']:
        if c['componentData']['type']=='gkViewMarker':
            assert int(c['bounds']['bounds'].split()[1])==(242 if tab['componentName']=='ENVELOPE|ENVELOPE' else 128)
