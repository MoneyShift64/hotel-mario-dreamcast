/* obj_wiggler.c - "wiggler" (type 0x23): a five-part caterpillar, five actors in a row in
 * the stage's actor list, a[4] the head (L1_s06_sub.o m_1107c .. m_12e06; L1_s08_sub's
 * copy is identical).
 *
 * Links (pointers in the original; here actor index + 1, 0 = none):
 *   x_min       the next part towards the tail   (a[i] -> a[i-1], a[0] -> none)
 *   x_max       the part before it               (a[i] -> a[i+1], a[4] -> none)
 *   unk_25c +0  the first head, a[4]            +4  the tail, a[0]
 *           +8  this part's delay: a[4] 0, a[3] 6, a[2] 12, a[1] 18, a[0] 24 (hotel 8: x3 / 12)
 *           +0xc 1 = this part is the head      +0x10 1 = turning round to chase Mario
 *
 * The head waits for Mario to be more than 72 pixels away (or on another floor), opens its
 * door and the parts come out one after another (each after its delay). The head pops open
 * any closed door it passes (1 in 16), and when Mario is behind it on its floor the whole
 * train turns round, part by part. Mario can only stomp the head (100 points): the part
 * behind it becomes the new head; when only the tail is left behind it, both go (another
 * 100). Touching any other part hurts him. A fireball or star kills the head and every part
 * behind it (100 each), all falling off the screen; then a[4] brings the whole wiggler back
 * (appear_count times).
 * Frames: 1-5/6-10 body walking, 11-15 body falling, 16-17 head stomped / tail going,
 * 18-19 body out of the door, 20-21 head falling, 22-25 head turning/stomped/out,
 * 26-29 head walking.
 */
#include "obj_common.h"

#define WG_HEAD0  0
#define WG_TAIL   4
#define WG_DELAY  8
#define WG_ISHEAD 0xc
#define WG_TURN   0x10

static actor_t *link_of(gds *g, int32_t v) { return v ? &g->room->actors[v - 1] : NULL; }
static int32_t link_to(gds *g, actor_t *p) { return p ? (int32_t)(p - g->room->actors) + 1 : 0; }
#define NEXT(g, p) link_of(g, (p)->x_min)
#define PREV(g, p) link_of(g, (p)->x_max)
#define IS_HEAD(p) (rd25c(p, WG_ISHEAD) != 0)

static void draw_frame(actor_t *a, gds *g, int f) { draw_at_xy(a, g, f); }

static void place(actor_t *p, actor_t *a, gds *g) {   /* (positions from part a) */
    p->state = 1;
    p->cur_floor = a->floor;
    p->y = p->drawn_y0 = p->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x19);
    p->box_y0 = a->y + 4; p->box_y1 = a->y + 0x19;
    p->x = g->door_x[a->col];
    p->box_x0 = a->x + 10; p->box_x1 = a->x + 0x16;
}

static int wg_init(actor_t *a, gds *g) {                /* m_1107c */
    int i, mul = HOTEL(g) == 8 ? 3 : 6;
    a->speed = HOTEL(g) == 8 ? 4 : 2;
    place(a, a, g);
    if (a->x_min != 0 || (a - g->room->actors) + 4 >= 24) return 0;
    for (i = 1; i < 5; i++) if (a[i].type != 0x23) return 0;
    a[4].appear_count--;
    wr25c(&a[4], WG_ISHEAD, 1);
    for (i = 4; i > 0; i--) {
        a[i].x_min = link_to(g, &a[i - 1]);
        a[i - 1].x_max = link_to(g, &a[i]);
        wr25c(&a[i], WG_DELAY, mul * (4 - i));
        wr25c(&a[i], WG_HEAD0, link_to(g, &a[4]));
        wr25c(&a[i], WG_TAIL, link_to(g, a));
        a[i].unk_04a = a[i].unk_048 = (int2)(a[4].appear_count + 1);
    }
    wr25c(a, WG_DELAY, mul << 2);
    wr25c(a, WG_HEAD0, link_to(g, &a[4]));
    wr25c(a, WG_TAIL, link_to(g, a));
    a->unk_04a = a->unk_048 = (int2)(a[4].appear_count + 1);
    return 0;
}

