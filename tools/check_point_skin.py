#!/usr/bin/env python3
"""Validate fixed-point native skin wiring, visibility and append-only ABI."""
import json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
params=json.loads((root/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
page=json.loads(next((root/'build/skin').glob('*/Plugin Skins/TUI.json')).read_text())['pageData']
defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
assert len(params)==7006
previous=json.loads(subprocess.check_output(['git','show','10b4db3:params.json'],cwd=root))['params']
assert params[:len(previous)]==previous,'Changed previous VST identities/ranges/defaults'
for tab in page['tabs']:
    if tab['componentName']!='ENVELOPE|ENVELOPE':continue
    children=defs[tab['componentName']]['componentsData']
    pads=[c for c in children if c['componentData']['type']=='gkPointXY']
    assert len(pads)==1
    mapping={m['key']:m['value'] for m in pads[0]['handle remapping']['map']}
    assert mapping['X']=='Parameter '+str(ids['xy_x']) and mapping['Y']=='Parameter '+str(ids['xy_y'])
    markers=[c for c in children if c['componentData']['type']=='gkPointDot'];assert len(markers)==16
    assert defs['gkPointDot']['ignoreMousePresses']
    assert not any(c['componentData']['type']=='gkFreeEnvelope' for c in children)
    curves=[c for c in children if c['componentData']['type'].startswith('gkEditCurve_')]
    assert len(curves)==432
print('All previous 3056 parameter identities preserved; absolute XY point proxies and full DSP curve PASS')
# Editor draws the same completed sound behind the draggable curve. No new
# data channel or per-column host callback path is introduced by this skin.
for tab in page['tabs']:
    if tab['componentName']!='ENVELOPE|ENVELOPE':continue
    children=defs[tab['componentName']]['componentsData']
    wave=[(i,c) for i,c in enumerate(children) if c['componentData']['type'].startswith('gkEditWave_')]
    assert len(wave)==864
    first_curve=min(i for i,c in enumerate(children) if c['componentData']['type']=='gkPointXY')
    assert max(i for i,c in wave)<first_curve
    for _,c in wave:
        assert c['bounds']['additionalInvalidatingHandles']==['Parameter '+str(ids['graph_revision'])]
        assert defs[c['componentData']['type']]['ignoreMousePresses']
    point_keys=['point_copy','free_select','free_level','free_time','free_bend','free_add','free_delete','free_prev','free_next']
    for c in children:
        if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids[k]) for k in point_keys for m in c.get('handle remapping',{}).get('map',[])):
            assert c['bounds']['acceptsHWFocus']=='No'
            assert not any(a.get('handler')=='Q-Link' for a in defs[c['componentData']['type']].get('actions',[]))
q=json.loads(next((root/'build/skin').glob('*/Plugin Skins/Q-Links.json')).read_text())
for m in q['Screen Mode Q-Links']['map']:
    if m.get('Tab')==5:
        assert all(v in (-1,ids['length_ms']) for v in m['Q-Links'].values())
print('Rendered waveform below native point editor, shared quiet-time invalidation and touch-only point controls PASS')

# Touchable bank/actions must not be hidden under the full-size native editor.
children=defs['ENVELOPE|ENVELOPE']['componentsData']
curve=next(c for c in children if c['componentData']['type']=='gkPointXY')
x,y,w,h=map(int,curve['bounds']['bounds'].split())
for c in children:
    if c is curve or c['componentData']['type']=='gkPointDot':continue
    if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids[k]) for k in ('free_select','free_add','free_delete','free_prev','free_next','free_level','free_time','free_bend') for m in c.get('handle remapping',{}).get('map',[])):
        xx,yy,ww,hh=map(int,c['bounds']['bounds'].split())
        assert yy>=y+h and 0<=xx and xx+ww<=1280 and yy+hh<=628
print('Point actions/selectors lie outside the native canvas and inside the display PASS')

from editor_geometry import OUTER,PLOT,CURVE_HEIGHT
xy=defs['gkPointXY']['componentsData'][0]['componentData']
assert xy['type']=='XYPad' and xy['data']['image']=='point-active.png'
assert tuple(map(int,curve['bounds']['bounds'].split()))==OUTER
curve_columns=[c for c in children if c['componentData']['type'].startswith('gkEditCurve_')]
rects=[tuple(map(int,c['bounds']['bounds'].split())) for c in curve_columns]
assert min(x for x,y,w,h in rects)==PLOT[0]
assert max(x+w for x,y,w,h in rects)==PLOT[0]+PLOT[2]
assert {y for x,y,w,h in rects}=={PLOT[1]} and {h for x,y,w,h in rects}=={CURVE_HEIGHT}
assert CURVE_HEIGHT-2==PLOT[3]
for c in children:
    if c['componentData']['type']=='gkPointDot':assert tuple(map(int,c['bounds']['bounds'].split()))==OUTER
print('Absolute XY puck centres and complete curve share one pixel/time/level rectangle PASS')
