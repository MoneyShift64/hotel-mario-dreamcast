"""Check the build gate rejects real calls, not game terminology or comments."""
import pathlib
import tempfile
import unittest
from check_sh4zam import violations

class CheckSH4ZAM(unittest.TestCase):
    def scan(self, text):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / 'sample.c'
            p.write_text(text)
            return violations(p)

    def test_calls_on_mixed_and_multiline_lines(self):
        self.assertEqual(len(self.scan('x = shz_sinf(a) + cosf (a);\nx = sqrtf\n(a);')), 2)

    def test_both_header_styles(self):
        self.assertEqual(len(self.scan('#include <math.h>\n#include "math.h"')), 2)

    def test_comments_strings_and_floor_variable(self):
        self.assertEqual(self.scan('int floor = 0; /* sqrtf(x) */\n// sinf(a)\nputs("cosf(x)");\nx = shz_sqrtf(x);'), [])

    def test_extended_math_family(self):
        self.assertEqual(len(self.scan('x = hypotf(a,b);\nx = log1pf(a);\nx = lroundf(a);')), 3)

if __name__ == '__main__':
    unittest.main()
