#!/usr/bin/env python3
"""Generate the initial frozen parameter table, C defaults and MPC layout."""
import json,math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
params=[]
def p(key,name,lo,hi,default,**extra):
    params.append(dict(key=key,name=name,min=lo,max=hi,default=default,**extra))
def opt(key,name,options,default=0,**extra):
    p(key,name,0,len(options)-1,default,options=options,**extra)

p('volume','Output',0,1,0.7)
p('length','Length',0.03,4,0.6,unit='s',dynamic_display=True)
p('tune','Tune',-24,24,0,unit='st')
opt('keytrack','Key Track',['Off','On'])
opt('playback','Playback',['Poly','Cut','Gate'])
for l in range(3):
    p(f'layer{l+1}','Layer '+str(l+1),0,1,1 if l==0 else 0)
opt('preset','Preset',['Init','Deep Kick','Tight Kick','Snare','Closed Hat','Open Hat','Metal','Tom'],refresh_display=True)
opt('audition','Audition',['Off','Hit'],momentary=True,debounce_ms=150)
for o in range(9):
    k=f'o{o+1}_'
    opt(k+'on',f'O{o+1} On',['Off','On'],1 if o%3==0 else 0)
    opt(k+'wave',f'O{o+1} Wave',['Sine','Square','Triangle','Saw','White','Brown'])
    p(k+'freq',f'O{o+1} Freq',20,12000,55,unit='Hz',log=True)
    p(k+'level',f'O{o+1} Level',0,1,0.8)
    p(k+'decay',f'O{o+1} Decay',0.005,4,0.5,unit='s',dynamic_display=True)
    p(k+'sweep',f'O{o+1} Sweep',0,48,24,unit='st')
    p(k+'sweep_time',f'O{o+1} Pitch Time',0.001,1,0.04,unit='s',dynamic_display=True)
    p(k+'phase',f'O{o+1} Phase',0,360,0,unit='deg')
    p(k+'density',f'O{o+1} Density',0.01,1,1)
    if o%3==0:p(k+'fm',f'O{o+1} FM',0,1,0)
opt('filter_on','Filter',['Off','On'])
opt('filter_type','Filter Type',['Low Pass','High Pass','Band Pass'])
p('cutoff','Cutoff',30,8000,8000,unit='Hz')
p('q','Resonance',10,1000,10)
opt('drive_on','Drive',['Off','On'])
opt('drive_type','Drive Type',['Hard','Tanh','Arctan','Exp','Poly','Log','Fold','Half','Full'],1)
p('drive','Drive Amount',1,20,2)
p('drive_out','Drive Output',0,1,0.7)
opt('reset','Reset Init',['Off','Reset'],momentary=True,refresh_display=True)
# Append-only extension: the first 103 parameters retain their 0.0.1 indices.
for o in range(9):
    k=f'o{o+1}_'
    p(k+'attack',f'O{o+1} Attack',0,1,0,unit='s',dynamic_display=True)
    opt(k+'filter_on',f'O{o+1} Filter',['Off','On'])
    opt(k+'filter_type',f'O{o+1} Filter Type',['Low Pass','High Pass','Band Pass'])
    p(k+'cutoff',f'O{o+1} Cutoff',30,8000,8000,unit='Hz')
    p(k+'q',f'O{o+1} Resonance',10,1000,10)
    opt(k+'drive_on',f'O{o+1} Drive',['Off','On'])
    opt(k+'drive_type',f'O{o+1} Drive Type',['Hard','Tanh','Arctan','Exp','Poly','Log','Fold','Half','Full'],1)
    p(k+'drive',f'O{o+1} Drive Amount',1,20,2)
    p(k+'drive_out',f'O{o+1} Drive Output',0,1,0.7)
opt('plot_osc','Envelope Focus',[f'L{l} / {o}' for l in range(1,4) for o in range(1,4)])
opt('plot_env','Envelope View',['Amplitude','Pitch'])
CONTROL_COUNT=len(params)
PLOT_COLUMNS=432
PLOT_WIDTH=2
PLOT_WAVE_LEVELS=97
PLOT_ENV_LEVELS=85
# Preserve all 288 original display indices before appending finer columns.
for channel in ('p','n','e'):
    for i in range(96):
        p(f'viz_{channel}_{i}',f'Graph {channel} {i}',0,1,0,live=True,legacy=True)
for channel in ('p','n','e'):
    for i in range(96,PLOT_COLUMNS):
        p(f'viz_{channel}_{i}',f'Graph {channel} {i}',0,1,0,live=True,legacy=True)
