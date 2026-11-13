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
from spec import PLOT_COLUMNS,PLOT_WIDTH,PLOT_WAVE_LEVELS,PLOT_ENV_LEVELS
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
        'version':5,'knobType':'FilmStrip','filmStrip':filename,'numFrames':PLOT_ENV_LEVELS if channel=='e' else PLOT_WAVE_LEVELS,
        'invert':False,'dragOrientation':'Vertical','handleName':'Data'
    },shadow_skin._bounds(0,0,PLOT_WIDTH,height),'Plot')])
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
    for column in range(PLOT_COLUMNS):
        for channel,height,y in [('p',96,145),('n',96,241),('e',192,145)]:
            children.append(shadow_skin._placed('gkPlot_'+channel,'Graph',ids[f'viz_{channel}_{column}'],100+column*PLOT_WIDTH,y+86,PLOT_WIDTH,height,focus='No'))
# MPC's native font-height unit is smaller than a rendered pixel. The previous
# offline composite overestimated the on-device Name/Value text size. Bake
# page-local captions at their intended pixel size and enlarge live Value fonts.
# Captions carry each widget's actual layout label, not its global VST name.
from copy import deepcopy
for definition in page['componentDefinitions']['localComponentDefinitions']:
    for sub in definition['value'].get('componentsData',[]):
        cd=sub['componentData']
        if cd['type']=='Label' and cd['data'].get('type')=='Value':
            cd['data']['textStyle']['font']['height']*=1.6
for i,tab in enumerate(page['tabs']):
    children=defs[tab['componentName']]['componentsData']
    labels=Image.new('RGBA',(1280,628),(0,0,0,0));draw=ImageDraw.Draw(labels);seen=set()
    clones=[]
    for child in children:
        component=defs.get(child['componentData']['type'])
        if component is None:continue
        names=[s for s in component.get('componentsData',[]) if s['componentData']['type']=='Label' and s['componentData']['data'].get('type')=='Name']
        if not names:continue
        x,y,_,_=map(int,child['bounds']['bounds'].split())
        for sub in names:
            sx,sy,w,h=map(int,sub['bounds']['bounds'].split());style=sub['componentData']['data']['textStyle']
            caption=child['componentData'].get('name','');key=(x+sx,y+sy,w,h,caption)
            if key in seen:continue
            seen.add(key);font=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),round(style['font']['height']))
            draw.text((x+sx+w/2,y+sy+h/2),caption,font=font,anchor='mm',fill='#'+style['colour'][-6:])
        name=child['componentData']['type']+'_captionless'
        if name not in defs:
            clone=deepcopy(component);clone['componentsData']=[s for s in clone['componentsData'] if not (s['componentData']['type']=='Label' and s['componentData']['data'].get('type')=='Name')]
            defs[name]=clone;clones.append({'key':name,'value':clone})
        child['componentData']['type']=name
    page['componentDefinitions']['localComponentDefinitions'].extend(clones)
    filename=f'gk_captions_{i}.png';labels.save(path.parent/filename)
    # Below interactive children: a full-page image placed on top can intercept
    # touches in MPC even when its pixels are transparent.
    children.insert(1,shadow_skin._sub('Image',{'version':2,'imageType':'Regular','colour':'0','image':filename},shadow_skin._bounds(0,0,1280,628),'Captions'))
path.write_text(json.dumps(d,indent=2)+'\n')
# Browser tile launches the real Init sound and presents the same name.
import xml.etree.ElementTree as ET
p=path.parent.parent/'Presets/0000-Default.xpl'
if p.exists():
    tree=ET.parse(p);tree.getroot().find('preset').text='Init'
    tree.write(p.with_name('0000-Init.xpl'),encoding='utf-8',xml_declaration=True);p.unlink()
