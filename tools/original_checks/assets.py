from support import *
cache={};checked=0;bad=[]
def asset(name):
 if name in cache:return cache[name]
 d=(root/f'dc/data/obj_{name}.tex').read_bytes();w,h,_,rows=struct.unpack_from('<4H',d,8);raw=d[16:16+w*h];m=min(w,h);sp=[A.spread(i) for i in range(m)];lm=m.bit_length()-1;pixels=bytearray(w*h)
 for y in range(h):
  for x in range(w):pixels[y*w+x]=raw[(sp[y&(m-1)]|(sp[x&(m-1)]<<1))+(((x>>lm)if w>h else(y>>lm))<<(2*lm))]
 d=(root/f'dc/data/obj_{name}.frm').read_bytes();nf,ns=struct.unpack_from('<2H',d,4);fr=[struct.unpack_from('<4H2h',d,8+i*12)for i in range(nf)];slots=struct.unpack_from('<'+str(ns)+'H',d,8+nf*12);cache[name]=pixels,w,fr,slots;return cache[name]
for h,st,u in [(1,9,0),(2,9,0),(3,9,0),(4,9,0),(5,9,0),(7,9,0),(7,9,1),(8,14,0),(8,14,1)]:
 mod,_=A.stage_record(str(extracted / f'L{h}_dat.rtf.raw'),st,bool(u))
 with tempfile.TemporaryDirectory()as tmp:
  for name,offs in A.object_frames(mod).items():
   if not offs:bad.append((h,u,name,'no frame map'));continue
   actual=D.run_frames(str(build/"sprite_runner"),mod,offs,tmp)
   pixels,pitch,frames,slots=asset(name)
   for slot,o in enumerate(offs):
    if slot==0:continue
    checked+=1
    ow,oh,ox,oy,op=actual[o];x,y,w,hh,ax,ay=frames[slots[slot]]
    if (ow,oh,ox,oy)!=(w,hh,ax,ay):bad.append((h,u,name,slot,'bounds'));continue
    expected=bytes((p&127)if p!=65535 else 255 for p in op)
    got=b''.join(pixels[(y+j)*pitch+x:(y+j)*pitch+x+w]for j in range(hh))
    if got!=expected:bad.append((h,u,name,slot,'pixels'))
print('Compared',checked,'original sprite slots against port atlases.')
print('Differences:',bad)
if bad:sys.exit(1)