ALIASES=[]
FREQ_FINE_MIN=math.floor(math.log2(20/55)*240)
FREQ_FINE_MAX=math.ceil(math.log2(12000/55)*240)
for o in range(9):
    key=f'o{o+1}_freq_fine';target=f'o{o+1}_freq'
    p(key,f'O{o+1} Frequency',FREQ_FINE_MIN,FREQ_FINE_MAX,0,display='int',nudge_pct=10,dynamic_display=True,unit='Hz')
    ALIASES.append((key,target,0))
for original in list(params[:CONTROL_COUNT]):
    key=original['key']
    if key=='length' or key.endswith(('_attack','_decay','_sweep_time')):
        alias=key+'_ms'
        p(alias,original['name'],round(original['min']*1000),round(original['max']*1000),round(original['default']*1000),display='int',nudge_pct=10,unit='ms')
        ALIASES.append((alias,key,1))
    elif key.endswith('_density'):
        alias=key+'_pct';p(alias,original['name'],1,100,100,display='int',nudge_pct=10,unit='%');ALIASES.append((alias,key,2))
    if key.endswith('_freq'):original['legacy']=True
opt('ui_env_focus','Envelope Context',[f'{o}-{mode}' for o in range(9) for mode in ('tone','noise')],live=True,legacy=True)
p('graph_revision','Graph Refresh',0,1023,1,live=True,legacy=True,display='int')
# Distinct continuous touch handles share physical storage with fine controls.
FINE_ALIASES=list(ALIASES)
TOUCH_BINDINGS={}
for fine,target,kind in FINE_ALIASES:
    original=next(x for x in params[:CONTROL_COUNT] if x['key']==target)
    key=target+'_touch'
    default=math.log(original['default']/20)/math.log(600) if kind==0 else (original['default']-original['min'])/(original['max']-original['min'])
    p(key,original['name']+' Touch',0,1,default,legacy=True,dynamic_display=True,unit='Hz' if kind==0 else 'ms' if kind==1 else '%')
    ALIASES.append((key,target,kind+3));TOUCH_BINDINGS[fine]=key


