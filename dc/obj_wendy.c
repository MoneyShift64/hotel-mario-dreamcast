/* obj_wendy.c - "wendy", the hotel 7 boss (stage 10), from L7_s10_sub.o (m_00504 .. m_02938).
 *
 * She lives on the top floor (floor 0) and can't be killed: the stage is won the usual
 * way, by shutting every door (exit_check -> cmd 4 to everyone, with unk_53a = 1).
 *  - comes out of a door (state 2) and walks along floor 0 at 4 px (walk_steps[0]),
 *    opening shut doors she passes; at any door (shut or open) with Mario more than 30 px
 *    away she stops to climb on her bon-bon (w_bon_bon, type 0x24) and throws kisses
 *    (w_lips 0x25 / w_kiss 0x26, at most 4 out) at Mario until she is hit
 *  - first stomp: back into a door (steps_b[2]); she comes straight out again, angry
 *    (unk_036 = 1): a pause (walk_steps[3]) then running at 16 px (walk_steps[1]), turning
 *    to face Mario; she calms down when Mario leaves floor 0
 *  - a stomp while angry, or a fireball / star: she falls off the screen (steps_b[3/4]),
 *    then comes back from the side away from Mario, sooner each time (delay 25 -> 15)
 *  - if she reaches Mario while he hides in a door or waits in a lift on floor 0, she
 *    pulls him out (walk_steps[8/9/10]): Mario state 7/1, unk_530 = 1 (and unk_53a = 2
 *    when she was walking); at the end (walk_steps[7]) unk_53a = 0 and Mario is dead
 *    (state 6/5, fell = 1), which exit_check turns into a lost life
 * The original's m_02a40 re-blits the door picture at her column during the pull-out
 * (the port redraws doors every frame, so it's left out).
 * Links (index+1 in unk_25c, the original keeps pointers): +0 int32 the pull-out column,
 * +4 the bon-bon, +8/+0xc the two w_lips, +0x10/+0x14 the two w_kiss.
 * Frames: 1-6 running, 7-16 walking, 17-19 angry pause, 20-27 on the bon-bon, 28-39
 * throwing, 40-44 falling, 45-46 out of the door, 47-49 turning, 50-60 the pull-out.
 */
#include "obj_common.h"

static actor_t *link_at(gds *g, actor_t *a, int off) {
    int32_t i = rd25c(a, off);
    (void)g;
    return (i > 0 && i <= hm_world->room.n_actors) ? &hm_world->room.actors[i - 1] : NULL;
}

static void draw_frame(actor_t *a, gds *g) { draw_at_xy(a, g, a->frame); }   /* m_02884 */

static void place(actor_t *a, gds *g) {
    a->floor = 0; a->cur_floor = 0;
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[0] - 0x1b);
    a->box_y0 = a->y + 4; a->box_y1 = a->y + 0x1b;
    a->x = g->door_x[a->col];
    a->box_x0 = a->x + 8; a->box_x1 = a->x + 0x1e;
}

static int wd_init(actor_t *a, gds *g) {                /* m_00504 */
    int i, k;
    actor_t *acts = hm_world->room.actors;
    int n = hm_world->room.n_actors;
    a->y = a->drawn_y0 = a->drawn_y1 = (int2)(g->floor_y[a->floor] - 0x1b);
    a->floor = 0; a->cur_floor = 0;
    a->state = 1; a->delay = 0x19;
    a->box_y0 = a->y + 4; a->box_y1 = a->y + 0x1b;
    a->x = g->door_x[a->col];
    a->box_x1 = a->x + 0x1e; a->box_x0 = a->x + 8;
    for (i = 0; i < 24 && i < n && acts[i].type != 0; i++)
        if (acts[i].type == 0x24) { wr25c(a, 4, i + 1); break; }
    for (i = 0, k = 4; i < 24 && i < n && k < 6 && acts[i].type != 0; i++)
        if (acts[i].type == 0x25) wr25c(a, 4 * k++ - 8, i + 1);
    for (i = 0, k = 6; i < 24 && i < n && k < 8 && acts[i].type != 0; i++)
        if (acts[i].type == 0x26) wr25c(a, 4 * k++ - 8, i + 1);
    g->unk_530 = 0;
    a->unk_04a = -1; a->unk_048 = -1;
    return 0;
}

