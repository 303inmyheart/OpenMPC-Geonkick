#!/usr/bin/env python3
"""Final flat touch UI; preserves native controls, wave binding and knob focus."""
import json,sys,os,re,hashlib
from copy import deepcopy
from pathlib import Path
from PIL import Image,ImageDraw
from skin_design import BG,PANEL,BORDER,INK,MUTED,GREEN,CYAN,AMBER,S,box,line,text,save,segment_image
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(Path(os.environ.get('MPC_VST',ROOT.parent/'mpc-vst-plugins'))/'tools'))
import shadow_skin as sh
p=next((ROOT/'build/skin').glob('*/Plugin Skins/TUI.json'));skin=p.parent
page=json.loads(p.read_text())['pageData'];defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
params=json.loads((ROOT/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
def data_key(c):
 v=next((m['value'] for m in c.get('handle remapping',{}).get('map',[]) if m['key']=='Data'),None)
 return params[int(v.split()[1])]['key'] if v and v.startswith('Parameter ') else None
def rect(c):return tuple(map(int,c['bounds']['bounds'].split()))
def add_def(name,value):
 defs[name]=value;page['componentDefinitions']['localComponentDefinitions'].append({'key':name,'value':value})
def button(template,key,label,bounds,primary=False,dynamic=False):
 c=deepcopy(template);name='modern_'+key;v=deepcopy(defs[c['componentData']['type']]);w,h=bounds[2:]
 c['componentData']['type']=name;c['componentData']['name']=label
 for m in c['handle remapping']['map']:
  if m['key']=='Data':m['value']=f'Parameter {ids[key]}'
 if not dynamic:c['handle remapping']['map']=[m for m in c['handle remapping']['map'] if m['key']!='Text']
 button_number=0
 for part in v['componentsData']:
  part['bounds']['bounds']=f'0 0 {w} {h}'
  cd=part['componentData']
  if cd['type']=='Button':
   button_number+=1
   for on in (False,True):
    fn=name+('_hit' if dynamic and button_number>1 else '')+('_on.png' if on else '_off.png');im=Image.new('RGBA',(w*S,h*S),(0,0,0,0));d=ImageDraw.Draw(im)
    if not(dynamic and button_number>1):box(d,(1,1,w-1,h-1),GREEN if primary or on else '#252c38',GREEN if primary or on else BORDER,7)
    if not dynamic:text(d,(18 if label.startswith(('BACK','NEXT')) else w/2,h/2),label,17,'#101722' if primary or on else INK,'lm' if label.startswith(('BACK','NEXT')) else 'mm')
    save(im,skin/fn);cd['data']['onImage' if on else 'offImage']=fn
  elif cd['type']=='Label':
   cd['data']['textStyle']['colour']='fff1f4fa'
   part['bounds']['bounds']=f'14 0 {w-28} {h}'
 v['hideQLinkBounds']=True;v['actions']=[a for a in v.get('actions',[]) if a.get('handler')!='Q-Link']
 add_def(name,v);c['bounds']['bounds']=' '.join(map(str,bounds));c['bounds']['acceptsHWFocus']='No';return c
# Restyle ordinary action buttons, keeping dimensions and transient behavior.
seen=set()
for tab in page['tabs']:
 for c in defs[tab['componentName']]['componentsData']:
  key=data_key(c);v=defs.get(c['componentData']['type'])
  if not key or not v:continue
  if not params[ids[key]].get('momentary'):continue
  if c['componentData']['type'] in seen:continue
  seen.add(c['componentData']['type'])
  for part in v.get('componentsData',[]):
   cd=part['componentData'];w,h=rect(part)[2:]
   if cd['type']!='Button':continue
   for field,on in [('onImage',True),('offImage',False)]:
    fn=cd['data'].get(field)
    if fn and (skin/fn).exists():segment_image(w,h,c['componentData'].get('name',''),on).save(skin/fn)
# Final headers use actual reachable pages, after SOUND consolidation.
for tab in page['tabs']:
 source,bank=tab['componentName'].split('|');children=defs[tab['componentName']]['componentsData']
 navs=[c for c in children if c['componentData'].get('name')=='Navigation']
 title='SOUND DESIGN' if source=='SYNTH' and bank!='MIX' else 'AMPLITUDE / PITCH' if source=='ENVELOPE' else 'PAD KIT' if source=='KIT' else 'SAMPLE LIBRARY' if source=='SAMPLES' else 'SOUND MIX' if bank=='MIX' else 'MASTER EFFECTS' if source=='EFFECTS' else source+'  /  '+('OSCILLATOR '+bank[-1] if 'OSC' in bank else 'PROCESSING '+bank[-1])
 if source.startswith('LAYER'):tab['tabName']=('OSC ' if 'OSC' in bank else 'FX ')+bank[-1]
 if tab['fnKeyIndex']==5:tab['tabName']={'KIT':'KIT','SAMPLES':'SAMPLES','MIX':'MIX','EFFECTS':'MASTER FX'}[bank]
 im=Image.new('RGB',(1280*S,70*S),BG);d=ImageDraw.Draw(im)
 box(d,(24,16,28,51),GREEN,None,2);text(d,(42,34),'GEONKICK',23,INK,'lm');text(d,(202,35),'MPC',12,GREEN,'lm')
 text(d,(850,34),title,18,INK)
 count=sum(t['fnKeyIndex']==tab['fnKeyIndex'] for t in page['tabs'])
 text(d,(1240,34),f'{tab["fnKeySubIndex"]+1} / {count}',14,MUTED,'rm');line(d,[(24,65),(1256,65)],BORDER)
 for nav in navs:save(im,skin/nav['componentData']['data']['image'])
# Dedicated sample workflow: files left, destination and explicit commit right.
children=defs['SAMPLES|SAMPLES']['componentsData'];existing={data_key(c):c for c in children if data_key(c)}
base=existing['sample_load'];new=[c for c in children if c['componentData'].get('name')=='Navigation' or data_key(c)=='kit_title']
im=Image.new('RGB',(1280*S,628*S),BG);d=ImageDraw.Draw(im)
box(d,(24,78,824,610),PANEL,BORDER,12);box(d,(844,78,1256,610),PANEL,BORDER,12)
text(d,(860,99),'TARGET: LAYER / OSCILLATOR',14,MUTED,'lm');text(d,(40,166),'TAP A WAV TO PREVIEW  /  TAP AGAIN TO STOP',12,MUTED,'lm')
text(d,(1050,300),'ASSIGNED SAMPLE',12,MUTED);text(d,(1050,242),'OSCILLATOR SOURCE',13,MUTED);text(d,(1050,351),'PREVIEW SELECTED FILE',13,MUTED)
text(d,(1050,420),'LOAD REPLACES THE SELECTED OSCILLATOR',11,MUTED)
fn='modern_samples_panel.png';save(im,skin/fn)
new.insert(0,sh._sub('Image',{'version':2,'imageType':'Regular','colour':'0','image':fn},sh._bounds(0,0,1280,628),'Sample library panel'))
for key,label,r in [('sample_open','OPEN DEFAULT',(40,92,210,38)),('sample_set_default','SET DEFAULT',(260,92,190,38)),('sample_root','DRIVES',(460,92,170,38)),('sample_up','UP',(640,92,166,38)),('sample_prev','BACK',(40,558,220,40)),('sample_next','NEXT',(586,558,220,40)),('sample_preview','PLAY PREVIEW',(864,366,178,42)),('sample_preview_stop','STOP',(1054,366,178,42)),('sample_load','LOAD SAMPLE',(864,438,368,52)),('sample_clear','REMOVE FROM OSC',(864,512,368,40))]:
 new.append(button(existing.get(key,base),key,label,r,key=='sample_load'))
for row in range(8):
 key=f'sample_pick{row}';new.append(button(existing[key],key,'',(40,180+44*row,766,40),dynamic=True))
for key,r in [('sample_folder',(40,132,766,30)),('sample_page',(280,562,286,30)),('sample_name',(864,310,368,32)),('sample_status',(858,571,380,30))]:
 c=deepcopy(existing[key]);c['bounds']['bounds']=' '.join(map(str,r));new.append(c)
# Selectors keep their complete native targets/visibility; translate as a group.
for key,target in [('plot_osc',(872,122)),('sample_source',(935,262))]:
 cs=[deepcopy(c) for c in children if data_key(c)==key];lx=min(rect(c)[0] for c in cs);ly=min(rect(c)[1] for c in cs)
 for c in cs:
  x,y,w,h=rect(c);c['bounds']['bounds']=f'{x-lx+target[0]} {y-ly+target[1]} {w} {h}';new.append(c)
defs['SAMPLES|SAMPLES']['componentsData']=new
# Replace the old miniature switch plus duplicate caption with one clear action.
env=defs['ENVELOPE|ENVELOPE']['componentsData']
env[:]=[c for c in env if data_key(c)!='point_copy']
env.append(button(base,'point_copy','COPY SIMPLE',(466,159,208,44)))

# Bright text values without old green cast. All native XY scales stay intact.
for value in defs.values():
 for part in value.get('componentsData',[]):
  cd=part['componentData']
  if cd['type']=='Label' and 'textStyle' in cd['data']:
   cd['data']['textStyle']['colour']='ff7ba9ff' if cd['data'].get('type')=='Value' else 'fff1f4fa'
# Serialize the same page object, not a screenshot/image-based UI.
doc=json.loads(p.read_text());doc['pageData']=page;p.write_text(json.dumps(doc,indent=2)+'\n')
print('Modern UI: 24 native pages, grouped oscillator/FX navigation, flat arc knobs, larger sample rows, tap-preview and separate Load/Stop.')
