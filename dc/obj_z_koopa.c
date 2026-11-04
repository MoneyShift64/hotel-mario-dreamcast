/* obj_z_koopa.c - "z_koopa" (the flying koopa of hotel 7), from L7_s07_sub.o.
 * The same code as f_koopa (obj_f_koopa.c) but for: 500 points instead of 400, a
 * different swoop (0x415abff, ends when x_max reaches 1), turning in mid-air doesn't
 * move it up or down, and it doesn't come back once the stage is over (unk_53a).
 */
#define FK_VARIANT 1
#include "obj_f_koopa.c"
