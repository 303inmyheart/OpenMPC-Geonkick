#!/usr/bin/env python3
"""Add a touch-only, native information overlay without changing DSP/state IDs."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
def bounds(x,y,w,h):
    return {'version':2,'acceptsHWFocus':'No','showWhenDataModelInvalid':'Show',
            'whenVisible':'Always','boundsType':'Absolute','bounds':f'{x} {y} {w} {h}',
            'additionalInvalidatingHandles':[]}

def entry(kind,name,data,x,y,w,h):
    return {'version':2,'componentData':{'version':1,'name':name,'type':kind,'data':data},
            'handle remapping':{'version':1,'map':[]},'bounds':bounds(x,y,w,h)}

def action(event,handler,handle='',extra=''):
    return {'version':2,'onAction':event,'handler':handler,'handleName':handle,
            'additionalData':extra,'handle remapping':{'version':1,'map':[]}}


ROOT=Path(__file__).resolve().parents[1]
CFG=json.loads((ROOT/'vst.json').read_text())
SKIN=ROOT/'build/skin'/f"{CFG['vendor']} - VST - {CFG['name']}"/'Plugin Skins'
NAME='geonkick preset compatibility help'
BUTTON='geonkick preset help button'
SCALE=2

def canvas(w,h,colour):
    im=Image.new('RGB',(w*SCALE,h*SCALE),colour)
    return im,ImageDraw.Draw(im)

def text(draw,x,y,value,size,colour='#F1F4FA'):
    font=ImageFont.truetype(str(ROOT/'art/DejaVuSans.ttf'),size*SCALE)
    box=draw.textbbox((0,0),value,font=font)
    assert x*SCALE+box[2] <= draw._image.width-20*SCALE,(value,'text overflow')
    draw.text((x*SCALE,y*SCALE),value,font=font,fill=colour)

def save(im,name):
    im.resize((im.width//SCALE,im.height//SCALE),Image.Resampling.LANCZOS).save(SKIN/name)

def install():
    path=SKIN/'TUI.json';doc=json.loads(path.read_text());pd=doc['pageData']
    definitions=pd['componentDefinitions']['localComponentDefinitions']
    definitions[:]=[d for d in definitions if d['key'] not in (NAME,BUTTON)]
    defs={d['key']:d['value'] for d in definitions}
    qlinks=(SKIN/'Q-Links.json').read_bytes()
    params=json.loads((ROOT/'params.json').read_text())['params']
    readonly_model=next(i for i,p in enumerate(params) if p['key']=='graph_revision')
    assert params[readonly_model]['live'] and CFG['defines']['LIVE_PARAMS_READONLY']

    im,draw=canvas(270,52,'#7BA9FF')
    draw.rounded_rectangle((2,2,538,102),radius=12,outline='#DCEAFF',width=4)
    text(draw,41,2,'PRESET INFO',23,'#101722')
    text(draw,26,31,'OLD PRESETS? READ THIS',14,'#101722')
    save(im,'geonkick_preset_help_button.png')
    image=entry('Image','Preset help artwork',{'version':2,'imageType':'Regular',
                'colour':'0','image':'geonkick_preset_help_button.png'},0,0,270,52)
    clear={'version':1,'colour':'0','image':''}
    definitions.append({'key':BUTTON,'value':{
        'version':4,'actions':[action('Mouse Down','Show Overlay','Data',NAME)],
        'backgroundData':{'version':1,'focussed':clear,'unfocussed':clear},
        'ignoreMousePresses':False,'disableCoarseDataWheel':False,
        'repeats':1,'hideQLinkBounds':True,'componentsData':[image]}})
    for tab in pd['tabs']:
        children=defs[tab['componentName']]['componentsData']
        children[:]=[c for c in children if c['componentData']['type']!=BUTTON]
        button=entry(BUTTON,'Read before loading old presets',{'version':1,'handleName':'Data'},298,9,270,52)
        # Read-only graph revision provides a valid native overlay model. It cannot
        # change sound settings, and the button is never assigned a Q-Link.
        button['handle remapping']['map']=[{'key':'Data','value':f'Parameter {readonly_model}'}]
        children.append(button)

    im,draw=canvas(1120,680,'#1A1E27')
    draw.rounded_rectangle((3,3,2236,1356),radius=22,outline='#7BA9FF',width=4)
    lines=[
        (30,20,"CAN'T LOAD AN OLDER PRESET?",30,'#7BA9FF'),
        (30,70,'The maker name changed from OpenMPC to 303inmyheart in this update.',22,'#F1F4FA'),
        (30,100,'An older preset may still contain the old name, so MPC refuses to load it.',22,'#F1F4FA'),
        (30,130,'Your sound may still be intact. Updating the plugin does not convert these files.',21,'#FFBC85'),
        (30,179,'1  FIRST: KEEP YOUR ORIGINALS SAFE',23,'#7BA9FF'),
        (60,212,'Back up your presets and projects before trying either option below.',21,'#F1F4FA'),
        (60,241,'Never overwrite the old preset with Init or another sound.',21,'#F1F4FA'),
        (30,289,'2  EASIEST OPTION: THE SOUND IS IN AN OLDER PROJECT',23,'#7BA9FF'),
        (60,322,"Open that project. If the sound plays correctly, use MPC's preset Save command.",21,'#F1F4FA'),
        (60,351,'Save under a NEW name. Try loading it in a fresh Geonkick instance.',21,'#F1F4FA'),
        (60,380,'This creates a preset for the new version without editing any files.',21,'#F1F4FA'),
        (30,428,'3  ONLY HAVE THE OLD PRESET FILE? USE THE CONVERTER',23,'#7BA9FF'),
        (60,461,'Download convert_mpc_presets.py from the Geonkick MPC GitHub page.',21,'#F1F4FA'),
        (60,490,'Run it on a computer with Python. Choose old presets and a new output folder.',21,'#F1F4FA'),
        (60,519,'Preview, then create copies. Originals and saved sound data stay unchanged.',21,'#F1F4FA'),
        (60,548,'Copy the new plugin folders to MPC Documents/Plugin Presets/Instruments.',21,'#FFBC85'),
        (30,596,'Full step-by-step file instructions: PRESET-MIGRATION.md',21,'#7BA9FF'),
        (30,628,'github.com/303inmyheart/OpenMPC-Geonkick  -  no automatic conversion in this window',20,'#9BA8BB'),
    ]
    for x,y,caption,size,colour in lines:text(draw,x,y,caption,size,colour)
    save(im,'geonkick_preset_help_panel.png')
    # Same native focus/dismissal primitive as the JC303 information overlay.
    # The panel is a partial-size Image: outside taps reach KnobOverlay rather
    # than being swallowed by a full-screen child Image.
    native=entry('KnobOverlay','Native overlay dismissal',{
        'version':1,'actions':[{'version':1,'onAction':'Focus Lost',
            'handler':'Focus Lost','handleName':'Focus Lost','additionalData':''}],
        'overlayType':'Knob With Header','scaleType':'Linear','polarityType':'Unipolar',
        'showMinusSign':True,'handleName':'Data'},0,0,1280,800)
    panel=entry('Image','Preset migration instructions',{'version':2,'imageType':'Regular',
        'colour':'0','image':'geonkick_preset_help_panel.png'},80,40,1120,680)
    backdrop,draw=canvas(1280,800,'#101218')
    text(draw,427,748,'TAP OUTSIDE TO CLOSE',25,'#7BA9FF')
    save(backdrop,'geonkick_preset_help_backdrop.png')
    background={'version':1,'colour':'0','image':'geonkick_preset_help_backdrop.png'}
    definitions.append({'key':NAME,'value':{'version':4,'actions':[],
        'backgroundData':{'version':1,'focussed':background,'unfocussed':background},
        'ignoreMousePresses':True,'disableCoarseDataWheel':True,'repeats':1,
        'hideQLinkBounds':True,'componentsData':[native,panel]}})
    path.write_text(json.dumps(doc,indent=2)+'\n')
    assert (SKIN/'Q-Links.json').read_bytes()==qlinks
    preview=backdrop.resize((1280,800),Image.Resampling.LANCZOS)
    with Image.open(SKIN/'geonkick_preset_help_panel.png') as p:preview.paste(p,(80,40))
    preview.save(ROOT/'build/preview_preset_help.png')
    print('Preset Info: touch-only header button on every page; English plain-language native overlay; Q-Links unchanged. Device dismissal test pending.')

if __name__=='__main__':install()
