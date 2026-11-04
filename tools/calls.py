#!/usr/bin/env python3
"""calls.py <module.o> <func_off> <end_off> <target_off> : show the 8 instructions before
each PIC call (movea.l #rel,A0; jsr (PC,A0.l)) to target - to see stack-passed arguments."""
import subprocess, sys
mod, a, b, tgt = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16), int(sys.argv[4], 16)
out = subprocess.run(['/home/claude/tools/sprite_runner', mod, 'dis', hex(a), str((b - a) // 2)],
                     capture_output=True, text=True).stdout.splitlines()
for i, l in enumerate(out):
    if 'jsr' in l and '(PC,A0.l)' in l and i > 0 and 'movea.l #$' in out[i - 1]:
        rel = int(out[i - 1].split('#$')[1].split(',')[0], 16)
        if rel >= 0x80000000: rel -= 0x100000000
        pc = int(l.split()[0], 16)
        if pc >= 0x100000: pc -= 0x100000
        if pc + 2 + rel == tgt:
            print('\n'.join(out[max(0, i - 9):i + 1])); print('--')
