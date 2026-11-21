"""Small shared native display strips, not a pre-rendered screenshot of a sound."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from spec import PLOT_WIDTH,PLOT_WAVE_LEVELS,PLOT_ENV_LEVELS
from editor_geometry import CURVE_HEIGHT
root=Path(__file__).resolve().parents[1]
for kind,height in [('positive',96),('negative',96),('envelope',192)]:
    levels=PLOT_ENV_LEVELS if kind=='envelope' else PLOT_WAVE_LEVELS
    assert height*levels<=16384
    im=Image.new('RGBA',(PLOT_WIDTH,height*levels),(0,0,0,0));d=ImageDraw.Draw(im)
    for frame in range(levels):
        offset=frame*height
        if kind=='envelope':
            y=offset+round((1-frame/(levels-1))*(height-2))
            d.rectangle((0,y,PLOT_WIDTH-1,y+1),fill='#ffbc85')
        elif frame:
            length=max(1,round(frame/(levels-1)*height))
            top=height-length if kind=='positive' else 0
            d.rectangle((0,offset+top,PLOT_WIDTH-1,offset+top+length-1),fill='#6de2c3')
    im.save(root/'art'/f'plot_{kind}.png')
grid=Image.new('RGB',(920,220),'#11151d');d=ImageDraw.Draw(grid)
f=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),10)
d.text((30,8),'HIT WAVEFORM',font=f,fill='#6de2c3')
d.text((390,8),'ENVELOPE / 0-1',font=f,fill='#ffbc85')
d.text((760,8),'0 -> DURATION',font=f,fill='#B2B5B5')
for i in range(5):
    y=23+round(i*191/4);d.line((30,y,894,y),fill='#303642',width=1)
for i in range(9):
    x=30+i*108;d.line((x,23,x,214),fill='#252d3b',width=1)
d.line((30,119,894,119),fill='#465267',width=1)
grid.save(root/'art/plot_grid.png')

# The editor shares the existing 432 actual-sound columns. Only the painted
# width/height/colour differs; no second sound or display-data path is added.
for width in (2,3):
    for channel in ('p','n'):
        height=67;im=Image.new('RGBA',(width,height*PLOT_WAVE_LEVELS),(0,0,0,0));draw=ImageDraw.Draw(im)
        for frame in range(1,PLOT_WAVE_LEVELS):
            length=max(1,round(frame/(PLOT_WAVE_LEVELS-1)*height))
            y=frame*height+(height-length if channel=='p' else 0)
            draw.rectangle((0,y,width-1,y+length-1),fill=(83,149,152,180))
        im.save(root/'art'/f'plot_edit_{channel}_{width}.png')

# Full variable envelope over the same waveform, native grip inset matched.
for width in (2,3):
    height=CURVE_HEIGHT;im=Image.new('RGBA',(width,height*PLOT_ENV_LEVELS),(0,0,0,0));d=ImageDraw.Draw(im)
    for frame in range(PLOT_ENV_LEVELS):
        y=frame*height+round((1-frame/(PLOT_ENV_LEVELS-1))*(height-2))
        d.rectangle((0,y,width-1,y+1),fill='#ffbc85')
    im.save(root/'art'/f'plot_edit_e_{width}.png')
