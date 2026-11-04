/* scene.c - run a stage with a scripted pad and print what is on screen at chosen fields,
 * for render.py (a picture check without a Dreamcast).
 *   scene <data dir> <H_SS> <fields to show, comma list> <script: n:keys ...>
 *   keys: L R U D A(button 1) B(button 2) -(nothing)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
static uint8_t *load(const char *p) { FILE *f = fopen(p, "rb"); long n; uint8_t *d; if (!f) return 0;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET); d = malloc(n); if (fread(d, 1, n, f)) {} fclose(f); return d; }
int main(int argc, char **argv) {
    static world w; char p[512]; int shows[64], ns = 0, fld = 0;
    char *list = strdup(argv[3]), *tok;
    for (tok = strtok(list, ","); tok && ns < 64; tok = strtok(0, ",")) shows[ns++] = atoi(tok);
    snprintf(p, sizeof p, "%s/stage_%s.inf", argv[1], argv[2]); uint8_t *inf = load(p);
    snprintf(p, sizeof p, "%s/room_%s.room", argv[1], argv[2]); uint8_t *room = load(p);
    world_start(&w, inf, room, argv[2][0] - '0', atoi(argv[2] + 2) - 1, 0);
    if (getenv("HM_POWER")) w.g.player[0].power = atoi(getenv("HM_POWER"));   /* (start with a power-up) */
    for (int k = 4; k < argc; k++) {
        int n = atoi(argv[k]); char *c = strchr(argv[k], ':'); uint32_t h = 0;
        for (c = c ? c + 1 : ""; *c; c++) {
            if (*c == 'L') h |= IN_LEFT; if (*c == 'R') h |= IN_RIGHT; if (*c == 'U') h |= IN_UP;
            if (*c == 'D') h |= IN_DOWN; if (*c == 'A') h |= IN_B1; if (*c == 'B') h |= IN_B2;
        }
        for (int i = 0; i < n; i++) {
            world_frame(&w, h);
            fld++;
            for (int s = 0; s < ns; s++) if (shows[s] == fld) {
                printf("FIELD %d group %d state %d power %d\n", fld, w.mario.state_group, w.mario.state, w.g.player[0].power);
                for (int f = 0; f < FLOORS; f++) for (int c2 = 0; c2 < COLS; c2++) {
                    int sx, sy;
                    if (w.tile[f][c2].on) printf("T %d %d %d %d\n", hm_door_x[c2], hm_floor_y[f] - 31, w.tile[f][c2].sx, w.tile[f][c2].sy);
                    else if (hm_doors_tile(&w.doors, f, c2, &sx, &sy)) printf("T %d %d %d %d\n", hm_door_x[c2], hm_floor_y[f] - 31, sx, sy);
                }
                if (w.msp.vis) printf("M %d %d %d\n", w.msp.slot, w.msp.x, w.msp.y);
                if (w.sparkle.slot >= 0) printf("M %d %d %d\n", w.sparkle.slot, w.sparkle.x, w.sparkle.y);
                { world_sprite hud[40]; int nh = world_hud(&w, hud, 40);
                  for (int s2 = 0; s2 < nh; s2++) printf("H %d %d %d\n", hud[s2].frame, hud[s2].x, hud[s2].y); }
                for (int s2 = 0; s2 < w.n_fx; s2++) printf("F %d %d %d\n", w.fx[s2].frame, w.fx[s2].x, w.fx[s2].y);
                for (int s2 = 0; s2 < w.n_spr; s2++) printf("S %d %d %d %d\n", w.spr[s2].obj, w.spr[s2].frame, w.spr[s2].x, w.spr[s2].y);
            }
        }
    }
    return 0;
}
