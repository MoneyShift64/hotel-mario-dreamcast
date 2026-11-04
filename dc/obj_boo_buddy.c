/* obj_boo_buddy.c - "boo_buddy", from L5_s07_sub.o (m_11f46 .. m_13406); the L5_s09
 * copy is the same code. Flies freely over the whole screen (x 4 .. 356, y 10 .. 212)
 * and counts as being on Mario's floor. It can't be stomped (touching it hurts: cmd 3
 * returns 1) and fireballs don't hurt it; only a star (item_hit > 9) or the end of the
 * stage knocks it off the screen (cmd 4 returns 2: no points).
 * start_mode bit 0: the big, fast kind (double speed); clear = the small kind (frames
 * +8 / +6 / +1). start_mode bit 1: starts hidden (state 4) and only shows itself to
 * swoop at Mario for 32 ticks, then fades away again.
 * Ordinary boo: while Mario walks or jumps it chases him (2 px a tick, re-aiming up/down
 * every 10 ticks) - unless he faces it, then it covers its face and waits until he
 * turns away. Otherwise it drifts about at random, changing course every 20 ticks
 * (when it is more than 106 pixels from Mario).
 * Hidden boo: drifts invisibly; when Mario is facing away and more than 72 px off, it
 * may fade in (3 frames) and become an ordinary-moving boo (state 3).
 * NB: the code keeps its x / y speed in floor / col.  unk_25c: int32 swoop counter.
 * x_min: 1 = chasing, 0 = wandering (state 3); x_max: 1 = fading out (state 4).
 * Frames: 1-8 big boo (1/5 chase, 3/7 wander, 2-4 / 6-8 covering its face),
 * 9-16 small boo, 17-20 turning, 21-26 fading, 27-32 small fading, 33/34 knocked off.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }

static int bb_init(actor_t *a, gds *g) {                /* m_11f46 */
    a->unk_02a = 0; a->unk_02c = 1; a->step = 0;
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x12);
    a->x = a->drawn_x0 = a->drawn_x1 = (int2)g->door_x[a->col];
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x1c;
    a->box_y0 = a->y + 1; a->box_y1 = a->y + 0x11;
    if (a->dir == -1) a->dir = 0;
    if ((a->start_mode & 2) == 0) a->state = 3;
    else { a->state = 4; a->x_max = 1; }
    a->step = 0; a->unk_02a = 0;
    return 0;
}

static int2 rnd_pm(gds *g, int n) { return (int2)((RND(g) & 1) == 0 ? -n : n); }

/* move by (floor, col), keep inside the screen */
static void move(actor_t *a, gds *g, int bounce) {
    a->x = (int2)(a->x + a->floor);
    a->y = (int2)(a->y + a->col);
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x14;
    a->box_y0 = a->y + 1; a->box_y1 = a->y + 0x11;
    a->cur_floor = g->mario->floor;
    if (a->x > 0x164) { a->x = 0x164; a->unk_036 = 1000; }
    else if (a->x < 4) { a->x = 4; a->unk_036 = 1000; }
    if (bounce) {
        if (a->y < 10) { a->y = 10; if (a->col < 0) a->col = (int2)-a->col; }
        else if (a->y > 0xd4) { a->y = 0xd4; if (a->col > 0) a->col = (int2)-a->col; }
    } else {
        if (a->y < 10) { a->y = 10; a->unk_036 = 1000; }
        else if (a->y > 0xd4) { a->y = 0xd4; a->unk_036 = 1000; }
    }
}

