/* snap.c - run a stage and print what's on screen at chosen frames (for a picture check) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
static uint8_t *load(const char *p) { FILE *f = fopen(p, "rb"); long n; uint8_t *d; if (!f) return 0;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET); d = malloc(n); if (fread(d, 1, n, f)) {} fclose(f); return d; }
int main(int argc, char **argv) {
    static world w; char p[256];
    snprintf(p, sizeof p, "%s/stage_%s.inf", argv[1], argv[2]); uint8_t *inf = load(p);
    snprintf(p, sizeof p, "%s/room_%s.room", argv[1], argv[2]); uint8_t *room = load(p);
    world_start(&w, inf, room, argv[2][0] - '0', atoi(argv[2] + 2) - 1, 0);
    for (int f = 1; f <= 1500; f++) {
        uint32_t held = (f / 200) % 2 ? IN_LEFT : IN_RIGHT, pressed = 0;
        if (f % 50 == 0) pressed |= IN_B2;
        world_frame(&w, held, pressed);
        if (f % 300 == 0) {
            printf("FRAME %d status %d open %d\n", f, w.status, w.doors.open);
            printf("MARIO %d %d %d %d\n", w.mario.x, w.mario.y, world_mario_slot(&w), world_mario_visible(&w));
            for (int fl = 0; fl < 5; fl++) for (int c = 0; c < 7; c++) { int sx, sy; if (hm_doors_tile(&w.doors, fl, c, &sx, &sy)) printf("DOOR %d %d %d %d\n", fl, c, sx, sy); }
            for (int s = 0; s < w.n_spr; s++) printf("SPR %d %d %d %d\n", w.spr[s].obj, w.spr[s].frame, w.spr[s].x, w.spr[s].y);
        }
    }
}
