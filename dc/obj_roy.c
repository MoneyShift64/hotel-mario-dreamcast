/* obj_roy.c - "roy", the hotel 2 boss (stage 10), from L2_s10_sub.o (m_01d4e .. m_048c4).
 *
 * Roy can't be killed: the stage is won the usual way, by shutting every door, while he
 * gets in the way.
 *  - He walks his floor 6 px a tick, turning towards Mario every other tick; passing a
 *    closed door he opens it 1 time in 8.
 *  - Mario straight above or below him on the next floor: he reaches up or down at him
 *    (walk steps 8/9, the reach box counts on that floor, sound 0xd).
 *  - Mario two or more floors away, or dodging (in a lift or a door) three times in a
 *    row: he runs to the wall, jumps up to the top of the screen, flies across above
 *    Mario and drops onto Mario's floor (or the one above or below, 3 in 8 each)
 *    (steps 2-7). While he's flying across the top he can't be stomped: touching him hurts.
 *  - Mario hiding in a door, or standing in a lift, right next to him (within 10 px):
 *    he goes into it after Mario and Mario loses a life (steps 10/11/12, then the laugh
 *    in step 13: Mario group 6 state 5, unk_53a back to 0).
 *  - Stomped while walking: squashed for 2 ticks (state 4 step 9), then flashes for 20
 *    ticks (step 1). Stomped while flashing, jumping or dropping, or hit by a fireball or
 *    a star: he falls off the bottom of the screen and comes back out of a door near
 *    Mario after a pause, which starts at 35 ticks and gets 10 shorter each time, down
 *    to 15. There are no points for any of it.
 * unk_25c (int32s): +0 jump mode (1 left wall, 2 right wall), +4 floor before a reach or
 * drop, +8 door column for the door/lift attack, +0xc target y of a drop, +0x10 hidden
 * (inside the door or lift: not drawn).
 * x_min: dodges counted; x_max: tick parity for turning, also the flash toggle.
 * Frames: 1-4 walk, 10 squash, 5-9 falling off, 11-14 turn and come out, 15-16 flash,
 * 17-20 reach up, 21-24 reach down, 25-28 jump, 29-36 rise, 37-40 fly, 41-46 into the
 * door or lift, 48-57 laugh.
 * The original also redraws the door picture under him while he goes in (m_048c4); the
 * port draws doors every frame, so that isn't needed.
 */
#include "obj_common.h"

static int iabs(int v) { return v < 0 ? -v : v; }

static void place(actor_t *a, gds *g) {
    a->y = a->drawn_y1 = a->drawn_y0 = (int2)(g->floor_y[a->floor] - 0x2e);
    a->cur_floor = a->floor;
    a->state = 1;
    a->box_y0 = a->y + 0x14; a->box_y1 = a->y + 0x2e;
    a->x = (int2)(a->col * 0x2c + 0x2c);
    a->box_x1 = a->x + 0x1e; a->box_x0 = a->x + 4;
}

static int is_door(gds *g, int f, int c) {
    uint16_t v = CELL(g, f, c) & 0xf000;
    return v == 0x2000 || v == 0x1000;
}

/* m_044a4: the door Roy comes back out of - one near Mario, on his floor if possible */
static void pick_door(actor_t *a, gds *g) {
    mario_t *m = g->mario;
    int mf = m->floor, fl = 0, col = 0, pass, f, c, found;
    for (pass = 0; pass < 2; pass++) {
        found = 0;
        for (f = mf - pass; f >= 0 && !found; f--)
            for (c = 0; c < 7; c++) if (is_door(g, f, c)) { found = 1; col = c; fl = f; break; }
        if (!found) fl = -10;
        found = 0;
        for (f = mf + 1; f <= 4 && f - mf < mf - fl && !found; f++)
            for (c = 0; c < 7; c++) if (is_door(g, f, c)) { found = 1; col = c; fl = f; break; }
        for (c = col + 1; c < 7; c++) {
            if (!is_door(g, fl, c)) continue;
            int dc = iabs(m->x - g->door_x[col]), dn = iabs(m->x - g->door_x[c]);
            if ((dn < dc && (dn > 0x54 || fl != m->floor)) || (dc < 0x54 && fl == m->floor)) col = c;
        }
        if (iabs(m->x - g->door_x[col]) > 0x54 || fl != m->floor) break;
    }
    if (fl >= 0 && fl < 5 && col >= 0 && col < 7) { a->floor = (int2)fl; a->col = (int2)col; }
}

