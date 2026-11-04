#!/usr/bin/env python3
"""Reject libm maths calls/includes in the port; leave reference decompilation intact."""
import pathlib
import re
import sys

DEFAULT_ROOT = pathlib.Path(__file__).resolve().parents[1] / 'dc'
# Include double/float/long-double calls; never discard an entire line because
# it also contains a shz_ call. Bare names (e.g. a floor index) are allowed.
FUNCTIONS = ('sin cos tan asin acos atan atan2 sinh cosh tanh asinh acosh atanh '
             'sqrt cbrt hypot pow fabs floor ceil trunc round lround llround nearbyint rint '
             'fmod remainder remquo exp exp2 expm1 log log2 log10 log1p fmin fmax fdim '
             'copysign frexp ldexp modf scalbn scalbln nextafter nexttoward erf erfc tgamma lgamma').split()
BAD = re.compile(r'\b(?:' + '|'.join(FUNCTIONS) + r')(?:f|l)?\s*\(|#\s*include\s*[<"]math\.h[>"]')
# Ignore comments and string/character contents, but keep include directives.
TOKENS = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)

def violations(path):
    source = path.read_text()
    def hide(match):
        prefix = source[source.rfind('\n', 0, match.start()) + 1:match.start()]
        if match.group().startswith('"') and re.match(r'^\s*#\s*include\s*$', prefix):
            return match.group()
        return re.sub(r'[^\n]', ' ', match.group())
    cleaned = TOKENS.sub(hide, source)
    lines = cleaned.splitlines()
    numbers = sorted({cleaned.count('\n', 0, m.start()) + 1 for m in BAD.finditer(cleaned)})
    return [(n, lines[n - 1].strip()) for n in numbers]

def main():
    root = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_ROOT
    paths = sorted(p for p in root.rglob('*') if p.suffix in ('.c', '.h'))
    if not paths:
        print('SH4ZAM check: no C/header files found', file=sys.stderr)
        return 1
    bad = [(p, n, line) for p in paths for n, line in violations(p)]
    for p, n, line in bad:
        print(f'{p}:{n}: use SH4ZAM: {line}', file=sys.stderr)
    print(f'SH4ZAM check: {"FAILED" if bad else "OK"} ({len(paths)} C/header files)')
    return bool(bad)

if __name__ == '__main__':
    sys.exit(main())
