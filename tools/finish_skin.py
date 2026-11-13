"""Include the complete label/value hit boxes in each native Q-Link frame."""
import json,os,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
mpc=Path(os.environ.get('MPC_VST',root.parent/'mpc-vst-plugins'))
sys.path.insert(0,str(mpc/'tools'))
import shadow_skin
import shutil
import re
from PIL import Image,ImageDraw,ImageFont
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
# Read-only native filmstrip display, using the same Knob schema as generated
# controls. No unsupported Graph/Envelope/Canvas component is introduced.
for channel,asset,height in [('p','positive',96),('n','negative',96),('e','envelope',192)]:
    filename=f'plot_{asset}.png';shutil.copy2(root/'art'/filename,path.parent/filename)
    name='gkPlot_'+channel
    item=shadow_skin._local(name,[],[shadow_skin._sub('Knob',{
        'version':5,'knobType':'FilmStrip','filmStrip':filename,'numFrames':64,
        'invert':False,'dragOrientation':'Vertical','handleName':'Data'
    },shadow_skin._bounds(0,0,9,height),'Plot')])
    item['value']['ignoreMousePresses']=True
    item['value']['hideQLinkBounds']=False
    page['componentDefinitions']['localComponentDefinitions'].append(item)
shutil.copy2(root/'art/plot_grid.png',path.parent/'plot_grid.png')
for tab in page['tabs']:
    if tab['fnKeyIndex']!=0:continue
    children=defs[tab['componentName']]['componentsData']
    # The shared generator keys segment definitions only by parameter/option.
    # Give compact SYNTH buttons their own names/assets, so detailed OSC pages
    # cannot overwrite them with their larger versions.
    for child in children:
        match=re.fullmatch(r'shSeg_o\d+_(on|wave)_(\d+)',child['componentData']['type'])
        if not match:continue
        kind,option=match.group(1),int(match.group(2));width=34 if kind=='on' else 52
        labels=['OFF','ON'] if kind=='on' else ['SIN','SQR','TRI','SAW','WHT','BRN']
        key=f'gkCompact_{kind}_{option}'
        if key not in defs:
            for state in ('on','off'):
                im=Image.new('RGB',(width,30),'#91B958' if state=='on' else '#46494B');dr=ImageDraw.Draw(im)
                f=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),10)
                dr.text((width/2,15),labels[option],font=f,anchor='mm',fill='#172015' if state=='on' else '#E0E4E5')
                im.save(path.parent/f'{key}_{state}.png')
            item=shadow_skin._local(key,[shadow_skin._action('Mouse Down','Q-Link')],[shadow_skin._button(key+'_on.png',key+'_off.png',option,len(labels),width,30)])
            item['value']['hideQLinkBounds']=False
            page['componentDefinitions']['localComponentDefinitions'].append(item);defs[key]=item['value']
        child['componentData']['type']=key
    children.append(shadow_skin._sub('Image',{'version':2,'imageType':'Regular','colour':'0','image':'plot_grid.png'},shadow_skin._bounds(70,122,920,220),'Plot background'))
    for column in range(96):
        for channel,height,y in [('p',96,145),('n',96,241),('e',192,145)]:
            children.append(shadow_skin._placed('gkPlot_'+channel,'Graph',ids[f'viz_{channel}_{column}'],100+column*9,y+86,9,height,focus='No'))
path.write_text(json.dumps(d,indent=2)+'\n')
# Browser tile launches the real Init sound and presents the same name.
import xml.etree.ElementTree as ET
p=path.parent.parent/'Presets/0000-Default.xpl'
if p.exists():
    tree=ET.parse(p);tree.getroot().find('preset').text='Init'
    tree.write(p.with_name('0000-Init.xpl'),encoding='utf-8',xml_declaration=True);p.unlink()