static int roy_init(actor_t *a, gds *g) {               /* m_01d4e */
    place(a, g);
    a->delay = 0x23;
    g->unk_530 = 0;
    a->unk_04a = -1; a->unk_048 = -1;
    return 0;
}

static int roy_respawn(actor_t *a, gds *g) {            /* m_02116 (state 0 cmd 2) */
    if (a->timer++ > a->delay) {
        a->timer = 0;
        if (a->delay > 0xf) a->delay -= 10;
        pick_door(a, g);
        place(a, g);
        a->unk_04e = 0;
    }
    return 0;
}

static int roy_wait(actor_t *a, gds *g) {               /* m_021e4 */
    mario_t *m = g->mario;
    int mx, dx, d;
    if (g->unk_53a != 0) return 0;
    mx = m->x + 0x10; dx = a->col * 0x2c + 0x3c;
    if (dx < mx) { if (a->dir == -1) a->dir = 0; d = mx - dx; }
    else         { if (a->dir == -1) a->dir = 1; d = dx - mx; }
    if (d > 0x48 || a->floor != m->floor) {
        uint16_t *c = &CELL(g, a->floor, a->col);
        if ((*c & 0xf000) != 0x2000) { *c = (*c & 0xfff) | 0x2009; g->unk_054 = 2; }
        a->state = 2; a->step = 0;
    }
    return 0;
}

static int roy_emerge(actor_t *a, gds *g) {             /* m_022fe */
    uint16_t *c = &CELL(g, a->floor, a->col);
    switch (a->step) {
    case 0: *c |= 4; a->step = 1; break;
    case 1: a->x = (int2)(a->col * 0x2c + 0x2c); a->unk_02a = 0xb; a->step = 2; break;
    case 2: a->unk_02a = 0xc; a->step = 3; break;
    case 3:
        *c = (*c & ~4) | 2;
        a->unk_02a = 1; a->step = 0; a->state = 3;
        wr25c(a, 0, 0);
        a->x_min = 0;
        break;
    }
    if (a->step != 1) draw_at(a, g, a->unk_02a);
    return 0;
}

static int roy_moving(actor_t *a, gds *g) {             /* m_024a0 */
    int r = CALL(a->walk_steps[a->step], a, g);
    if (rd25c(a, 0x10) == 0) draw_at_xy(a, g, a->frame);
    return r;
}

/* Mario in a lift or a door right by him: in after him (steps 10 lift up, 11 lift down, 12 door) */
static void catch_mario(actor_t *a, gds *g, int step) {
    if(world_invulnerable(g))return;
    mario_t *m = g->mario;
    m->state_group = 7; m->state = 1;
    g->unk_53a = 2;
    a->step = (int2)step; a->unk_02a = 0;
}

static int roy_walk(actor_t *a, gds *g) {               /* m_0258c */
    mario_t *m = g->mario;
    uint16_t *c;
    if (a->dir == 1) { if (a->unk_02a < 1) a->unk_02a = 3; else a->unk_02a--; }
    else if (a->unk_02a < 3) a->unk_02a++;
    else a->unk_02a = 0;
    a->x += (a->dir == 1) ? -6 : 6;
    a->frame = (int2)(a->unk_02a + 1);
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x1e;
    a->box_y0 = a->y + 0x14; a->box_y1 = a->y + 0x2e;
    if (rd25c(a, 0) == 0 && ++a->x_max > 0) {
        a->x_max = 0;
        if (iabs(a->cur_floor - m->floor) < 2 && a->x_min < 3) {
            if (a->cur_floor == m->floor) a->dir = (m->x < a->x) ? 1 : 0;
            else a->dir = (m->x - 0xc < a->x) ? 1 : 0;
        } else if (a->x < 0xc1) { wr25c(a, 0, 1); a->dir = 1; }
        else { wr25c(a, 0, 2); a->dir = 0; }
    }
    if (iabs(m->x - a->x) < 10) {
        if (m->floor == a->cur_floor) {
            if ((m->state_group == 1 || m->state_group == 2) &&
                ((m->state == 1 && m->sub == 3) || m->state == 0 ||
                 (m->state == 2 && (m->sub == 5 || m->sub == 6)))) {
                c = DOOR_CELL(m->floor, m->x + 0x10);
                if (c) {
                    if ((*c & 0xf000) == 0xf000) catch_mario(a, g, 10);
                    else if ((*c & 0xf000) == 0xe000) catch_mario(a, g, 11);
                }
            } else if (m->state_group == 4) catch_mario(a, g, 12);
        }
    } else if (iabs(m->x - 0xc - a->x) < 10) {
        int dodging = m->state_group == 1 || m->state_group == 2 || m->state_group == 4;
        if (a->cur_floor - 1 == m->floor || a->cur_floor + 1 == m->floor) {
            a->x_min = dodging ? a->x_min + 1 : 0;
            a->step = (a->cur_floor - 1 == m->floor) ? 8 : 9;
            a->unk_02a = 0;
            wr25c(a, 4, a->cur_floor);
        }
    } else if (m->state_group == 0 &&
               ((m->state == 0 && iabs(m->x + 0x24 - a->x) < 10) ||
                (m->state == 1 && iabs(m->x - 0x3c - a->x) < 10))) {
        if (a->cur_floor - 1 == m->floor || a->cur_floor + 1 == m->floor) {
            a->step = (a->cur_floor - 1 == m->floor) ? 8 : 9;
            a->unk_02a = 0; a->x_min = 0;
            wr25c(a, 4, a->cur_floor);
        }
    }
    if (a->x >= 0x13d) {
        a->x = 0x13c;
        if (rd25c(a, 0) == 2) { a->step = 2; a->unk_02a = 0; }
    } else if (a->x < 0x22) {
        a->x = 0x22;
        if (rd25c(a, 0) == 1) { a->step = 2; a->unk_02a = 0; }
    }
    c = DOOR_CELL(a->cur_floor, a->x);
    if (c && (*c & 0xf000) == 0x1000 && (RND(g) & 7) == 1) {
        *c = (*c & 0xfff) | 0x2009;
        g->unk_054 = 2;
    }
    return 0;
}