static int wd_respawn(actor_t *a, gds *g) {             /* m_00a76 */
    if (a->timer++ > a->delay || a->unk_036 != 0) {
        a->timer = 0;
        if (a->delay > 0xf) a->delay -= 5;
        if (g->mario->x < 0xc1) { a->col = 6; a->dir = 1; }
        else { a->col = 0; a->dir = 0; }
        place(a, g);
        a->state = 1;
        g->unk_530 = 0;
        a->unk_04e = 0;
    }
    return 0;
}

static int wd_wait(actor_t *a, gds *g) {                /* m_00b88 */
    if (g->unk_53a == 0) { a->state = 2; a->step = 0; a->unk_02a = 0; }
    return 0;
}

static int wd_emerge(actor_t *a, gds *g) {              /* m_00bd6 */
    a->frame = (int2)(a->unk_02a + 0x2d);
    if (++a->unk_02a > 1) {
        a->unk_02a = 0;
        if (g->unk_53a == 0) {
            a->x_min = 0; a->state = 3;
            a->step = a->unk_036 == 0 ? 0 : 3;
        } else {
            a->unk_02c = 0; a->state = 4;
            a->step = (g->unk_53a == 1 && g->unk_82e == 0) ? 2 : 3;
        }
    }
    draw_frame(a, g);
    return 0;
}

static int wd_moving(actor_t *a, gds *g) {              /* m_00cac */
    int r = CALL(a->walk_steps[a->step], a, g);
    if (a->x_max == 0) draw_frame(a, g);
    return r;
}

static void cycle(actor_t *a, int top) {                /* 0..top..0 in unk_02a */
    if (a->unk_02e == 0) {
        if (a->unk_02a < top) a->unk_02a++; else { a->unk_02a--; a->unk_02e = 1; }
    } else if (a->unk_02a < 1) { a->unk_02a++; a->unk_02e = 0; }
    else a->unk_02a--;
}

static void set_box(actor_t *a) {
    a->box_x0 = a->x + 8; a->box_x1 = a->x + 0x1e;
    a->box_y0 = a->y + 4; a->box_y1 = a->y + 0x1b;
}

/* Mario within 10 px on her floor, hiding in a door or waiting in a lift: pull him out */
static void catch_mario(actor_t *a, gds *g, int set_53a) {
    if(world_invulnerable(g))return;
    mario_t *m = g->mario;
    if (m->floor != a->cur_floor || abs(m->x - a->x) >= 10) return;
    if ((m->state_group == 1 || m->state_group == 2) &&
        ((m->state == 1 && m->sub == 3) || m->state == 0 ||
         (m->state == 2 && (m->sub == 5 || m->sub == 6)))) {
        uint16_t *c = DOOR_CELL(m->floor, m->x + 0x10);
        int st;
        if (!c) return;
        if ((*c & 0xf000) == 0xf000) st = 8;
        else if ((*c & 0xf000) == 0xe000) st = 9;
        else return;
        m->state_group = 7; m->state = 1;
        if (set_53a) g->unk_53a = 2;
        a->step = (int2)st; a->unk_02a = 0;
        g->unk_530 = 1;
    } else if (m->state_group == 4) {
        m->state_group = 7; m->state = 1;
        if (set_53a) g->unk_53a = 2;
        a->step = 10; a->unk_02a = 0;
        g->unk_530 = 1;
    }
}

