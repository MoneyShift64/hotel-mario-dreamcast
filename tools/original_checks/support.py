"""Arguments and shared libraries supplied by check_original.py."""
import csv
import ctypes as C
import pathlib
import struct
import sys
import tempfile
root = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'tools'))
import make_dc_assets as A
import dump_sprites as D
extracted = pathlib.Path(sys.argv[1]).resolve()
build = pathlib.Path(sys.argv[2]).resolve()
o = C.CDLL(str(build / 'boss_oracle.dylib'))
n = C.CDLL(str(build / 'native_boss.dylib'))
o.oracle_init.argtypes = [C.c_char_p] * 3
n.native_init.argtypes = [C.c_char_p, C.c_int, C.c_int, C.c_int]
