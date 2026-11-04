/* obj_bat.c - "bat" (Swoop), from L5_s07_sub.o (m_0f890 .. m_106a8).
 * Hangs at the bottom of the screen (y 220) with a second bat hanging 16 pixels to its
 * right (that one is its next life: appear_count 1). After its delay it flies off
 * (sound 0xd), speeding up to 6 pixels a tick, bobbing up and down by a stream of
 * pseudo-random 2-bit steps, and wanders between the floors: it picks a target floor
 * band (floor_y-35 .. floor_y-20) and bounces inside it; while it is outside the band
 * it counts as being on Mario's floor (cur_floor = Mario's). It slows down and turns
 * at random and at the screen edges (x 24 .. 340), and opens closed doors it passes
 * (1 in 16). Stomp or fireball: 400 points, a short fall off the screen, then the
 * spare bat (if any, and more than 20 seconds left) takes off 40 ticks later.
 * unk_25c: int32 [0] target floor, [4]/[8] band top/bottom, [0xc] x of the spare bat
 * (0 = none). x_max: bob direction (1 down, 0 up); x_min: the bobbing bit stream.
 * Frames: 1-3 fly right, 4-6 fly left, 7-9 turn, 10-12 dying, 13 hanging.
 */
#include "obj_common.h"

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_106a8 */

static void band(actor_t *a, gds *g, int floor) {
    wr25c(a, 4, g->floor_y[floor] - 0x23);
    wr25c(a, 8, g->floor_y[floor] - 0x14);
}

static void hang(actor_t *a) {
    a->y = a->drawn_y0 = a->drawn_y1 = 0xdc;
    a->cur_floor = 5;
    a->box_y0 = a->y + 7; a->box_y1 = a->y + 0x10;
}

static int bat_init(actor_t *a, gds *g) {               /* m_0f890 */
    hang(a);
    a->x = (int2)((a->start_mode * 3 + 0x18) & 0xfffe);
    wr25c(a, 0xc, a->x + 0x10);
    a->box_x1 = a->x + 0x10; a->box_x0 = a->x + 4;
    wr25c(a, 0, a->floor);
    band(a, g, a->floor);
    a->appear_count = 1;
    a->unk_04a = 2; a->unk_048 = 2;
    if (a->delay != 0) a->delay = (int2)(6 * (*(int32_t *)(g->stage_info + 8) - a->delay));
    a->timer = 0;
    a->state = 1;
    a->unk_018 = 1;
    return 0;
}

static int bat_again(actor_t *a, gds *g) {              /* m_0fa66: the spare bat */
    int f;
    if (a->appear_count > 0 && g->time_left > 0x14) {
        a->appear_count--;
        hang(a);
        if (rd25c(a, 0xc) != 0) { a->x = (int2)rd25c(a, 0xc); wr25c(a, 0xc, 0); }
        a->box_x1 = a->x + 0x10; a->box_x0 = a->x + 4;
        f = smod((int)(RND(g) & 0xffff), 5);
        a->floor = (int2)f;
        wr25c(a, 0, f);
        band(a, g, f);
        a->timer = 0; a->delay = 0x28; a->unk_04e = 0;
        a->state = 1;
    }
    return 0;
}

static int bat_wait(actor_t *a, gds *g) {               /* m_0fb80 */
    if (g->unk_53a != 0) return 0;                       /* (just erased) */
    if (++a->timer >= a->delay) {
        a->state = 3; a->step = 0; a->unk_02a = 1;
        if (a->dir == -1) a->dir = (g->mario->x < a->x) ? 1 : 0;
        g->unk_054 = 0xd;
    }
    a->frame = 0xd;
    draw_frame(a, g);
    return 0;
}