static int roy_reach_up(actor_t *a, gds *g) {           /* m_02c9a */
    if (a->unk_02a < 4) a->frame = (int2)(a->unk_02a + 0x11);
    if (++a->unk_02a > 3) {
        a->cur_floor = (int2)(rd25c(a, 4) - 1);
        a->box_y0 = a->y;
        a->box_x0 = a->x + 0x14; a->box_x1 = a->x + 0x20;
        if (a->unk_02a == 4) g->unk_054 = 0xd;
    }
    if (a->unk_02a > 6) {
        a->step = 0; wr25c(a, 0, 0);
        a->cur_floor = (int2)rd25c(a, 4);
        a->unk_02a = 0;
    }
    return 0;
}

static int roy_reach_down(actor_t *a, gds *g) {         /* m_02d42 */
    if (a->unk_02a < 4) a->frame = (int2)(a->unk_02a + 0x15);
    if (++a->unk_02a == 4) {
        a->cur_floor = (int2)(rd25c(a, 4) + 1);
        a->y += 0x11;
        a->box_y1 = a->y + 0x2e;
        a->box_x0 = a->x + 0x14; a->box_x1 = a->x + 0x20;
        g->unk_054 = 0xd;
    }
    if (a->unk_02a > 6) {
        a->step = 0; wr25c(a, 0, 0);
        a->cur_floor = (int2)rd25c(a, 4);
        a->y -= 0x11;
        a->unk_02a = 0;
    }
    return 0;
}

/* m_043ba: the door column under x */
static int column_at(int x) {
    x += 0x10;
    if (x < 0x58) return 0;
    if (x < 0x84) return 1;
    if (x < 0xb0) return 2;
    if (x < 0xdc) return 3;
    if (x < 0x108) return 4;
    if (x < 0x134) return 5;
    return 6;
}

static void go_in_start(actor_t *a, gds *g) {
    int col = column_at(a->x);
    wr25c(a, 8, col);
    a->x = g->door_x[col];
    a->frame = (a->dir == 1) ? 0x2a : 0x29;
}

static int roy_into_lift(actor_t *a, gds *g) {          /* m_02dec (lift up) / m_03098 (lift down) */
    switch (a->unk_02a++) {
    case 0: go_in_start(a, g); break;
    case 1: a->frame = 0x2b; break;
    case 2: case 3: case 4: case 5: a->frame++; break;
    case 6: wr25c(a, 0x10, 1); break;
    case 0x11: wr25c(a, 0x10, 0); a->frame = 0xe; break;
    case 0x12: a->frame = 0xd; break;
    case 0x13: a->frame = 0xb; break;
    case 0x14: a->frame++; break;
    case 0x15: a->frame = 1; break;
    case 0x17: a->unk_02a = 0; a->step = 0xd; break;
    }
    return 0;
}

