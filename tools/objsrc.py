#!/usr/bin/env python3
"""objsrc.py <module dir> <object> : print an object's decompiled code compactly
(entry point + every m_ function it reaches), for porting by hand."""
import os, re, sys
d, name = sys.argv[1], sys.argv[2]
skip = set(sys.argv[3:])
def body(f):
    p = os.path.join(d, f + '.c')
    if not os.path.exists(p): return None
    out = []
    for l in open(p).read().splitlines():
        s = l.strip()
        if not s or s.startswith('/*'): continue
        if re.match(r'^a->frames\[[0-9a-fx]+\] = spr_', s): continue
        if re.match(r'^(int[124]|uint[124]|xunknown[124]|char|bool|code|actor_t|gds|mario_t|room_t)\s+\**[a-zA-Z_]\w*(\s*\[\w+\])?;$', s): continue
        out.append(l.replace('    ', ' '))
    return '\n'.join(out)
todo, done = [name], []
# helpers whose meaning is known (rnd, door cell, drops_out, abs, mul, div)
def helper(b):
    if '0x20020' in b: return 'RND (rnd: x<<1|top, ^0x20020)'
    if 'steps_b[0xc]' in b and 'in_stack' in b: return 'DOOR_CELL_AT(floor, x pushed on stack)'
    if "case 1:\n case 4:\n case 7:" in b or ('case 0:' in b and "'\\t'" in b): return 'DROPS_OUT(hotel, stage)'
    if re.search(r'if \(\(int4\)a < 0\)', b) and b.count('\n') < 10: return 'ABS'
    if 'CONCAT22' in b and '0xffff' in b and b.count('\n') < 14: return 'MUL32'
    return None
while todo:
    f = todo.pop(0)
    if f in done or f in skip: continue
    b = body(f)
    if b is None: continue
    done.append(f)
    h = helper(b)
    if h:
        print('// ----', f, '=', h)
        continue
    print('// ----', f)
    print(b)
    for m in re.findall(r'\b(m_[0-9a-f]{5})\b', b):
        if m not in done and m not in todo:
            todo.append(m)
