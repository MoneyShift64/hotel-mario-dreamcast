/* obj_w_lips.c - "w_lips" (type 0x25), the homing kiss Wendy blows in the hotel 7 boss
 * stage, from L7_s10_sub.o (m_13e2c .. m_14480). Idle in state 0 until Wendy sets state 3
 * with a position and speeds (unk_036 = x speed, dir = y speed). It steers towards Mario
 * (speeds change by 2 a tick, up to 8) for 40 ticks, and gives up early once it has
 * overshot him both ways or left the play area. Touching it hurts Mario (cmd 3 returns 1);
 * a fireball or star pops it for 10 points. It always counts as on Mario's floor.
 * Frames: 1-3 flying, 4-6 popping.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_14480 */

static int wl_reset(actor_t *a, gds *g) {               /* m_13e2c / m_13eba */
    (void)g;
    a->unk_02a = 0; a->x_min = 0;
    return 0;
}

static int wl_moving(actor_t *a, gds *g) {              /* m_13ee6 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static int wl_touch(actor_t *a, gds *g) { (void)a; (void)g; return 1; }   /* m_141da */

static int wl_destroy(actor_t *a, gds *g) {             /* m_141fe */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 10);
        else a->unk_04c = 0;
        a->state = 4; a->step = 1; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int wl_fly(actor_t *a, gds *g) {                 /* m_13f32 */
    mario_t *m = g->mario;
    int mx = m->x + 0xe, my = m->y + 0xe;
    a->frame = (int2)(a->unk_02a + 1);
    if (a->unk_02e == 0) {
        if (a->unk_02a < 2) a->unk_02a++; else { a->unk_02a--; a->unk_02e = 1; }
    } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    a->x += a->unk_036;
    a->y += a->dir;
    a->box_x0 = a->x + 8; a->box_x1 = a->x + 0x15;
    a->box_y0 = a->y + 8; a->box_y1 = a->y + 0x14;
    a->cur_floor = m->floor;
    if (a->x < 0xe || a->x > 0x154 || a->y < 0x14 || a->y > 0xd8 ||
        (((mx < a->box_x0 && a->unk_036 > 0) || (a->box_x1 < mx && a->unk_036 < 0)) &&
         ((my < a->box_y0 && a->dir > 0) || (a->box_y1 < my && a->dir < 0)))) {
        a->state = 4; a->step = 1; a->unk_02a = 0;
    }
    if (++a->x_min > 0x27) { a->state = 4; a->step = 1; a->unk_02a = 0; }
    if (a->x < m->x) { if (a->unk_036 < 8) a->unk_036 += 2; }
    else if (a->unk_036 > -8) a->unk_036 -= 2;
    if (a->y < m->y) { if (a->dir < 8) a->dir += 2; }
    else if (a->dir > -8) a->dir -= 2;
    return 0;
}

static int wl_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int wl_pop(actor_t *a, gds *g) {                 /* m_142ca */
    a->frame = (int2)(a->unk_02a + 4);
    draw_frame(a, g);
    if (++a->unk_02a > 2) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

static int wl_gone(actor_t *a, gds *g) {                /* m_1431a */
    (void)g;
    if (a->unk_02a-- < 2) {
        a->state = 0; a->step = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
        a->unk_04e = 0;
    }
    return 0;
}

int w_lips(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, wl_reset);
        H(0, 2, wl_reset);
        H(3, 1, obj_erase);
        H(3, 2, wl_moving);
        H(3, 3, wl_touch);
        H(3, 4, wl_destroy);
        H(4, 1, obj_erase);
        H(4, 2, wl_dying);
        W(0, wl_fly);
        B(1, wl_pop);
        B(3, wl_gone);
    }
    return obj_dispatch(a, g);
}
