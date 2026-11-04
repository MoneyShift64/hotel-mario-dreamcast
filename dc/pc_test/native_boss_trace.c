/* Minimal native counterpart for original-68000 object differential tests. */
#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static world w;
static uint8_t *retained_extra;
static uint8_t *load(const char *dir, const char *stem, const char *tag, const char *ext,
                     unsigned *size) {
    char p[512];
    snprintf(p, sizeof p, "%s/%s_%s.%s", dir, stem, tag, ext);
    FILE *f = fopen(p, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    uint8_t *b = malloc(n);
    if (fread(b, 1, n, f) != (size_t)n)
        abort();
    fclose(f);
    if (size)
        *size = n;
    return b;
}
int native_init(const char *dir, int h, int stage, int upper) {
    char tag[16];
    snprintf(tag, sizeof tag, "%d_%02d%s", h, stage + 1, upper ? "_u" : "");
    unsigned n = 0;
    free(retained_extra);
    retained_extra = NULL;
    uint8_t *inf = load(dir, "stage", tag, "inf", NULL),
            *room = load(dir, "room", tag, "room", NULL), *ext = load(dir, "stage", tag, "ext", &n);
    if (!inf || !room || !ext) {
        free(inf);
        free(room);
        free(ext);
        return 0;
    }
    retained_extra = ext;
    player_t p[2] = {{0}};
    p[0].hotel = h;
    p[0].stage = stage;
    p[0].lives = 3;
    world_start_full(&w, inf, room, p, 0, 0, upper, ext, n);
    free(inf);
    free(room);
    w.g.unk_826 = w.g.unk_82a = 12345;
    return w.room.n_actors;
}
void native_tick(int t, int mode) {
    w.n_spr = 0;
    w.mario.floor = (t / 80) % 5;
    w.mario.x = 44 + (t * 7) % 264;
    w.mario.y = hm_floor_y[w.mario.floor] - 30;
    w.mario.state_group = mode && t % 150 < 40 ? 4 : 0;
    w.mario.state = 0;
    w.g.frame = t * 6;
    w.g.draw_field = 1 + t % 2;
    if (t == 700)
        w.g.unk_53a = 1;
    for (int phase = 1; phase <= 2; phase++)
        for (int i = 0; i < w.room.n_actors; i++) {
            actor_t *a = &w.room.actors[i];
            a->cmd = phase;
            ((int (*)(actor_t *, gds *))a->handler)(a, &w.g);
            if (mode && phase == 2 && (t == 90 || t == 91 || t == 300) && a->type > 9) {
                a->cmd = t == 300 ? 4 : 3;
                ((int (*)(actor_t *, gds *))a->handler)(a, &w.g);
            }
        }
    if (t == 700)
        for (int i = 0; i < w.room.n_actors; i++) {
            actor_t *a = &w.room.actors[i];
            a->cmd = 4;
            ((int (*)(actor_t *, gds *))a->handler)(a, &w.g);
        }
}
int native_actor(int i, int off) {
    actor_t *a = &w.room.actors[i];
    if (off == 0x254)
        return a->x_min;
    if (off == 0x258)
        return a->x_max;
    return (&a->type)[(off - 4) / 2];
}
int native_gds(int off) {
    switch (off) {
    case 0x826:
        return w.g.unk_826;
    case 0x82a:
        return w.g.unk_82a;
    case 0x530:
        return w.g.unk_530;
    case 0x53a:
        return w.g.unk_53a;
    default:
        return 0;
    }
}

void native_seed(unsigned seed) {
    w.g.unk_826 = w.g.unk_82a = (int32_t)seed;
}
int native_lift_mode(void) {
    return w.lift_kind;
}
int native_lift_grid(int layout, int cell) {
    return w.lift_grid[layout][cell];
}
