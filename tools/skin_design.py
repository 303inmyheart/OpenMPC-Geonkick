#!/usr/bin/env python3
"""Original percussion-lab artwork. Native geometry/actions/handles stay intact."""
from pathlib import Path
import json,math,os,sys,re,hashlib
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
BG='#101218';PANEL='#1a1e27';BORDER='#303642';INK='#f1f4fa';MUTED='#9ba8bb';GREEN='#7ba9ff';AMBER='#ffbc85';CYAN='#6de2c3'
FONT=ROOT/'art/DejaVuSans.ttf';S=3
PRESET_FRAME=(132,153,1148,474)

def font(n):return ImageFont.truetype(str(FONT),round(n*S))
def text(d,xy,label,size=13,fill=INK,anchor='mm'):
    d.text(tuple(round(v*S) for v in xy),label,font=font(size),fill=fill,anchor=anchor)
def box(d,rect,fill,outline=None,radius=8,width=1):
    d.rounded_rectangle(tuple(round(v*S) for v in rect),radius=round(radius*S),fill=fill,outline=outline,width=round(width*S))
def line(d,points,fill,width=1):d.line([tuple(round(v*S) for v in xy) for xy in points],fill=fill,width=round(width*S))
def ellipse(d,rect,fill,outline=None,width=1):d.ellipse(tuple(round(v*S) for v in rect),fill=fill,outline=outline,width=round(width*S))
def save(im,p):im.resize((im.width//S,im.height//S),Image.Resampling.LANCZOS).save(p)
def segment_image(width,height,label,on):
    im=Image.new('RGBA',(width*S,height*S),(0,0,0,0));d=ImageDraw.Draw(im)
    box(d,(1,1,width-1,height-1),GREEN if on else '#242a35',GREEN if on else BORDER,6)
    size=min(22,height*.43)
    while font(size).getlength(label)>(width-12)*S and size>9:size-=1
    text(d,(width/2,height/2),label,size,'#101722' if on else INK)
    return im.resize((width,height),Image.Resampling.LANCZOS)

def make_knob(path):
    radius=int(re.search(r'_r(\d+)',path.name)[1]);size=2*radius+10
    old=Image.open(path);frames=old.height//size;strip=Image.new('RGBA',(size,size*frames));c=size/2
    for n in range(frames):
        im=Image.new('RGBA',(size*S,size*S),(0,0,0,0));d=ImageDraw.Draw(im)
        rect=tuple(round(v*S) for v in (c-radius,c-radius,c+radius,c+radius))
        d.arc(rect,135,405,fill=BORDER,width=3*S)
        if n:d.arc(rect,135,135+270*n/(frames-1),fill=GREEN,width=3*S)
        ellipse(d,(c-radius+7,c-radius+7,c+radius-7,c+radius-7),'#242b37')
        a=math.radians(135+270*n/(frames-1))
        line(d,[(c+(radius*.27)*math.cos(a),c+(radius*.27)*math.sin(a)),(c+(radius-11)*math.cos(a),c+(radius-11)*math.sin(a))],INK,2.4)
        strip.paste(im.resize((size,size),Image.Resampling.LANCZOS),(0,n*size))
    assert strip.size==old.size;strip.save(path)

def sliders(skin):
    for path in skin.glob('sh_slider_h_*.png'):
        m=re.fullmatch(r'sh_slider_h_(\d+)x(\d+)(?:_touch(\d+)_(\d+))?\.png',path.name)
        if not m:continue
        w,h=int(m[1]),int(m[2]);top,bottom=int(m[3] or 0),int(m[4] or 0);old=Image.open(path);fh=h+top+bottom;frames=old.height//fh
        strip=Image.new('RGBA',(w,fh*frames))
        for n in range(frames):
            im=Image.new('RGBA',(w*S,h*S),(0,0,0,0));d=ImageDraw.Draw(im);x=8+(w-16)*n/(frames-1)
            box(d,(1,h/2-3,w-1,h/2+3),'#091518','#4b6566',3)
            line(d,[(7,h/2),(x,h/2)],GREEN,2)
            ellipse(d,(x-5,h/2-5,x+5,h/2+5),'#adc6b1','#e8f6db')
            strip.paste(im.resize((w,h),Image.Resampling.LANCZOS),(0,n*fh+top))
        assert strip.size==old.size;strip.save(path)

def panel_background(layout,bank,children,defs,skin):
    im=Image.new('RGB',(1280*S,628*S),BG);d=ImageDraw.Draw(im)
    def card(r):box(d,r,PANEL,BORDER,12)
    name=layout['name']
    if name=='SYNTH':
        card((60,58,998,346));card((1016,58,1238,346))
        if bank=='MIX':
            card((60,356,1220,501));card((60,504,296,611));card((348,506,1220,611))
            for x in (328,626,924):line(d,[(x,374),(x,486)],'#344b50')
        else:
            card((60,349,1238,404))
            for x in (54,354,654):card((x,407,x+296,611))
            card((1016,407,1238,611))
    elif name.startswith('LAYER') and 'OSC' in bank:
        for r in [(58,72,1222,223),(58,233,1222,399),(58,430,1222,604)]:card(r)
    elif name.startswith('LAYER') or name=='EFFECTS':
        card((58,72,1222,270));card((58,305,1222,513))
        line(d,[(60,86),(60,253)],CYAN,3);line(d,[(60,319),(60,496)],AMBER,3)
        text(d,(640,615),'FILTER  /  DRIVE   ·   '+('GLOBAL PROCESSING' if name=='EFFECTS' else 'OSCILLATOR PROCESSING'),13,MUTED)
    elif name=='ENVELOPE':
        card((70,72,1210,225));card((70,228,1210,423));card((70,424,1210,508));card((70,510,1256,625))
        text(d,(90,240),'WAVEFORM + ENVELOPE',15,MUTED,'lm');text(d,(1190,240),'SELECT POINT NUMBER / DRAG AMBER POINT',12,AMBER,'rm')
    elif name=='KIT':
        card((70,72,660,510));card((690,72,1220,450));card((70,520,1220,603))
        text(d,(955,508),'SAVE COMPLETE KITS IN THE MPC PRESET MENU',10,MUTED)
        text(d,(955,535),'STOP / MPC BANK / LEARN 16 / PRESS PADS 01-16',10,MUTED)
        text(d,(955,594),'RELEARN AFTER CHANGING THE MPC NOTE LAYOUT',10,MUTED)
        text(d,(365,495),'Selection edits a sound. PLAY auditions it.',14,MUTED)
    elif name=='SAMPLES':
        card((60,72,810,606));card((830,72,1240,606))
        text(d,(1040,540),'WAV / MONO CH1 / FIRST 4 SECONDS',11,MUTED)
    elif name=='PRESETS':
        card(PRESET_FRAME);text(d,(640,515),'EIGHT STARTING POINTS  /  SHAPE YOUR OWN SOUND',13,MUTED)
    # Group headings are static; actual conditional names remain inside native
    # controls. All target-dependent MODE headings occupy the same one place.
    seen=set()
    for w in layout['widgets']:
        if w.get('banks') and bank not in w['banks'].split('|'):continue
        if not w['kind'].startswith('enum') or not w.get('label'):continue
        props=dict(w);props.setdefault('options',PARAMS[w['key']].get('options',[]))
        top=min(r[1] for r in SHADOW.seg_rects(props))-86
        pos=(w['cx'],max(65,top-17),w['label'])
        if pos in seen:continue
        seen.add(pos);text(d,pos[:2],pos[2],14,MUTED)
    return im

def main():
    global SHADOW,PARAMS
    sys.path.insert(0,str(Path(os.environ.get('MPC_VST',ROOT.parent/'mpc-vst-plugins'))/'tools'));import shadow_skin as SHADOW
    layouts,_=SHADOW.parse_layout(ROOT/'layout.conf');layouts={l['name']:l for l in layouts}
    PARAMS={p['key']:p for p in json.loads((ROOT/'params.json').read_text())['params']}
    path=next((ROOT/'build/skin').glob('*/Plugin Skins/TUI.json'));skin=path.parent;doc=json.loads(path.read_text());page=doc['pageData'];defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
    for p in skin.glob('sh_knob_r*.png'):make_knob(p)
    sliders(skin)
    for state in ('on','off'):
        p=skin/f'sh_pill_{state}.png';old=Image.open(p);w,h=old.size;im=Image.new('RGBA',(w*S,h*S),(0,0,0,0));d=ImageDraw.Draw(im)
        box(d,(1,2,w-1,h-2),'#0b161a','#4b6265',h/3);x=w-23 if state=='on' else 4
        box(d,(x,5,x+19,h-5),GREEN if state=='on' else '#5b7379',None,3)
        for xx in range(x+6,x+16,3):line(d,[(xx,9),(xx,h-9)],'#355142' if state=='on' else '#2b4149')
        save(im,p)
    # Change only painted caption colour. Keep original alpha/metrics/visibility.
    for p in skin.glob('gkCaption_*.png'):
        old=Image.open(p).convert('RGBA');tint=Image.new('RGBA',old.size,INK);tint.putalpha(old.getchannel('A'));tint.save(p)
    additions=[]
    for key,value in list(defs.items()):
        parts=value.get('componentsData',[])
        rotary=any(c['componentData']['type']=='Knob' and c['componentData']['data'].get('filmStrip','').startswith('sh_knob_r') for c in parts)
        updated=[]
        for sub in parts:
            cd=sub['componentData']
            if cd['type']=='Label' and cd['data'].get('type')=='Value':
                cd['data']['textStyle']['colour']='ff7ba9ff'
                if rotary:
                    x,y,w,h=map(int,sub['bounds']['bounds'].split())
                    focus=next(c for c in parts if c['componentData']['type']=='Focus')
                    _,_,cw,ch=map(int,focus['bounds']['bounds'].split())
                    fn=f'gk_value_{cw}x{ch}_{y}.png'
                    if not (skin/fn).exists():
                        im=Image.new('RGBA',(cw*S,ch*S),(0,0,0,0));dr=ImageDraw.Draw(im)
                        box(dr,(x+8,y+1,x+w-8,y+h-1),'#10141c',BORDER,6)
                        line(dr,[(x+14,y+h-3),(x+w-14,y+h-3)],'#1e3639');save(im,skin/fn)
                    # Background paint cannot capture gestures over the value.
                    value['backgroundData']={'version':1,**{state:{'version':1,'colour':'00000000','image':fn} for state in ('focussed','unfocussed')}}
            if cd['type']=='Envelope' and key!='gkFreeEnvelope':
                for field,color in [('lineColour','ffffbc85'),('handleLineColour','ffffbc85'),('handleFillColour','ff1a1e27'),('handleFillColourOnPressed','ff7ba9ff'),('scaleColour','ff303642')]:cd['data'][field]=color
            updated.append(sub)
        if rotary:value['componentsData']=updated
    for tab in page['tabs']:
        source,bank=tab['componentName'].split('|');children=defs[tab['componentName']]['componentsData']
        fn='gk_panel_'+hashlib.sha256(tab['componentName'].encode()).hexdigest()[:10]+'.png'
        save(panel_background(layouts[source],bank,children,defs,skin),skin/fn)
        children[:]=[c for c in children if not (c['componentData']['type']=='Image' and re.match(r'sh_(bg|mode)_',c['componentData']['data'].get('image','')))]
        children.insert(0,SHADOW._sub('Image',{'version':2,'imageType':'Regular','colour':'0','image':fn},SHADOW._bounds(0,0,1280,628),'Percussion panel'))
        nav=next(c for c in children if c['componentData'].get('name')=='Navigation')
        file=skin/nav['componentData']['data']['image'];im=Image.new('RGBA',(1280*S,70*S),(0,0,0,0));d=ImageDraw.Draw(im)
        accent=[GREEN,GREEN,CYAN,AMBER,AMBER,CYAN][tab['fnKeyIndex']]
        box(d,(24,14,1256,56),'#081317','#455d60',5);line(d,[(31,15),(1249,15)],accent,2)
        # Original geometric percussion mark, no manufacturer branding.
        d.polygon([(round(x*S),round(y*S)) for x,y in [(45,25),(53,21),(61,25),(61,41),(53,46),(45,41)]],outline=accent,width=2*S)
        line(d,[(47,36),(50,29),(53,39),(56,32),(59,35)],accent,1)
        text(d,(76,35),'GEONKICK',22,INK,'lm');text(d,(225,36),'MPC',11,accent,'lm')
        title=tab['tabName'].replace('SOUND L','SOUND / LAYER ').replace('MASTER ','MASTER / ')
        if source.startswith('LAYER'):title=source+' / '+bank.split(' ',1)[1]
        text(d,(650,35),title,16,INK)
        for k in range(6):box(d,(1010+k*15,32,1018+k*15,36),accent if k==tab['fnKeyIndex'] else '#324449',None,1)
        count=sum(t['fnKeyIndex']==tab['fnKeyIndex'] and not(tab['fnKeyIndex']==5 and t['componentName']=='PRESETS|PRESETS') for t in page['tabs']);text(d,(1225,35),f'{tab["fnKeySubIndex"]+1:02d} / {count:02d}',11,MUTED,'rm');save(im,file)
    # Dense display geometry and its shared revision binding are unchanged.
    im=Image.new('RGB',(1100,170),'#11151d');d=ImageDraw.Draw(im)
    for j in range(9):x=18+round(j*1064/8);d.line((x,18,x,152),fill='#252d3b')
    d.line((18,85,1082,85),fill='#465267');im.save(skin/'point-wave-background.png')
    path.write_text(json.dumps(doc,indent=2)+'\n')
    referenced=set()
    def scan(v):
        if isinstance(v,dict):
            for k,x in v.items():
                if k in ('image','filmStrip','onImage','offImage') and isinstance(x,str):referenced.add(x)
                scan(x)
        elif isinstance(v,list):
            for x in v:scan(x)
    scan(doc)
    for pattern in ['sh_bg_*.png','sh_mode_*.png']:
        for p in skin.glob(pattern):
            if p.name not in referenced:p.unlink()
    print('Modern flat interface: restrained blue/mint accents, clear cards, arc knobs, no surface texture.')
if __name__=='__main__':main()
