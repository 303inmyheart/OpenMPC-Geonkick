"""Small shared native display strips, not a pre-rendered screenshot of a sound."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
for kind,height in [('positive',96),('negative',96),('envelope',192)]:
    im=Image.new('RGBA',(9,height*64),(0,0,0,0));d=ImageDraw.Draw(im)
    for frame in range(64):
        offset=frame*height
        if kind=='envelope':
            y=offset+round((1-frame/63)*(height-3))
            d.rectangle((0,y,8,y+2),fill='#D5D7D7')
        elif frame:
            length=max(1,round(frame/63*height))
            top=height-length if kind=='positive' else 0
            d.rectangle((0,offset+top,8,offset+top+length-1),fill='#83B33E')
    im.save(root/'art'/f'plot_{kind}.png')
grid=Image.new('RGB',(920,220),'#252728');d=ImageDraw.Draw(grid)
f=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),10)
d.text((30,8),'GREEN: RENDERED HIT',font=f,fill='#91B958')
d.text((390,8),'GRAY: SELECTED ENVELOPE / 0-1',font=f,fill='#D5D7D7')
d.text((760,8),'0 -> DURATION',font=f,fill='#B2B5B5')
for i in range(5):
    y=23+round(i*191/4);d.line((30,y,894,y),fill='#474A4B',width=1)
for i in range(9):
    x=30+i*108;d.line((x,23,x,214),fill='#3D4041',width=1)
d.line((30,119,894,119),fill='#74797B',width=1)
grid.save(root/'art/plot_grid.png')