static int bb_fly(actor_t *a, gds *g) {                 /* m_122ee */
    mario_t *m = g->mario;
    int aim;
    if (a->start_mode & 2) {                             /* swooping at Mario */
        if ((a->dir == 1 && a->x < m->x - 0x18) || (a->dir == 0 && m->x + 0x18 < a->x) ||
            a->x == 4 || a->x == 0x164) {
            a->state = 4; a->unk_02a = 3; a->x_max = 1;
        } else {
            int32_t n = rd25c(a, 0);
            wr25c(a, 0, n + 1);
            if (n > 0x1f) { a->state = 4; a->unk_02a = 3; a->x_max = 1; }
        }
        if ((a->start_mode & 1) == 0) {
            a->floor = (int2)(a->dir == 1 ? -4 : 4);
            if (a->y < m->y - 8) { if (++a->col > 4) a->col = 4; }
            else if (m->y + 8 < a->y) { if (--a->col < -4) a->col = -4; }
            a->frame = (a->dir == 1) ? 0xd : 9;
        } else {
            a->floor = (int2)(a->dir == 1 ? -6 : 6);
            if (a->y < m->y) { if (++a->col > 6) a->col = 6; }
            else if (--a->col < -6) a->col = -6;
            a->frame = (a->dir == 1) ? 5 : 1;
        }
        move(a, g, 1);
        return 0;
    }
    if (a->unk_036 < a->unk_02e) {
        if (a->x_min != 0 || abs(a->x - m->x) > 0x6a || abs(a->y - m->y) > 0x6a) a->unk_036++;
        aim = 0;
    } else {
        a->unk_036 = 0;
        a->unk_02e = (a->x_min == 0) ? 0x14 : 10;
        aim = 1;
    }
    if (m->state_group == 0 || m->state_group == 3) {    /* chase */
        if (a->x < m->x) {
            a->floor = 2;
            if (m->prev_state & 1) { a->step = 2; a->unk_02a = 0; a->unk_02c = 1; }
            if (a->dir == 1) a->step = 1;
        } else if (m->x < a->x) {
            a->floor = -2;
            if ((m->prev_state & 1) == 0) { a->step = 2; a->unk_02a = 0; a->unk_02c = 1; }
            if (a->dir == 0) a->step = 1;
        } else a->floor = 0;
        if (aim) {
            if ((m->state_group == 3 && m->y < a->y + 0x28) || m->y < a->y) a->col = -2;
            else if (a->y + 0x12 < m->y + 0x1e) a->col = 2;
            else a->col = 0;
            if (a->start_mode & 1) a->col = (int2)(a->col * 2);
        }
        a->x_min = 1;
        a->frame = (a->dir == 1) ? 5 : 1;
    } else {                                             /* wander */
        if (aim) {
            if (a->x == 4) a->floor = 2;
            else if (a->x == 0x164) a->floor = -2;
            else a->floor = rnd_pm(g, 2);
            if ((a->floor > 0 && a->dir == 1) || (a->floor < 0 && a->dir == 0)) a->step = 1;
            if (a->y == 10) a->col = 2;
            else if (a->y == 0xd4) a->col = -2;
            else a->col = rnd_pm(g, 2);
            if (a->start_mode & 1) a->col = (int2)(a->col * 2);
        }
        a->x_min = 0;
        a->frame = (a->dir == 1) ? 7 : 3;
    }
    if ((a->start_mode & 1) == 0) a->frame += 8;
    move(a, g, 1);
    return 0;
}

static int bb_turn(actor_t *a, gds *g) {                /* m_12cf8 */
    (void)g;
    a->frame = (a->x_min == 0) ? 0x12 : 0x11;
    if ((a->start_mode & 1) == 0) a->frame += 2;
    a->dir = (a->dir == 1) ? 0 : 1;
    a->step = 0;
    return 0;
}

static int bb_cover(actor_t *a, gds *g) {               /* m_12ac0 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? a->unk_02a + 6 : a->unk_02a + 2);
    if ((a->start_mode & 1) == 0) a->frame += 8;
    if (a->unk_02c-- < 2) { a->unk_02c = 1; if (++a->unk_02a > 0) a->step = 3; }
    return 0;
}

static int bb_shy(actor_t *a, gds *g) {                 /* m_12b4e */
    mario_t *m = g->mario;
    int moving = (m->state_group == 0 || m->state_group == 3);
    if (a->dir == 1) {
        a->frame = 8;
        if ((a->start_mode & 1) == 0) a->frame += 8;
        if (a->x < m->x || (m->prev_state & 1) != 0 || !moving) { a->unk_02a = 0; a->unk_02c = 1; a->step = 4; }
    } else {
        a->frame = 4;
        if ((a->start_mode & 1) == 0) a->frame += 8;
        if (m->x < a->x || (m->prev_state & 1) == 0 || !moving) { a->unk_02a = 0; a->unk_02c = 1; a->step = 4; }
    }
    return 0;
}

