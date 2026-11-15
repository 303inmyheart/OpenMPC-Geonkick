"""Original percussion-lab browser tile; generated art, no external branding."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from skin_design import S,BG,GREEN,AMBER,INK,MUTED,font,box,line,text,save
root=Path(__file__).resolve().parents[1]
im=Image.new('RGB',(270*S,110*S),BG);d=ImageDraw.Draw(im)
box(d,(4,4,265,105),'#16272c','#455f60',10);line(d,[(15,6),(254,6)],GREEN,2)
d.polygon([(x*S,y*S) for x,y in [(17,23),(26,18),(35,23),(35,41),(26,46),(17,41)]],outline=GREEN,width=2*S)
line(d,[(20,35),(23,26),(26,38),(29,29),(32,34)],GREEN,1)
text(d,(45,32),'GEONKICK',24,INK,'lm')
box(d,(214,21,253,43),GREEN,None,4);text(d,(233,32),'MPC',11,'#17251b')
line(d,[(18,63),(42,63),(48,52),(55,76),(62,56),(70,68),(80,60),(91,65),(104,62),(122,64),(252,64)],GREEN,1.6)
text(d,(18,88),'PERCUSSION SYNTHESIZER',10,MUTED,'lm');line(d,[(225,84),(252,84)],AMBER,2)
save(im,root/'art/browser-tile.png')
