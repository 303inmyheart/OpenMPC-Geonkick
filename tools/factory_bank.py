#!/usr/bin/env python3
"""Pinned original factory packs -> immutable DSP definitions and native MPC states.
Sound data retains its pack licence; conversion adapter GPL-3.0-or-later.
"""
import base64,hashlib,json,math,re,struct,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FLAT=[[0,1,False],[1,1,False]]
def f32(v):return struct.unpack('<f',struct.pack('<f',float(v)))[0]
def num(v):
 v=f32(v);assert math.isfinite(v);s=format(v,'.9g');return s

def cnum(v):
 s=num(v);return s+('f' if '.' in s or 'e' in s else '.0f')
def curve(points,apply=0):
 assert len(points)<=32
 out=[]
 for p in points:
  x,y=map(f32,p[:2]);assert 0<=x<=1 and 0<=y<=1
  out.append([x,y,bool(p[2]) if len(p)>2 else False])
 return {'points':out,'apply':int(apply)}
def unit(o,master=False):
 a=o.get('ampl_env',{});fr=o.get('freq_env',{});pit=o.get('pitchshift_env',{});den=o.get('noise_density_env',{});fi=o.get('filter',{});di=o.get('distortion',{})
 raw=[o.get('enabled',False),o.get('function',0),fr.get('amplitude',800),a.get('amplitude',.8 if master else .26),o.get('phase',0),den.get('amplitude',1),pit.get('amplitude',12),o.get('seed',0),o.get('is_fm',False),10 if 'fm_k' in o else 1,fi.get('enabled',False),fi.get('type',0),fi.get('cutoff',800),fi.get('factor',10),di.get('enabled',False),di.get('type',1),di.get('drive',1),di.get('in_limiter',1),di.get('volume',di.get('out_limiter',1))]
 curves=[curve(a.get('points',FLAT)),curve(fr.get('points',FLAT),fr.get('apply_type',0)),curve(pit.get('points',[[0,.5],[1,.5]])),curve(den.get('points',FLAT)),curve(fi.get('cutoff_env',FLAT),fi.get('apply_type',0)),curve(fi.get('qfactor_env',FLAT)),curve(di.get('drive_env',FLAT)),curve(di.get('volume_env',FLAT))]
 return {'scalar':list(map(f32,raw)),'curves':curves}
def load():
 provenance=json.loads((ROOT/'factory/sources.json').read_text());records=[];presets=[];notices=[]
 for archive,sha in provenance['archives'].items():
  path=ROOT/'factory/original'/archive;assert hashlib.sha256(path.read_bytes()).hexdigest()==sha
  pack=archive.split('.tar')[0]
  with tarfile.open(path) as t:
   for m in sorted(t.getmembers(),key=lambda m:m.name):
    if not m.isfile():continue
    if 'readme' in Path(m.name).name.lower() or 'license' in Path(m.name).name.lower():notices.append((pack,m.name,t.extractfile(m).read().decode()))
    if not m.name.endswith(('.gkick','.gkit')):continue
    data=json.load(t.extractfile(m));items=data.get('percussions',data.get('instruments',[data]));ids=[]
    assert len(items)<=16
    for index,d in enumerate(items):
     digest=hashlib.sha256(json.dumps(d,sort_keys=True,separators=(',',':')).encode()).hexdigest()
     record={'id':len(records)+1,'pack':pack,'source':m.name,'slot':index,'data':d,'digest':digest,'units':[unit(d['kick'],True)]+[unit(d.get(f'osc{o}',{})) for o in range(9)]}
     records.append(record);ids.append(record['id'])
    presets.append({'pack':pack,'source':m.name,'name':Path(m.name).stem,'kit':m.name.endswith('.gkit'),'ids':ids})
 return records,presets,notices

def definitions():
 keys=[];defaults={};bounds={}
 for k,lo,hi,de in re.findall(r'\{"([^"]+)", ([^,]+), ([^,]+), ([^,]+), [01]\}',(ROOT/'src/param_defs.h').read_text()):
  keys.append(k);defaults[k]=float(de.rstrip('f'));bounds[k]=(float(lo.rstrip('f')),float(hi.rstrip('f')))
 return keys,defaults,bounds

