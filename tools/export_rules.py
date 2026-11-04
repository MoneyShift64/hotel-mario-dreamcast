#!/usr/bin/env python3
"""Export original named rule tables into a native read-only header."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parents[1]
image=(root/'work/hm_image.bin').read_bytes()
tables=[('secret',0x101d9e,100),('light_icf',0x1020a2,9),('dark_icf',0x1020b4,10),('row_start',0x1020c8,6),('puff_time',0x1020d4,40),('ghost_time',0x102124,40),('ghost_count',0x102174,10),('coin_time',0x102188,40)]
s=['/* Original rule tables, exported by tools/export_rules.py. */','#ifndef HM_RULES_H','#define HM_RULES_H','#include <stdint.h>']
for name,addr,n in tables:
 values=struct.unpack_from('>'+str(n)+'H',image,addr)
 s.append(f'static const uint16_t hm_{name}[{n}] = {{'+','.join(hex(v) for v in values)+'};')
s.append('#endif')
(root/'dc/hm_rules.h').write_text('\n'.join(s)+'\n')