static int wg_again(actor_t *a, gds *g) {               /* m_1146e: state 0 cmd 2, the first head */
    actor_t *p;
    if (a != link_of(g, rd25c(a, WG_HEAD0)) || !IS_HEAD(a) || a->timer++ <= a->delay) return 0;
    a->timer = 0;
    if (a->appear_count-- > 0) {
        for (p = a; p; p = NEXT(g, p)) {
            place(p, a, g);
            p->timer = 0;
            wr25c(p, WG_TURN, 0);
            p->unk_04e = 0;
        }
    }
    return 0;
}

static int wg_wait(actor_t *a, gds *g) {                /* m_11562 */
    uint16_t *c;
    actor_t *p;
    if (g->unk_53a != 0 || a != link_of(g, rd25c(a, WG_HEAD0)) || !IS_HEAD(a)) return 0;
    if (door_far_from_mario(a, g, a->x + 0x10, 0xe) > 0x48 || a->floor != g->mario->floor) {
        c = &CELL(g, a->floor, a->col);
        if ((*c & 0xf000) != 0x2000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
        for (p = a; p; p = NEXT(g, p)) { p->state = 2; p->step = 0; p->dir = a->dir; }
    }
    return 0;
}

static int wg_emerge(actor_t *a, gds *g) {              /* m_116bc */
    uint16_t *c = &CELL(g, a->floor, a->col);
    int head = IS_HEAD(a);
    switch (a->step) {
    case 0:
        *c |= 4;
        if (a->timer++ >= rd25c(a, WG_DELAY)) {
            a->timer = 0;
            a->x = g->door_x[a->col];
            a->unk_02a = !head ? 0x12 : a->dir == 1 ? 0x19 : 0x17;
            a->step = 1;
        }
        break;
    case 1:
        a->unk_02a = !head ? 0x13 : a->dir == 1 ? 0x18 : 0x16;
        a->step = 2;
        break;
    case 2:
        if (a == link_of(g, rd25c(a, WG_TAIL))) {
            if ((*c & 0xf000) == 0x2000) *c = (*c & 0xfffb) | 2;
            else { *c = ((*c & 0xfffb) & 0xfff) | 0x2009; g->unk_054 = 2; }
        }
        a->unk_02a = !head ? 1 : a->dir == 1 ? 0x1c : 0x1a;
        if (head) a->y -= 8;
        a->step = 0; a->state = 3;
        break;
    }
    if (a->step != 0 || a->state != 2) draw_frame(a, g, a->unk_02a);
    return 0;
}

static int wg_moving(actor_t *a, gds *g) {              /* m_11952 */
    int r = CALL(a->walk_steps[a->step], a, g);
    draw_frame(a, g, a->frame);
    return r;
}

static void walls(actor_t *a) {
    if (a->x >= 0x13f) { a->x = 0x13e; a->dir = 1; a->unk_02a = 1; a->unk_02c = 1; }
    else if (a->x < 0x22) { a->x = 0x22; a->dir = 0; a->unk_02a = 1; a->unk_02c = 1; }
}

static int wg_walk(actor_t *a, gds *g) {                /* m_11a2e */
    int head = IS_HEAD(a);
    mario_t *m = g->mario;
    actor_t *p;
    a->unk_02c--;
    if (!head) {
        if (a->unk_02c < 1) { a->unk_02c = 1; walk_cycle(a); }
    } else if (a->unk_02c < 1) { a->unk_02c = 2; a->unk_02a = (a->unk_02a == 0); }
    else if (a->unk_02a > 1) a->unk_02a = 0;
    if (a->dir == 1) { a->x -= a->speed; a->frame = (int2)(a->unk_02a + (head ? 0x1c : 6)); }
    else             { a->x += a->speed; a->frame = (int2)(a->unk_02a + (head ? 0x1a : 1)); }
    if (rd25c(a, WG_TURN) == 0) walls(a);
    else if (a->timer++ >= rd25c(a, WG_DELAY)) {
        wr25c(a, WG_TURN, 0);
        a->dir = (a->dir == 1) ? 0 : 1;
    }
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x16;
    a->box_y0 = a->y + 4;  a->box_y1 = a->y + 0x19;
    if (!head) {
        p = PREV(g, a);
        if (p && IS_HEAD(p)) a->box_y0 += 8;
    } else {
        uint16_t *door;
        a->box_y0 += 8;
        door = DOOR_CELL(a->cur_floor, a->x + 0x10);
        if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 0xf) == 1) {
            *door = (*door & 0xfff) | 0x2009;
            g->unk_054 = 2;
        }
        if (m->state_group == 0 && m->floor == a->cur_floor &&
            ((m->x < a->x && a->dir == 0) || (a->x < m->x && a->dir == 1))) {
            int all = 1;
            for (p = a; p; p = NEXT(g, p))
                if (p->dir != a->dir || rd25c(p, WG_TURN) != 0 || p->step != 0 || p->state != 3) all = 0;
            if (all)
                for (p = a; p; p = NEXT(g, p)) {
                    wr25c(p, WG_TURN, 1);
                    p->timer = (int2)rd25c(a, WG_DELAY);
                }
        }
    }
    return 0;
}

