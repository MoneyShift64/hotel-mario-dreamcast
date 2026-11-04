#include "light_regions.h"
#include <string.h>
int light_regions_build(int (*light)(void *, int, int), void *context, hm_light_region *out,
                        int capacity) {
    int previous[384], current[384], nprevious = 0, count = 0;
    if (!light)
        return 0;
    for (int y = 0; y < 280; y++) {
        int ncurrent = 0, left = 0, value = light(context, 0, y);
        for (int x = 1; x <= 384; x++) {
            int next = x < 384 ? light(context, x, y) : -1;
            if (next == value)
                continue;
            if (value < 255) {
                int found = -1;
                unsigned alpha = 255 - (value < 0 ? 0 : value);
                for (int i = 0; i < nprevious; i++) {
                    hm_light_region *r = &out[previous[i]];
                    if (r->x == left && r->w == x - left && r->alpha == alpha) {
                        found = previous[i];
                        r->h++;
                        break;
                    }
                }
                if (found < 0) {
                    if (count >= capacity)
                        return -1;
                    found = count++;
                    out[found] = (hm_light_region){left, y, x - left, 1, alpha};
                }
                current[ncurrent++] = found;
            }
            left = x;
            value = next;
        }
        memcpy(previous, current, ncurrent * sizeof *previous);
        nprevious = ncurrent;
    }
    return count;
}
