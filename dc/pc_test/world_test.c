/* world_test.c - run stages headless on a PC: the ported objects + doors on real data.
 *   cc -fsanitize=address,undefined -I.. -I../../src world_test.c ../world.c ../mario.c ../doors.c ../obj_goomba_coin.c
 *   ./a.out <data dir> [frames]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"

static uint8_t *load(const char *dir, const char *fmt, const char *tag) {
    char p[512], n[64]; FILE *f; long sz; uint8_t *d;
    snprintf(n, sizeof n, fmt, tag); snprintf(p, sizeof p, "%s/%s", dir, n);
    if (!(f = fopen(p, "rb"))) return NULL;
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    d = malloc(sz); if (fread(d, 1, sz, f) != (size_t)sz) {} fclose(f); return d;
}

int main(int argc, char **argv) {
    const char *dir = argv[1]; int frames = argc > 2 ? atoi(argv[2]) : 6000;
    char tags[100][8]; int nt = 0; FILE *f; char line[32];
    static world w; uint32_t seed = 12345;
    snprintf(line, sizeof line, "%s/stages.txt", dir);
    f = fopen(line, "r");
    { char p[512]; snprintf(p, sizeof p, "%s/stages.txt", dir); f = fopen(p, "r"); }
    while (f && fgets(line, sizeof line, f)) { line[strcspn(line, "\r\n")] = 0; if (*line) strcpy(tags[nt++], line); }
    if (f) fclose(f);
    int total_actors = 0;
    for (int t = 0; t < nt; t++) {
        uint8_t *inf = load(dir, "stage_%s.inf", tags[t]), *room = load(dir, "room_%s.room", tags[t]);
        int hotel = tags[t][0] - '0', stage = atoi(tags[t] + 2) - 1;
        world_start(&w, inf, room, hotel, stage, 0);
        int n = w.room.n_actors; total_actors += n;
        int emerged = 0, opened_by_enemy = 0, maxspr = 0, deaths = 0, status = 0, open0 = w.doors.open;
        int coins0 = 0, f2, ends[WORLD_STATUS_COUNT] = {0}, jumps = 0, stomps = 0, rides = 0, hides = 0, lastg = -1;
        uint32_t held = 0;
        for (f2 = 0; f2 < frames; f2++) {
            mario_t *m = &w.mario;
            /* a random player: hold a direction a while, tap the buttons, push up/down */
            if ((f2 % 29) == 0) {
                hm_rnd(&seed); hm_rnd(&seed);
                held = (seed & 1) ? IN_LEFT : IN_RIGHT;
                if ((seed & 6) == 0) held = 0;
                if ((seed & 0x18) == 0x08) held |= IN_UP;
                if ((seed & 0x18) == 0x10) held |= IN_DOWN;
            }
            if ((f2 % 29) < 8) {
                if (seed & 0x20) held |= IN_B1;
                if (seed & 0x40) held |= IN_B2;
            } else held &= ~(uint32_t)(IN_B1 | IN_B2);
            status = world_frame(&w, held);
            if (m->state_group != lastg) {
                if (m->state_group == 3) jumps++;
                if (m->state_group == 4) hides++;
                if ((m->state_group == 1 || m->state_group == 2) && lastg == 0) rides++;
                lastg = m->state_group;
            }
            if (m->hit) { stomps++; m->hit = 0; }
            if (w.msp.vis && (w.msp.slot < 0 || w.msp.slot > 56 || w.msp.x < -40 || w.msp.x > 420 || w.msp.y < -40 || w.msp.y > 300)) {
                printf("%s: odd Mario picture %d at %d,%d (group %d state %d)\n", tags[t], w.msp.slot, w.msp.x, w.msp.y, m->state_group, m->state); return 1;
            }
            if (m->state_group == 0 && m->y != hm_floor_y[m->floor] - 30) {
                printf("%s: Mario off the floor: y %d floor %d (state %d)\n", tags[t], m->y, m->floor, m->state); return 1;
            }
            for (int i = 0; i < n; i++) if (w.room.actors[i].state == 2 && w.room.actors[i].step == 0) emerged++;
            if (w.n_spr > maxspr) maxspr = w.n_spr;
            for (int s = 0; s < w.n_spr; s++) {
                world_sprite *sp = &w.spr[s];
                if (sp->x < -40 || sp->x > 420 || sp->y < -40 || sp->y > 300 || sp->frame < 1 || sp->frame > 64) {
                    printf("%s: odd sprite obj %d frame %d at %d,%d\n", tags[t], sp->obj, sp->frame, sp->x, sp->y); return 1;
                }
            }
            if (status != WORLD_PLAYING) {
                ends[status]++;
                if (status == WORLD_MARIO_DIED) deaths++;
                world_start(&w, inf, room, hotel, stage, 0);    /* play it again */
                lastg = -1;
            }
        }
        (void)opened_by_enemy;
        printf("%-5s jumps %3d stomps %2d rides %3d hides %3d | cleared %d all-open %d died %d time-up %d\n",
               tags[t], jumps, stomps, rides, hides, ends[1], ends[2], ends[3], ends[4]);
        coins0 = w.g.player[0].coins;
        printf("%-5s actors %2d  doors %2d (open at start %d)  max sprites %2d  emerges %4d  deaths %3d  coins %3d score %6d\n",
               tags[t], n, w.doors.count, open0, maxspr, emerged, deaths, coins0, w.g.player[0].score);
        free(inf); free(room);
    }
    printf("total ported actors across stages: %d\n", total_actors);
    return 0;
}
