#!/usr/bin/env python3
"""Validate fixed-point native skin wiring, visibility and append-only ABI."""
import json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
params=json.loads((root/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
page=json.loads(next((root/'build/skin').glob('*/Plugin Skins/TUI.json')).read_text())['pageData']
defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
assert len(params)==1782
previous=json.loads(subprocess.check_output(['git','show','712500a:params.json'],cwd=root))['params']
assert params[:len(previous)]==previous,'Changed previous VST identities/ranges/defaults'
for tab in page['tabs']:
    if tab['fnKeyIndex']!=6:continue
    children=defs[tab['componentName']]['componentsData'];envelopes=[c for c in children if c['componentData']['type']=='gkPointEnvelope']
    assert len(envelopes)==18
    for n,c in enumerate(envelopes):
        osc=n//2+1;kind='p' if n%2 else 'a';prefix=f'o{osc}_{kind}_'
        mapping={m['key']:m['value'] for m in c['handle remapping']['map']}
        assert c['bounds']['additionalInvalidatingHandles']==[f'IndexedEnabling/{n}/18/Parameter '+str(ids['ui_point_target']),'IndexedEnabling/1/2/Parameter '+str(ids[prefix+'mode'])]
        for j in range(5):assert mapping[f'Level{j}']=='Parameter '+str(ids[prefix+f'l{j}'])
        for j in range(1,5):assert mapping[f'Time{j}']=='Parameter '+str(ids[prefix+f't{j}'])
    for key in ('point_level_fine','point_time_fine'):
        controls=[c for c in children if any(m['key']=='Data' and m['value']=='Parameter '+str(ids[key]) for m in c.get('handle remapping',{}).get('map',[]))]
        assert len(controls)==1
        c=controls[0];d=defs[c['componentData']['type']]
        assert d['disableCoarseDataWheel']
        assert any(p['componentData']['type']=='Knob' and p['componentData']['data']['handleName']=='Touch' for p in d['componentsData'])
        assert {'key':'Touch','value':'Parameter '+str(ids[key.replace('_fine','_drag')])} in c['handle remapping']['map']
print('1782 append-only VST parameters; 18 native envelopes / 162 point handles; conditional modes and fine/direct point knob bindings PASS')
