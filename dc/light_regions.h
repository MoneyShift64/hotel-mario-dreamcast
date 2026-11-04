#ifndef HM_LIGHT_REGIONS_H
#define HM_LIGHT_REGIONS_H
#include <stdint.h>
#define HM_LIGHT_REGION_MAX 2048
typedef struct {
    uint16_t x, y, w, h;
    uint8_t alpha;
} hm_light_region;
/* Build once when lighting changes; vertically join identical horizontal runs. */
int light_regions_build(int (*light)(void *, int, int), void *context, hm_light_region *out,
                        int capacity);
#endif
