#!/usr/bin/env python3
"""Decompile every function of Hotel Mario (CD-i) using Ghidra's decompiler engine.

Usage:  python3 decompile.py work_dir src_dir [function names...]

Inputs (in work_dir, made by build_image.py):
         hm_image.bin  (code at 0x0, game data area at 0x100000, initialised + relocated)
         syms.txt      (decoded from cdi_hotel.stb: value flags name)
         thunks.json   (_jmptbl entries -> target function)
Output : src_dir/<group>/<function>.c  and  src_dir/all_functions.c
         groups: game, streaming (ds_*), cdi_os (CD-i system-call wrappers), lib (C library)

Needs Ghidra's console decompiler (decomp_opt) and the 68000 language files; set
DECOMP to the decomp_opt binary and SLEIGHHOME to the Ghidra source root.
"""
import json, os, re, subprocess, sys

ARGS = sys.argv[1:]
MODULE = None                       # --module file.o : decompile a stage module instead
if '--module' in ARGS:
    k = ARGS.index('--module'); MODULE = ARGS[k + 1]; del ARGS[k:k + 2]
if len(ARGS) < 2:
    sys.exit(__doc__)
HERE = os.path.abspath(ARGS[0])
SRC = os.path.abspath(ARGS[1])
MOD_BASE = 0x200000                 # where a stage module goes in the image
DEC = os.environ.get('DECOMP', '/home/claude/gsrc/Ghidra/Features/Decompiler/src/decompile/cpp/decomp_opt')
ENV = dict(os.environ, SLEIGHHOME=os.environ.get('SLEIGHHOME', '/home/claude/gsrc'))
DBASE, A6 = 0x100000, 0x108000

ents = []
for line in open(os.path.join(HERE, 'syms.txt')).read().splitlines():
    v, fl, n = line.split(' ', 2)
    ents.append((int(v), int(fl, 16), n))

def clean(n):
    return n[3:] if n.startswith('_T$') else n

# code symbols (one name per address; prefer the first, e.g. memcpy over memmove)
funcs = {}
for v, fl, n in ents:
    if fl & 4 and v not in funcs and n not in ('btext', 'bname', 'etext', 'stacksiz'):
        funcs[v] = clean(n)

# data symbols with sizes taken from the gap to the next symbol
data = sorted({(A6 + v, clean(n)) for v, fl, n in ents if not fl & 4})
dsyms = []
for i, (a, n) in enumerate(data):
    nxt = data[i + 1][0] if i + 1 < len(data) else DBASE + 0x2600
    size = nxt - a
    if n in ('end', '_jmptbl'):     # _jmptbl entries are mapped as thunk functions instead
        continue
    if size in (1, 2, 4):
        dsyms.append((a, {1: 'int1', 2: 'int2', 4: 'int4'}[size], n, None))
    elif size > 0:
        dsyms.append((a, 'int1', n, size))

thunks = [(a, n) for a, t, n in json.load(open(os.path.join(HERE, 'thunks.json'))) if t]

def group(addr, name):
    if addr < 0x36a or addr >= 0x178c2 and not name.startswith('jt_'):
        return 'lib'
    if 0x15f70 <= addr < 0x171e8:
        return 'streaming'
    if 0x171e8 <= addr < 0x178c2:
        return 'cdi_os'
    return 'game'

# --- OS-9 system calls: 'trap #0' is followed by a data word naming the call.
# Record each one, and give the decompiler a copy with that word replaced by NOP.
OS9 = dict(enumerate('F$Link F$Load F$UnLink F$Fork F$Wait F$Chain F$Exit F$Mem F$Send F$Icpt F$Sleep '
    'F$SSpd F$ID F$SPrior F$STrap F$PErr F$PrsNam F$CmpNam F$SchBit F$AllBit F$DelBit F$Time F$STime '
    'F$CRC F$GPrDsc F$GBlkMp F$GModDr F$CpyMem F$SUser F$UnLoad F$RTE F$GPrDBT F$Julian F$TLink '
    'F$DFork F$DExec F$DExit F$DatMod F$SetCRC F$SetSys F$SRqMem F$SRtMem F$IRQ F$IOQu F$AProc '
    'F$NProc F$VModul F$FindPD F$AllPD F$RetPD F$SSvc F$IODel'.split()))
OS9.update({0x52: 'F$SysDbg', 0x53: 'F$Event', 0x54: 'F$Gregor', 0x55: 'F$SysID', 0x56: 'F$Alarm',
            0x57: 'F$SigMask', 0x5c: 'F$SRqCMem'})
OS9.update(dict(zip(range(0x80, 0x90), 'I$Attach I$Detach I$Dup I$Create I$Open I$MakDir I$ChgDir '
    'I$Delete I$Seek I$Read I$Write I$ReadLn I$WritLn I$GetStt I$SetStt I$Close'.split())))
