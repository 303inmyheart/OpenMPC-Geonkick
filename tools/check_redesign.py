#!/usr/bin/env python3
"""Deliverable navigation/touch wiring and preview-versus-load separation."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=next((root/'build/skin').glob('*/Plugin Skins/TUI.json'));page=json.loads(p.read_text())['pageData'];defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
assert len(defs)==len(page['componentDefinitions']['localComponentDefinitions']),'Duplicate component definitions'
params=json.loads((root/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
def key(c):
 s=next((m['value'] for m in c.get('handle remapping',{}).get('map',[]) if m['key']=='Data'),None)
 return params[int(s.split()[1])]['key'] if s and s.startswith('Parameter ') else None
children=defs['SAMPLES|SAMPLES']['componentsData'];targets={key(c):c for c in children if key(c)}
rects=[]
for k in ['sample_preview','sample_preview_stop','sample_load','sample_clear']+[f'sample_pick{i}' for i in range(8)]:
 c=targets[k];x,y,w,h=map(int,c['bounds']['bounds'].split());assert 24<=x and x+w<=1256 and 78<=y and y+h<=610
 assert w>=100 and h>=40 and c['bounds']['acceptsHWFocus']=='No'
 d=defs[c['componentData']['type']];assert not any(a.get('handler')=='Q-Link' for a in d['actions'])
 rects.append((k,x,y,w,h))
 if k.startswith('sample_pick'):
  labels=[p for p in d['componentsData'] if p['componentData']['type']=='Label'];assert labels and labels[0]['componentData']['data']['handleName']=='Text'
  # The top native gesture button must be transparent, so row text stays visible.
  buttons=[p for p in d['componentsData'] if p['componentData']['type']=='Button'];assert '_hit_' in buttons[-1]['componentData']['data']['offImage']
for i,(k,x,y,w,h) in enumerate(rects):
 for kk,xx,yy,ww,hh in rects[i+1:]:assert x+w<=xx or xx+ww<=x or y+h<=yy or yy+hh<=y,(k,kk,'touch overlap')
for layer in range(1,4):
 assert [t['tabName'] for t in page['tabs'] if t['fnKeyIndex']==layer]==[f'L{layer} / {kind} {osc}' for kind in ('OSC','FX') for osc in range(1,4)]
print('Final redesign: grouped OSC/FX pages, unique definitions, distinct touch-only Preview/Stop/Load, eight legible file targets, no overlapping sample actions PASS')

# Check actual final native hardware groups after every visual postprocessor.
q=json.loads((p.parent/'Q-Links.json').read_text())['Screen Mode Q-Links']['map']
for tab in page['tabs']:
 mapping=next(m for m in q if m['Tab']==tab['fnKeyIndex']+1 and m['SubTab']==tab['fnKeySubIndex']+1)
 children=defs[tab['componentName']]['componentsData']
 for row in range(4):
  members={mapping['Q-Links'][f'Q-Link {13+col-row*4}'] for col in range(4)}-{-1}
  if not members:continue
  x,y,w,h=map(int,tab['qlinkBoundsData'][row].split())
  assert 0<=x<x+w<=1280 and 0<=y<y+h<=628
  found=set()
  for c in children:
   i=ids.get(key(c),-1)
   if i not in members:continue
   xx,yy,ww,hh=map(int,c['bounds']['bounds'].split());assert x<=xx and y<=yy and xx+ww<=x+w and yy+hh<=y+h,(tab['tabName'],key(c),'focus cuts a control')
   definition=defs[c['componentData']['type']]
   assert any(part['componentData']['type']=='Knob' for part in definition.get('componentsData',[])),(tab['tabName'],key(c),'Q-Link bound to non-knob')
   found.add(i)
  assert found==members,(tab['tabName'],'missing Q-Link control')
print('All final pages: Q-Link maps match visible rotary controls; frames include entire native label/value/hit bounds PASS')
