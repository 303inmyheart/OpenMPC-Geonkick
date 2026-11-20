#!/usr/bin/env python3
"""Validate fixed-point native skin wiring, visibility and append-only ABI."""
import json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
params=json.loads((root/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
page=json.loads(next((root/'build/skin').glob('*/Plugin Skins/TUI.json')).read_text())['pageData']
defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
assert len(params)==2935
previous=json.loads(subprocess.check_output(['git','show','91b2975:params.json'],cwd=root))['params']
assert params[:len(previous)]==previous,'Changed previous VST identities/ranges/defaults'
for tab in page['tabs']:
    if tab['componentName']!='ENVELOPE|ENVELOPE':continue
    children=defs[tab['componentName']]['componentsData']
    envelopes=[c for c in children if c['componentData']['type']=='gkFreeEnvelope']
    assert len(envelopes)==1
    mapping={m['key']:m['value'] for m in envelopes[0]['handle remapping']['map']}
    for j in range(5):assert mapping[f'Level{j}']=='Parameter '+str(ids[f'edit_l{j}'])
    for j in range(1,5):assert mapping[f'Time{j}']=='Parameter '+str(ids[f'edit_t{j}'])
    assert defs['gkFreeEnvelope']['componentsData'][0]['componentData']['data']['lineColour']=='00000000'
    curves=[c for c in children if c['componentData']['type'].startswith('gkEditCurve_')]
    assert len(curves)==432
print('Original 1782 parameter identities preserved; variable point grip proxies and full DSP curve PASS')
# Editor draws the same completed sound behind the draggable curve. No new
# data channel or per-column host callback path is introduced by this skin.
for tab in page['tabs']:
    if tab['componentName']!='ENVELOPE|ENVELOPE':continue
    children=defs[tab['componentName']]['componentsData']
    wave=[(i,c) for i,c in enumerate(children) if c['componentData']['type'].startswith('gkEditWave_')]
    assert len(wave)==864
    first_curve=min(i for i,c in enumerate(children) if c['componentData']['type']=='gkFreeEnvelope')
    assert max(i for i,c in wave)<first_curve
    for _,c in wave:
        assert c['bounds']['additionalInvalidatingHandles']==['Parameter '+str(ids['graph_revision'])]
        assert defs[c['componentData']['type']]['ignoreMousePresses']
    point_keys=['point_copy','free_bank','free_level','free_time','free_bend','free_add','free_delete','free_prev','free_next']
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
curve=next(c for c in children if c['componentData']['type']=='gkFreeEnvelope')
x,y,w,h=map(int,curve['bounds']['bounds'].split())
for c in children:
    if c is curve:continue
    if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids[k]) for k in ('free_bank','free_add','free_delete','free_prev','free_next','free_level','free_time','free_bend') for m in c.get('handle remapping',{}).get('map',[])):
        xx,yy,ww,hh=map(int,c['bounds']['bounds'].split())
        assert yy>=y+h and 0<=xx and xx+ww<=1280 and yy+hh<=628
print('Point actions/groups lie outside the native canvas and inside the display PASS')
