"""Original Sounds browser tile; no desktop or Akai artwork."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
im=Image.new('RGB',(270,110),'#202A30');d=ImageDraw.Draw(im)
font=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),27)
small=ImageFont.truetype(str(root/'art/DejaVuSans.ttf'),13)
d.rounded_rectangle((4,4,265,105),12,outline='#57D7C1',width=2)
d.text((17,19),'GEONKICK',font=font,fill='#9AFFF0')
d.text((19,66),'PERCUSSION / OPENMPC',font=small,fill='#EDF4F4')
im.save(root/'art/browser-tile.png')
