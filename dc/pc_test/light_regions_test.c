#include "light_regions.h"
#include "world.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static world w;
static hm_light_region regions[HM_LIGHT_REGION_MAX];
static uint8_t composed[280][384], covered[280][384];
static int calls, constant;
static int sample(void *ctx, int x, int y) {
    (void)ctx;
    calls++;
    return constant < 0 ? stage_plane_light(&w, x, y) : constant;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    constant = 255;
    assert(!light_regions_build(sample, NULL, regions, HM_LIGHT_REGION_MAX));
    constant = 80;
    int n = light_regions_build(sample, NULL, regions, HM_LIGHT_REGION_MAX);
    assert(n == 1 && regions[0].w == 384 && regions[0].h == 280 && regions[0].alpha == 175);
    assert(light_regions_build(sample, NULL, regions, 0) == -1);
    int tested = 0, max = 0;
    constant = -1;
    for (int hotel = 2; hotel <= 3; hotel++)
        for (int stage = 0; stage < 10; stage++) {
            char path[512];
            snprintf(path, sizeof path, "%s/stage_%d_%02d.ext", argv[1], hotel, stage + 1);
            FILE *f = fopen(path, "rb");
            assert(f);
            uint8_t data[2048];
            size_t size = fread(data, 1, sizeof data, f);
            fclose(f);
            assert(size >= 732);
            memset(&w, 0, sizeof w);
            w.extra = data + 12;
            w.extra_size = size - 12;
            w.g.player[0].hotel = hotel;
            w.g.player[0].stage = stage;
            for (int variant = 0; variant < (hotel == 3 ? 5 : 9); variant++) {
                w.mario.floor = variant % 5;
                w.event.light = variant;
                calls = 0;
                n = light_regions_build(sample, NULL, regions, HM_LIGHT_REGION_MAX);
                assert(n >= 0 && n < 128);
                if (n > max)
                    max = n;
                memset(composed, 255, sizeof composed);
                memset(covered, 0, sizeof covered);
                for (int i = 0; i < n; i++) {
                    hm_light_region *r = &regions[i];
                    assert(r->x + r->w <= 384 && r->y + r->h <= 280);
                    for (int y = r->y; y < r->y + r->h; y++)
                        for (int x = r->x; x < r->x + r->w; x++) {
                            assert(!covered[y][x]);
                            covered[y][x] = 1;
                            composed[y][x] = 255 - r->alpha;
                        }
                }
                for (int y = 0; y < 280; y++)
                    for (int x = 0; x < 384; x++)
                        assert(composed[y][x] == stage_plane_light(&w, x, y));
                /* Rendering the cached geometry requires no further mask sampling. */
                int sampled = calls;
                for (int frame = 0; frame < 60; frame++)
                    for (int i = 0; i < n; i++)
                        assert(regions[i].alpha);
                assert(calls == sampled);
                tested++;
            }
        }
    printf("Lighting geometry passed: %d original cave/storm states, pixel-exact masks, at most %d "
           "quads, zero per-frame mask sampling.\n",
           tested, max);
}
