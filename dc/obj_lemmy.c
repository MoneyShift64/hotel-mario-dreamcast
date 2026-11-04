/* obj_lemmy.c - "lemmy", the hotel 4 boss, from L4_s10_sub.o (m_04368 .. m_05a40).
 * He rides his ball diagonally round the whole screen (not tied to a floor), bouncing
 * off the top/bottom (y 0x1d / 0xb9) and the side walls (x 0x12 / 0x140), turning when
 * Mario is behind him on his floor, opening closed doors he passes (1 in 4, door at
 * x+0x16), and now and then throwing one of up to four balls (lemmy_ap actors, type 0x33).
 * His speed grows by 2 every 141 ticks up to 8. A stomp stuns him (20 ticks, sinking);
 * a second stomp while stunned, a fireball or a star knocks him off: he drops to the
 * bottom and comes back after a shorter wait (delay -10 each time, down to 15).
 * He is never beaten for good: the stage is won the usual way (all doors shut).
 * unk_25c: int32 +0 = step to go back to after a turn, +4 = 1 going up / 0 down,
 *          int32 +8,+0xc,+0x10,+0x14 = his balls (actor index + 1; 0 = none).
 * x_min/x_max: 2-bit dx/dy streams for the swoop (step 1).
 * Frames: 1-4/5-8 riding, 9-14/15-20 throwing, 21-23 turning, 24-28 falling,
 * 29-32 stunned, 33-37 appearing.
 */
#include "obj_common.h"

#define BALL_OFS(k) (8 + 4 * (k))           /* k = 0..3 */

static int floor_of_y(int y) {                          /* m_05a40 */
    int v = y + 0x18;
    if (v < 0x48) return 0;
    if (v < 0x6c) return 1;
    if (v < 0x90) return 2;
    if (v < 0xb4) return 3;
    return 4;
}

static actor_t *ball(actor_t *a, gds *g, int k) {
    int32_t i = rd25c(a, BALL_OFS(k));
    return i > 0 ? &g->room->actors[i - 1] : NULL;
}

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_0598c */

static void set_boxes(actor_t *a) {
    a->box_x0 = a->x + 0xc; a->box_x1 = a->x + 0x22;
    a->box_y0 = a->y + 0x12; a->box_y1 = a->y + 0x24;
}

static void place_start(actor_t *a) {
    a->x = a->drawn_x0 = a->drawn_x1 = 0xb0;
    a->y = a->drawn_y0 = a->drawn_y1 = 0xd4;
    set_boxes(a);
}

static void face_mario(actor_t *a, gds *g) { a->dir = (g->mario->x < 0xb0) ? 1 : 0; }

static int lm_init(actor_t *a, gds *g) {                /* m_04368 */
    int i, k = 0;
    if (a->start_mode == 0) a->state = 1;
    else {
        place_start(a);
        a->cur_floor = a->floor;
        a->state = 3; a->step = 0;
    }
    for (i = 0; i < 4; i++) wr25c(a, BALL_OFS(i), 0);
    for (i = 0; i < 24 && k < 4 && g->room->actors[i].type != 0; i++)
        if (g->room->actors[i].type == 0x33) wr25c(a, BALL_OFS(k++), i + 1);
    a->unk_036 = 4;
    if (a->dir == -1) face_mario(a, g);
    a->delay = 0x2d;
    a->timer = a->delay;
    a->unk_018 = 1;
    a->unk_04a = a->unk_048 = -1;
    return 0;
}

static int lm_respawn(actor_t *a, gds *g) {             /* m_04718 */
    a->unk_04e = 0;
    a->timer = 0;
    if (a->delay > 0xf) a->delay -= 10;
    face_mario(a, g);
    a->state = 1;
    return 0;
}

static int lm_wait(actor_t *a, gds *g) {                /* m_0478c */
    if (g->unk_53a == 0 && a->timer++ > a->delay) {
        a->state = 2;
        place_start(a);
    }
    return 0;
}

static int lm_appear(actor_t *a, gds *g) {              /* m_04836 */
    a->frame = (int2)(a->unk_02a + 0x21);
    if (++a->unk_02a > 4) {
        a->frame = (a->dir == 1) ? 0x17 : 0x15;
        if (g->unk_53a == 0) {
            a->state = 3; a->step = 0;
            a->unk_02a = a->unk_02c = a->unk_02e = 0;
            wr25c(a, 4, 1);
        } else {
            a->state = 4; a->step = 4; a->unk_02a = 0x18;
        }
    }
    draw_frame(a, g);
    return 0;
}

static int lm_moving(actor_t *a, gds *g) {              /* m_048fa */
    CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return 0;
}