def generate():
    (ROOT/'params.json').write_text(json.dumps(dict(name='Geonkick MPC',params=params),indent=2)+'\n')
    # Engine uses this independent, plain table rather than wrapper-private types.
    h=['/* Generated by tools/spec.py. Parameter indices are frozen after 0.0.1. */',
       'typedef struct { const char *key; float lo,hi,def; int discrete; } GParam;',
       'static const GParam gparams[] = {']
    for x in params[:CONTROL_COUNT]:
        h.append('    {"%s", %sf, %sf, %sf, %d},'%(x['key'],float(x['min']),float(x['max']),float(x['default']),bool(x.get('options'))))
    h+=['};',f'#define GP_COUNT {CONTROL_COUNT}',f'#define PLOT_COLUMNS {PLOT_COLUMNS}',
        f'#define PLOT_WAVE_LEVELS {PLOT_WAVE_LEVELS}',f'#define PLOT_ENV_LEVELS {PLOT_ENV_LEVELS}']
    h+=['typedef struct { const char *key,*target; int kind; float lo,hi; } GAlias;',
        'static const GAlias galiases[] = {']
    for key,target,kind in ALIASES:
        original=next(x for x in params[:CONTROL_COUNT] if x['key']==target)
        h.append('    {"%s", "%s", %d, %sf, %sf},'%(key,target,kind,float(original['min']),float(original['max'])))
    h += ['};',f'#define G_ALIAS_COUNT {len(ALIASES)}',f'#define FREQ_FINE_MIN {FREQ_FINE_MIN}',f'#define FREQ_FINE_MAX {FREQ_FINE_MAX}']
    (ROOT/'src/param_defs.h').write_text('\n'.join(h)+'\n')
    theme='''style=dark
font_label=art/DejaVuSans.ttf
font_title=art/DejaVuSans.ttf
theme_bg=343637
theme_accent=91B958
theme_accent_hi=B5D979
theme_ink=EDF4F4
theme_ink_dim=A8BFC4
theme_knob_face=484A4C
theme_knob_ring=65696B
theme_knob_dot=B5D979
theme_seg_active=91B958
theme_seg_active_tx=16252A
theme_seg_inactive=34434C
theme_lcd=101B22
theme_line=526A74
theme_btn_bg=34434C
theme_btn_on=91B958
theme_btn_text=EDF4F4
qlink_bounds=column
'''
    lines=[theme,'[tab SYNTH]', 'frame x=24 y=104 w=1230 h=590 title="GEONKICK MPC / SYNTH"']
    def knob(k,label,x,y,b=''):
        lines.append(f'knob cx={x} cy={y} r=36 bw=212 ns=17 vs=22 key={k} label="{label}"'+b)
    lines += ['enum_h cx=526 cy=190 sw=86 sh=36 label="ENVELOPE FOCUS: LAYER / OSCILLATOR" key=plot_osc',
              'enum_h cx=1126 cy=175 sw=105 sh=30 rows=2 label="ENVELOPE" key=plot_env',
              'frame x=70 y=226 w=920 h=202 title=""']
    # Native read-only plot components are added by finish_skin.py so the port
    # can use its own shared display strips without a Chromium build dependency.
    lines.append('knob cx=1126 cy=335 r=30 bw=180 ns=16 vs=20 label="DURATION" key=length')
    lines.append('knob cx=1126 cy=615 r=24 bw=150 ns=14 vs=18 label="OUTPUT" key=volume')
    for o in range(9):
        k=f'o{o+1}_';when=f' when=plot_osc:{o}'
        for suffix,label,x in [('attack','ATTACK',178),('decay','DECAY',476),('sweep','PITCH DROP',774),('sweep_time','PITCH TIME',1072)]:
            condition=when if suffix in ('attack','decay') else f' when=ui_env_focus:{o*2}'
            lines.append(f'slider_h cx={x} cy=440 w=210 h=16 ns=13 vs=17 label="{label}" key={k}{suffix}'+condition)
        lines.append(f'slider_h cx=774 cy=440 w=210 h=16 ns=13 vs=17 label="NOISE DENSITY" key={k}density when=ui_env_focus:{o*2+1}')
    for l in range(3):
        bank=f'LAYER {l+1}';b=f' banks="{bank}"'
        lines.append(f'slider_h cx=1126 cy=513 w=180 h=16 ns=12 vs=16 label="LAYER {l+1} GAIN" key=layer{l+1}'+b)
        for o in range(3):
            ix=l*3+o+1;k=f'o{ix}_';x=o*300
            lines.append(f'enum_h cx={92+x} cy=538 sw=34 sh=30 label="OSC {o+1}" key={k}on'+b)
            lines.append(f'enum_h cx={252+x} cy=538 sw=52 sh=30 rows=2 label="" key={k}wave options="SIN,SQR,TRI,SAW,WHT,BRN"'+b)
            lines.append(f'knob cx={124+x} cy=615 r=24 bw=130 ns=14 vs=18 label="FREQUENCY" key={k}freq'+b)
            lines.append(f'knob cx={260+x} cy=615 r=24 bw=130 ns=14 vs=18 label="LEVEL" key={k}level'+b)
        first=l*3+1
        lines.append(f'qlinks "{bank}" = o{first}_freq,o{first}_level,-,-,o{first+1}_freq,o{first+1}_level,-,-,o{first+2}_freq,o{first+2}_level,-,-,length,volume,-,-')
    # Keep mix/tuning/playback controls as a subpage, with the graph still visible.
    # Their positions replace the lower oscillator strip only on this subpage.
    for i,(key,label) in enumerate([('layer1','LAYER 1'),('layer2','LAYER 2'),('layer3','LAYER 3'),('tune','TUNE')]):
        lines.append(f'knob cx={178+i*298} cy=545 r=28 bw=210 ns=15 vs=19 key={key} label="{label}" banks="MIX"')
    # Output's shared bottom placement collides with MIX's tuning, so it is visible
    # only on the oscillator banks. DURATION stays available on every subpage.
    lines=[line+' banks="LAYER 1|LAYER 2|LAYER 3"' if 'key=volume' in line else line for line in lines]
    lines += ['enum_h cx=476 cy=672 sw=75 sh=25 label="" key=keytrack banks="MIX"',
              'enum_h cx=774 cy=672 sw=70 sh=25 label="" key=playback banks="MIX"',
              'enum_h cx=1072 cy=672 sw=75 sh=25 label="" key=audition banks="MIX"',
              'qlinks "MIX" = layer1,-,-,-,layer2,keytrack,-,-,layer3,playback,-,-,tune,audition,length,-']
    for l in range(3):
        lines += [f'\n[tab LAYER {l+1}]']
        for o in range(3):
            ix=l*3+o+1;k=f'o{ix}_';bank=f'L{l+1} OSC{o+1}';b=f' banks="{bank}"'
            lines.append(f'frame x=24 y=104 w=1230 h=590 title="LAYER {l+1} / OSCILLATOR {o+1}"'+b)
            lines.append(f'enum_h cx=178 cy=220 sw=90 sh=34 label="OSCILLATOR" key={k}on'+b)
            knob(k+'attack','AMP ATTACK',476,205,b)
            knob(f'layer{l+1}','LAYER LEVEL',774,205,b)
            lines.append(f'enum_h cx=1072 cy=205 sw=90 sh=32 rows=3 label="WAVE" key={k}wave'+b)
            controls=[('freq','FREQUENCY'),('level','LEVEL'),('decay','AMP DECAY'),('sweep','PITCH DROP'),('sweep_time','PITCH TIME'),('phase','PHASE'),('density','NOISE DENSITY'),('fm','FM TO NEXT OSC')]
            for i,(s,label) in enumerate(controls):
                x,y=178+(i%4)*298,370+(i//4)*200
                if s=='fm':
                    if o==0:knob(k+s,'FM -> OSC 2',x,y,b+f' when=o{ix+1}_on:1')
                elif s=='density':
                    for mode in (4,5):knob(k+s,label,178,370,b+f' when={k}wave:{mode}')
                elif s in ('freq','sweep','sweep_time','phase'):
                    for mode in range(4):knob(k+s,label,x,y,b+f' when={k}wave:{mode}')
                else:knob(k+s,label,x,y,b)
            lines.append(f'qlinks "{bank}" = {k}freq,{k}level,-,-,{k}attack,{k}decay,-,-,layer{l+1},{k}wave,-,-,{k}on,-,-,-')
        for o in range(3):
            ix=l*3+o+1;k=f'o{ix}_';bank=f'L{l+1} FX{o+1}';b=f' banks="{bank}"'
            lines += [f'frame x=24 y=104 w=1230 h=590 title="LAYER {l+1} / OSCILLATOR {o+1} EFFECTS"'+b,
                      f'enum_h cx=178 cy=235 sw=88 sh=34 label="OSC FILTER" key={k}filter_on'+b,
                      f'enum_h cx=476 cy=235 sw=90 sh=34 rows=3 label="FILTER TYPE" key={k}filter_type'+b,
                      f'enum_h cx=178 cy=470 sw=88 sh=34 label="OSC DRIVE" key={k}drive_on'+b,
                      f'enum_h cx=476 cy=470 sw=78 sh=34 rows=3 label="DRIVE TYPE" key={k}drive_type'+b]
            for suffix,label,x,y in [('cutoff','CUTOFF',774,235),('q','RESONANCE',1072,235),('drive','DRIVE',774,470),('drive_out','DRIVE OUTPUT',1072,470)]:knob(k+suffix,label,x,y,b)
            lines.append(f'qlinks "{bank}" = {k}filter_on,{k}drive_on,-,-,{k}filter_type,{k}drive_type,-,-,{k}cutoff,{k}drive,-,-,{k}q,{k}drive_out,-,-')
    lines += ['\n[tab EFFECTS]','frame x=24 y=104 w=1230 h=590 title="GEONKICK / FILTER + DRIVE"',
              'enum_h cx=178 cy=235 sw=88 sh=34 label="FILTER" key=filter_on',
              'enum_h cx=476 cy=235 sw=90 sh=34 rows=3 label="FILTER TYPE" key=filter_type',
              'enum_h cx=178 cy=470 sw=88 sh=34 label="DRIVE" key=drive_on',
              'enum_h cx=476 cy=470 sw=78 sh=34 rows=3 label="DRIVE TYPE" key=drive_type']
    for k,label,x,y in [('cutoff','CUTOFF',774,235),('q','RESONANCE',1072,235),('drive','DRIVE',774,470),('drive_out','DRIVE OUTPUT',1072,470)]:knob(k,label,x,y)
    lines += ['qlinks "EFFECTS" = filter_on,drive_on,-,-,filter_type,drive_type,-,-,cutoff,drive,-,-,q,drive_out,-,-',
              '\n[tab PRESETS]','frame x=24 y=104 w=1230 h=590 title="GEONKICK / STARTING POINTS"',
              'enum_h cx=640 cy=405 sw=470 sh=64 rows=4 label="PRESETS" key=preset',
              'toggle cx=1090 cy=672 bw=210 label="RESET INIT" key=reset',
              'qlinks "PRESETS" = preset,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-']
    # The physical Hz/seconds controls remain for state/old automation. Visible
    # controls use appended fine pitch and millisecond adapters, never reindexing.
    import re
    replacements={target:key for key,target,kind in ALIASES if kind<3}
    def replace_token(match):return replacements.get(match.group(),match.group())
    lines=[re.sub(r'\b(?:length|o\d+_(?:freq|attack|decay|sweep_time|density))\b',replace_token,line) for line in lines]
    lines=[line.replace('label="FREQUENCY"','label="FREQUENCY (Hz)"').replace('label="DURATION"','label="LENGTH (ms)"')
           .replace('label="AMP ATTACK"','label="ATTACK (ms)"').replace('label="AMP DECAY"','label="DECAY (ms)"')
           .replace('label="ATTACK"','label="ATTACK (ms)"').replace('label="DECAY"','label="DECAY (ms)"')
           .replace('label="PITCH TIME"','label="PITCH TIME (ms)"').replace('label="PITCH DROP"','label="PITCH DROP (st)"') for line in lines]
    lines=[line.replace('label="NOISE DENSITY"','label="NOISE DENSITY (%)"') for line in lines]
    (ROOT/'layout.conf').write_text('\n'.join(lines)+'\n')

if __name__=='__main__':generate()