static int roy_into_door(actor_t *a, gds *g) {          /* m_03348 */
    switch (a->unk_02a++) {
    case 0: go_in_start(a, g); break;
    case 1: a->frame = 0x2b; break;
    case 2: case 3: a->frame++; break;
    case 4: wr25c(a, 0x10, 1); break;
    case 5: g->unk_054 = 1; break;
    case 0x12: wr25c(a, 0x10, 0); a->frame = 0xb; g->unk_054 = 2; break;
    case 0x13: a->frame = 0xc; break;
    case 0x14: a->frame = 1; break;
    case 0x15: a->unk_02a = 0; a->step = 0xd; break;
    }
    return 0;
}

static int roy_jump(actor_t *a, gds *g) {               /* m_03580 */
    a->frame = (rd25c(a, 0) == 1) ? 0x19 : 0x1b;
    a->step = 3;
    a->cur_floor = g->mario->floor;
    return 1;
}

static int roy_rise(actor_t *a, gds *g) {               /* m_035ce */
    if (++a->unk_02a > 3) a->unk_02a = 0;
    a->frame = (int2)(a->unk_02a + ((rd25c(a, 0) == 1) ? 0x1d : 0x21));
    a->y -= 6;
    if (a->y < 0x24) { a->y = 0x23; a->unk_02a = 0; a->step = 4; }
    a->cur_floor = g->mario->floor;
    a->box_x0 = a->x; a->box_x1 = a->x + 0x22;
    a->box_y0 = a->y; a->box_y1 = a->y + 0x2e;
    return 0;
}

static int roy_top(actor_t *a, gds *g) {                /* m_0367e */
    (void)g;
    if (rd25c(a, 0) == 1) { a->frame = 0x1a; a->dir = 0; }
    else { a->frame = 0x1c; a->dir = 1; }
    a->step = 5;
    a->cur_floor = 0;
    return 1;
}

static int roy_fly(actor_t *a, gds *g) {                /* m_036d0 */
    mario_t *m = g->mario;
    if (a->dir == 0) {
        if (++a->unk_02a > 3) a->unk_02a = 0;
        a->x += 6;
    } else {
        if (--a->unk_02a < 1) a->unk_02a = 3;
        a->x -= 6;
    }
    a->frame = (int2)(a->unk_02a + 0x25);
    a->dir = (m->x < a->x) ? 1 : 0;
    if (iabs(a->x - m->x) < 10) {
        int f = m->floor;
        if (a->x_min < 3) {
            int r = RND(g) & 7;
            if (r < 3) { if (f != 0) f--; }
            else if (r < 6) { if (f != 4) f++; }
        }
        wr25c(a, 0xc, g->floor_y[f] - 0x2e);
        wr25c(a, 4, f);
        a->unk_02a = 0;
        a->step = 6;
    }
    a->box_y0 = a->y; a->box_y1 = a->y + 0x1a;
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x1e;
    return 0;
}

static int roy_drop(actor_t *a, gds *g) {               /* m_038e0 */
    if (a->unk_02a++ == 0) a->frame = 0x1a;
    else { a->frame = 0x19; a->step = 7; a->unk_02a = 0; }
    a->y += 0xc;
    if (rd25c(a, 0xc) < a->y) a->y = (int2)rd25c(a, 0xc);
    a->cur_floor = g->mario->floor;
    a->box_x0 = a->x + 10; a->box_x1 = a->x + 0x18;
    a->box_y0 = a->y + 10; a->box_y1 = a->y + 0x24;
    return 0;
}

static int roy_land(actor_t *a, gds *g) {               /* m_03980 */
    a->frame = 1;
    a->y += 0xc;
    a->cur_floor = g->mario->floor;
    if (rd25c(a, 0xc) <= a->y) {
        a->y = (int2)rd25c(a, 0xc);
        a->cur_floor = (int2)rd25c(a, 4);
        a->unk_02a = 0; a->x_min = 0;
        wr25c(a, 0, 0);
        a->step = 0;
    }
    a->box_x0 = a->x + 4; a->box_x1 = a->x + 0x1e;
    a->box_y0 = a->y + 0x14; a->box_y1 = a->y + 0x2e;
    return 0;
}

static int roy_laugh(actor_t *a, gds *g) {              /* m_03a06: Mario's lost his life */
    if (a->unk_02a < 10) {
        a->frame = (int2)(a->unk_02a + 0x30);
        if (a->frame == 0x33) g->unk_054 = 0xb;
    } else if (a->unk_02a == 10) a->frame = 0x36;
    else if (a->unk_02a == 0xb) a->frame = 0x35;
    else if (a->unk_02a > 0x14) {
        g->mario->state_group = 6;
        g->mario->state = 5;
        g->unk_53a = 0;
        g->mario->fell = 1;
    }
    a->unk_02a++;
    return 0;
}

