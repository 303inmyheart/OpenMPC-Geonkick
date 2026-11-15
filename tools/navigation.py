#!/usr/bin/env python3
"""Six native menu keys; Q-Links follow rotary controls, never selectors or points.
Run after finish_skin.py so geometry includes captions, fine/direct touch aliases
and the existing waveform/editor. No DSP or parameter identities are changed.
"""
from pathlib import Path
from copy import deepcopy
import json,os,sys,hashlib
ROOT=Path(__file__).resolve().parents[1]
MPC=Path(os.environ.get('MPC_VST',ROOT.parent/'mpc-vst-plugins'))
sys.path.insert(0,str(MPC/'tools'))
import shadow_skin
from PIL import Image,ImageDraw,ImageFont
GROUPS=[('SOUND',[f'SYNTH|LAYER {l}' for l in range(1,4)])]
for l in range(1,4):GROUPS.append((f'LAYER {l}',[f'LAYER {l}|L{l} {kind}{o}' for o in range(1,4) for kind in ('OSC','FX')]))
GROUPS += [('ENVELOPE',['ENVELOPE|ENVELOPE']),('MASTER',['SYNTH|MIX','EFFECTS|EFFECTS','PRESETS|PRESETS'])]
LOCATION={name:(main,sub,title) for main,(title,names) in enumerate(GROUPS) for sub,name in enumerate(names)}
POINT_KEYS={'point_select','point_level_fine','point_time_fine','point_copy'}
def widgets(layout,bank):return [w for w in layout['widgets'] if not w.get('banks') or bank in w['banks'].split('|')]
def knob_groups(layout,bank):
    rotary={w['key']:w for w in widgets(layout,bank) if w['kind']=='knob' and w['key'] not in POINT_KEYS}
    if layout['name']=='SYNTH' and bank!='MIX':
        first=(int(bank[-1])-1)*3+1
        keys=[[f'o{o}_freq_fine',f'o{o}_level'] for o in range(first,first+3)]+[['length_ms','volume']]
        return [[rotary[k] for k in group] for group in keys]
    rows={}
    for w in sorted(rotary.values(),key=lambda w:(w['cy'],w['cx'])):rows.setdefault(w['cy'],[]).append(w)
    groups=[]
    for row in rows.values():
        for i in range(0,len(row),4):groups.append(row[i:i+4])
    assert len(groups)<=4,(layout['name'],bank)
    return groups