static int bat_moving(actor_t *a, gds *g) {             /* m_0fc70 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

/* the 0..2..0 wing cycle */
static void flap(actor_t *a) {
    if (a->unk_02e == 0) {
        if (a->unk_02a < 2) a->unk_02a++; else { a->unk_02a--; a->unk_02e = 1; }
    } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
}

static void advance(actor_t *a) {
    if (a->dir == 1) { a->x = (int2)(a->x - (a->unk_036 & 0xfffe)); a->frame = (int2)(a->unk_02a + 4); }
    else             { a->x = (int2)(a->x + (a->unk_036 & 0xfffe)); a->frame = (int2)(a->unk_02a + 1); }
}

static int bat_fly(actor_t *a, gds *g) {                /* m_0fcbc */
    uint16_t *door;
    flap(a);
    if (--a->unk_02c <= 0) {
        a->unk_02c = 10;
        if (rd25c(a, 4) < a->y && a->y < rd25c(a, 8) && (RND(g) & 3) == 0) {
            a->x_max = (a->x_max == 0);
        } else if ((RND(g) & 3) == 0) {
            a->step = 1; a->unk_02a = 0;
        } else if ((a->x < 0x4c && a->dir == 1) || (a->x > 0x120 && a->dir == 0)) {
            if ((RND(g) & 3) != 0) { a->step = 1; a->unk_02a = 0; }
        } else if ((RND(g) & 3) == 0) {
            int f = rd25c(a, 0);
            if (((g->unk_826 >> 15) & 1) == 0) { if (f > 0) f--; }
            else if (f < 4) f++;
            wr25c(a, 0, f);
            band(a, g, f);
        }
    }
    if (a->unk_036 < 7) a->unk_036++;
    advance(a);
    if (a->x_min == 0) a->x_min = 0x5be416e4;
    if (a->x_max == 1) a->y = (int2)(a->y + (a->x_min & 3));
    else a->y = (int2)(a->y - (a->x_min & 3));
    a->x_min >>= 2;
    if (a->x > 0x154 && a->dir == 0) { a->x = 0x154; a->step = 2; a->unk_02a = 0; }
    else if (a->x < 0x18 && a->dir == 1) { a->x = 0x18; a->step = 2; a->unk_02a = 0; }
    a->cur_floor = (int2)rd25c(a, 0);
    if (a->y < rd25c(a, 4)) { a->x_max = 1; a->cur_floor = g->mario->floor; }
    else if (a->y > rd25c(a, 8)) { a->x_max = 0; a->cur_floor = g->mario->floor; }
    else {
        door = DOOR_CELL(a->cur_floor, a->x + 0xa);
        if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0xf) == 0) {
            *door = (*door & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
    }
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x10;
    a->box_y0 = a->y + 7; a->box_y1 = a->y + 0x10;
    return 0;
}

static int bat_slow(actor_t *a, gds *g) {               /* m_10142 */
    (void)g;
    if (a->unk_036 > 0) a->unk_036--;
    flap(a);
    advance(a);
    if (a->x > 0x154) { a->x = 0x154; a->unk_036 = 0; a->unk_02a = 0; }
    else if (a->x < 0x18) { a->x = 0x18; a->unk_036 = 0; a->unk_02a = 0; }
    if (a->unk_036 == 0) { a->step = 2; a->unk_02a = 0; }
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x10;
    return 0;
}

static int bat_turn(actor_t *a, gds *g) {               /* m_102a0 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 9 - a->unk_02a : a->unk_02a + 7);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1; a->unk_02c = 10; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int bat_hit(actor_t *a, gds *g) {                /* m_1033e: stomped */
    if (g->unk_496 == 0) {
        SCORE(g, 400);
        a->state = 4; a->step = 4; a->unk_02a = 10;
        a->unk_048--; a->unk_04a--;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int bat_destroy(actor_t *a, gds *g) {            /* m_103ae */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 400); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 10;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int bat_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int bat_gone(actor_t *a, gds *g) {               /* m_10550 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->dir = -1; }
    return 0;
}

static int bat_die1(actor_t *a, gds *g) {               /* m_10482 */
    a->frame = a->unk_02a; draw_frame(a, g); a->unk_02a++; a->step = 5; return 0;
}
static int bat_die2(actor_t *a, gds *g) {               /* m_104b6 */
    a->frame = a->unk_02a; draw_frame(a, g); a->unk_02a++; a->step = 6; return 0;
}
static int bat_fall(actor_t *a, gds *g) {               /* m_104ea */
    a->y += 0x10;
    if (a->y > 0xee) a->y = 0xee;
    a->frame = a->unk_02a;
    draw_frame(a, g);
    if (a->y > 0xdd) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int bat(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, bat_init);
        H(0, 2, bat_again);
        H(1, 2, bat_wait);
        H(3, 1, obj_erase);
        H(3, 2, bat_moving);
        H(3, 3, bat_hit);
        H(3, 4, bat_destroy);
        H(4, 1, obj_erase);
        H(4, 2, bat_dying);
        W(0, bat_fly);
        W(1, bat_slow);
        W(2, bat_turn);
        B(3, bat_gone);
        B(4, bat_die1);
        B(5, bat_die2);
        B(6, bat_fall);
    } else if (rd25c(a, 0xc) != 0) {                     /* the spare bat, hanging */
        if (g->unk_53a == 0) { if (a->cmd == 2) obj_draw(a, 0xd, rd25c(a, 0xc), 0xdc); }
        else wr25c(a, 0xc, 0);
    }
    return obj_dispatch(a, g);
}
