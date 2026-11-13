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
from spec import PLOT_COLUMNS,PLOT_WIDTH,PLOT_WAVE_LEVELS,PLOT_ENV_LEVELS,TOUCH_BINDINGS
layouts,_=shadow_skin.parse_layout(root/'layout.conf')
params=json.loads((root/'params.json').read_text())['params']
ids={p['key']:i for i,p in enumerate(params)}
path=next((root/'build/skin').glob('*/Plugin Skins/TUI.json'))
d=json.loads(path.read_text());page=d['pageData']
defs={c['key']:c['value'] for c in page['componentDefinitions']['localComponentDefinitions']}
# Increase the real native slider hit box with transparent filmstrip padding.
# Keep every painted pixel and caption at its old global position.
from copy import deepcopy
for tab in page['tabs']:
    children=defs[tab['componentName']]['componentsData']
    boxes=[]
    for c in children:
        typ=c['componentData']['type']
        if typ!='Image':boxes.append(tuple(map(int,c['bounds']['bounds'].split())))
    for c in children:
        original=defs.get(c['componentData']['type'])
        if not original:continue
        sliders=[p for p in original.get('componentsData',[]) if p['componentData']['type']=='Knob' and p['componentData']['data'].get('dragOrientation')=='Horizontal']
        if not sliders:continue
        x,y,w,h=map(int,c['bounds']['bounds'].split())
        free=min([y-(by+bh) for bx,by,bw,bh in boxes if bx<x+w and bx+bw>x and by+bh<=y] or [24])
        top=min(24,max(0,free));bottom=24
        key=c['componentData']['type']+f'_touch{top}_{bottom}'
        if key not in defs:
            clone=deepcopy(original);parts=clone['componentsData'];hit=[]
            for part in parts:
                px,py,pw,ph=map(int,part['bounds']['bounds'].split());pd=part['componentData']
                if pd['type']=='Focus':part['bounds']['bounds']=f'{px} {py} {pw} {ph+top}'
                else:part['bounds']['bounds']=f'{px} {py+top} {pw} {ph}'
                if pd['type']=='Knob' and pd['data'].get('dragOrientation')=='Horizontal':
                    data=pd['data'];source=path.parent/data['filmStrip'];strip=Image.open(source).convert('RGBA');frames=strip.height//ph
                    height=ph+top+bottom;assert height*frames<=16384
                    filename=source.stem+f'_touch{top}_{bottom}.png'
                    padded=Image.new('RGBA',(strip.width,height*frames))
                    for frame in range(frames):padded.alpha_composite(strip.crop((0,frame*ph,strip.width,(frame+1)*ph)),(0,frame*height+top))
                    padded.save(path.parent/filename);data['filmStrip']=filename
                    part['bounds']['bounds']=f'{px} {py} {pw} {height}';hit.append(part)
            # The transparent hit area must receive gestures over its own caption.
            clone['componentsData']=[p for p in parts if p not in hit]+hit
            defs[key]=clone;page['componentDefinitions']['localComponentDefinitions'].append({'key':key,'value':clone})
        c['componentData']['type']=key;c['bounds']['bounds']=f'{x} {y-top} {w} {h+top}'
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
# Each segment is generated at its own actual size and caption. The upstream
# generator otherwise shares image names between compact and detailed widgets.
from copy import deepcopy
import hashlib
for tab in page['tabs']:
    layout=layouts[tab['fnKeyIndex']];bank=layout['qlinks'][tab['fnKeySubIndex']][0]
    children=defs[tab['componentName']]['componentsData']
    for child in children:
        match=re.fullmatch(r'shSeg_(.+)_(\d+)',child['componentData']['type'])
        if not match:continue
        param,option=match.group(1),int(match.group(2))
        widget=next(w for w in layout['widgets'] if w.get('key')==param and w['kind'].startswith('enum') and (not w.get('banks') or bank in w['banks'].split('|')))
        options=widget.get('options') or params[ids[param]]['options']
        label=str(options[option]).upper()
        x,y,width,height=map(int,child['bounds']['bounds'].split())
        signature=hashlib.sha256(f'{param}/{option}/{width}/{height}/{label}'.encode()).hexdigest()[:12]
        key='gkSegment_'+signature
        if key not in defs:
            size=min(22,round(height*0.45));font=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),size)
            while font.getlength(label)>width-6 and size>8:
                size-=1;font=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),size)
            for state in ('on','off'):
                im=Image.new('RGB',(width,height),'#91B958' if state=='on' else '#46494B');draw=ImageDraw.Draw(im)
                draw.text((width/2,height/2),label,font=font,anchor='mm',fill='#172015' if state=='on' else '#E0E4E5')
                im.save(path.parent/f'{key}_{state}.png')
            item=shadow_skin._local(key,[shadow_skin._action('Mouse Down','Q-Link')],[shadow_skin._button(key+'_on.png',key+'_off.png',option,len(options),width,height)])
            item['value']['hideQLinkBounds']=False
            page['componentDefinitions']['localComponentDefinitions'].append(item);defs[key]=item['value']
        child['componentData']['type']=key
