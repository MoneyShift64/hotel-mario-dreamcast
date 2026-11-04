#!/usr/bin/env python3
"""Semantic guard for the missing hotel-one background reported on hardware.
Uses original map entries + an independent readback of the generated HMV1 chain.
Run with the Pillow/numpy conversion Python and the local extracted-disc directory.
"""
from pathlib import Path
import struct,sys,zlib
import numpy as np
from PIL import Image,ImageDraw
from make_movie_assets import Disc,DEST,SIZE,animation
from make_frontend_assets import decode_rl7
src=Path(sys.argv[1]);out=Path(sys.argv[2]);preview=Path(sys.argv[3])
d=Disc(src,'L1_am');bg,pal=d.pictures(1)[0]
with (out/'movie_entry_1.hmv').open('rb') as f:
 h=f.read(16);count,duration,size=struct.unpack('<III',h[4:]);previous=np.zeros(SIZE,dtype=np.uint8)
 for i in range(count):
  pts,n=struct.unpack('<II',f.read(8));packet=zlib.decompress(f.read(n))
  previous^=np.frombuffer(packet[768:],dtype=np.uint8)
  if i==0:first=previous.copy();firstpal=packet[:768]
 lastpal=packet[:768]
# Untwiddle without calling the converter's compositing code.
def pixels(tx):
 top=tx[:512*256][DEST[0]].reshape(256,512)[:,:384]
 bottom=tx[512*256:][DEST[1]].reshape(32,512)[:24,:384]
 return np.concatenate((top,bottom))
last=pixels(previous);mask=last>=128
assert mask.sum()>50000
assert np.array_equal(last[mask]-128,bg[mask]),'Final shot is missing the zoomed hotel background'
assert lastpal[384:]==pal,'Final background palette is wrong'
# The first image comes from cd_clut(animation), not the last background entry.
secs=list(d.sectors(3));video=[p for _,sm,_,p in secs if sm&2]
first_size=d.sizes(3,d.vt,3,len(video))[0]
front=np.frombuffer(decode_rl7(b''.join(video[:first_size])),dtype=np.uint8).reshape(280,384)
back,bpal=d.pictures(0)[0];expected=np.where(front!=0,front,back.astype(np.uint16)+128).astype(np.uint8)
assert np.array_equal(pixels(first),expected),'Wrong first movie image'
assert firstpal[384:]==bpal
im=Image.fromarray(last);im.putpalette(lastpal);im.save(preview/'hotel1-corrected-final-shot.png')
# Every active trigger must select a real decoded background; no silent clamp.
for stem,bg_record,tail,audio,anim in [('intro',1,2,3,4),('exit',3,4,5,6)]+[(f'L{h}_am',b,t,a,n) for h in (1,2,3,4,5,7,8) for b,t,a,n in ((0,1,2,3),(11,12,13,14))]:
 disc=Disc(src,stem);n_bg=len(disc.pictures(bg_record))+len(disc.pictures(tail))
 triggers=sum(bool(sm&16)for _,sm,_,_ in disc.sectors(anim))
 assert triggers<n_bg,(stem,bg_record,triggers,n_bg)
 for _ in animation(disc,bg_record,tail,audio,anim):pass
 print(stem,bg_record,':',n_bg,'backgrounds,',triggers,'triggered cuts, all in range')
print('Semantic movie checks passed: correct first foreground and final hotel-one background; all source movie cuts resolve.')
