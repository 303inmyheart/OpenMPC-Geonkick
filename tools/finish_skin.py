"""Include the complete label/value hit boxes in each native Q-Link frame."""
import json,os,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
mpc=Path(os.environ.get('MPC_VST',root.parent/'mpc-vst-plugins'))
sys.path.insert(0,str(mpc/'tools'))
import shadow_skin
layouts,_=shadow_skin.parse_layout(root/'layout.conf')
params=json.loads((root/'params.json').read_text())['params']
ids={p['key']:i for i,p in enumerate(params)}
path=next((root/'build/skin').glob('*/Plugin Skins/TUI.json'))
d=json.loads(path.read_text());page=d['pageData']
defs={c['key']:c['value'] for c in page['componentDefinitions']['localComponentDefinitions']}
for tab in page['tabs']:
    layout=layouts[tab['fnKeyIndex']]
    bank,keys=layout['qlinks'][tab['fnKeySubIndex']]
    children=defs[tab['componentName']]['componentsData']
    frames=[]
    for col in range(4):
        boxes=[]
        for key in keys[col*4:col*4+4]:
            if key=='-':continue
            target='Parameter '+str(ids[key])
            for child in children:
                if child['componentData']['type']=='Image':continue
                if not any(m['value']==target for m in child.get('handle remapping',{}).get('map',[])):continue
                x,y,w,h=map(lambda v:int(float(v)),child['bounds']['bounds'].split());boxes.append((x,y,x+w,y+h))
            widget=next(w for w in layout['widgets'] if w.get('key')==key)
            if widget['kind'].startswith('enum'):
                w=dict(widget);w.setdefault('options',params[ids[key]]['options'])
                for x,y,wi,hi in shadow_skin.seg_rects(w):boxes.append((x,y-111,x+wi,y-86+hi))
        if not boxes:frames.append('0 0 0 0');continue
        x=min(r[0] for r in boxes)-6;y=min(r[1] for r in boxes)-6
        right=max(r[2] for r in boxes)+6;bottom=max(r[3] for r in boxes)+6
        assert 0<=x<right<=1280 and 0<=y<bottom<=628,(tab,col,boxes)
        frames.append(f'{x} {y} {right-x} {bottom-y}')
    while frames and frames[-1]=='0 0 0 0':frames.pop()
    tab['qlinkBoundsData']=frames
path.write_text(json.dumps(d,indent=2)+'\n')
# Browser tile launches the real Init sound and presents the same name.
import xml.etree.ElementTree as ET
p=path.parent.parent/'Presets/0000-Default.xpl'
if p.exists():
    tree=ET.parse(p);tree.getroot().find('preset').text='Init'
    tree.write(p.with_name('0000-Init.xpl'),encoding='utf-8',xml_declaration=True);p.unlink()
