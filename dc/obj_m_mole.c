/* obj_m_mole.c - "m_mole" (Monty Mole: Larry's hotel, hotel 8), from L3_s04_sub.o
 * (m_0826c .. m_094ac). Pops out of the wall at a door position, then scurries along the
 * floor speeding up, sometimes skidding round, hopping, or bumping into the walls.
 * With start_mode set it reappears at a random door after a random delay.
 * Frames: 1-6 run, 7-12 skid, 13-17 destroyed, 18-19 hit, 20-22 out of the wall,
 * 23-27 hop, 28-33 wall bump, 34-36 dizzy.
 */
#include "obj_common.h"

static void at_door(actor_t *a, gds *g) {
    a->y = a->drawn_y1 = a->drawn_y0 = (int2)(g->floor_y[a->floor] - 0x20);
    a->cur_floor = a->floor;
    a->box_y0 = a->y + 0xc;
    a->box_y1 = a->y + 0x20;
    a->x = g->door_x[a->col];
    a->box_x1 = a->x + 0x18;
    a->box_x0 = a->x + 0xc;
}

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_094ac */

static int mole_init(actor_t *a, gds *g) {              /* m_0826c */
    a->speed = HOTEL(g) == 8 ? 8 : 6;
    at_door(a, g);
    a->appear_count = 3;
    a->unk_04a = a->unk_048 = a->appear_count + 1;
    a->delay = 0x14;
    a->timer = 0;
    a->state = 1;
    return 0;
}

static int mole_again(actor_t *a, gds *g) {             /* m_0854e */
    if (a->appear_count > 0 && g->time_left > 0x14) {
        a->appear_count--;
        if (a->start_mode != 0) {
            a->floor = (int2)smod((int)(RND(g) & 0xffff), 5);
            a->col = (int2)smod((int)(RND(g) & 0xffff), 7);
            a->delay = (int2)((RND(g) & 0x7f) + 10);
        }
        at_door(a, g);
        a->timer = 0;
        a->state = 1;
        a->unk_04e = 0;
    }
    return 0;
}

static int mole_wait(actor_t *a, gds *g) {              /* m_08688 */
    int mx, ax, dist;
    if (g->unk_53a != 0 || ++a->timer < a->delay) return 0;
    if (g->mario->floor != a->cur_floor && a->start_mode == 0) return 0;
    mx = g->mario->x + 0x10;
    ax = a->col * 0x2c + 0x3c;
    if (ax < mx) { if (a->dir == -1) a->dir = 0; dist = mx - ax; }
    else         { if (a->dir == -1) a->dir = 1; dist = ax - mx; }
    if (dist > 0x48 || g->mario->floor != a->cur_floor) { a->state = 2; a->unk_02a = 0; }
    return 0;
}

static int mole_emerge(actor_t *a, gds *g) {            /* m_087a4 */
    a->frame = (int2)(a->unk_02a + 0x14);
    if (++a->unk_02a > 2) {
        if (g->unk_53a == 0) { a->unk_02a = 0; a->state = 3; a->step = 3; }
        else { a->state = 4; a->step = 4; a->unk_02a = 0xd; a->unk_02c = 0; }
    }
    draw_frame(a, g);
    return 0;
}

static int mole_moving(actor_t *a, gds *g) {            /* m_08828 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g);
    return r;
}

static void walls(actor_t *a) {
    if (a->x >= 0x145) { a->x = 0x144; a->step = 5; a->unk_02a = 0; }
    else if (a->x < 0x18) { a->x = 0x18; a->step = 5; a->unk_02a = 0; }
}

static int mole_run(actor_t *a, gds *g) {               /* m_08874 */
    uint16_t *door;
    if (--a->unk_02c < 1) {
        a->unk_02c = 1;
        if (a->unk_02e == 0) {
            if (a->unk_02a < 2) a->unk_02a++; else { a->unk_02a--; a->unk_02e = 1; }
        } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
        else a->unk_02a--;
    }
    if (a->unk_036 < a->speed + 1) a->unk_036++;    /* speeding up */
    if (a->dir == 1) { a->x -= a->unk_036 & 0xfffe; a->frame = (int2)(a->unk_02a + 4); }
    else             { a->x += a->unk_036 & 0xfffe; a->frame = (int2)(a->unk_02a + 1); }
    if (a->x_min-- < 2) {                               /* every 10 ticks: a decision */
        a->x_min = 10;
        if ((RND(g) & 3) == 1) { a->step = 1; a->unk_02a = 0; }
        else if (a->x < 0x6f || a->x > 0xed || (RND(g) & 3) != 1) {
            if (((a->x < 0x4c && a->dir == 1) || (a->x > 0x110 && a->dir == 0)) && (RND(g) & 3) != 1) {
                a->step = 1; a->unk_02a = 0;
            }
        } else {                                        /* a hop */
            a->unk_02a = 0;
            a->frame = (int2)(a->dir == 1 ? 6 : 3);
            a->x_max = 0x3a516b;
            a->step = 2;
        }
    }
    walls(a);
    a->box_x0 = a->x + 0xc;
    a->box_x1 = a->x + 0x18;
    door = DOOR_CELL(a->cur_floor, a->x + 0x12);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0xf) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
    return 0;
}

