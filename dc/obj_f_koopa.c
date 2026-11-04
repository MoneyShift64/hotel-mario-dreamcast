/* obj_f_koopa.c - "f_koopa" (the flying koopa), from L4_s06_sub.o (m_09be6 .. m_0aad4).
 * It doesn't use a door: it flies in from the left (col < 3) or right edge of the screen
 * on its floor's height, 4 pixels a tick across and 4 up or down (x_min holds the
 * vertical speed), bouncing between y 11 and 210 and turning round at the screen edges.
 * It counts as being on Mario's floor wherever it is (cur_floor = Mario's floor).
 * With start_mode > 0 it swoops instead at the top and bottom: an arc whose horizontal
 * steps are coded two bits at a time in x_max (the "hop" trick of the ninji).
 * Stomp or fireball/star: 400 points, it drops off the screen; then up to appear_count
 * (2) more times it comes back on a random floor and side (state 0 cmd 2).
 * Waits (state 1) until Mario is more than 72 pixels away in x or y.
 * Frames: 1-3 fly right, 4-6 fly left, 7-9 turn, 10-11 hit, 12-14 falling,
 * 15-20 coming in (right), 21-26 (left).
 * unk_25c: big-endian int16 at +2 = the step to go back to after turning.
 * (The original's eraser also sets g->unk_532 when it overlaps the score bar, to have
 * it redrawn; the Dreamcast port redraws the bar every frame.)
 */
#include "obj_common.h"

#ifndef FK_VARIANT
#define FK_VARIANT 0            /* 0 f_koopa, 1 z_koopa */
#endif
#if FK_VARIANT == 0
#define FK_POINTS   400
#define FK_ARC      0x16bfa5
#define FK_ARC_END  0
#define FK_ENTRY    f_koopa
#else
#define FK_POINTS   500
#define FK_ARC      0x415abff
#define FK_ARC_END  1
#define FK_ENTRY    z_koopa
#endif

static void set_back_step(actor_t *a, int step) {
    a->unk_25c[0] = 0; a->unk_25c[1] = 0;
    a->unk_25c[2] = (int1)(step >> 8); a->unk_25c[3] = (int1)step;
}
static int back_step(const actor_t *a) {
    return (int2)(((uint8_t)a->unk_25c[2] << 8) | (uint8_t)a->unk_25c[3]);
}

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_0aad4 */

static void place(actor_t *a, gds *g) {
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x1d);
    a->cur_floor = a->floor;
    if (a->col < 3) { a->x = 0; a->dir = 0; }
    else { a->x = 0x15c; a->dir = 1; }
}

static int fk_init(actor_t *a, gds *g) {                /* m_09be6 */
    a->x_min = (a->dir == 1) ? -4 : 4;
    place(a, g);
    a->appear_count = 2;
    a->unk_04a = a->unk_048 = a->appear_count + 1;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    a->timer = 0;
    a->state = 1;
    a->unk_018 = 1;
    return 0;
}

static int fk_again(actor_t *a, gds *g) {               /* m_09e5c: comes back */
    int go;
#if FK_VARIANT == 0
    go = a->appear_count-- >= 1;
#else
    go = g->unk_53a == 0 && a->appear_count-- > 0;
#endif
    if (!go) { a->appear_count = 0; return 0; }
    a->floor = (int2)smod((int)(RND(g) & 0xffff), 5);
    a->col = (int2)(RND(g) & 4);
    place(a, g);
    a->state = 1;
    a->hit_x0 = a->hit_x1 = a->hit_y0 = a->hit_y1 = 0;
    a->box_x0 = a->box_x1 = a->box_y0 = a->box_y1 = 0;
    a->delay = 0x14;
    a->timer = 0;
    a->unk_04e = 0;
    return 0;
}

static int fk_wait(actor_t *a, gds *g) {                /* m_09f70 */
    int dx, dy;
    if (g->unk_53a != 0 || a->timer++ <= a->delay) return 0;
    dx = abs((a->x + 0x10) - (g->mario->x + 0xe));
    dy = abs(a->y - g->mario->y);
    if (dx > 0x48 || dy > 0x48) { a->state = 2; a->unk_02a = 0; }
    return 0;
}

static int fk_come_in(actor_t *a, gds *g) {             /* m_0a060 */
    a->frame = (int2)(a->unk_02a + 0xf + (a->dir == 1 ? 6 : 0));
    if (++a->unk_02a > 5) {
        if (g->unk_53a == 0) { a->unk_02a = 1; a->state = 3; a->step = 0; }
        else { a->state = 4; a->step = 0; a->unk_02a = 0; }
    }
    draw_frame(a, g);
    return 0;
}

static int fk_moving(actor_t *a, gds *g) {              /* m_0a100 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static void flap(actor_t *a) {                          /* the 0..2..0 wing cycle */
    if (a->unk_02e == 0) {
        if (a->unk_02a == 2) { a->unk_02a--; a->unk_02e = 1; } else a->unk_02a++;
    } else if (a->unk_02a == 0) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
}

