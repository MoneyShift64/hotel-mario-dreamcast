/* obj_iggy.c - "iggy" (type 0x0f): the boss of the last stage (hotel 8 stage 15), from
 * L8_s15_sub.o (m_00550 .. m_01e1c).
 *
 * Drops in at the top left or right (whichever is away from Mario), walks floor 0 and
 * then hops between floors (x_max = the floor it is on). Between walks it picks one of:
 *   - a jump up or down a floor towards Mario (steps 3-8),
 *   - a breath of flame (igg_jet) and a fireball (bow_fb) thrown ahead (step 9),
 *   - a chain (igg_l links) dropped from the top of the screen above Mario (steps 12-13).
 * A stomp stuns it (the chain is pulled back); a second stomp while stunned, or one
 * landing in the middle of a jump, knocks it off the screen. A fireball or star does
 * that too. It always comes back, sooner each time (delay 20, -5 down to 10). It can't
 * be beaten for good: the stage ends the usual way. When the stage is over (unk_53a)
 * a fireball / star makes it do its exit (steps 15-19) and jump off the screen.
 * Links: unk_25c+4 jet, +8 fireball, +0xc first chain link (actor index + 1, 0 = none);
 * int32 unk_25c+0x10 = 1 when leaving: the jump ignores the floors.
 * Frames 1-10 walk, 11-13 turn, 14-15 breathe, 16-19 jump, 20-21 chain, 22-25 exit,
 * 26-29 knocked off, 30-32 arriving.
 */
#include "obj_common.h"

#define LINK_JET   4
#define LINK_FB    8
#define LINK_CHAIN 0xc
#define LEAVING    0x10

static actor_t *link_of(actor_t *a, gds *g, int off) {
    int32_t i = rd25c(a, off);
    return (i > 0 && i <= 24) ? &g->room->actors[i - 1] : NULL;
}

static void set_link(actor_t *a, gds *g, int off, int type) {
    int i;
    for (i = 0; i < 24 && g->room->actors[i].type != 0; i++)
        if (g->room->actors[i].type == type) { wr25c(a, off, i + 1); return; }
}

static void ig_draw(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }        /* m_01e1c */

static void ig_box(actor_t *a) {
    a->box_x0 = a->x + 8; a->box_x1 = a->x + 0x1e;
    a->box_y0 = a->y + 8; a->box_y1 = a->y + 0x29;
}

static int face_frame(actor_t *a, int left, int right) { return a->dir == 1 ? left : right; }

static int mario_behind(actor_t *a, gds *g) {
    mario_t *m = g->mario;
    return (m->x < a->x && a->dir == 0) || (a->x < m->x && a->dir == 1);
}

/* pull the chain back up if it is hanging */
static void chain_up(actor_t *a, gds *g) {
    actor_t *c = link_of(a, g, LINK_CHAIN);
    if (c && c->state == 3) { c->state = 4; c->step = 0; c->unk_02a = 0; }
}

static void back_to_walk(actor_t *a) {
    a->step = 0; a->unk_02a = 1; a->unk_02e = 0; a->x_min = 0x14;
}

static void knocked_off(actor_t *a) {
    a->state = 4; a->step = 4; a->unk_02a = 0x1a; a->unk_02c = 0;
}

static int ig_init(actor_t *a, gds *g) {                /* m_00550 */
    actor_t *c;
    a->state = 1;
    a->delay = 0x14;
    set_link(a, g, LINK_JET, 0x3e);
    set_link(a, g, LINK_FB, 0x3a);
    set_link(a, g, LINK_CHAIN, 0x3f);
    if ((c = link_of(a, g, LINK_CHAIN)) != NULL) c->x_min = 1;
    g->unk_530 = 0;
    a->unk_04a = -1; a->unk_048 = -1;
    return 0;
}

static int ig_away(actor_t *a, gds *g) {                /* m_008c0: off screen, waiting */
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->delay > 10) a->delay -= 5;
        a->state = 1;
        a->unk_04e = 0;
    }
    g->unk_530 = 0;
    return 0;
}

static int ig_appear(actor_t *a, gds *g) {              /* m_00924 */
    if (g->unk_53a != 0) return 0;
    if (g->mario->x + 0xe < 0xc1) { a->dir = 1; a->x = 0x142; }
    else { a->dir = 0; a->x = 0x18; }
    a->drawn_x0 = a->drawn_x1 = a->x;
    a->y = a->drawn_y0 = a->drawn_y1 = 2;
    a->cur_floor = 0;
    a->x_max = 0;
    ig_box(a);
    a->unk_02a = 0;
    a->state = 2; a->step = 0;
    return 0;
}