static int wd_walk(actor_t *a, gds *g) {                /* m_00d08 */
    uint16_t *c;
    cycle(a, 4);
    if (a->dir == 1) { a->x -= 4; a->frame = (int2)(a->unk_02a + 0xc); }
    else             { a->x += 4; a->frame = (int2)(a->unk_02a + 7); }
    set_box(a);
    c = DOOR_CELL(a->cur_floor, a->x + 0x12);
    if (c && ((*c & 0xf000) == 0x1000 || (*c & 0xf000) == 0x2000)) {
        if ((*c & 0xf000) == 0x1000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
        if (abs(a->x - g->mario->x) > 0x1e) a->step = 4;
        a->unk_02a = 0;
    }
    if (a->x > 0x138) {
        a->x = 0x138;
        if (a->dir == 0) { a->step = 2; a->unk_02a = 0; }
    } else if (a->x < 0x22) {
        a->x = 0x22;
        if (a->dir == 1) { a->step = 2; a->unk_02a = 0; }
    }
    catch_mario(a, g, 1);
    return 0;
}

static int wd_run(actor_t *a, gds *g) {                 /* m_010ca: angry */
    mario_t *m = g->mario;
    uint16_t *c;
    cycle(a, 2);
    if (a->dir == 1) { a->x -= 0x10; a->frame = (int2)(a->unk_02a + 4); }
    else             { a->x += 0x10; a->frame = (int2)(a->unk_02a + 1); }
    set_box(a);
    c = DOOR_CELL(a->cur_floor, a->x + 0x12);
    if (c && (*c & 0xf000) == 0x1000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
    if (a->x > 0x138) {
        a->x = 0x138;
        if (a->dir == 0) { a->step = 2; a->unk_02a = 0; }
    } else if (a->x < 0x22) {
        a->x = 0x22;
        if (a->dir == 1) { a->step = 2; a->unk_02a = 0; }
    } else if ((m->x < a->x && a->dir == 0) || (a->x < m->x && a->dir == 1)) {
        a->step = 2; a->unk_02a = 0;
    }
    if (m->floor == a->cur_floor) catch_mario(a, g, 0);
    else { a->unk_036 = 0; a->step = 0; a->unk_02a = 1; }
    return 0;
}

static int wd_turn(actor_t *a, gds *g) {                /* m_02388 */
    (void)g;
    a->frame = (int2)((a->dir == 1) ? 0x31 - a->unk_02a : a->unk_02a + 0x2f);
    if (++a->unk_02a > 2) {
        a->unk_02a = 0;
        a->step = a->unk_036 == 0 ? 0 : 1;
        a->dir = (a->dir == 0) ? 1 : 0;
    }
    return 0;
}

static int wd_angry(actor_t *a, gds *g) {               /* m_0243e */
    (void)g;
    a->frame = 0x12;
    if (++a->unk_02a > 9) {
        a->frame = (a->dir == 1) ? 0x13 : 0x11;
        a->step = 1; a->unk_02a = 0;
    }
    return 0;
}

static int wd_climb(actor_t *a, gds *g) {               /* m_0149c: onto the bon-bon */
    actor_t *b;
    (void)g;
    switch (++a->unk_02a) {
    case 1: a->frame = (a->dir == 1) ? 0x31 : 0x2f; break;
    case 2: a->frame = 0x30; break;
    case 3:
        a->frame = 0x14; a->y += 1; a->step = 5; a->unk_02a = 0;
        b = link_at(g, a, 4);
        if (b && b->state == 0) { b->x = (int2)(a->x - 0x1a); b->state = 2; }
        break;
    }
    return 0;
}

static int wd_sit(actor_t *a, gds *g) {                 /* m_01596 */
    (void)g;
    a->frame = 0x14;
    if (++a->unk_02a > 0xb) { a->step = 6; a->unk_02a = 0; a->x_min = 0; }
    return 0;
}

static int wd_bob(actor_t *a, gds *g) {                 /* m_015d6: wait for a free kiss */
    int i;
    a->frame = (int2)(a->unk_02a + 0x14);
    if (++a->unk_02a > 7) a->unk_02a = 0;
    if (++a->x_min > 0x13) {
        for (i = 4; i < 8; i++) {
            actor_t *k = link_at(g, a, 4 * i - 8);
            if (k && k->state == 0) {
                int dx = a->x - g->mario->x, dy = a->y - g->mario->y;
                if (abs(dx) < abs(dy)) a->dir = -1;
                else a->dir = (dx < 1) ? 0 : 1;
                a->step = 0xb; a->unk_02a = 0;
                return 0;
            }
        }
        a->step = 5; a->unk_02a = 0;
    }
    return 0;
}

static int launch(actor_t *a, gds *g, actor_t *k, int lips_first) {
    k->state = 3;
    g->unk_054 = 0xd;
    switch (a->dir) {
    case 0:
        k->x = (int2)(a->x + (lips_first ? 0x1c : 0x12));
        k->y = (int2)(a->y + (lips_first ? 0 : 6));
        k->unk_036 = 4; k->dir = 0; a->frame = 0x26; break;
    case 1:
        k->x = (int2)(a->x - (lips_first ? 0x20 : 0x16));
        k->y = (int2)(a->y + (lips_first ? 0 : 6));
        k->unk_036 = -4; k->dir = 0; a->frame = 0x22; break;
    case -1:
        k->x = (int2)(a->x - 2);
        k->y = (int2)(a->y + (lips_first ? 0xf : 0x15));
        k->unk_036 = 0; k->dir = 4; a->frame = 0x1e; break;
    }
    return 0;
}

static int wd_blow(actor_t *a, gds *g) {                /* m_0173a: blow a kiss */
    int i;
    switch (a->dir) {
    case 0:  a->frame = a->unk_02a == 0 ? 0x24 : 0x25; break;
    case 1:  a->frame = a->unk_02a == 0 ? 0x20 : 0x21; break;
    case -1: a->frame = a->unk_02a == 0 ? 0x1c : 0x1d; break;
    }
    if (++a->unk_02a > 10) {
        a->unk_02a = 0; a->step = 0xc;
        if ((RND(g) & 1) == 0) {
            for (i = 7; i > 3; i--) {
                actor_t *k = link_at(g, a, 4 * i - 8);
                if (k && k->state == 0) return launch(a, g, k, 0);
            }
        } else {
            for (i = 4; i < 8; i++) {
                actor_t *k = link_at(g, a, 4 * i - 8);
                if (k && k->state == 0) return launch(a, g, k, 1);
            }
        }
    }
    return 0;
}

static int wd_after(actor_t *a, gds *g) {               /* m_01aac */
    (void)g;
    if (++a->unk_02a > 9) { a->unk_02a = 0; a->step = 5; }
    switch (a->dir) {
    case 0:  a->frame = a->unk_02a == 0 ? 0x27 : 0x26; break;
    case 1:  a->frame = a->unk_02a == 0 ? 0x23 : 0x22; break;
    case -1: a->frame = a->unk_02a == 0 ? 0x1f : 0x1e; break;
    }
    return 0;
}

static int door_col(int x) {                            /* m_02938 */
    x += 0x12;
    if (x < 0x58) return 0;
    if (x < 0x84) return 1;
    if (x < 0xb0) return 2;
    if (x < 0xdc) return 3;
    if (x < 0x108) return 4;
    if (x < 0x134) return 5;
    return 6;
}

/* the pull-out: from a down lift (8), up lift (9) or a door (10); all end in step 7 */
static int pull_out(actor_t *a, gds *g, int door) {     /* m_01bae / m_01e54 / m_020fe */
    int t = a->unk_02a++;
    if (t == 0) {
        int c = door_col(a->x);
        wr25c(a, 0, c);
        a->x = (int2)(g->door_x[c] - 2);
        a->frame = (a->dir == 1) ? 0x37 : 0x36;
        return 0;
    }
    if (!door) {
        switch (t) {
        case 1: a->frame = 0x38; break;
        case 2: a->frame = 0x39; break;
        case 3: case 4: case 5: a->frame++; break;
        case 6: a->x_max = 1; break;
        case 0x11: a->x_max = 0; a->frame = 0x34; break;
        case 0x12: a->frame = 0x35; break;
        case 0x13: a->frame = 0x32; break;
        case 0x14: a->frame++; break;
        case 0x15: a->frame = 0x30; break;
        case 0x17: a->unk_02a = 0; a->step = 7; break;
        }
    } else {
        switch (t) {
        case 1: a->frame = 0x38; break;
        case 2: a->frame = 0x39; break;
        case 3: a->frame++; break;
        case 4: a->x_max = 1; break;
        case 5: g->unk_054 = 1; break;
        case 0x12: g->unk_054 = 2; break;
        case 0x13: a->x_max = 0; a->frame = 0x32; break;
        case 0x14: a->frame = 0x33; break;
        case 0x15: a->frame = 0x30; a->unk_02a = 0; a->step = 7; break;
        }
    }
    return 0;
}
static int wd_pull_lift_down(actor_t *a, gds *g) { return pull_out(a, g, 0); }
static int wd_pull_lift_up(actor_t *a, gds *g)   { return pull_out(a, g, 0); }
static int wd_pull_door(actor_t *a, gds *g)      { return pull_out(a, g, 1); }

static int wd_got_him(actor_t *a, gds *g) {             /* m_02330 */
    a->frame = 0x30;
    if (++a->unk_02a > 10) {
        g->unk_53a = 0;
        g->mario->state_group = 6;
        g->mario->state = 5;
        g->mario->fell = 1;
        g->unk_530 = 0;
    }
    return 0;
}

static int wd_hit(actor_t *a, gds *g) {                 /* m_024a2: stomped */
    if (g->unk_496 == 0) {
        a->state = 4; a->unk_02a = 0; a->unk_02c = 0;
        if (a->unk_036 == 0) { a->unk_036 = 1; a->step = 2; }
        else a->step = 3;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int wd_destroy(actor_t *a, gds *g) {             /* m_0253e: fireball, star, stage end */
    if (g->unk_496 == 0) {
        if (a->unk_04c != 0 || g->unk_53a != 0) a->unk_04c = 0;
        a->unk_02a = 0; a->unk_02c = 0; a->state = 4;
        a->step = (g->unk_53a == 1 && g->unk_82e == 0) ? 2 : 3;
    } else a->unk_04e = a->cmd;
    return 0;
}

static int wd_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static void bonbon_away(gds *g, actor_t *a) {
    actor_t *b = link_at(g, a, 4);
    if (b && b->state == 3) b->state = 4;
}

static int wd_gone(actor_t *a, gds *g) {                /* m_0279c */
    (void)g;
    if (a->unk_02a-- < 2) a->state = 0;
    return 0;
}

static int wd_retreat(actor_t *a, gds *g) {             /* m_0263a: back into a door */
    a->y -= 1;
    a->frame = (int2)(0x2e - a->unk_02a);
    draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 1; a->unk_02a = 2; bonbon_away(g, a); }
    return 0;
}

static int wd_struck(actor_t *a, gds *g) {              /* m_026b8 */
    a->frame = (int2)(a->unk_02a + 0x28);
    draw_frame(a, g);
    if (++a->unk_02a > 1) { a->step = 4; a->unk_02a = 0; bonbon_away(g, a); }
    return 0;
}

static int wd_fall(actor_t *a, gds *g) {                /* m_02730 */
    a->y += 0x10;
    a->frame = (int2)(a->unk_02a + 0x2a);
    draw_frame(a, g);
    if (++a->unk_02a > 2) a->unk_02a = 0;
    if (a->y > 0xd6) { a->step = 1; a->unk_02a = 2; }
    return 0;
}

int wendy(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, wd_init);
        H(0, 2, wd_respawn);
        H(1, 2, wd_wait);
        H(2, 1, obj_erase);
        H(2, 2, wd_emerge);
        H(3, 1, obj_erase);
        H(3, 2, wd_moving);
        H(3, 3, wd_hit);
        H(3, 4, wd_destroy);
        H(4, 1, obj_erase);
        H(4, 2, wd_dying);
        W(0, wd_walk);
        W(1, wd_run);
        W(2, wd_turn);
        W(3, wd_angry);
        W(4, wd_climb);
        W(5, wd_sit);
        W(6, wd_bob);
        W(7, wd_got_him);
        W(8, wd_pull_lift_down);
        W(9, wd_pull_lift_up);
        W(10, wd_pull_door);
        W(11, wd_blow);
        W(12, wd_after);
        B(1, wd_gone);
        B(2, wd_retreat);
        B(3, wd_struck);
        B(4, wd_fall);
    }
    return obj_dispatch(a, g);
}
