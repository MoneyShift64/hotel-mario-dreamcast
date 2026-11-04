from support import *
fields=[4,6,8,10,12,14]+list(range(0x1a,0x50,2))+[0x254,0x258]
for h,st,upper in [(8,14,1),(1,9,0),(2,9,0),(3,9,0),(4,9,0),(5,9,0),(7,9,0),(8,14,0),(7,9,1)]:
 tag=f'{h}_{st+1:02d}'+('_u' if upper else '')
 mod,rest=A.stage_record(str(extracted/f'L{h}_dat.rtf.raw'),st,bool(upper))
 (build/'oracle-module.o').write_bytes(mod);(build/'oracle-rest.bin').write_bytes(rest)
 for seed,mode in [(seed,mode) for seed in [0,1,12345,0x80000000,0x1234abcd] for mode in [0,1]]:
  count=o.oracle_init(str(build/'oracle-module.o').encode(),str(build/'oracle-rest.bin').encode(),str(root/'work/hm_image.bin').encode());assert count>0,(tag,'init')
  o.oracle_setup(h,st);assert n.native_init(str(root/'dc/data').encode(),h,st,upper)==count
  o.oracle_seed(seed);n.native_seed(seed)
  mismatch=None
  for t in range(800):
   assert o.oracle_tick(t,mode),(tag,'runaway',t)
   n.native_tick(t,mode)
   for i in range(count):
    for off in fields:
     a=o.oracle_actor(i,off);b=n.native_actor(i,off)
     if off>=0x254 and 0x500034<=a<0x500034+24*0x274 and (a-0x500034)%0x274==0:a=(a-0x500034)//0x274+1
     # Morton uses a->frame as the native draw queue slot while his original
     # falling routine draws directly from unk_02a without storing frame.
     if off==0x3a and o.oracle_actor(i,4)==10:continue
     if a!=b:mismatch=(t,i,hex(off),a,b);break
    if mismatch:break
   if mismatch:break
   for off in [0x826,0x530,0x53a]:
    a=o.oracle_gds(off);b=n.native_gds(off)
    if a!=b:mismatch=(t,'gds',hex(off),a,b);break
   if mismatch:break
  print(tag,'seed',hex(seed),'mode',mode,'MATCH' if not mismatch else ('DIFF '+repr(mismatch)+' type='+str(o.oracle_actor(mismatch[1],4)) if isinstance(mismatch[1],int) else 'DIFF '+repr(mismatch)),flush=True)

  if mismatch: raise AssertionError((tag,seed,mode,mismatch))