/* passing a closed door: 1 in 4 he opens it */
static void lm_doors(actor_t *a, gds *g) {
    uint16_t *door = DOOR_CELL(floor_of_y(a->y), a->x + 0x16);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 3) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
}

static void lm_speed_up(actor_t *a) {
    if (++a->unk_02c > 0x8c) { a->unk_02c = 0; if (a->unk_036 < 8) a->unk_036 += 2; }
}

/* send ball k out from his hand (speed v, x offset dx) */
static void throw_ball(actor_t *a, actor_t *b, int v, int dx) {
    b->unk_036 = (int2)v;
    b->dir = a->dir;
    b->x = b->drawn_x0 = b->drawn_x1 = (int2)(a->x + 0x16 + dx);
    b->y = b->drawn_y0 = b->drawn_y1 = (int2)(a->y + 0xe);
    b->box_x0 = b->box_x1 = b->box_y0 = b->box_y1 = 0;
    b->state = 3; b->step = 0; b->unk_02a = 0;
}

/* any ball at rest from slot k on? */
static int idle_ball_from(actor_t *a, gds *g, int k) {
    for (; k < 4; k++) { actor_t *b = ball(a, g, k); if (b && b->state == 0) return k; }
    return -1;
}

static void ride_frame(actor_t *a) {
    if (a->dir == 1) a->frame = (int2)(a->unk_02a + (a->unk_02e == 0 ? 5 : 0xf));
    else             a->frame = (int2)(a->unk_02a + (a->unk_02e == 0 ? 1 : 9));
}

static void wall_turn(actor_t *a) {
    if ((a->x > 0x140 && a->dir == 0) || (a->x < 0x12 && a->dir == 1)) {
        a->step = 2; wr25c(a, 0, 0); a->unk_02a = 0;
    }
}

static int lm_ride(actor_t *a, gds *g) {                /* m_04946: plain diagonal ride */
    mario_t *m = g->mario;
    lm_doors(a, g);
    if ((RND(g) & 0x1f) == 1) {
        a->step = 1;
        if (a->dir == rd25c(a, 4)) { a->x_min = 0xfaa550; a->x_max = 0x55aaf; }
        else                       { a->x_min = 0x55aaf; a->x_max = 0xfaa550; }
    }
    if (a->unk_02e == 0) a->x = (int2)(a->x + (a->dir == 1 ? -a->unk_036 : a->unk_036));
    ride_frame(a);
    if (rd25c(a, 4) == 1) a->y = (int2)(a->y - (a->unk_02e == 0 ? a->unk_036 : 4));
    else                  a->y = (int2)(a->y + (a->unk_02e == 0 ? a->unk_036 : 4));
    if (++a->unk_02a >= (a->unk_02e == 0 ? 4 : 6)) {
        a->unk_02a = 0;
        if (a->unk_02e == 0) {
            if ((RND(g) & 1) == 1 || floor_of_y(a->y) == m->floor)
                if (idle_ball_from(a, g, 0) >= 0) a->unk_02e = 1;
        } else {
            int k = idle_ball_from(a, g, 0);
            if (k >= 0) throw_ball(a, ball(a, g, k), (RND(g) & 3) * 2 + 8, a->dir == 1 ? -10 : 10);
            a->unk_02e = 0;
        }
    }
    lm_speed_up(a);
    if (floor_of_y(a->y) == m->floor &&
        ((a->dir == 0 && m->x < a->x) || (a->dir == 1 && a->x < m->x))) {
        a->step = 2; a->unk_02a = 0;
    }
    wall_turn(a);
    if (a->y < 0x1d) wr25c(a, 4, 0);
    else if (a->y > 0xb9) wr25c(a, 4, 1);
    a->cur_floor = m->floor;
    set_boxes(a);
    return 0;
}