static void edges_and_boxes(actor_t *a, gds *g) {
    if ((a->x > 0x15a && a->dir == 0) || (a->x < 4 && a->dir == 1)) {
        a->step = 1; set_back_step(a, 0); a->unk_02a = 0;
    }
    (void)g;
}

static void boxes(actor_t *a, gds *g) {
    a->cur_floor = g->mario->floor;
    a->box_y0 = a->y + 3; a->box_y1 = a->y + 0xf;
    a->hit_y0 = a->y + 9; a->hit_y1 = a->y + 0x1b;
    if (a->dir == 1) { a->box_x0 = a->x + 6; a->box_x1 = a->x + 0x10; a->hit_x0 = a->x + 0x14; a->hit_x1 = a->x + 0x1a; }
    else { a->box_x0 = a->x + 0x12; a->box_x1 = a->x + 0x1c; a->hit_x0 = a->x + 8; a->hit_x1 = a->x + 0xe; }
    a->unk_03c = 1;
}

static int fk_fly(actor_t *a, gds *g) {                 /* m_0a14c */
    flap(a);
    if (a->dir == 1) { a->x -= 4; a->frame = (int2)(a->unk_02a + 4); }
    else             { a->x += 4; a->frame = (int2)(a->unk_02a + 1); }
    a->y += (int2)a->x_min;
    edges_and_boxes(a, g);
    if (a->y < 0xb) {
        a->x_min = 4;
        if (a->start_mode > 0 && a->step == 0) { a->x_max = FK_ARC; a->step = 2; a->unk_02a = 1; }
    } else if (a->y > 0xd2) {
        a->x_min = -4;
        if (a->start_mode > 0 && a->step == 0) { a->x_max = FK_ARC; a->step = 2; a->unk_02a = 1; }
    }
    boxes(a, g);
    return 0;
}

static int fk_turn(actor_t *a, gds *g) {                /* m_0a654 */
    (void)g;
#if FK_VARIANT == 0
    if (a->start_mode > 0) {
        a->y += (int2)a->x_min;
        if (a->y < 0xb) a->x_min = 4;
        else if (a->y > 0xd2) a->x_min = -4;
    }
#endif
    a->unk_03c = 0;
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x18;
    a->box_y0 = a->y + 4; a->box_y1 = a->y + 0x1c;
    a->frame = (int2)((a->dir == 1) ? 9 - a->unk_02a : a->unk_02a + 7);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1;
        a->step = back_step(a);
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int fk_swoop(actor_t *a, gds *g) {               /* m_0a3d0 */
    int d;
    flap(a);
    d = a->x_max & 3;
    a->x_max >>= 2;
    if (a->x_max == FK_ARC_END) {
        a->x_max = FK_ARC;
        set_back_step(a, 2);
        a->step = 1; a->unk_02a = 0;
    }
    if (a->dir == 1) { a->x -= (int2)(d * 2); a->frame = (int2)(a->unk_02a + 4); }
    else             { a->x += (int2)(d * 2); a->frame = (int2)(a->unk_02a + 1); }
    a->y += (int2)a->x_min;
    edges_and_boxes(a, g);
    if (a->y < 0xb) {
        a->x_min = 4;
        if (a->start_mode > 0 && a->step == 0) { a->step = 2; a->unk_02a = 0; }
    } else if (a->y > 0xd2) a->x_min = -4;
    boxes(a, g);
    return 0;
}

static int fk_hit(actor_t *a, gds *g) {                 /* m_0a770: stomped */
    if (g->unk_496 == 0) {
        SCORE(g, FK_POINTS);
        a->unk_048--; a->unk_04a--;
        a->state = 4; a->step = 0; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int fk_destroy(actor_t *a, gds *g) {             /* m_0a7dc: fireball / star */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, FK_POINTS); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 0; a->unk_02a = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int fk_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_0a872 */

static int fk_struck(actor_t *a, gds *g) {              /* m_0a8ac */
    a->frame = (int2)(a->unk_02a + 10);
    draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 1; a->unk_02a = 0; }
    return 0;
}

static int fk_fall(actor_t *a, gds *g) {                /* m_0a8fa */
    a->y += 0x10;
    if (a->y > 0xe5) a->y = 0xe5;
    a->frame = (int2)(a->unk_02a + 0xc);
    draw_frame(a, g);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xd4) { a->step = 2; a->unk_02a = 2; }
    return 0;
}

static int fk_gone(actor_t *a, gds *g) {                /* m_0a97e */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->timer = 0; a->dir = -1; }
    return 0;
}

int FK_ENTRY(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, fk_init);
        H(0, 2, fk_again);
        H(1, 2, fk_wait);
        H(2, 1, obj_erase);
        H(2, 2, fk_come_in);
        H(3, 1, obj_erase);
        H(3, 2, fk_moving);
        H(3, 3, fk_hit);
        H(3, 4, fk_destroy);
        H(4, 1, obj_erase);
        H(4, 2, fk_dying);
        W(0, fk_fly);
        W(1, fk_turn);
        W(2, fk_swoop);
        B(0, fk_struck);
        B(1, fk_fall);
        B(2, fk_gone);
    }
    return obj_dispatch(a, g);
}