def baseline(record,defaults,bounds):
 p=defaults.copy();d=record['data'];k=d['kick']
 def setp(key,v):
  lo,hi=bounds[key];p[key]=f32(max(lo,min(hi,v)))
 setp('volume',k.get('limiter',1));setp('length',k['ampl_env']['length']/1000)
 # Original playback: FullLength0 / NoteOff1 / Cut2.
 setp('playback',{0:0,1:2,2:1}.get(k.get('playbackMode',1 if k.get('noteOffEnabled',False) else 0),0))
 setp('keytrack',k.get('tuned_output',False))
 for l in range(3):setp(f'layer{l+1}',k.get('layers_amplitude',[1,1,1])[l] if l in k.get('layers',[0]) else 0)
 for i,u in enumerate(record['units']):
  s=u['scalar'];pre=f'o{i}_' if i else ''
  for suffix,field in [('filter_on',10),('filter_type',11),('cutoff',12),('q',13),('drive_on',14),('drive_type',15),('drive',16),('drive_out',18)]:setp(pre+suffix,s[field])
  setp(pre+'filter_model',0);setp(pre+'cutoff_wide',s[12]);setp(pre+'q_wide',s[13]/10)
  if not i:continue
  for suffix,v in [('on',s[0]),('wave',{0:0,1:1,2:2,3:3,4:4,6:5,7:0}[int(s[1])]),('freq',s[2]),('level',s[3]),('phase',s[4]*180/math.pi),('density',s[5]),('sweep',0)]:setp(pre+suffix,v)
  sample=int(s[1])==7
  if sample:setp(pre+'freq',55)
  if i%3==1:setp(pre+'fm',1 if s[8] else 0)
  # Native editor's existing sixteen-knot representation. Original complete
  # curves remain immutable DSP input until this curve is explicitly edited.
  for kind,ci in [('a',0),('p',1)]:
   edit_curve=u['curves'][2 if sample and kind=='p' else ci]
   pts=[q for q in edit_curve['points'] if not q[2]]
   if not pts:pts=[[0,1,False],[1,1,False]]
   if len(pts)>16:pts=[pts[round(j*(len(pts)-1)/15)] for j in range(16)]
   if len(pts)==1:pts=[pts[0],pts[0]]
   xs=[q[0] for q in pts];xs[0]=0;xs[-1]=1
   if any(b-a<1e-6 for a,b in zip(xs,xs[1:])):xs=[j/(len(pts)-1) for j in range(len(pts))]
   setp(pre+kind+'_mode',1);setp(f'f{i}_{kind}_n',len(pts));upper=min(20000,p[pre+'freq']*16)
   for j,q in enumerate(pts):
    y=q[1]
    if kind=='p':
     hz=55*2**((2*y-1)*s[6]/12) if sample else 20*(max(20,s[2])/20)**y if u['curves'][ci]['apply']==1 else s[2]*y
     y=(hz-20)/(upper-20)
    setp(f'f{i}_{kind}_x{j}',xs[j]);setp(f'f{i}_{kind}_y{j}',y)
 return p

def prepare():
 records,presets,notices=load();keys,defaults,bounds=definitions();lines=['/* Generated from pinned, individually licensed factory archives. */','#define FACTORY_COUNT '+str(len(records)),'static const FactorySound factory_sounds[FACTORY_COUNT]={']
 projection=json.loads((ROOT/'factory/projection.json').read_text()) if (ROOT/'factory/projection.json').exists() else None
 for r in records:
  p=baseline(r,defaults,bounds);r['baseline']=p
  if projection is not None:assert projection[str(r['id'])]==hashlib.sha256(' '.join(num(p[k]) for k in keys).encode()).hexdigest(),'Factory projection changes require an explicit state migration'
  units='{'+','.join('{'+','.join(map(cnum,u['scalar']))+'}' for u in r['units'])+'}'
  curves='{'+','.join('{'+','.join('{'+str(len(c['points']))+','+str(c['apply'])+',{'+','.join('{'+cnum(x)+','+cnum(y)+','+('true' if flag else 'false')+'}' for x,y,flag in c['points'])+'}}' for c in u['curves'])+'}' for u in r['units'])+'}'
  lines.append('{'+cnum(r['data']['kick'].get('limiter',1))+','+units+','+curves+',{'+','.join(cnum(p[k]) for k in keys)+'},"'+r['digest']+'"},')
 lines.append('};');(ROOT/'build/factory_data.h').write_text('\n'.join(lines)+'\n')
 index=[{'id':r['id'],'pack':r['pack'],'source':r['source'],'slot':r['slot'],'digest':r['digest']} for r in records]
 assert index==json.loads((ROOT/'factory/index.json').read_text()),'Factory IDs/data must remain immutable'
 (ROOT/'build/factory-index.json').write_text(json.dumps(index,indent=2)+'\n')
 return records,presets,notices