static int lm_swoop(actor_t *a, gds *g) {               /* m_04eda: patterned swoop */
    int dx, dy, k = 0;
    lm_doors(a, g);
    lm_speed_up(a);
    dx = (int)((uint32_t)a->x_min & 3); dy = (int)((uint32_t)a->x_max & 3);
    a->x_min = (int4)((uint32_t)a->x_min >> 2);
    a->x_max = (int4)((uint32_t)a->x_max >> 2);
    if (a->x_min == 0 && a->x_max == 0) {
        if ((RND(g) & 3) == 1) a->step = 0;
        else if (a->dir == rd25c(a, 4)) {
            wr25c(a, 4, rd25c(a, 4) == 1 ? 0 : 1);
            a->x_min = 0x55aaf; a->x_max = 0xfaa550;
        } else {
            a->step = 2; a->unk_02a = 0; wr25c(a, 0, 1);
            a->x_min = 0xfaa550; a->x_max = 0x55aaf;
        }
    }
    a->x = (int2)(a->x + (a->dir == 1 ? -dx * 2 : dx * 2));
    ride_frame(a);
    a->y = (int2)(a->y + (rd25c(a, 4) == 1 ? -dy * 2 : dy * 2));
    if (++a->unk_02a >= (a->unk_02e == 0 ? 4 : 6)) {
        a->unk_02a = 0;
        if (a->unk_02e != 0) {
            k = idle_ball_from(a, g, 0);
            if (k >= 0) throw_ball(a, ball(a, g, k), 10, 0);
            else k = 4;
        }
        a->unk_02e = 0;
        if ((RND(g) & 1) == 1 && idle_ball_from(a, g, k) >= 0) a->unk_02e = 1;
    }
    wall_turn(a);
    if (a->y < 0x1d)       { wr25c(a, 4, 0); a->step = 0; a->unk_02a = 0; }
    else if (a->y > 0xb9)  { wr25c(a, 4, 1); a->step = 0; a->unk_02a = 0; }
    a->cur_floor = g->mario->floor;
    set_boxes(a);
    return 0;
}

static int lm_turn(actor_t *a, gds *g) {                /* m_053c4 */
    int v = rd25c(a, 0) == 0 ? a->unk_036 : 3;
    (void)g;
    a->y = (int2)(a->y + (rd25c(a, 4) == 1 ? -v : v));
    if (a->y < 0x1d) wr25c(a, 4, 0);
    else if (a->y > 0xb9) wr25c(a, 4, 1);
    a->box_y0 = a->y + 0x12; a->box_y1 = a->y + 0x24;
    a->frame = (int2)(a->dir == 1 ? 0x17 - a->unk_02a : a->unk_02a + 0x15);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1; a->unk_02e = 0;
        a->step = (int2)rd25c(a, 0);
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int lm_stunned(actor_t *a, gds *g) {             /* m_05518 */
    a->frame = (int2)(a->unk_02a + 0x1d);
    a->y += 1;
    a->cur_floor = g->mario->floor;
    a->box_y0 = a->y + 0x12; a->box_y1 = a->y + 0x24;
    if (++a->unk_02a > 3) a->unk_02a = 0;
    if (++a->unk_02c < 0x14) { if (a->unk_02c == 1) g->unk_054 = 0xc; }
    else { a->step = 0; a->unk_02a = a->unk_02c = a->unk_02e = 0; }
    return 0;
}

static int lm_hit(actor_t *a, gds *g) {                 /* m_055bc: stomped */
    if (g->unk_496 == 0) {
        if (a->step == 3) {
            a->state = 4; a->step = 4;
            a->box_y0 = a->y + 0x12;
            a->unk_02a = 0x18;
        } else { a->step = 3; a->unk_02a = 0; a->unk_02c = 0; }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int lm_destroy(actor_t *a, gds *g) {             /* m_05676: fireball / star */
    if (g->unk_496 == 0) {
        if (a->unk_04c != 0 || g->unk_53a != 0) a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0x18;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int lm_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_05704 */

static int lm_knock1(actor_t *a, gds *g) {              /* m_0573e */
    a->frame = a->unk_02a; draw_frame(a, g); a->unk_02a++; a->step = 5; return 0;
}

static int lm_knock2(actor_t *a, gds *g) {              /* m_05772 */
    a->frame = a->unk_02a; draw_frame(a, g); a->unk_02a++; a->step = 6; return 0;
}

static int lm_drop(actor_t *a, gds *g) {                /* m_057a6 */
    a->y += 0x10;
    if (a->y + 0x2e > 0x102) a->y = 0xd4;
    a->frame = a->unk_02a;
    draw_frame(a, g);
    if (++a->unk_02a > 0x1c) a->unk_02a -= 3;
    if (a->y > 0xc3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

static int lm_gone(actor_t *a, gds *g) {                /* m_05830 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

int lemmy(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, lm_init);
        H(0, 2, lm_respawn);
        H(1, 2, lm_wait);
        H(2, 1, obj_erase);
        H(2, 2, lm_appear);
        H(3, 1, obj_erase);
        H(3, 2, lm_moving);
        H(3, 3, lm_hit);
        H(3, 4, lm_destroy);
        H(4, 1, obj_erase);
        H(4, 2, lm_dying);
        W(0, lm_ride);
        W(1, lm_swoop);
        W(2, lm_turn);
        W(3, lm_stunned);
        B(3, lm_gone);
        B(4, lm_knock1);
        B(5, lm_knock2);
        B(6, lm_drop);
    }
    return obj_dispatch(a, g);
}