def generate():
    layouts,_=shadow_skin.parse_layout(ROOT/'layout.conf');byname={t['name']:t for t in layouts}
    params=json.loads((ROOT/'params.json').read_text())['params'];ids={p['key']:i for i,p in enumerate(params)}
    path=next((ROOT/'build/skin').glob('*/Plugin Skins/TUI.json'));doc=json.loads(path.read_text());page=doc['pageData']
    defs={d['key']:d['value'] for d in page['componentDefinitions']['localComponentDefinitions']}
    qpath=path.parent/'Q-Links.json';q=json.loads(qpath.read_text());maps=[]
    for tab in page['tabs']:
        source,bank=tab['componentName'].split('|');layout=byname[source]
        main,sub,title=LOCATION[tab['componentName']];tab['fnKeyIndex']=main;tab['fnKeySubIndex']=sub
        label=bank
        if title=='SOUND':label='SOUND L'+bank[-1]
        if title=='MASTER':label='MASTER '+('FX' if bank=='EFFECTS' else bank)
        tab['tabName']=label
        children=defs[tab['componentName']]['componentsData'];groups=knob_groups(layout,bank)
        rotary={ids[w['key']] for group in groups for w in group}
        # Disable hardware focus only on this placed non-rotary control. Native
        # Button toggles and continuous Touch handles retain their original data.
        for c in children:
            mapping=c.get('handle remapping',{}).get('map',[])
            data=next((m['value'] for m in mapping if m['key']=='Data'),None)
            if not data or not data.startswith('Parameter '):continue
            i=int(data.split()[1]);original=defs.get(c['componentData']['type'])
            if i in rotary or original is None or original.get('ignoreMousePresses'):continue
            if c['bounds'].get('acceptsHWFocus')=='No' and not any(a.get('handler')=='Q-Link' for a in original.get('actions',[])):continue
            key=c['componentData']['type']+'_touchOnly'
            if key not in defs:
                clone=deepcopy(original);clone['actions']=[a for a in clone.get('actions',[]) if a.get('handler')!='Q-Link']
                defs[key]=clone;page['componentDefinitions']['localComponentDefinitions'].append({'key':key,'value':clone})
            c['componentData']['type']=key;c['bounds']['acceptsHWFocus']='No'
        assigned={f'Q-Link {i}':-1 for i in range(1,17)};frames=[]
        for row,group in enumerate(groups):
            boxes=[]
            for col,w in enumerate(group):
                target=f'Parameter {ids[w["key"]]}';assigned[f'Q-Link {13+col-4*row}']=ids[w['key']]
                found=[]
                for c in children:
                    if any(m['key']=='Data' and m['value']==target for m in c.get('handle remapping',{}).get('map',[])):
                        x,y,ww,hh=map(int,c['bounds']['bounds'].split());found.append((x,y,x+ww,y+hh))
                assert found,(source,bank,w['key']);boxes+=found
            x=min(b[0] for b in boxes)-6;y=min(b[1] for b in boxes)-6;r=max(b[2] for b in boxes)+6;bottom=max(b[3] for b in boxes)+6
            assert 0<=x<r<=1280 and 0<=y<bottom<=628,(source,bank,row,boxes)
            frames.append(f'{x} {y} {r-x} {bottom-y}')
        tab['qlinkBoundsData']=frames or ['0 0 0 0']
        maps.append({'Tab':main+1,'SubTab':sub+1,'Bank Direction':'RowTopDown','Q-Links':assigned})
        # A small static banner replaces only the old title area. Controls start
        # below it; waveforms and native envelope geometry remain untouched.
        token=hashlib.sha256(tab['componentName'].encode()).hexdigest()[:10];filename=f'gk_nav_{token}.png'
        im=Image.new('RGBA',(1280,70),(0,0,0,0));draw=ImageDraw.Draw(im)
        draw.rounded_rectangle((24,14,1256,56),radius=5,fill='#252b28',outline='#91b958',width=1)
        draw.text((45,35),'GEONKICK MPC',font=ImageFont.truetype(str(ROOT/'art/DejaVuSans.ttf'),22),anchor='lm',fill='#edf4f4')
        subtitle=title if title=='ENVELOPE' else title+' / '+('LAYER '+bank[-1] if title=='SOUND' else ('MIX' if bank=='MIX' else 'FX' if bank=='EFFECTS' else bank))
        draw.text((640,35),subtitle,font=ImageFont.truetype(str(ROOT/'art/DejaVuSans.ttf'),18),anchor='mm',fill='#b5d979')
        draw.text((1225,35),f'{sub+1}/{len(GROUPS[main][1])}',font=ImageFont.truetype(str(ROOT/'art/DejaVuSans.ttf'),14),anchor='rm',fill='#a8bfc4')
        im.save(path.parent/filename)
        children.append(shadow_skin._sub('Image',{'version':2,'imageType':'Regular','colour':'0','image':filename},shadow_skin._bounds(0,0,1280,70),'Navigation'))
    assert set(LOCATION)=={t['componentName'] for t in page['tabs']}
    page['tabs'].sort(key=lambda t:(t['fnKeyIndex'],t['fnKeySubIndex']));maps.sort(key=lambda m:(m['Tab'],m['SubTab']))
    q['Screen Mode Q-Links']['map']=maps
    # Outside page-follow mode, keep the same knob-only overview assignments.
    program=q.get('Program Mode Q-Links',{});target=program.get('map',program)
    if isinstance(target,dict):
        for k in target:
            if k.startswith('Q-Link '):target[k]=-1
        for row,group in enumerate(knob_groups(byname['SYNTH'],'LAYER 1')):
            for col,w in enumerate(group):target[f'Q-Link {13+col-4*row}']=ids[w['key']]
    path.write_text(json.dumps(doc,indent=2)+'\n');qpath.write_text(json.dumps(q,indent=2)+'\n')
    print('25 pages under six keys: SOUND, LAYER1–3, ENVELOPE, MASTER; rotary-only Q-Links, RowTopDown frames; touch-only points/selectors.')
if __name__=='__main__':generate()