static int mole_skid(actor_t *a, gds *g) {              /* m_08e3c */
    (void)g;
    if (a->unk_036 > 0) a->unk_036--;
    if (a->unk_036 == 5 || a->unk_036 == 0) a->unk_02a++;
    if (a->dir == 1) { a->x -= a->unk_036 & 0xfffe; a->frame = (int2)(a->unk_02a + 10); }
    else             { a->x += a->unk_036 & 0xfffe; a->frame = (int2)(a->unk_02a + 7); }
    if (a->x >= 0x145) { a->x = 0x144; a->step = 5; a->unk_02a = 0; }
    else if (a->x < 0x18) { a->x = 0x18; a->step = 5; a->unk_02a = 0; }
    else if (a->unk_02a > 1) {
        a->unk_02a = 1; a->x_min = 10; a->step = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    a->box_x0 = a->x + 0xc;
    a->box_x1 = a->x + 0x18;
    return 0;
}

static int mole_hop(actor_t *a, gds *g) {               /* m_08c54 */
    (void)g;
    if (++a->unk_02a < 0xc) {
        int dy = (int)(a->x_max & 3) * 2;
        a->y += (a->unk_02a < 6) ? -dy : dy;
        a->x_max >>= 2;
    } else { a->unk_02a = 1; a->step = 0; }
    a->x += (a->dir == 1) ? -a->speed : a->speed;
    a->box_x0 = a->x + 0xc; a->box_x1 = a->x + 0x18;
    a->box_y0 = a->y + 0xc; a->box_y1 = a->y + 0x20;
    return 0;
}

static int mole_out_1(actor_t *a, gds *g) {             /* m_08ba6 */
    a->frame = (int2)(a->unk_02a + 0x17);
    if (++a->unk_02a > 2) { a->step = 4; a->unk_02a = 0; g->unk_054 = 0xc; }
    return 0;
}

static int mole_out_2(actor_t *a, gds *g) {             /* m_08bec */
    (void)g;
    a->frame = (int2)(a->unk_02a + 0x1a);
    if (++a->unk_02a > 2) {
        a->frame = (a->dir == 1) ? 10 : 7;
        a->step = 0; a->unk_02a = 1; a->x_min = 10; a->unk_036 = 0;
    }
    return 0;
}

static int mole_bump(actor_t *a, gds *g) {              /* m_08d18 */
    a->frame = (int2)(a->unk_02a + 0x1c + (a->dir == 1 ? 3 : 0));
    if (++a->unk_02a < 3) {
        if (a->frame == 0x1c || a->frame == 0x1f) g->unk_054 = 0xe;
    } else { a->step = 6; a->unk_02a = 0; a->x_min = 10; }
    return 0;
}

static int mole_dizzy(actor_t *a, gds *g) {             /* m_08daa */
    (void)g;
    a->frame = (int2)(a->unk_02a + 0x22);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->x_min-- < 2) {
        if (a->dir == 1) { a->dir = 0; a->frame = 7; }
        else { a->dir = 1; a->frame = 10; }
        a->step = 0; a->unk_02a = 1; a->x_min = 10; a->unk_036 = 0;
    }
    return 0;
}

static int mole_hit(actor_t *a, gds *g) {               /* m_08f92 */
    if (g->unk_496 == 0) {
        SCORE(g, 300);
        a->unk_048--; a->unk_04a--;
        a->state = 4;
        if (a->step == 2) { a->step = 4; a->unk_02a = 0xd; a->unk_02c = 0; }
        else { a->step = 1; a->unk_02a = 0; }
    } else a->unk_04e = a->cmd;
    return 0;
}

static int mole_destroy(actor_t *a, gds *g) {           /* m_0902c */
    if (g->unk_496 == 0) {
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 300); a->unk_04a--; }
        else a->unk_04c = 0;
        a->state = 4; a->step = 4; a->unk_02a = 0xd; a->unk_02c = 0;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int mole_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int mole_hit_1(actor_t *a, gds *g) {             /* m_092c2 */
    a->frame = 0x12; draw_frame(a, g);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}
static int mole_hit_2(actor_t *a, gds *g) {             /* m_09308 */
    a->frame = 0x13; draw_frame(a, g);
    if (++a->unk_02a > 2) { a->step = 3; a->unk_02a = 2; }
    return 0;
}
static int mole_gone(actor_t *a, gds *g) {              /* m_09354 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->dir = -1; }
    return 0;
}
static int mole_flip(actor_t *a, gds *g) {              /* m_09104 / m_09138 */
    a->frame = a->unk_02a; draw_frame(a, g);
    a->unk_02a++;
    a->step = (int2)(a->step == 4 ? 5 : 6);
    return 0;
}
static int mole_fall(actor_t *a, gds *g) {              /* m_0916c / m_091de / m_09250 */
    a->y += 0x10;
    a->frame = a->unk_02a; draw_frame(a, g);
    if (++a->unk_02c > 0) {
        a->unk_02c = 0;
        if (a->step == 8) { a->unk_02a -= 2; a->step = 6; }
        else { a->unk_02a++; a->step++; }
    }
    if (a->y > 0xd1) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int m_mole(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, mole_init);
        H(0, 2, mole_again);
        H(1, 2, mole_wait);
        H(2, 1, obj_erase);
        H(2, 2, mole_emerge);
        H(3, 1, obj_erase);
        H(3, 2, mole_moving);
        H(3, 3, mole_hit);
        H(3, 4, mole_destroy);
        H(4, 1, obj_erase);
        H(4, 2, mole_dying);
        W(0, mole_run);
        W(1, mole_skid);
        W(2, mole_hop);
        W(3, mole_out_1);
        W(4, mole_out_2);
        W(5, mole_bump);
        W(6, mole_dizzy);
        B(1, mole_hit_1);
        B(2, mole_hit_2);
        B(3, mole_gone);
        B(4, mole_flip);
        B(5, mole_flip);
        B(6, mole_fall);
        B(7, mole_fall);
        B(8, mole_fall);
    }
    return obj_dispatch(a, g);
}
