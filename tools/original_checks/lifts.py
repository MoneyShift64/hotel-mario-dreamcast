from support import *
r=root
syms={x['name']:int(x['address'],16)for x in csv.DictReader((r/'work/symbols.csv').open())};count=0
for h in [0,1,2,3,4,5,7,8]:
 for st in range(15 if h==8 else 10):
  for upper in range(2 if h>=7 else 1):
   n.native_init(str(r/'dc/data').encode(),h,st,upper)
   if not n.native_lift_mode():continue
   mod,rest=A.stage_record(str(extracted / f'L{h}_dat.rtf.raw'),st,bool(upper));(build/'oracle-module.o').write_bytes(mod);(build/'oracle-rest.bin').write_bytes(rest)
   assert o.oracle_init(str(build/'oracle-module.o').encode(),str(build/'oracle-rest.bin').encode(),str(r/'work/hm_image.bin').encode())>0
   o.oracle_setup(h,st)
   for fn in ['dm','ddm','ds_dispatch','dc_wrli','dc_wrfct']:o.oracle_patch(syms[fn])
   o.oracle_lift_base(upper)
   assert o.oracle_main_call(syms['elev_switch'],0x600000,1)
   for layout in [1,0]:
    assert o.oracle_main_call(syms['do_switch'],0x600000,0)
    for i in range(35):
     a=o.oracle_cell(i);b=n.native_lift_grid(layout,i)
     if a!=b:print('DIFF',h,st,upper,layout,i,hex(a),hex(b));sys.exit(1)
   count+=1
print('Original/native lift layouts match:',count,'stages, both layouts.')