shutil.copy2(root/'art/plot_grid.png',path.parent/'plot_grid.png')
for tab in page['tabs']:
    if tab['fnKeyIndex']!=0:continue
    children=defs[tab['componentName']]['componentsData']
    children.append(shadow_skin._sub('Image',{'version':2,'imageType':'Regular','colour':'0','image':'plot_grid.png'},shadow_skin._bounds(70,122,920,220),'Plot background'))
    for column in range(PLOT_COLUMNS):
        for channel,height,y in [('p',96,145),('n',96,241),('e',192,145)]:
            child=shadow_skin._placed('gkPlot_'+channel,'Graph',ids[f'viz_{channel}_{column}'],100+column*PLOT_WIDTH,y+86,PLOT_WIDTH,height,focus='No')
            child['bounds']['additionalInvalidatingHandles'].append('Parameter '+str(ids['graph_revision']))
            children.append(child)
# Keep each caption INSIDE its native control so conditional visibility remains
# attached to the same widget. Full-page text overlays produced ghost labels.
for definition in page['componentDefinitions']['localComponentDefinitions']:
    for sub in definition['value'].get('componentsData',[]):
        cd=sub['componentData']
        if cd['type']=='Label' and cd['data'].get('type')=='Value':cd['data']['textStyle']['font']['height']*=1.6
for tab in page['tabs']:
    children=defs[tab['componentName']]['componentsData']
    for child in children:
        original=defs.get(child['componentData']['type'])
        if original is None:continue
        names=[s for s in original.get('componentsData',[]) if s['componentData']['type']=='Label' and s['componentData']['data'].get('type')=='Name']
        if not names:continue
        caption=child['componentData'].get('name','')
        signature=hashlib.sha256((child['componentData']['type']+'/'+caption).encode()).hexdigest()[:12]
        key='gkCaption_'+signature
        if key not in defs:
            clone=deepcopy(original);parts=[]
            for sub in clone['componentsData']:
                data=sub['componentData']
                if data['type']!='Label' or data['data'].get('type')!='Name':parts.append(sub);continue
                x,y,width,height=map(int,sub['bounds']['bounds'].split());style=data['data']['textStyle']
                image=Image.new('RGBA',(width,height),(0,0,0,0));draw=ImageDraw.Draw(image)
                size=round(style['font']['height']);font=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),size)
                while font.getlength(caption)>width-4 and size>10:
                    size-=1;font=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),size)
                draw.text((width/2,height/2),caption,font=font,anchor='mm',fill='#'+style['colour'][-6:])
                filename=f'{key}_{len(parts)}.png';image.save(path.parent/filename)
                parts.append(shadow_skin._sub('Image',{'version':2,'imageType':'Regular','colour':'0','image':filename},deepcopy(sub['bounds']),'Caption'))
            clone['componentsData']=parts;defs[key]=clone
            page['componentDefinitions']['localComponentDefinitions'].append({'key':key,'value':clone})
        if any(m.get('value')=='Parameter '+str(ids[k]) for k in ids if k.endswith(('_freq_fine','_ms','_pct')) for m in child.get('handle remapping',{}).get('map',[])):
            defs[key]['disableCoarseDataWheel']=True
        child['componentData']['type']=key
# Native gestures write a continuous Touch handle; Q-Link selection, bounds
# and readout retain the fine Data handle. Both address one physical value.
for tab in page['tabs']:
    children=defs[tab['componentName']]['componentsData']
    for child in children:
        mapping=child.get('handle remapping',{}).get('map',[])
        fine=next((k for k in TOUCH_BINDINGS if any(m.get('key')=='Data' and m.get('value')=='Parameter '+str(ids[k]) for m in mapping)),None)
        if fine is None:continue
        original=defs.get(child['componentData']['type'])
        if not original or not any(p['componentData']['type']=='Knob' for p in original.get('componentsData',[])):continue
        key=child['componentData']['type']+'_touchInput'
        if key not in defs:
            clone=deepcopy(original)
            for part in clone['componentsData']:
                cd=part['componentData']
                if cd['type']=='Knob' and cd['data'].get('handleName')=='Data':cd['data']['handleName']='Touch'
            defs[key]=clone;page['componentDefinitions']['localComponentDefinitions'].append({'key':key,'value':clone})
        mapping.append({'key':'Touch','value':'Parameter '+str(ids[TOUCH_BINDINGS[fine]])})
        child['componentData']['type']=key
path.write_text(json.dumps(d,indent=2)+'\n')
# Browser tile launches the real Init sound and presents the same name.
import xml.etree.ElementTree as ET
p=path.parent.parent/'Presets/0000-Default.xpl'
if p.exists():
    tree=ET.parse(p);tree.getroot().find('preset').text='Init'
    tree.write(p.with_name('0000-Init.xpl'),encoding='utf-8',xml_declaration=True);p.unlink()