img = bytearray(open(os.path.join(HERE, 'hm_image.bin'), 'rb').read())
fstarts = sorted(funcs)
def owner(a):
    o = None
    for f in fstarts:
        if f <= a: o = f
        else: break
    return o
syscalls = {}
for i in range(0x36a, 0x1b940, 2):
    if img[i:i + 2] == b'\x4e\x40':
        code = int.from_bytes(img[i + 2:i + 4], 'big')
        if code < 0x100:
            syscalls.setdefault(owner(i), []).append(OS9.get(code, 'call 0x%02x' % code))
            img[i + 2:i + 4] = b'\x4e\x71'
# --- a stage module: append it, find its functions (LINK A5) and entry points
mod_funcs, mod_entries = {}, {}
targets_mod = []
if MODULE:
    mod = open(MODULE, 'rb').read()
    img += bytes(MOD_BASE - len(img)) + mod
    exe = int.from_bytes(mod[0x30:0x34], 'big')
    o = exe
    while o + 8 <= len(mod):
        no, co = int.from_bytes(mod[o:o + 4], 'big'), int.from_bytes(mod[o + 4:o + 8], 'big')
        if no == 0 or no >= len(mod) or co >= len(mod):
            break
        mod_entries[MOD_BASE + co] = mod[no:mod.index(b'\0', no)].decode('latin1')
        o += 8
    for i in range(0x30, len(mod) - 4, 2):
        if mod[i:i + 2] == b'\x4e\x55':                     # link a5,#n
            mod_funcs[MOD_BASE + i] = 'm_%05x' % i
        if mod[i:i + 4] == b'\x2f\x09\x22\x40':             # compiled sprite frame
            mod_funcs[MOD_BASE + i] = 'spr_%05x' % i
    # Position-independent code: "movea.l #off,An ; jmp/jsr/lea (PC,An.l)". Rewrite each
    # into its plain equivalent in the decompiler's copy (same behaviour):
    #   jmp -> bra.l target, jsr -> bsr.l target, lea -> movea.l #absolute,An
    pic = bytearray(mod)
    for i in range(0x30, len(mod) - 10, 2):
        if mod[i:i + 2] in (b'\x20\x7c', b'\x22\x7c'):
            an = 0 if mod[i + 1] == 0x7c and mod[i] == 0x20 else 1
            op = mod[i + 6:i + 8]
            ext = mod[i + 8]
            if op not in (b'\x4e\xbb', b'\x4e\xfb', b'\x41\xfb', b'\x43\xfb') or (ext >> 4) != (0x8 | an):
                continue
            off = int.from_bytes(mod[i + 2:i + 6], 'big', signed=True)
            t = (i + 8) + off + int.from_bytes(mod[i + 9:i + 10], 'big', signed=True)
            if op in (b'\x4e\xbb', b'\x4e\xfb'):
                disp = t - (i + 2)
                pic[i:i + 10] = (b'\x61\xff' if op == b'\x4e\xbb' else b'\x60\xff') + \
                    disp.to_bytes(4, 'big', signed=True) + b'\x4e\x71\x4e\x71'
                if op == b'\x4e\xbb':
                    mod_funcs.setdefault(MOD_BASE + t, 'm_%05x' % t)
            else:
                pic[i:i + 10] = mod[i:i + 2] + (MOD_BASE + t).to_bytes(4, 'big') + b'\x4e\x71\x4e\x71'
    img[MOD_BASE:MOD_BASE + len(mod)] = pic
    mod_funcs.update(mod_entries)
open(os.path.join(HERE, 'hm_image_decomp.bin'), 'wb').write(img)

cmds = ['load file 68000:BE:32:default ' + os.path.join(HERE, 'hm_image_decomp.bin'),
        'set track A6 0x%x' % A6]
# NB: globals must be mapped before any 'map function' (that changes the current scope)
# --- optional type information: tools/hm_decomp_types.h -------------------
# One declaration per line.  'struct'/'typedef' lines define types; lines of the form
#   //@global <type> <name>      give a global variable (from the .stb) a type
# and every other line containing '(' is a function prototype applied to that function.
TYPES = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'hm_decomp_types.h')
type_decls, protos, gtypes = [], [], {}
if os.path.exists(TYPES):
    for line in open(TYPES).read().splitlines():
        line = line.strip()
        if line.startswith('//@global'):
            _, t, n = line.split(None, 2)
            gtypes[n] = t
        elif not line or line.startswith(('//', '#')):
            continue
        elif line.startswith(('struct', 'typedef', 'union', 'enum')):
            type_decls.append(line)
        elif '(' in line:
            protos.append(line if line.startswith('extern') else 'extern ' + line)
