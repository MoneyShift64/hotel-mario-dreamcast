/* obj_cloud.c - "cloud" (L4_s03_sub.o, m_161fc .. m_16d64): a little cloud that drifts
 * after a target, re-aiming every 5..12 ticks at 4 or 6 px (x) and 3 or 5 px (y) a tick.
 * The target is the first (start_mode 0) or last (otherwise) live enemy (type > 9, not
 * type 0x2a) on Mario's floor; with none, it is Mario himself. It re-picks whenever Mario
 * changes floor or its enemy stops walking. Kept inside x 14..320, y 20..203.
 * It waits (state 0) while player.unk_10 != 0, and when unk_10 becomes 1 it whooshes off
 * sideways 20 px a tick. No box: it never touches Mario.
 * appear_count: Mario's floor when it last picked; timer: 1 = chasing Mario;
 * x_min: the target enemy (the original keeps the pointer; here its index + 1, 0 = none);
 * floor / col: x / y speed; unk_036 / unk_02e: ticks since / until the next re-aim.
 * Frames 1-4, a 4-picture loop.
 */
#include "obj_common.h"

static int cl_init(actor_t *a, gds *g) {                /* m_161fc (m_162d6 without appear_count) */
    int2 v;
    a->unk_02a = 0; a->unk_02c = 1; a->step = 0;
    v = (int2)(g->floor_y[a->floor] - 0x11);
    a->y = a->drawn_y0 = a->drawn_y1 = v;
    v = (int2)(g->door_x[a->col] + 8);
    a->x = a->drawn_x0 = a->drawn_x1 = v;
    if (a->cmd == 0) a->appear_count = -1;
    if (g->player[g->cur_player].unk_10 == 0) a->state = 3;
    return 0;
}

static int cl_erase(actor_t *a, gds *g) {               /* m_16c66: the HUD band needs a redraw */
    int y = (g->draw_field == 1) ? a->drawn_y0 : a->drawn_y1;
    if (y < 0x24 && 0x13 < y + 0x11) g->unk_532 = 1;
    return 0;
}

static int cl_moving(actor_t *a, gds *g) {              /* m_16364 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_at_xy(a, g, a->frame);
    return r;
}

static int aim(int from, int to, int near, int slow, int fast) {
    if (from < to) return (from + near < to) ? fast : slow;
    if (to < from) return (to < from - near) ? -fast : -slow;
    return 0;
}

static int cl_drift(actor_t *a, gds *g) {               /* m_16440 */
    mario_t *m = g->mario;
    room_t *rm = g->room;
    actor_t *t = a->x_min ? &rm->actors[a->x_min - 1] : NULL;
    if (g->player[g->cur_player].unk_10 == 1) {
        if (a->x < 0xa7) { a->dir = 1; a->x += 0x14; }
        else             { a->dir = 0; a->x -= 0x14; }
        a->state = 4; a->step = 2;
        return 0;
    }
    if (a->appear_count != m->floor || (t && t->state != 3)) {
        int n = rm->n_actors, i, found = -1;
        a->appear_count = m->floor;
        if (a->start_mode == 0) {
            for (i = 0; i < n && found < 0; i++) {
                actor_t *p = &rm->actors[i];
                if (p->cur_floor == a->appear_count && p->state == 3 && p->type != 0x2a && p->type > 9) found = i;
            }
        } else {
            for (i = n - 1; i >= 0 && found < 0; i--) {
                actor_t *p = &rm->actors[i];
                if (p->cur_floor == a->appear_count && p->state == 3 && p->type != 0x2a && p->type > 9) found = i;
            }
        }
        if (found >= 0) { a->timer = 0; a->x_min = found + 1; t = &rm->actors[found]; }
        else { a->timer = 1; a->x_min = 0; t = NULL; }
        a->unk_036 = a->unk_02e;
    }
    if (++a->unk_036 >= a->unk_02e) {
        int tx, ty;
        a->unk_036 = 0;
        a->unk_02e = (int2)((RND2(g) & 7) + 5);
        if (a->timer == 0 && t) { tx = t->x; ty = t->y; } else { tx = m->x; ty = m->y; }
        a->floor = (int2)aim(a->x, tx, 0x5a, 4, 6);
        a->col = (int2)aim(a->y, ty, 0x3c, 3, 5);
    }
    a->x += a->floor;
    a->y += a->col;
    if (a->x < 0xe) { a->x = 0xe; a->unk_036 = a->unk_02e; }
    else if (a->x > 0x140) { a->x = 0x140; a->unk_036 = a->unk_02e; }
    if (a->y < 0x14) { a->y = 0x14; a->unk_036 = a->unk_02e; }
    else if (a->y > 0xcb) { a->y = 0xcb; a->unk_036 = a->unk_02e; }
    if (--a->unk_02c == 0) {
        a->unk_02c = 1;
        if (++a->unk_02a == 4) a->unk_02a = 0;
    }
    a->frame = (int2)(a->unk_02a + 1);
    return 0;
}

static int cl_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_16ae2 */

static int cl_whoosh(actor_t *a, gds *g) {              /* m_16b1c */
    a->x += (a->dir == 1) ? 0x14 : -0x14;
    if (a->x > 0x13f || a->x < 0xf) { a->step = 3; a->unk_02c = 2; }
    draw_at_xy(a, g, a->frame);
    return 0;
}

static int cl_gone(actor_t *a, gds *g) {                /* m_16c36 */
    (void)g;
    if (a->unk_02c-- < 1) a->state = 0;
    return 0;
}

int cloud(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, cl_init);
        H(0, 2, cl_init);
        H(3, 1, cl_erase);
        H(3, 2, cl_moving);
        H(4, 1, cl_erase);
        H(4, 2, cl_dying);
        W(0, cl_drift);
        B(2, cl_whoosh);
        B(3, cl_gone);
    }
    return obj_dispatch(a, g);
}