static int ig_arrive(actor_t *a, gds *g) {              /* m_00a06 */
    a->frame = (int2)(a->unk_02a + 0x1e);
    if (++a->unk_02a > 2) {
        a->frame = 0x1a; a->unk_02a = 0;
        a->step = 0xe; a->state = 3;
        if (g->unk_53a != 0) knocked_off(a);
    }
    ig_draw(a, g);
    return 0;
}

static int ig_moving(actor_t *a, gds *g) {              /* m_00a96 */
    CALL(a->walk_steps[a->step], a, g);
    ig_draw(a, g);
    return 0;
}

static int ig_walk(actor_t *a, gds *g) {                /* m_00ae2 */
    uint16_t *door;
    walk_cycle(a);
    if (a->x_min-- < 2) {
        if ((a->x < 0xaf || a->dir != 0) && (a->x > 0xad || a->dir != 1) && a->x_max != 0) {
            a->step = 9; a->unk_02a = 0;
        } else {
            if ((RND(g) & 1) == 0) { a->step = 3; a->unk_02a = 0; }
            else { a->step = 0xc; a->unk_02a = 0; }
        }
    }
    if (a->dir == 1) { a->x -= 6; a->frame = (int2)(a->unk_02a + 6); }
    else             { a->x += 6; a->frame = (int2)(a->unk_02a + 1); }
    if (a->x > 0x142) { a->x = 0x142; a->step = 1; a->unk_02a = 0; }
    else if (a->x < 0x18) { a->x = 0x18; a->step = 1; a->unk_02a = 0; }
    door = DOOR_CELL(a->x_max, a->x + 0x12);
    if (door && (*door & 0xf000) == 0x1000 && (RND(g) & 3) == 1) {
        *door = (*door & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
    ig_box(a);
    a->cur_floor = (int2)a->x_max;
    return 0;
}

static int ig_turn(actor_t *a, gds *g) {                /* m_0164e */
    (void)g;
    a->frame = (int2)(a->dir == 1 ? 0xd - a->unk_02a : a->unk_02a + 0xb);
    if (++a->unk_02a > 2) {
        a->unk_02a = 1;
        a->step = (a->unk_02e == -1) ? 4 : 0;
        a->unk_02e = 0;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int ig_stunned(actor_t *a, gds *g) {             /* m_01712 */
    a->frame = (int2)face_frame(a, 0x12, 0x10);
    if (a->unk_02a == 0) chain_up(a, g);
    if (++a->unk_02a > 4) { a->step = 0; a->unk_02e = 0; a->unk_02a = 1; a->unk_02c = 0; }
    return 0;
}

static int ig_crouch(actor_t *a, gds *g) {              /* m_01060: about to jump */
    if (a->unk_02a++ == 0) {
        g->unk_054 = 0xc;
        a->frame = (int2)face_frame(a, 0xf, 0xe);
    } else if (a->unk_02a > 8) { a->unk_02a = 0; a->x_min = 5; a->step = 4; }
    return 0;
}

static int ig_choose_floor(actor_t *a, gds *g) {        /* m_010e8 */
    mario_t *m = g->mario;
    a->frame = (int2)face_frame(a, 0x12, 0x10);
    if (a->x_min-- > 0 &&
        (m->floor != a->x_max || (m->state_group != 1 && m->state_group != 2 && m->state_group != 4))) {
        if (mario_behind(a, g)) { a->x_min++; a->step = 1; a->unk_02e = -1; return 0; }
        if (a->x_max == 4 || (m->floor <= a->x_max && a->x_max != 0)) { a->step = 5; a->x_max--; }
        else { a->step = 6; a->x_max++; }
        return 0;
    }
    a->unk_02e = 0;
    a->x_min = 0x14;
    if (mario_behind(a, g)) { a->step = 1; a->unk_02a = 0; }
    else { a->step = 0; a->unk_02a = 1; }
    return 0;
}

static int ig_jump_up(actor_t *a, gds *g) {             /* m_012be */
    (void)g;
    a->frame = (int2)face_frame(a, 0x12, 0x10);
    if (++a->unk_02a > 2) { a->unk_02a = 0; a->step = 7; a->unk_036 = -0xb; }
    return 0;
}

static int ig_jump_down(actor_t *a, gds *g) {           /* m_01322 */
    (void)g;
    a->frame = (int2)face_frame(a, 0x12, 0x10);
    if (++a->unk_02a > 2) { a->unk_02a = 0; a->step = 7; a->unk_036 = -2; }
    return 0;
}

static int ig_in_air(actor_t *a, gds *g) {              /* m_01386 */
    a->unk_02a++;
    if (a->dir == 1) { a->frame = 0x13; a->x -= 6; if (a->x < 0x18) a->x = 0x18; }
    else { a->frame = 0x11; a->x += 6; if (a->x > 0x142) a->x = 0x142; }
    if (rd25c(a, LEAVING) == 0) {
        int land = g->floor_y[a->x_max] - 0x29;
        a->y += a->unk_036;
        if (a->y >= land && a->unk_036 > 0) { a->y = (int2)land; a->unk_02a = 0; a->step = 8; }
    } else {
        a->y += a->unk_036;
        if (a->y > 200) { a->state = 4; a->step = 3; a->unk_02a = 2; }
    }
    if (a->unk_036 < 0x10) a->unk_036 += 2;
    ig_box(a);
    a->cur_floor = g->mario->floor;
    return 0;
}

static int ig_land(actor_t *a, gds *g) {                /* m_014d6 */
    (void)g;
    a->frame = (int2)face_frame(a, 0x12, 0x10);
    if (++a->unk_02a > 2) { a->unk_02a = 0; a->step = 4; }
    return 0;
}

static int ig_breathe(actor_t *a, gds *g) {             /* m_00ec4: flame, then a fireball */
    int2 n = a->unk_02a++;
    if (n == 0) {
        g->unk_054 = 0xc;
        a->frame = (int2)face_frame(a, 0xf, 0xe);
    } else if (a->unk_02a < 6) {
        if (a->unk_02a == 3) {
            actor_t *j = link_of(a, g, LINK_JET);
            if (j && j->state == 0) {
                j->state = 3;
                j->x = (int2)(a->dir == 1 ? a->x - 6 : a->x + 0x20);
                j->y = (int2)(a->y - 4);
            }
        } else if (a->unk_02a > 3) {
            actor_t *f = link_of(a, g, LINK_FB);
            if (f && f->state == 0) {
                f->state = 3;
                f->dir = a->dir;
                f->x = (int2)(a->dir == 1 ? a->x - 6 : a->x + 0x20);
                f->y = (int2)(a->y + 3);
                f->x_min = 8;
                f->unk_036 = -10;
            }
        }
    } else back_to_walk(a);
    return 0;
}

static int ig_arrive_land(actor_t *a, gds *g) {         /* m_0157c */
    (void)g;
    a->frame = 0x20;
    a->y = 0x1f;
    a->box_y0 = a->y + 8; a->box_y1 = 0x48;
    a->unk_02a = 0; a->step = 0xb;
    return 0;
}

static int ig_arrive_turn(actor_t *a, gds *g) {         /* m_015c6 */
    (void)g;
    if (a->unk_02a++ == 0) a->frame = 0xc;
    else { a->frame = (int2)face_frame(a, 0xd, 0xb); back_to_walk(a); }
    return 0;
}

static int ig_chain_turn(actor_t *a, gds *g) {          /* m_00d62 */
    (void)g;
    if (a->unk_02a++ == 0) a->frame = (int2)face_frame(a, 0xd, 0xb);
    else { a->frame = 0xc; a->step = 0xd; a->unk_02a = 0; }
    return 0;
}

static int ig_chain(actor_t *a, gds *g) {               /* m_00dde: drop the chain on Mario */
    actor_t *c = link_of(a, g, LINK_CHAIN);
    if (a->unk_02a == 0) a->frame = 0x14;
    if (a->unk_02a == 5) {
        if (c && c->state == 0) {
            c->state = 3; c->y = 2; c->x = (int2)(g->mario->x + 8);
            g->unk_054 = 8;
        }
        a->frame = 0x15;
    }
    if (++a->unk_02a > 0xf) {
        a->step = 0xb; a->unk_02a = 0;
        chain_up(a, g);
    }
    return 0;
}

static int ig_drop_in(actor_t *a, gds *g) {             /* m_01534 */
    (void)g;
    a->y += 0x10;
    a->box_y0 = a->y + 8; a->box_y1 = a->y + 0x29;
    if (a->y > 0xe) a->step = 10;
    return 0;
}

static int ig_exit_1(actor_t *a, gds *g) {              /* m_017ba */
    (void)g;
    if (a->unk_02a++ == 0) a->frame = (int2)face_frame(a, 0xd, 0xb);
    else { a->frame = 0xc; a->unk_02a = 0; a->step = 0x10; }
    return 0;
}

static int ig_exit_2(actor_t *a, gds *g) {              /* m_01828 */
    (void)g;
    if (a->unk_02a++ == 0) a->frame = 0x16;
    else { a->frame = 0x17; if (a->unk_02a > 6) { a->unk_02a = 0; a->step = 0x11; } }
    return 0;
}

static int ig_exit_3(actor_t *a, gds *g) {              /* m_01886 */
    (void)g;
    if (a->unk_02a++ < 9) a->frame = 0x18;
    else { a->frame = 0x19; if (a->unk_02a > 0x10) { a->unk_02a = 0; a->step = 0x12; } }
    return 0;
}

static int ig_exit_4(actor_t *a, gds *g) {              /* m_018e6 */
    (void)g;
    a->frame = (int2)(0x19 - a->unk_02a);
    if (++a->unk_02a > 3) { a->unk_02a = 0; a->step = 0x13; }
    return 0;
}

static int ig_exit_5(actor_t *a, gds *g) {              /* m_01926: jump off the screen */
    (void)g;
    if (a->unk_02a++ == 0) a->frame = 0xc;
    else {
        a->frame = (int2)face_frame(a, 0xd, 0xb);
        a->unk_02a = 0; a->step = 7; a->unk_036 = -2;
        wr25c(a, LEAVING, 1);
    }
    return 0;
}

static int ig_hit(actor_t *a, gds *g) {                 /* m_019a2: stomped */
    if (g->unk_496 != 0) { a->unk_04e = a->cmd; return 0; }
    switch (a->step) {
    case 7:
        if (a->unk_02a == 0) { a->step = 2; a->unk_02a = 0; break; }
        /* fall through */
    case 2: case 0xf: case 0x10: case 0x11: case 0x12: case 0x13:
        knocked_off(a);
        break;
    default:
        a->step = 2; a->unk_02a = 0;
        break;
    }
    return 0;
}

static int ig_destroy(actor_t *a, gds *g) {             /* m_01abc: fireball / star */
    if (g->unk_496 != 0) { a->unk_04e = a->cmd; return 0; }
    if (g->unk_53a == 0 || a->step == 7 || a->step == 0xe) {
        a->unk_04c = 0;
        knocked_off(a);
    } else { a->step = 0xf; a->unk_02a = 0; }
    return 0;
}

static int ig_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }   /* m_01b86 */

static int ig_hit_1(actor_t *a, gds *g) {               /* m_01bc0 */
    a->frame = a->unk_02a;
    ig_draw(a, g);
    a->step = 5;
    chain_up(a, g);
    return 0;
}

static int ig_hit_2(actor_t *a, gds *g) {               /* m_01c22 */
    a->frame = a->unk_02a;
    ig_draw(a, g);
    a->unk_02a++;
    a->step = 6;
    return 0;
}

static int ig_fall(actor_t *a, gds *g) {                /* m_01c56 */
    a->y += 0x10;
    a->frame = a->unk_02a;
    ig_draw(a, g);
    if (++a->unk_02a > 0x1d) a->unk_02a = 0x1b;
    if (a->y > 200) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

static int ig_gone(actor_t *a, gds *g) {                /* m_01cc0 */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

int iggy(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, ig_init);
        H(0, 2, ig_away);
        H(1, 2, ig_appear);
        H(2, 1, obj_erase);
        H(2, 2, ig_arrive);
        H(3, 1, obj_erase);
        H(3, 2, ig_moving);
        H(3, 3, ig_hit);
        H(3, 4, ig_destroy);
        H(4, 1, obj_erase);
        H(4, 2, ig_dying);
        W(0, ig_walk);
        W(1, ig_turn);
        W(2, ig_stunned);
        W(3, ig_crouch);
        W(4, ig_choose_floor);
        W(5, ig_jump_up);
        W(6, ig_jump_down);
        W(7, ig_in_air);
        W(8, ig_land);
        W(9, ig_breathe);
        W(10, ig_arrive_land);
        W(11, ig_arrive_turn);
        W(12, ig_chain_turn);
        W(13, ig_chain);
        W(14, ig_drop_in);
        W(15, ig_exit_1);
        W(16, ig_exit_2);
        W(17, ig_exit_3);
        W(18, ig_exit_4);
        W(19, ig_exit_5);
        B(3, ig_gone);
        B(4, ig_hit_1);
        B(5, ig_hit_2);
        B(6, ig_fall);
    }
    return obj_dispatch(a, g);
}
