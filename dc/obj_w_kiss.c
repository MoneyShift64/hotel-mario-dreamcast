/* obj_w_kiss.c - "w_kiss" (type 0x26), a kiss Wendy blows in the hotel 7 boss stage,
 * from L7_s10_sub.o (m_112de .. m_11bae). Idle in state 0 until Wendy sets state 3 with a
 * position and speeds (unk_036 = x speed, dir = y speed). Then it flutters about at random
 * (a new +-6 heading every 32 ticks, nudged by 2 every other tick), bouncing off the screen
 * edges, for 100 ticks, opening a shut door it passes 1 time in 16 (door cell at x+20 on
 * the floor its y is on). Touching it hurts Mario (cmd 3 returns 1); a fireball or star
 * pops it for 10 points and it falls away. It always counts as on Mario's floor.
 * Frames: 1-5 flutter, 6-10 popping, 11-14 falling.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_11afa */

static int wk_reset(actor_t *a, gds *g) {               /* m_112de / m_113dc */
    (void)g;
    a->unk_02a = 0; a->x_min = 0;
    return 0;
}

static int wk_moving(actor_t *a, gds *g) {              /* m_11408 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static int wk_touch(actor_t *a, gds *g) { (void)a; (void)g; return 1; }   /* m_117d2 */

static int wk_destroy(actor_t *a, gds *g) {             /* m_117f6 */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) SCORE(g, 10);
        else a->unk_04c = 0;
        a->state = 4; a->step = 1; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int floor_of_y(int y) {                          /* m_11bae */
    y += 8;
    if (y < 0x48) return 0;
    if (y < 0x6c) return 1;
    if (y < 0x90) return 2;
    if (y < 0xb4) return 3;
    return 4;
}

static int wk_fly(actor_t *a, gds *g) {                 /* m_11454 */
    uint16_t *c;
    a->frame = (int2)(a->unk_02a + 1);
    if (a->unk_02e == 0) {
        if (a->unk_02a < 4) a->unk_02a++; else { a->unk_02a--; a->unk_02e = 1; }
    } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
    a->x += a->unk_036;
    a->y += a->dir;
    a->box_x0 = a->x + 0x10; a->box_x1 = a->x + 0x1a;
    a->box_y0 = a->y + 1; a->box_y1 = a->y + 0xf;
    a->cur_floor = g->mario->floor;
    if ((a->x < 0xe && a->unk_036 < 0) || (a->x > 0x148 && a->unk_036 > 0)) a->unk_036 = (int2)-a->unk_036;
    if ((a->y < 0x14 && a->dir < 0) || (a->y > 0xd8 && a->dir > 0)) a->dir = (int2)-a->dir;
    if (++a->x_min < 100) {
        if ((a->x_min & 0x1f) == 0) {
            a->unk_036 = (RND(g) & 1) ? 6 : -6;
            a->dir = (RND(g) & 1) ? 6 : -6;
        } else if ((a->x_min & 1) == 0) {
            if ((RND(g) & 1) == 0) { if (a->unk_036 > -6) a->unk_036 -= 2; }
            else if (a->unk_036 < 6) a->unk_036 += 2;
            if ((RND(g) & 1) == 0) { if (a->dir > -6) a->dir -= 2; }
            else if (a->dir < 6) a->dir += 2;
        }
    } else { a->state = 4; a->step = 1; a->unk_02a = 0; }
    c = DOOR_CELL(floor_of_y(a->y), a->x + 0x14);
    if (c && (*c & 0xf000) == 0x1000 && (RND(g) & 0xf) == 1) {
        *c = (*c & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
    return 0;
}

static int wk_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int wk_pop(actor_t *a, gds *g) {                 /* m_118c2 */
    a->frame = (int2)(a->unk_02a + 6);
    draw_frame(a, g);
    if (++a->unk_02a > 4) { a->step = 2; a->unk_02a = 0; }
    return 0;
}

static int wk_fall(actor_t *a, gds *g) {                /* m_11910 */
    a->y += 0x10;
    if (a->y > 0xf2) a->y = 0xf2;
    a->frame = (int2)(a->unk_02a + 0xb);
    draw_frame(a, g);
    if (++a->unk_02a > 3) a->unk_02a = 0;
    if (a->y > 0xe1) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

static int wk_gone(actor_t *a, gds *g) {                /* m_11994 */
    (void)g;
    if (a->unk_02a-- < 2) {
        a->state = 0; a->step = 0;
        a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
        a->unk_04e = 0;
    }
    return 0;
}

int w_kiss(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, wk_reset);
        H(0, 2, wk_reset);
        H(3, 1, obj_erase);
        H(3, 2, wk_moving);
        H(3, 3, wk_touch);
        H(3, 4, wk_destroy);
        H(4, 1, obj_erase);
        H(4, 2, wk_dying);
        W(0, wk_fly);
        B(1, wk_pop);
        B(2, wk_fall);
        B(3, wk_gone);
    }
    return obj_dispatch(a, g);
}