static int wg_grow1(actor_t *a, gds *g) {               /* m_11e9a: becoming the head */
    (void)g;
    a->x += (a->dir == 1) ? -a->speed : a->speed;
    a->frame = 0x10;
    walls(a);
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x16;
    a->step = 2;
    return 0;
}

static int wg_grow2(actor_t *a, gds *g) {               /* m_11f6c */
    (void)g;
    if (a->dir == 1) { a->x -= a->speed; a->frame = 0x18; }
    else             { a->x += a->speed; a->frame = 0x16; }
    walls(a);
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x16;
    a->step = 0;
    return 0;
}

static int wg_hit(actor_t *a, gds *g) {                 /* m_1203c: cmd 3 */
    actor_t *n;
    if (!IS_HEAD(a)) return 1;                           /* a body part: Mario is hurt */
    if (g->unk_496 != 0) { a->unk_04e = a->cmd; return 0; }
    SCORE(g, 100);
    a->unk_048--; a->unk_04a--;
    a->state = 4; a->step = 4; a->unk_02a = 0;
    n = NEXT(g, a);
    if (n && n->x_min == 0) {                            /* only the tail left: it goes too */
        actor_t *h0 = link_of(g, rd25c(a, WG_HEAD0));
        if (h0) wr25c(h0, WG_ISHEAD, 1);
        SCORE(g, 100);
        n->unk_048--; n->unk_04a--;
        n->state = 4; n->step = 1; n->unk_02a = 0;
    }
    return 0;
}

static void door_shut_behind(actor_t *a, gds *g) {
    uint16_t *c = &CELL(g, a->floor, a->col);
    if ((*c & 0xf000) == 0x2000) *c = (*c & 0xfffb) | 2;
    else *c = ((*c & 0xfffb) & 0xfff) | 0x2009;
}

static int wg_destroy(actor_t *a, gds *g) {             /* m_12114: cmd 4 */
    actor_t *h, *h0;
    if (g->unk_496 != 0) { a->unk_04e = a->cmd; return 0; }
    for (h = link_of(g, rd25c(a, WG_HEAD0)); h && !IS_HEAD(h); h = NEXT(g, h)) ;
    if (!h) { h = link_of(g, rd25c(a, WG_HEAD0)); if (!h) h = a; wr25c(h, WG_ISHEAD, 1); }
    if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 100); h->unk_04a--; }
    else h->unk_04c = 0;
    h->state = 4; h->step = 7;
    h->unk_02a = (h->dir == 1) ? 0x15 : 0x14;
    h->unk_02c = 0;
    wr25c(h, WG_ISHEAD, 0);
    h0 = link_of(g, rd25c(h, WG_HEAD0));
    if (h0) wr25c(h0, WG_ISHEAD, 1);
    for (h = NEXT(g, h); h; h = NEXT(g, h)) {
        if (h->state == 2) door_shut_behind(a, g);
        if (a->unk_04c == 0 && g->unk_53a == 0) { SCORE(g, 100); h->unk_04a--; }
        else h->unk_04c = 0;
        h->state = 4; h->step = 7; h->unk_02a = 0xb; h->unk_02c = 0;
    }
    return 0;
}

