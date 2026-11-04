#!/usr/bin/env python3
"""Compare native boss/lift states and sprite assets to original 68000 code.
Requires extracted original disc data, a generated Musashi checkout, and a C compiler.
OS I/O and drawing are stubbed in the oracle; this is not whole-game emulation.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('extracted', type=Path)
parser.add_argument('--musashi', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
checks = root / 'tools/original_checks'
build = root / 'dc/build/original'
build.mkdir(parents=True, exist_ok=True)
cc = os.environ.get('CC', 'cc')
m = args.musashi.resolve()
msrc = [str(m / p) for p in ['m68kcpu.c', 'm68kops.c', 'm68kdasm.c', 'softfloat/softfloat.c']]
def run(command):
    subprocess.run(list(map(str, command)), cwd=root, check=True)
run([cc, '-O2', '-fPIC', '-shared', '-I'+str(m), checks/'boss_oracle.c', *msrc, '-lm', '-o', build/'boss_oracle.dylib'])
run([cc, '-O2', '-I'+str(m), root/'tools/sprite_runner.c', *msrc, '-lm', '-o', build/'sprite_runner'])
sources = [root/'dc'/p for p in ['lifts.c', 'stage_features.c', 'secrets.c', 'world.c', 'mario.c', 'doors.c', 'fx.c']]
sources += sorted((root/'dc').glob('obj_*.c'))
run([cc, '-O1', '-fPIC', '-shared', '-Idc', '-Isrc', root/'dc/pc_test/native_boss_trace.c', *sources, '-o', build/'native_boss.dylib'])
for script in ['bosses.py', 'lifts.py', 'assets.py']:
    run([sys.executable, checks/script, args.extracted.resolve(), build])