cmds += ['parse line ' + d for d in type_decls]
for a, t, n, arr in dsyms:
    if n in gtypes:
        cmds.append('map address r0x%x %s %s' % (a, gtypes[n], n))
    else:
        cmds.append('map address r0x%x %s %s%s' % (a, t, n, '[%d]' % arr if arr else ''))
for a, n in sorted(funcs.items()):
    cmds.append('map function r0x%x %s' % (a, n))
for a, n in thunks:
    cmds.append('map function r0x%x jt_%s' % (a, n))
cmds += ['parse line ' + pr for pr in protos]
for a, n in sorted(mod_funcs.items()):
    cmds.append('map function r0x%x %s' % (a, n))
for a, n in sorted(mod_funcs.items()):          # object handlers are all f(actor, gds)
    if not n.startswith('spr_'):
        cmds.append('parse line extern int4 %s(actor_t *a,gds *g);' % n)
targets_mod = [(a, n) for a, n in sorted(mod_funcs.items()) if not n.startswith('spr_')]

targets = targets_mod if MODULE else sorted(funcs.items())
if len(ARGS) > 2:
    targets = [(a, n) for a, n in targets if n in ARGS[2:]]
for a, n in targets:
    cmds += ['echo @@@BEGIN %s 0x%x' % (n, a), 'load function %s' % n, 'decompile', 'print C',
             'echo @@@END']
cmds.append('quit')

out = subprocess.run([DEC], input='\n'.join(cmds) + '\n', capture_output=True, text=True, env=ENV).stdout
errs = [l for l in out.splitlines() if l.startswith(('ERROR', 'Low-level', 'Error', 'Execution error', 'Command parsing'))][:30]

# Calls through the jump table are just indirections: show the real target name
STR_RE = re.compile(r'\b0x([0-9a-f]{3,5})\b')
def string_at(a):
    if not (0x36a <= a < 0x1c000):
        return None
    end = img.find(b'\0', a, a + 120)
    t = img[a:end]
    if end < 0 or len(t) < 4 or any(c < 32 or c > 126 for c in t):
        return None
    txt = t.decode('latin1')
    good = sum(ch.isalnum() or ch in " .,!?'_/-:" for ch in txt)
    if good < 0.9 * len(txt) or sum(ch.isalpha() for ch in txt) < 2 or 'NuN' in txt:
        return None
    if a > 0 and 32 <= img[a - 1] <= 126:          # must start a string, not sit inside one
        return None
    return t.decode('latin1')

def label_strings(c):
    def one(mo):
        t = string_at(int(mo.group(1), 16))
        return mo.group(0) + ('/* "%s" */' % t.replace('*/', '* /') if t else '')
    return STR_RE.sub(one, c)

FUNC_ADDR = {a + 2: n for a, n in thunks}          # JMP operand = the function's address
def tidy(c):
    c = label_strings(c)
    c = re.sub(r'\b[a-z]Ram(00[0-9a-f]{6})\b',
               lambda mo: FUNC_ADDR.get(int(mo.group(1), 16), mo.group(0)) if int(mo.group(1), 16) in FUNC_ADDR else mo.group(0), c)
    c = re.sub(r'\(\*_jt_([A-Za-z_][A-Za-z0-9_]*)\)', r'\1', c)
    return re.sub(r'\b_?jt_([A-Za-z_][A-Za-z0-9_]*)\b', r'\1', c)

os.makedirs(os.path.join(SRC), exist_ok=True)
allc = []
ok = 0
for blk in re.findall(r'@@@BEGIN (\S+) (0x[0-9a-f]+)\n(.*?)@@@END', out, re.S):
    n, a, body = blk
    body = re.sub(r'^(\[decomp\]>|@@@).*\n?', '', body, flags=re.M)
    body = re.sub(r'^(Decompiling|Decompilation complete|Function \S+: 0x).*\n?', '', body, flags=re.M)
    body = tidy(body).strip('\n')
    g = os.path.join('modules', os.path.basename(MODULE)[:-2]) if MODULE else group(int(a, 16), n)
    d = os.path.join(SRC, g)
    os.makedirs(d, exist_ok=True)
    sc = syscalls.get(int(a, 16))
    note = '\n/* OS-9 system call(s): %s */' % ', '.join(sc) if sc else ''
    text = '/* %s @ %s  (group: %s) */%s\n%s\n' % (n, a, g, note, body)
    open(os.path.join(d, n + '.c'), 'w').write(text)
    allc.append(text)
    ok += 1 if body.strip() else 0
open(os.path.join(SRC, 'all_functions.c'), 'w').write('\n'.join(allc))
print('functions decompiled:', ok, 'of', len(targets))
if errs:
    print('messages:', *errs, sep='\n  ')