static int wg_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_12316 */

static int wg_tail1(actor_t *a, gds *g) {               /* m_12850 */
    draw_frame(a, g, 0x10);
    if (++a->unk_02a > 0) a->step = 2;
    return 0;
}

static int wg_tail2(actor_t *a, gds *g) {               /* m_12a7e */
    draw_frame(a, g, 0x11);
    if (++a->unk_02a > 3) { a->step = 3; a->unk_02a = 3; }
    return 0;
}

static int wg_gone(actor_t *a, gds *g) {                /* m_12cb4 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

static int wg_stomp1(actor_t *a, gds *g) {              /* m_12910 */
    a->y += 8;
    draw_frame(a, g, a->dir == 1 ? 0x18 : 0x16);
    if (++a->unk_02a > 0) a->step = 5;
    return 0;
}

static int wg_stomp2(actor_t *a, gds *g) {              /* m_12b44 */
    draw_frame(a, g, a->dir == 1 ? 0x19 : 0x17);
    if (++a->unk_02a > 3) { a->step = 6; a->unk_02a = 3; }
    return 0;
}

static int wg_stomp_end(actor_t *a, gds *g) {           /* m_12cee: the next part takes over */
    actor_t *n;
    if (--a->unk_02a != 0) return 0;
    n = NEXT(g, a);
    a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1;
    wr25c(a, WG_ISHEAD, 0);
    if (n && n->x_min != 0) {
        wr25c(n, WG_ISHEAD, 1);
        n->y -= 8; n->box_y1 -= 8;
        n->unk_02a = 1; n->step = 1;
    }
    return 0;
}

static void fall_frame_up(actor_t *a) { if (a->unk_02a != 0x15 && a->unk_02a != 0x14) a->unk_02a++; }

static int wg_shot1(actor_t *a, gds *g) {               /* m_12350 */
    draw_frame(a, g, a->unk_02a);
    fall_frame_up(a);
    a->step = 8;
    return 0;
}

static int wg_shot2(actor_t *a, gds *g) {               /* m_12432 */
    draw_frame(a, g, a->unk_02a);
    fall_frame_up(a);
    a->step = 9;
    return 0;
}

static void fall(actor_t *a, gds *g, int next, int back) {   /* m_12514 / m_12628 / m_1273c */
    a->y += 0x10;
    draw_at_y(a, g, a->unk_02a);
    if (++a->unk_02c > 0) {
        a->unk_02c = 0;
        if (a->unk_02a != 0x15 && a->unk_02a != 0x14) a->unk_02a += back ? -2 : 1;
        a->step = (int2)next;
    }
    if (a->y > 0xd8) { a->step = 3; a->unk_02a = 3; }
}
static int wg_fall9(actor_t *a, gds *g)  { fall(a, g, 10, 0); return 0; }
static int wg_fall10(actor_t *a, gds *g) { fall(a, g, 11, 0); return 0; }
static int wg_fall11(actor_t *a, gds *g) { fall(a, g, 9, 1); return 0; }

int wiggler(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, wg_init);
        H(0, 2, wg_again);
        H(1, 2, wg_wait);
        H(2, 1, obj_erase);
        H(2, 2, wg_emerge);
        H(3, 1, obj_erase);
        H(3, 2, wg_moving);
        H(3, 3, wg_hit);
        H(3, 4, wg_destroy);
        H(4, 1, obj_erase);
        H(4, 2, wg_dying);
        W(0, wg_walk);
        W(1, wg_grow1);
        W(2, wg_grow2);
        B(1, wg_tail1);
        B(2, wg_tail2);
        B(3, wg_gone);
        B(4, wg_stomp1);
        B(5, wg_stomp2);
        B(6, wg_stomp_end);
        B(7, wg_shot1);
        B(8, wg_shot2);
        B(9, wg_fall9);
        B(10, wg_fall10);
        B(11, wg_fall11);
    }
    return obj_dispatch(a, g);
}