static int roy_flash(actor_t *a, gds *g) {              /* m_03ad6: stunned after a stomp */
    (void)g;
    if (++a->unk_02a % 3 == 0) a->x_max = (a->x_max == 0);
    a->frame = (int2)(a->x_max + 0xf);
    if (a->unk_02a > 0x13) {
        a->step = 0; wr25c(a, 0, 0);
        a->x_min = 0;
        a->box_y0 = a->y + 0x14;
        a->unk_02a = 0; a->unk_02c = 0;
    }
    return 0;
}

static void knock_off(actor_t *a) { a->state = 4; a->step = 4; a->unk_02a = 5; a->unk_02c = 0; }

static int roy_hit(actor_t *a, gds *g) {                /* m_03b68: stomped */
    switch (a->step) {
    case 1: case 2: case 3: case 6: case 7:
        if (g->unk_496 == 0) { knock_off(a); a->box_y0 = a->y + 0x14; }
        else a->unk_04e = a->cmd;
        break;
    case 4: case 5: case 8:
        return 1;                                        /* can't be stomped: Mario's hurt */
    default:
        if (g->unk_496 == 0) { a->state = 4; a->step = 9; a->unk_02a = 0; }
        else a->unk_04e = a->cmd;
    }
    return 0;
}

static int roy_destroy(actor_t *a, gds *g) {            /* m_03c80: fireball or star */
    if (g->unk_496 == 0) {
        if (a->unk_04c != 0 || g->unk_53a != 0) a->unk_04c = 0;
        knock_off(a);
    } else a->unk_04e = a->cmd;
    return 0;
}

static int roy_dying(actor_t *a, gds *g) { CALL(a->steps_b[a->step], a, g); return 0; }

static int roy_squash(actor_t *a, gds *g) {             /* m_03d4c */
    if (g->unk_53a != 0) { a->step = 4; a->unk_02a = 5; a->unk_02c = 0; }
    draw_at(a, g, 10);
    if (++a->unk_02a > 1) {
        a->state = 3; wr25c(a, 0, 0);
        a->step = 1;
        a->box_y0 += 8;
        a->x_max = 0; a->unk_02a = 0;
    }
    return 0;
}

static int roy_gone(actor_t *a, gds *g) {               /* m_0425e */
    (void)g;
    if (a->unk_02a-- < 2) { a->state = 0; a->cmd = 2; a->timer = 0; a->dir = -1; }
    return 0;
}

static int roy_knocked(actor_t *a, gds *g) {            /* m_03e36 (step 4) / m_03ee8 (step 5) */
    draw_at(a, g, a->unk_02a);
    a->unk_02a++;
    a->step++;
    return 0;
}

/* m_03f8e / m_0407e / m_0416e: falling off, frames 7, 8, 9, 7, 8, 9 ... */
static int roy_fall(actor_t *a, gds *g) {
    a->y += 0x10;
    draw_at_y(a, g, a->unk_02a);
    if (++a->unk_02c > 0) {
        a->unk_02c = 0;
        if (a->step == 8) { a->unk_02a -= 2; a->step = 6; }
        else { a->unk_02a++; a->step++; }
    }
    if (a->y > 0xc3) { a->step = 3; a->unk_02a = 2; }
    return 0;
}

int roy(actor_t *a, gds *g) {
    if (a->state == 0 && a->cmd == 0) {
        obj_tables(a);
        H(0, 0, roy_init);
        H(0, 2, roy_respawn);
        H(1, 2, roy_wait);
        H(2, 1, obj_erase);
        H(2, 2, roy_emerge);
        H(3, 1, obj_erase);
        H(3, 2, roy_moving);
        H(3, 3, roy_hit);
        H(3, 4, roy_destroy);
        H(4, 1, obj_erase);
        H(4, 2, roy_dying);
        W(0, roy_walk);
        W(1, roy_flash);
        W(2, roy_jump);
        W(3, roy_rise);
        W(4, roy_top);
        W(5, roy_fly);
        W(6, roy_drop);
        W(7, roy_land);
        W(8, roy_reach_up);
        W(9, roy_reach_down);
        W(10, roy_into_lift);
        W(11, roy_into_lift);
        W(12, roy_into_door);
        W(13, roy_laugh);
        B(3, roy_gone);
        B(4, roy_knocked);
        B(5, roy_knocked);
        B(6, roy_fall);
        B(7, roy_fall);
        B(8, roy_fall);
        B(9, roy_squash);
    }
    return obj_dispatch(a, g);
}