def state(preset,records):
 keys,defaults,_=definitions();lookup={r['id']:r for r in records};ids=preset['ids'];padrows=[lookup[i]['baseline'] for i in ids]+[defaults]*(16-len(ids))
 tokens=['GKMP9']+[num(padrows[0][k]) for k in keys]+['KIT1',str(int(preset['kit'])),'36','0','1']
 for row in padrows[1:]:tokens+=['D'] if row==defaults else ['P']+[num(row[k]) for k in keys]
 # Use the pack's original note map for kits when valid/unique; hardware Learn16
 # can replace it. No missing sound receives a duplicate assigned note.
 notes=[int(lookup[i]['data']['kick'].get('playing_key',36+j)) for j,i in enumerate(ids)]
 for j in range(len(ids),16):notes.append(next(n for n in range(36,128) if n not in notes))
 valid=len(set(notes))==16 and all(0<=n<=127 for n in notes)
 tokens+=['MAP1',str(int(valid and preset['kit']))]+list(map(str,notes if valid else range(36,52)))
 assets=[];total=0
 for pad,i in enumerate(ids):
  for osc in range(9):
   o=lookup[i]['data'].get(f'osc{osc}',{});encoded=o.get('sample','')
   if not encoded:continue
   raw=base64.b64decode(encoded);raw=raw[:min(len(raw)//4,44100*4)*4]
   assert all(math.isfinite(v) and abs(v)<=1 for (v,) in struct.iter_unpack('<f',raw))
   total+=len(raw);assert total<=11*1024*1024
   assets += [str(pad),str(osc),str(int(o.get('function')==7)),str(len(raw)//4),('Factory-%02d.wav'%(osc+1)).encode().hex(),base64.b64encode(raw).decode()]
 if assets:tokens+=['SMP1',str(len(assets)//6)]+assets
 tokens+=['FCT1',str(len(ids))]
 for pad,i in enumerate(ids):tokens += [str(pad),str(i),lookup[i]['digest']]
 return (' '.join(tokens)+'\0').encode()

if __name__=='__main__':
 records,presets,notices=prepare();out=ROOT/'build/factory-chunks';out.mkdir(exist_ok=True)
 manifest=[];fixtures=[];samples=ROOT/'build/factory-samples';samples.mkdir(exist_ok=True)
 for r in records:
  for osc in range(9):
   encoded=r['data'].get(f'osc{osc}',{}).get('sample','')
   if encoded:
    raw=base64.b64decode(encoded);raw=raw[:min(len(raw)//4,44100*4)*4];(samples/f'{r["id"]}-{osc}.f32').write_bytes(raw)
 # Legacy no-extension state is the reset oracle.
 keys,defaults,_=definitions();(out/'init-legacy.chunk').write_bytes(('GKMP9 '+' '.join(num(defaults[k]) for k in keys)+' KIT1 0 36 0 1'+' D'*15+' MAP1 0 '+' '.join(map(str,range(36,52)))+'\0').encode())
 for j,p in enumerate(presets):
  name=f'{2000+j:04d}-{p["pack"]} - {p["name"]}'+(' (Kit)' if p['kit'] else '')
  name=re.sub(r'[\\/:*?"<>|]','_',name);raw=state(p,records);(out/(name+'.chunk')).write_bytes(raw)
  fixtures.append('{'+json.dumps(name)+','+str(len(p['ids']))+',{'+','.join(map(str,p['ids']))+'}}')
  manifest.append({'stem':name,'pack':p['pack'],'source':p['source'],'ids':p['ids'],'kit':p['kit'],'sha256':hashlib.sha256(raw).hexdigest()})
 (ROOT/'build/factory-presets.json').write_text(json.dumps(manifest,indent=2)+'\n')
 (ROOT/'build/factory_fixtures.h').write_text('typedef struct {const char *stem;int count,ids[16];} FactoryFixture;\n#define FACTORY_FIXTURE_COUNT '+str(len(fixtures))+'\nstatic const FactoryFixture factory_fixtures[]={'+',\n'.join(fixtures)+'};\n')
 print(f'Original factory bank: {len(records)} sound definitions, {len(presets)} native presets/kits, all pinned archives verified')
