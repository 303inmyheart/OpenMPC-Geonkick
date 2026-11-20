#!/usr/bin/env python3
"""Original percussion-lab artwork. Native geometry/actions/handles stay intact."""
from pathlib import Path
import json,math,os,sys,re,hashlib
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
BG='#101a1e';PANEL='#1b292e';BORDER='#34484d';INK='#e9f0e9';MUTED='#94aaa9';GREEN='#b9ed68';AMBER='#efbc73';CYAN='#6cbab6'
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
    box(d,(1,1,width-1,height-1),GREEN if on else '#24383e',GREEN if on else '#42585d',4)
    line(d,[(7,3),(width-7,3)],'#d7f9ab' if on else '#53696e')
    size=min(22,height*.43)
    while font(size).getlength(label)>(width-12)*S and size>9:size-=1
    text(d,(width/2,height/2),label,size,'#152019' if on else '#d1dfdb')
    return im.resize((width,height),Image.Resampling.LANCZOS)

def make_knob(path):
    radius=int(re.search(r'_r(\d+)',path.name)[1]);size=2*radius+10
    old=Image.open(path);frames=old.height//size;strip=Image.new('RGBA',(size,size*frames))
    c=size/2;cap=radius-7
    for n in range(frames):
        im=Image.new('RGBA',(size*S,size*S),(0,0,0,0));d=ImageDraw.Draw(im)
        d.arc(tuple(round(v*S) for v in (c-radius,c-radius,c+radius,c+radius)),135,405,fill='#3d5357',width=2*S)
        for tick in range(25):
            a=math.radians(135+270*tick/24);ri=radius-3;ro=radius+1
            line(d,[(c+ri*math.cos(a),c+ri*math.sin(a)),(c+ro*math.cos(a),c+ro*math.sin(a))],GREEN if tick<=n*24/(frames-1) else '#617373',1)
        ellipse(d,(c-cap-2,c-cap,c+cap+3,c+cap+5),'#060d10')
        ellipse(d,(c-cap-1,c-cap-1,c+cap+1,c+cap+1),'#53666a','#82958d')
        for r in range(cap,0,-1):
            v=31+round(17*(cap-r)/cap);ellipse(d,(c-r,c-r,c+r,c+r),(v,v+11,v+14,255))
        for j in range(36):
            a=math.pi*2*j/36
            line(d,[(c+(cap-3)*math.cos(a),c+(cap-3)*math.sin(a)),(c+cap*math.cos(a),c+cap*math.sin(a))],'#152328',1)
        ellipse(d,(c-cap+4,c-cap+4,c+cap-4,c+cap-4),'#2e4045','#16272b')
        a=math.radians(135+270*n/(frames-1))
        line(d,[(c+cap*.3*math.cos(a),c+cap*.3*math.sin(a)),(c+(cap-5)*math.cos(a),c+(cap-5)*math.sin(a))],GREEN,2.6)
        ellipse(d,(c-2,c-2,c+2,c+2),'#192a2e')
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
    # Fine deterministic horizontal finish, without texture under the live plots.
    for y in range(0,628,3):line(d,[(0,y),(1280,y)],'#121e23')
    box(d,(16,10,1264,616),None,'#34464a',12)
    def card(r):box(d,r,PANEL,BORDER,10);line(d,[(r[0]+12,r[1]+1),(r[2]-12,r[1]+1)],'#41575b')
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
        text(d,(90,240),'WAVEFORM + ENVELOPE',13,MUTED,'lm');text(d,(1190,240),'SELECT POINT NUMBER / DRAG AMBER POINT',12,AMBER,'rm')
    elif name=='HUMANIZER':
        card((70,72,1210,190));card((210,205,620,465));card((660,205,1070,465))
        text(d,(420,423),'VELOCITY  +/-  AMOUNT',13,MUTED)
        text(d,(860,423),'DELAY  0 TO MAXIMUM',13,MUTED)
        text(d,(640,510),'RANDOM VARIATION PER HIT  /  ORIGINAL GEONKICK HUMANIZER',14,INK)
        text(d,(640,545),'Starts OFF. Existing notes keep their sound. Timing never moves a hit earlier.',13,MUTED)
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
        seen.add(pos);text(d,pos[:2],pos[2],12,MUTED)
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
                cd['data']['textStyle']['colour']='ffb9ed68'
                if rotary:
                    x,y,w,h=map(int,sub['bounds']['bounds'].split())
                    focus=next(c for c in parts if c['componentData']['type']=='Focus')
                    _,_,cw,ch=map(int,focus['bounds']['bounds'].split())
                    fn=f'gk_value_{cw}x{ch}_{y}.png'
                    if not (skin/fn).exists():
                        im=Image.new('RGBA',(cw*S,ch*S),(0,0,0,0));dr=ImageDraw.Draw(im)
                        box(dr,(x+8,y+1,x+w-8,y+h-1),'#0a171a','#395356',5)
                        line(dr,[(x+14,y+h-3),(x+w-14,y+h-3)],'#1e3639');save(im,skin/fn)
                    # Background paint cannot capture gestures over the value.
                    value['backgroundData']={'version':1,**{state:{'version':1,'colour':'00000000','image':fn} for state in ('focussed','unfocussed')}}
            if cd['type']=='Envelope' and key!='gkFreeEnvelope':
                for field,color in [('lineColour','ffefbc73'),('handleLineColour','ffefbc73'),('handleFillColour','ff192b31'),('handleFillColourOnPressed','ffb9ed68'),('scaleColour','ff31494c')]:cd['data'][field]=color
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
        count=sum(t['fnKeyIndex']==tab['fnKeyIndex'] for t in page['tabs']);text(d,(1225,35),f'{tab["fnKeySubIndex"]+1:02d} / {count:02d}',11,MUTED,'rm');save(im,file)
    # Dense display geometry and its shared revision binding are unchanged.
    im=Image.new('RGB',(1100,170),'#0b1a1f');d=ImageDraw.Draw(im)
    for j in range(9):x=18+round(j*1064/8);d.line((x,18,x,152),fill='#20383e')
    d.line((18,85,1082,85),fill='#496769');im.save(skin/'point-wave-background.png')
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
    print('Percussion Lab: own rotary filmstrips, recessed values, graphite panels, green/amber displays, native targets/geometry unchanged.')
if __name__=='__main__':main()