static int bb_uncover(actor_t *a, gds *g) {             /* m_12c6c */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? a->unk_02a + 7 : a->unk_02a + 3);
    if ((a->start_mode & 1) == 0) a->frame += 8;
    if (a->unk_02c-- < 2) { a->unk_02c = 1; if (++a->unk_02a > 0) a->step = 0; }
    return 0;
}

static int bb_moving(actor_t *a, gds *g) {              /* m_12212 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static int bb_hit(actor_t *a, gds *g) { (void)a; (void)g; return 1; }   /* m_12d82 */

static int bb_destroy(actor_t *a, gds *g) {             /* m_12da6 */
    if (g->unk_53a != 0 || g->item_hit > 9) { a->state = 4; a->step = 5; }
    return 2;
}

static int bb_hidden(actor_t *a, gds *g) {              /* m_12df8 */
    CALL(a->steps_b[a->step], a, g);
    if (a->state != 0) draw_frame(a, g);                 /* (state 0: erased, gone) */
    return 0;
}

static int bb_ghost(actor_t *a, gds *g) {               /* m_12f14 */
    mario_t *m = g->mario;
    if (++a->unk_036 >= a->unk_02e) {
        a->unk_036 = 0; a->unk_02e = 0x14;
        if (a->y == 10) a->col = 1;
        else if (a->y == 0xd4) a->col = -1;
        else a->col = rnd_pm(g, 1);
        if (a->x == 4) a->floor = 2;
        else if (a->x == 0x164) a->floor = -2;
        else a->floor = rnd_pm(g, 2);
        if ((a->floor > 0 && a->dir == 1) || (a->floor < 0 && a->dir == 0)) a->step = 1;
        else if (((a->x < m->x && a->dir == 0) || (m->x < a->x && a->dir == 1)) &&
                 (abs(a->x - m->x) > 0x48 || abs(a->y - m->y) > 0x48) &&
                 ((g->unk_826 >> 1) & 1) == 0) {
            a->x_max = 0; a->unk_02a = 0;
        }
    }
    move(a, g, 0);
    if (a->x_max == 0) {                                 /* fading in */
        a->frame = (int2)((a->dir == 1) ? 0x1a - a->unk_02a : 0x17 - a->unk_02a);
        if (++a->unk_02a > 2) { a->state = 3; wr25c(a, 0, 0); }
    } else {
        if (a->unk_02a > 0) a->unk_02a--;
        a->frame = (int2)((a->dir == 1) ? 0x1a - a->unk_02a : 0x17 - a->unk_02a);
    }
    if ((a->start_mode & 1) == 0) a->frame += 6;
    return 0;
}

static int bb_ghost_turn(actor_t *a, gds *g) {          /* m_13336 */
    (void)g;
    a->frame = 0x21;
    if ((a->start_mode & 1) == 0) a->frame++;
    a->dir = (a->dir == 1) ? 0 : 1;
    a->step = 0;
    return 0;
}

static int bb_fall(actor_t *a, gds *g) {                /* m_133a0 */
    (void)g;
    a->frame = 0x21;
    if ((a->start_mode & 1) == 0) a->frame++;
    a->y += 0x12;
    if (a->y > 0xdd) a->state = 0;
    if (a->y > 0xf0) a->y = 0xf0;
    return 0;
}

int boo_buddy(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, bb_init);
        H(3, 1, obj_erase);
        H(3, 2, bb_moving);
        H(3, 3, bb_hit);
        H(3, 4, bb_destroy);
        H(4, 1, obj_erase);
        H(4, 2, bb_hidden);
        H(4, 3, bb_destroy);
        W(0, bb_fly);
        W(1, bb_turn);
        W(2, bb_cover);
        W(3, bb_shy);
        W(4, bb_uncover);
        B(0, bb_ghost);
        B(1, bb_ghost_turn);
        B(5, bb_fall);
    }
    return obj_dispatch(a, g);
}
