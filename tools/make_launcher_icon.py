#!/usr/bin/env python3
"""Draw the original launcher P icon using the menu's blue/cyan window palette."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
S=8
im=Image.new('RGB',(48*S,48*S),(0,0,16))
# Opaque navy window with the title/body gradient and bright cyan border.
for y in range(48*S):
 for x in range(48*S):
  t=y/(48*S-1);u=x/(48*S-1)
  im.putpixel((x,y),(0,round(10+28*(1-t)),round(32+65*(1-t)+10*u)))
d=ImageDraw.Draw(im)
for y in range(S,47*S):
 t=(y-S)/(46*S-1)
 c=(0,round(96+144*(1-t)),round(191+64*(1-t)))
 d.line((S,y,2*S,y),fill=c,width=S)
 d.line((46*S,y,47*S,y),fill=c,width=S)
for x in range(S,47*S):
 t=(x-S)/(46*S-1);c=(0,round(96+144*t),round(191+64*t))
 d.line((x,S,x,2*S),fill=c,width=S)
 d.line((x,46*S,x,47*S),fill=(0,65,136),width=S)
# Restrained title strip and divider echo the actual window frame.
for y in range(3*S,9*S):
 t=(y-3*S)/(6*S)
 d.line((3*S,y,45*S,y),fill=(0,round(65*(1-t)+18*t),round(140*(1-t)+70*t)),width=1)
d.line((3*S,9*S,45*S,9*S),fill=(0,120,192),width=S//2)
# Hand-drawn block capital P; no font or game bitmap is bundled.
# Draw at the supersampled size for clean edges at both SMDH sizes.
pts=[(16,13),(27,13),(31,14),(34,17),(34,24),(31,27),(27,28),(21,28),(21,38),(16,38)]
d.polygon([(x*S,y*S) for x,y in pts],fill=(255,255,255))
d.rounded_rectangle((21*S,18*S,29*S,23*S),radius=S,fill=(0,27,72))
im.resize((48,48),Image.Resampling.LANCZOS).save(ROOT/'icon.png')
im.resize((24,24),Image.Resampling.LANCZOS).save(ROOT/'icon-small.png')
im.save(ROOT/'build/launcher-icon-preview.png')
print('Generated icon.png (48x48) and icon-small.png (24x24)')
