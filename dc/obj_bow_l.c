/* Bowser's linked falling-fire columns; upper-room module 0x20b66..0x21540. */
#include "obj_common.h"
static actor_t *next(actor_t *a, gds *g) {
    int n = rd25c(a, 0);
    return n > 0 && n <= g->room->n_actors ? &g->room->actors[n - 1] : NULL;
}
static int init(actor_t *a, gds *g) {
    a->x_min = 1;
    for (int i = 0; i < g->room->n_actors; i++)
        if (g->room->actors[i].type == 0x3d && !g->room->actors[i].x_min) {
            wr25c(a, 0, i + 1);
            g->room->actors[i].x_min = 1;
            break;
        }
    for (int i = 0; i < g->room->n_actors; i++)
        if (g->room->actors[i].type == 0x3b) {
            a->x_max = i;
            break;
        }
    return 0;
}
static void box(actor_t *a) {
    a->box_x0 = a->x + 8;
    a->box_x1 = a->x + 22;
    a->box_y0 = a->y;
    a->box_y1 = a->y + 36;
}
static int update(actor_t *a, gds *g) {
    a->frame = a->unk_02a + 1;
    if (++a->unk_02a > 3)
        a->unk_02a = 0;
    a->cur_floor = g->mario->floor;
    if (a->step == 0) {
        if (a->unk_02c++ == 0) {
            box(a);
            draw_at_xy(a, g, a->frame);
            return 0;
        }
        int end = 0;
        actor_t *b = next(a, g);
        if (!b)
            end = 1;
        else {
            int straight = a->box_y1 < 72;
            if (!straight && a->box_y1 < 216)
                straight = (RND(g) & 15) > 2;
            if (straight) {
                b->unk_02a = 0;
                b->x = a->x;
                b->y = a->y + 36;
                wr25c(b, 4, rd25c(a, 4));
            } else {
                b->unk_02a = RND(g) & 3;
                b->x = (RND(g) & 31) * 10 + 32;
                b->y = 2;
                end = 1;
                int v = rd25c(a, 4) + 1;
                wr25c(b, 4, v);
                if (v > 1) {
                    g->unk_054 = 8;
                    wr25c(b, 4, 0);
                }
            }
            b->unk_02c = 0;
            b->state = 3;
            b->step = 0;
        }
        a->unk_02c = 0;
        a->step = 1;
        if (end) {
            int chosen = (int)a->x_max;
            if (chosen >= 0 && chosen < g->room->n_actors) {
                int best = chosen;
                for (int i = chosen; i < g->room->n_actors && g->room->actors[i].type == 0x3b;
                     i++) {
                    if (!g->room->actors[i].state) {
                        best = i;
                        break;
                    }
                    if (g->room->actors[best].unk_02c < g->room->actors[i].unk_02c)
                        best = i;
                }
                actor_t *f = &g->room->actors[best];
                f->state = 3;
                f->unk_02c = 0;
                f->x = a->x + 4;
                f->y = a->box_y1 - 16;
            }
        }
    } else if (++a->unk_02c > 3) {
        a->x = (RND(g) & 31) * 10 + 32;
        a->y = 2;
        box(a);
        a->step = 0;
        a->unk_02c = 1;
        a->unk_02a = a->frame = 1;
        g->unk_054 = 8;
    }
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
            b->step = 0;
            b->unk_02a = 0;
            b->unk_02c = 0;
        }
        if (++a->unk_02a > 1) {
            a->step = 3;
            a->unk_02a = 1;
        }
    } else if (a->unk_02a-- < 2) {
        a->state = a->step = a->unk_02a = a->unk_02c = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    }
    return 0;
}
int bow_l(actor_t *a, gds *g) {
    if (!a->state && !a->cmd) {
        obj_tables(a);
        H(0, 0, init);
        H(3, 2, update);
        H(3, 3, hit);
        H(3, 4, kill);
        H(4, 2, fade);
    }
    return obj_dispatch(a, g);
}
