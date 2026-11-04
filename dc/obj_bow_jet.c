/* Bowser's linked flame jet, original upper-room module 0x1cf00..0x1d776. */
#include "obj_common.h"
static actor_t *next(actor_t *a, gds *g) {
    int n = rd25c(a, 0);
    return n > 0 && n <= g->room->n_actors ? &g->room->actors[n - 1] : NULL;
}
static int init(actor_t *a, gds *g) {
    a->unk_02a = 0;
    a->unk_03c = 1;
    a->x_min = 1;
    for (int i = 0; i < g->room->n_actors; i++)
        if (g->room->actors[i].type == 0x3c && !g->room->actors[i].x_min) {
            wr25c(a, 0, i + 1);
            g->room->actors[i].x_min = 1;
            break;
        }
    return 0;
}
static int reset(actor_t *a, gds *g) {
    (void)g;
    a->unk_02a = a->step = 0;
    return 0;
}
static void clear(actor_t *a) {
    a->state = a->step = 0;
    a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    a->hit_x0 = a->hit_x1 = a->hit_y0 = a->hit_y1 = 0;
}
static int update(actor_t *a, gds *g) {
    a->frame = a->unk_02a + (a->dir ? 5 : 1);
    if (++a->unk_02a > 3)
        a->unk_02a = 0;
    a->cur_floor = g->mario->floor;
    a->box_y0 = a->y + 3;
    a->box_y1 = a->y + 11;
    a->hit_y0 = a->y + 17;
    a->hit_y1 = a->y + 25;
    a->box_x0 = a->x + (a->dir ? 3 : 23);
    a->box_x1 = a->x + (a->dir ? 11 : 31);
    a->hit_x0 = a->x + (a->dir ? 15 : 10);
    a->hit_x1 = a->x + (a->dir ? 24 : 19);
    if (a->x < 0 || a->x > 348 || a->y < 2 || a->y > 224)
        clear(a);
    else if (a->unk_02a == 2) {
        actor_t *b = next(a, g);
        if (b && !b->state) {
            b->x = a->x + (a->dir ? -34 : 34);
            b->y = a->y - 34;
            b->dir = a->dir;
            b->state = 3;
        }
    }
    if (a->state == 3)
        draw_at_xy(a, g, a->frame);
    return 0;
}
static int hit(actor_t *a, gds *g) {
    (void)a;
    (void)g;
    return 1;
}
static int kill(actor_t *a, gds *g) {
    (void)a;
    (void)g;
    return 2;
}
static int fade(actor_t *a, gds *g) {
    if (!a->step) {
        actor_t *b = next(a, g);
        if (a->unk_02a > 0 && b && b->state == 3) {
            b->state = 4;
            b->unk_02a = 0;
        }
        if (++a->unk_02a > 1) {
            a->step = 3;
            a->unk_02a = 1;
        }
    } else if (a->unk_02a-- < 2)
        clear(a);
    return 0;
}
int bow_jet(actor_t *a, gds *g) {
    if (!a->state && !a->cmd) {
        obj_tables(a);
        H(0, 0, init);
        H(0, 2, reset);
        H(3, 2, update);
        H(3, 3, hit);
        H(3, 4, kill);
        H(4, 2, fade);
    }
    return obj_dispatch(a, g);
}
