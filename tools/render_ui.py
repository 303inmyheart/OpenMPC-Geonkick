#!/usr/bin/env python3
"""Offline native-skin composite with actual VST values, not an MPC screenshot.
No fake waveform: the pixel strips are positioned by the compiled DSP's values.
Requires an optimized native build, built skin and Pillow.
"""
import ctypes as C,json,os,sys,time
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
mpc=Path(os.environ.get('MPC_VST',root.parent/'mpc-vst-plugins'))
sys.path.insert(0,str(mpc/'tools'));import studio
cfg=json.loads((root/'vst.json').read_text());params=json.loads((root/'params.json').read_text())['params']
ids={p['key']:i for i,p in enumerate(params)}
skin=root/'build/skin'/f"{cfg['vendor']} - VST - {cfg['name']}"/'Plugin Skins'
page=json.loads((skin/'TUI.json').read_text())['pageData']
defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
class Effect(C.Structure):
    _fields_=[('magic',C.c_int32),('dispatch',C.c_void_p),('process',C.c_void_p),('set',C.c_void_p),('get',C.c_void_p),
              ('np',C.c_int32),('npar',C.c_int32),('ni',C.c_int32),('no',C.c_int32),('flags',C.c_int32),
              ('r1',C.c_ssize_t),('r2',C.c_ssize_t),('a',C.c_int32),('b',C.c_int32),('c',C.c_int32),('io',C.c_float),
              ('obj',C.c_void_p),('user',C.c_void_p),('uid',C.c_int32),('ver',C.c_int32),('render',C.c_void_p)]
Call=C.CFUNCTYPE(C.c_ssize_t,C.c_void_p,C.c_int32,C.c_int32,C.c_ssize_t,C.c_void_p,C.c_float)
reported=set()
def host(fx,op,i,v,p,o):
    if op==0:reported.add(i)
    return 2400 if op==1 else 0
callback=Call(host)
lib=C.CDLL(str(root/'build/native'/cfg['so']));lib.VSTPluginMain.restype=C.POINTER(Effect)
e=lib.VSTPluginMain(callback);assert e and e.contents.magic==0x56737450
call=Call(e.contents.dispatch)
get=C.CFUNCTYPE(C.c_float,C.c_void_p,C.c_int32)(e.contents.get)
setp=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_float)(e.contents.set)
render=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p,C.POINTER(C.POINTER(C.c_float)),C.c_int32)(e.contents.render)
left=(C.c_float*128)();right=(C.c_float*128)();buffers=(C.POINTER(C.c_float)*2)(left,right)
def set_value(key,value):
    p=params[ids[key]];setp(e,ids[key],(value-p['min'])/(p['max']-p['min']))
def settle():
    for _ in range(50):render(e,None,buffers,128);time.sleep(0.01)
def display(i):
    b=C.create_string_buffer(128);call(e,7,i,0,b,0);return b.value.decode(errors='replace')
def rect(b):return tuple(int(float(x)) for x in b['bounds'].split())
try:
    call(e,0,0,0,None,0)
    set_value('o1_attack',0.035);set_value('o1_decay',0.45);settle()
    # The actual wrapper's dedicated UI reporter must notify a host of the plot.
    assert ids['viz_e_0'] in reported,'Plot was never reported to the VST host'
    values={i:get(e,i) for i in range(len(params))}
    state={i:round(values[i]*(len(p['options'])-1)) for i,p in enumerate(params) if p.get('options')}
    for filename,tab in [('synth',page['tabs'][0])]:
        im=Image.new('RGBA',(1280,628));dr=ImageDraw.Draw(im)
        for c in studio.tab_components(tab,defs):
            if not studio.shown(c,state):continue
            cd=c['componentData'];x,y,cw,ch=rect(c['bounds'])
            if cd['type']=='Image':
                im.alpha_composite(Image.open(skin/cd['data']['image']).convert('RGBA'),(x,y));continue
            mapping={m['key']:int(m['value'].split()[-1]) for m in c.get('handle remapping',{}).get('map',[])}
            for sub in defs[cd['type']]['componentsData']:
                sd=sub['componentData'];data=sd['data'];sx,sy,w,h=rect(sub['bounds'])
                i=mapping.get(data.get('handleName','Data'),mapping.get('Data',-1));value=values.get(i,0)
                if sd['type']=='Knob':
                    strip=Image.open(skin/data['filmStrip']).convert('RGBA')
                    n=int(data.get('numFrames',128));n=n+1 if strip.height==(n+1)*strip.width else n
                    height=strip.height//n;k=max(0,min(n-1,round(value*(n-1))))
                    im.alpha_composite(strip.crop((0,k*height,strip.width,(k+1)*height)),(x+sx,y+sy))
                elif sd['type']=='Button':
                    count=data['numButtonsInGroup'];on=value>=0.5 if count==1 else round(value*(count-1))==data['buttonId']
                    im.alpha_composite(Image.open(skin/data['onImage' if on else 'offImage']).convert('RGBA'),(x+sx,y+sy))
                elif sd['type']=='Label':
                    style=data['textStyle'];caption=cd['name'] if data.get('type')=='Name' else display(i)
                    font=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),round(style['font']['height']))
                    dr.text((x+sx+w/2,y+sy+h/2),caption,font=font,anchor='mm',fill='#'+style['colour'][-6:])
        dest=root/'build'/f'ui-{filename}.png';im.convert('RGB').save(dest);print(dest)
    (root/'build/ui-values.json').write_text(json.dumps({p['key']:values[i] for i,p in enumerate(params)},indent=2)+'\n')
finally:call(e,1,0,0,None,0)
