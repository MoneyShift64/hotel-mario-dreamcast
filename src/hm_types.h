/* hm_types.h - type names used by the raw Ghidra output in src/.
 *
 * The decompiler names types by size: int1/int2/int4 (signed), uint1/2/4,
 * and xunknownN where it could not tell signed from unsigned. CD-i (68000) is
 * big-endian with 32-bit int and pointers; the Dreamcast (SH-4) is little-endian
 * with 32-bit int and pointers, so sizes carry straight over but any data read
 * from the disc must be byte-swapped when it is loaded.
 */
#ifndef HM_TYPES_H
#define HM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef int8_t   int1;
typedef int16_t  int2;
typedef uint8_t  uint1;
typedef uint16_t uint2;
#ifdef HM_TEST_LONG32            /* (test builds: make int32_t-vs-int mix-ups visible on a PC) */
typedef long          int4;
typedef unsigned long uint4;
#else
typedef int32_t  int4;
typedef uint32_t uint4;
#endif
#ifndef _arch_dreamcast          /* (KallistiOS has its own int8/uint8, meaning 8-bit) */
typedef int64_t  int8;
typedef uint64_t uint8;
#endif
typedef uint8_t  xunknown1;
typedef uint16_t xunknown2;
typedef uint32_t xunknown4;
typedef uint64_t xunknown8;
typedef int      code();          /* object handlers: int f(actor_t *, gds *) */

/* Ghidra's helpers for values built from register pairs */
#define CONCAT44(hi, lo)  (((uint64_t)(uint32_t)(hi) << 32) | (uint32_t)(lo))
#define CONCAT22(hi, lo)  (((uint32_t)(uint16_t)(hi) << 16) | (uint16_t)(lo))
#define CONCAT11(hi, lo)  (((uint16_t)(uint8_t)(hi) << 8) | (uint8_t)(lo))

#endif
