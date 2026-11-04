/* mario.c - Mario's state machine, ported from the CD-i code.
 *
 *   mario()           mario_field: called every field (60 Hz) from the signal handler
 *   state_ops[8][15]  the per-state routines (movi_*, in_u_e_*, in_d_e_*, jump_*,
 *                     in_do_*, winn_*, dyin_*, dead_*)
 *   mario_coll_det    collisions with the room's actors (stomps, hits, items)
 *   is_door_elev, sparkle_anim
 *
 * Drawing: the CD-i draws straight into plane B, so a picture stays on screen until
 * something erases it. dm(..., clean_bg) erases (copies the clean background back),
 * slot[n](screen + x + y*384) draws one of Mario's pictures, and dm(..., sprite_gfx)
 * draws a 32x31 tile of the door sheet (the door and lift animations, which have
 * Mario painted in). The port keeps the same model: one persistent Mario picture and
 * a set of door tiles, both removed by erase().
 *
 * Fireballs and kill stars are in fx.c. Not ported yet: sound, the easter egg, the
 * roof exit (unk_820).
 */
#include <stdlib.h>
#include <string.h>
#include "world.h"

static const int16_t jump_height[36] = {             /* jump_height (0x102046) */
    0, 0, 3, 4, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    -1, -1, -1, -1, -1, -1, -2, -2, -2, -3, -3, -3, -3, -3, -4, -3, 0, 0
};
static const int16_t fall_off_edge[10] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0 };   /* per hotel */

#define HOTEL(g)   ((g)->player[(g)->cur_player].hotel)
#define PLAYER(g)  (&(g)->player[(g)->cur_player])
#define FALLS(g)   (HOTEL(g) >= 0 && HOTEL(g) < 10 && fall_off_edge[HOTEL(g)])

/* ------------------------------------------------------------------ drawing */
static void draw(world *w, int slot, int x, int y) {          /* slot[n](screen + x + y*384) */
    w->msp.vis = 1;
    w->msp.slot = slot;
    w->msp.x = x;
    w->msp.y = y;
}

/* dm(x, y, screen, ..., clean_bg): put the background back over Mario's picture */
static void erase(world *w, int x, int y) {
    int f, c;
    w->msp.vis = 0;
    for (f = 0; f < FLOORS; f++)
        for (c = 0; c < COLS; c++) {
            int tx = hm_door_x[c], ty = hm_floor_y[f] - 0x1f;
            if (w->tile[f][c].on && tx < x + 32 && x < tx + 32 && ty < y + 0x29 && y < ty + 31)
                w->tile[f][c].on = 0;
        }
}

/* dm(x, floor_y[f] - 0x1f, screen, sx, sy, sprite_gfx): a door sheet tile at a door */
static void tile(world *w, int x, int floor, int sx, int sy) {
    int c, best = 0;
    for (c = 1; c < COLS; c++)
        if (abs(hm_door_x[c] - x) < abs(hm_door_x[best] - x)) best = c;
    if (floor < 0 || floor >= FLOORS) return;
    w->tile[floor][best].on = 1;
    w->tile[floor][best].sx = (int16_t)sx;
    w->tile[floor][best].sy = (int16_t)sy;
}

/* ------------------------------------------------------------------ helpers */
uint16_t *hm_is_door_elev(world *w, int floor, int x) {        /* is_door_elev */
    int col = hm_doors_at(&w->doors, floor, x);
    if (col < 0) return NULL;
    w->mario.door_col = (int2)col;
    return &w->doors.cell[floor * COLS + col];
}

static void die(mario_t *m) {                 /* the common "start dying" block */
    m->saved_group = m->state_group;
    m->state_group = 6;
    m->fell = 1;
    m->state = 0;
    m->step = 0;
}

static void bump(gds *g, mario_t *m) {
    (void)g;
    if (m->bumped == 0) {
        hm_afx(0xe);
        m->bumped = 1;
    }
}

/* the screen-edge test at the end of every moving routine */
static void edge(gds *g, mario_t *m, int left) {
    if (!FALLS(g)) {
        if (!left) {
            if (m->x >= 0x14b) { m->x = 0x14a; bump(g, m); }
            else if (m->x < 0x14a) m->bumped = 0;
        } else {
            if (m->x < 0x18) { m->x = 0x18; bump(g, m); }
            else if (m->x > 0x18) m->bumped = 0;
        }
    } else if (!left ? m->x > 0x156 : m->x < 0xc) {
        die(m);
    }
}

static int snap_door_x(int x) {               /* the door_x boundaries of in_do_go_in etc. */
    if (x < 0xb0) return x < 0x58 ? 0x2c : (x < 0x84 ? 0x58 : 0x84);
    if (x < 0x108) return x < 0xdc ? 0xb0 : 0xdc;
    return x < 0x134 ? 0x108 : 0x134;
}

/* ------------------------------------------------------------------ group 0: moving */
static void movi_ru(world *w, gds *g, mario_t *m, int left) {     /* movi_ru_fo / movi_ru_ba */
    erase(w, m->old_x, m->y - 1);
    if (--m->anim_count < 1) {
        m->anim_count = m->anim_speed * (w->move_step == 1 ? 2 : 1);
        m->anim = (m->anim + 1) % 4;
    }
    m->x += left ? -w->move_step : w->move_step;
    edge(g, m, left);
    draw(w, m->anim + (left ? 4 : 0), m->x, m->y);
    m->old_x = m->x;
}

static void movi_sh_ru(world *w, gds *g, mario_t *m, int left) {  /* movi_sh_ru_fo / _ba */
    if (--m->throw_timer == 0) m->state = left;
    erase(w, m->old_x, m->y - 1);
    m->x += left ? -w->move_step : w->move_step;
    edge(g, m, left);
    draw(w, left ? 0x36 : 0x35, m->x, m->y);
    m->old_x = m->x;
}

static void movi_tu(world *w, mario_t *m, int left) {              /* movi_tu_fo / _ba */
    erase(w, m->old_x, m->y - 1);
    draw(w, m->anim + (left ? 0x21 : 0x1e), m->x, m->y);
    m->anim_count -= 2;
    if (m->anim_count < 1) {
        m->anim_count = m->anim_speed;
        if (++m->anim == 3) {
            m->anim = 0;
            m->state = left;
        }
    }
    m->old_x = m->x;
}

static void movi_st(world *w, gds *g, mario_t *m, int left) {     /* movi_st_fo / _ba */
    int base = left ? 4 : 0;
    erase(w, m->old_x, m->y - 1);
    switch (m->anim) {
    case 1: case 3: case 5: case 7: m->anim = 0; break;
    case 2: m->anim = 1; break;
    case 6: m->anim = 7; break;
    default:
        draw(w, m->anim + base, m->x, m->y);
        return;
    }
    m->x += left ? -w->move_step : w->move_step;
    edge(g, m, left);
    draw(w, m->anim + base, m->x, m->y);
    m->old_x = m->x;
}

static void movi_sh_st(world *w, mario_t *m, int left) {           /* movi_sh_st_fo / _ba */
    if (--m->throw_timer == 0) m->state = 6 + left;
    erase(w, m->x, m->y - 1);
    draw(w, left ? 0x36 : 0x35, m->x, m->y);
}

static void movi_sk(world *w, gds *g, mario_t *m, int left) {     /* movi_sk_fo / _ba: skid */
    erase(w, m->old_x, m->y - 1);
    if (HOTEL(g) == 3) {
        m->x += left ? -w->move_step : w->move_step;
        m->anim_count -= 1;
    } else {
        m->anim_count -= 3;
    }
    edge(g, m, left);
    draw(w, m->anim + (left ? 0x33 : 0x31), m->x, m->y);
    if (m->anim_count < 1) {
        m->anim_count = m->anim_speed;
        if (++m->anim == 2) {
            m->anim = 0;
            if (left) m->state = g->pad_x == 0 ? 7 : 4;
            else      m->state = g->pad_x == 0 ? 6 : 5;
        }
    }
    m->old_x = m->x;
}

static void movi_du(world *w, gds *g, mario_t *m, int left) {     /* movi_du_fo / _ba: duck */
    erase(w, m->old_x, m->y - 1);
    if (HOTEL(g) == 3 && m->anim_count > 0) {
        m->anim_count--;
        m->x += left ? -w->move_step : w->move_step;
    }
    edge(g, m, left);
    draw(w, left ? 0x38 : 0x37, m->x, m->y);
    m->old_x = m->x;
}

static void movi_stan(world *w, mario_t *m) {
    erase(w, m->x, m->y - 1);
    draw(w, 24, m->x, m->y);
}

/* ------------------------------------------------------------------ group 3: jumping */
static void jump_erase(world *w, gds *g, mario_t *m) {
    (void)g;
    erase(w, m->x, m->y - 1);
}

static int jump_slot(mario_t *m, int st) {
    if (m->throw_timer < 1) return st + ((m->prev_state & 1) ? 0x10 : 8);
    return (m->prev_state & 1) ? 0x36 : 0x35;
}

static void jump_drift(world *w, mario_t *m, int step) {
    int left = m->prev_state & 1;
    if (m->jump_move == 1) {
        if (m->jump_drift == 0) m->x += left ? -w->move_step : w->move_step;
    } else if (m->jump_drift != 0 && step < 0x15 && (m->drift_toggle ^= 1) != 0) {
        m->x += left ? -w->move_step : w->move_step;
    }
}

static void jump_edge(gds *g, mario_t *m) {
    edge(g, m, m->prev_state & 1);
}

static void jump_crou_leap(world *w, gds *g, mario_t *m, int next) {   /* jump_crou / jump_leap */
    int y = m->y, st = m->state, s = m->step + 1, dy;
    jump_erase(w, g, m);
    dy = jump_height[s];
    m->throw_timer -= 2;
    draw(w, jump_slot(m, st), m->x, y - dy);
    m->step = (int2)s;
    m->y = (int2)(y - dy);
    m->state = (int2)next;
}

static void jump_rise(world *w, gds *g, mario_t *m) {
    int s0 = m->step, y = m->y, st = m->state, s = s0 + 1, dy;
    jump_erase(w, g, m);
    dy = jump_height[s];
    m->throw_timer--;
    jump_drift(w, m, s);
    draw(w, jump_slot(m, st), m->x, y - dy);
    if (s == 10) {
        if (m->jump_held == 0) {            /* let go early: a lower jump */
            m->jump_skip = 1;
            s = s0 + 2;
        }
        st = 3;
    }
    m->state = (int2)st;
    m->step = (int2)s;
    jump_edge(g, m);
    m->y = (int2)(y - dy);
}

static void jump_ar_1(world *w, gds *g, mario_t *m) {     /* the top of the jump */
    int s0 = m->step, y = m->y, st = m->state, s = s0 + 1;
    jump_erase(w, g, m);
    if (m->jump_hang == 0) y -= jump_height[s];
    m->throw_timer--;
    jump_drift(w, m, s);
    draw(w, jump_slot(m, st), m->x, y);
    if (s == 0x11) {
        if (m->jump_held == 0 || m->jump_hang != 0) {
            if (--m->anim_count < 1 || m->jump_held == 0) {
                st = 4;
                m->anim_count = 0;
                m->jump_hang = 0;
                goto done;
            }
        } else {                            /* still holding the button: hang a while */
            m->anim_count = 6;
            m->jump_hang = 1;
        }
        s = s0;
    }
done:
    m->state = (int2)st;
    m->step = (int2)s;
    jump_edge(g, m);
    m->y = (int2)y;
}

static void jump_ar_2(world *w, gds *g, mario_t *m) {
    int s0 = m->step, y = m->y, st = m->state, s = s0 + 1, dy;
    jump_erase(w, g, m);
    dy = jump_height[s];
    m->throw_timer--;
    jump_drift(w, m, s);
    draw(w, jump_slot(m, st), m->x, y - dy);
    if (s == 0x18) {
        if (m->jump_skip != 0) {
            s = s0 + 2;
            m->jump_skip = 0;
        }
        st = 5;
    }
    m->state = (int2)st;
    m->step = (int2)s;
    jump_edge(g, m);
    m->y = (int2)(y - dy);
}

static void jump_fall_touc(world *w, gds *g, mario_t *m, int touch) {   /* jump_fall / jump_touc */
    int y = m->y, st = m->state, s = m->step + 1, dy;
    jump_erase(w, g, m);
    dy = jump_height[s];
    m->throw_timer--;
    jump_drift(w, m, s);
    draw(w, jump_slot(m, st), m->x, y - dy);
    if (touch) st = 7;
    else if (s == 0x21) st = 6;
    m->state = (int2)st;
    m->step = (int2)s;
    jump_edge(g, m);
    m->y = (int2)(y - dy);
}

static void jump_land(world *w, gds *g, mario_t *m) {
    int y = m->y, st = m->state, s = m->step + 1, dy;
    jump_erase(w, g, m);
    dy = jump_height[s];
    m->throw_timer--;
    draw(w, jump_slot(m, st), m->x, y - dy);
    m->state_group = 0;
    m->anim = 1;
    m->old_x = m->x;
    m->step = (int2)s;
    m->y = (int2)(y - dy);
    m->state = (int2)((m->prev_state & 1) | 6);
}

static void jump_boun(world *w, gds *g, mario_t *m) {     /* bouncing off an enemy */
    int y = m->y, s = 0x23 - m->step, dy, st;
    jump_erase(w, g, m);
    dy = jump_height[s];
    st = s < 0x11 ? (s < 10 ? 2 : 3) : 4;
    m->throw_timer--;
    draw(w, jump_slot(m, st), m->x, y - dy);
    m->step = (int2)s;
    m->y = (int2)(y - dy);
    m->state = (int2)st;
}

/* ------------------------------------------------------------------ groups 1, 2: lifts */
#define INV(m) ((m)->invincible != 0)

/* in_u_e_wait / in_d_e_wait */
static void in_e_wait(world *w, gds *g, mario_t *m, int up) {
    if (INV(m)) tile(w, m->x, m->floor, up ? 0x66 : 0x66, 0x84);
    if ((*m->door & 0xf000) == (up ? 0xe000 : 0xf000)) {
        m->state_group = up ? 2 : 1;
        m->state = 0;
        m->step = 0;
        return;
    }
    if ((g->pad_buttons & 1) == 0 && m->lift_used == 0) {
        if (g->pad_y != 4) return;
        m->state = 2;                       /* step out */
        m->sub = 5;
        m->step = 0;
    } else {
        m->lift_used = 0;
        m->state = up ? 3 : 4;              /* ride */
        m->sub = 0x1f;
        m->step = 0;
        hm_afx(3);
    }
    m->ready = 0;
}

/* in_u_e_go_in / in_d_e_go_in: walk into the lift */
static void in_e_go_in(world *w, gds *g, mario_t *m, int up) {
    int s = ++m->step, sx = -1, sy = 0x42;
    int a = up ? 0 : 0xcc;                  /* tile columns: up 0,22,44,66,88 / down cc,ee,110,132,154 */
    (void)g;
    switch (m->sub) {
    case 1:
        if (s == 10) { tile(w, m->x, m->floor, a + 0x22, 0x42); m->sub = 2; return; }
        if (s < 6 || !INV(m)) {
            if (s != 5) return;
            erase(w, m->x, hm_floor_y[m->floor] - 0x1f);
            m->x = (int2)snap_door_x(m->x + 0x10);
            tile(w, m->x, m->floor, a, 0x42);
            return;
        }
        sx = a;
        break;
    case 2:
        if (s == 0x14) { tile(w, m->x, m->floor, a + 0x66, 0x42); m->sub = 3; return; }
        if ((s < 0x10 || !INV(m)) && s != 0xf) {
            if (!INV(m)) return;
            sx = a + 0x22;
        } else sx = a + 0x44;
        break;
    case 3:
        if (s == 0x23) {
            tile(w, m->x, m->floor, a + 0x66, 0x84);
            m->state = 0;
            m->step = 0;
            m->lift_used = 1;
            return;
        }
        if ((s < 0x1f || !INV(m)) && s != 0x1e) {
            if ((s < 0x1a || !INV(m)) && s != 0x19) {
                if (!INV(m)) return;
                sx = a + 0x66;
            } else sx = a + 0x88;
        } else { sx = a + 0x88; sy = 0x63; }
        break;
    default:
        return;
    }
    tile(w, m->x, m->floor, sx, sy);
}

/* in_u_e_come / in_d_e_come: step out of the lift */
static void in_e_come(world *w, gds *g, mario_t *m, int up) {
    int s = ++m->step, sx, sy;
    int a = up ? 0 : 0xcc;
    switch (m->sub) {
    case 5:
        if (s == 10) { tile(w, m->x, m->floor, a + 0x88, 0x84); m->sub = 6; return; }
        if (!INV(m)) return;
        sx = a + 0x66; sy = 0x84;
        break;
    case 6:
        if (s == 0xf) { tile(w, m->x, m->floor, a + 0x66, 0x63); m->sub = 7; return; }
        if (!INV(m)) return;
        sx = a + 0x88; sy = 0x84;
        break;
    case 7:
        if (s == 0x1e) {
            tile(w, m->x, m->floor, a, 0x63);
            m->sub = 2;
            m->state_group = 0;
            m->state = 0xe;
            g->pad_prev = 0;
            return;
        }
        sy = 0x63;
        if ((s > 0x19 && INV(m)) || s == 0x19) sx = a + 0x22;
        else if ((s > 0x14 && INV(m)) || s == 0x14) sx = a + 0x44;
        else if (!INV(m)) return;
        else sx = a + 0x66;
        break;
    default:
        return;
    }
    tile(w, m->x, m->floor, sx, sy);
}

/* in_u_e_go_up / in_d_e_go_do: the ride. The old position is (old_x, old_floor). */
static void in_e_ride(world *w, gds *g, mario_t *m, int up) {
    int s = ++m->step;
    int a = up ? 0 : 0xcc;                  /* this lift's tile column base */
    int b = up ? 0xcc : 0;                  /* the other end's ("arrival") lift art */
    switch (m->sub) {
    case 0x1f:                              /* doors close, Mario moves to the other end */
        if (s == 5) {
            if (INV(m)) tile(w, m->old_x, m->old_floor, a + 0x66, 0x84);
            m->sub = 7;
            m->step = 0;
            m->lift_moved = 0;
            return;
        }
        if (s > 0) {
            if (g->unk_49a == 0 && m->lift_moved == 0) {
                uint16_t v = *m->door;
                m->old_floor = m->floor;
                m->old_x = m->x;
                m->x = g->door_x[(v & 0xf0) >> 4];
                m->floor = (int2)((v & 0xf00) >> 8);
                m->old_y = m->y;
                m->y = (int2)(g->floor_y[m->floor] - 0x1e);
                m->unk_146 = 1;
                m->lift_moved = 1;
            }
            if (INV(m)) {
                if (m->lift_moved == 0) tile(w, m->x, m->floor, a + 0x66, 0x84);
                else tile(w, m->old_x, m->old_floor, a + 0x66, 0x84);
            }
        }
        if (s != 0 || !INV(m)) return;
        tile(w, m->x, m->floor, a + 0x66, 0x84);
        return;
    case 7:                                 /* the lift doors close behind him */
        if (s == 0xf) { tile(w, m->old_x, m->old_floor, a, 0x84); m->sub = up ? 0xb : 0xd; return; }
        if ((s < 0xb || !INV(m)) && s != 10) {
            if ((s < 6 || !INV(m)) && s != 5) {
                if (!INV(m)) return;
                tile(w, m->old_x, m->old_floor, a + 0x66, 0x84);
            } else tile(w, m->old_x, m->old_floor, a + 0x44, 0x84);
        } else tile(w, m->old_x, m->old_floor, a + 0x22, 0x84);
        return;
    case 0xb:                               /* (up) waiting */
    case 0xd:                               /* (down) */
        if (up && w->roof_col >= 0 && !w->upper &&
            m->door == &w->doors.cell[w->roof_col]) return;
        if (s == 0x14) {
            tile(w, m->old_x, m->old_floor, 0xaa, up ? 0x21 : 0x63);
            m->sub = up ? 0xc : 0xe;
            m->step = 0;
            m->unk_146 = 0;
            return;
        }
        if (INV(m)) tile(w, m->old_x, m->old_floor, a, 0x84);
        return;
    case 0xc:                               /* (up) the arrow light moves over */
    case 0xe:                               /* (down) */
        if (s == 10) {
            tile(w, m->old_x, m->old_floor, a, 0x21);           /* back to a plain lift */
            tile(w, m->x, m->floor, 0xaa, up ? 0x63 : 0x21);
            m->sub = 1;
            return;
        }
        if (s < 6 || !INV(m)) {
            if (s != 5) {
                if (!INV(m)) return;
                tile(w, m->old_x, m->old_floor, 0xaa, up ? 0x21 : 0x63);
                tile(w, m->x, m->floor, b, 0x21);
                return;
            }
            tile(w, m->old_x, m->old_floor, 0xaa, up ? 0x42 : 0x84);
        }
        tile(w, m->x, m->floor, 0xaa, up ? 0x84 : 0x42);
        return;
    case 1:                                 /* arrival: the doors open with Mario inside */
        if (s == 0x19) {
            tile(w, m->x, m->floor, b + 0x66, 0x84);
            m->door = hm_is_door_elev(w, m->floor, m->x);
            m->state_group = up ? 2 : 1;
            m->state = 0;
            m->step = 0;
            return;
        }
        if ((s < 0x15 || !INV(m)) && s != 0x14) {
            if ((s < 0x10 || !INV(m)) && s != 0xf) {
                if (!INV(m)) return;
                tile(w, m->x, m->floor, 0xaa, up ? 0x63 : 0x21);
            } else tile(w, m->x, m->floor, b + 0x22, 0x84);
        } else tile(w, m->x, m->floor, b + 0x44, 0x84);
        return;
    }
}

/* ------------------------------------------------------------------ group 4: doors */
static void in_do_wait(world *w, gds *g, mario_t *m) {
    if (INV(m)) tile(w, m->x, m->floor, 0x110, 0);
    if (++m->step >= 6 && g->pad_y == 4) {
        m->state = 2;
        m->sub = 0x1d;
        m->step = 0;
    }
}

static void in_do_go_in(world *w, gds *g, mario_t *m) {
    int s = ++m->step;
    (void)g;
    if (m->sub == 0x16) {
        if (s == 5) {
            erase(w, m->x, hm_floor_y[m->floor] - 0x1f);
            m->x = (int2)snap_door_x(m->x + 0x10);
            tile(w, m->x, m->floor, 0xcc, 0);
            m->sub = 0x1c;
        }
    } else if (m->sub == 0x1c) {
        if (s == 0xf) {
            tile(w, m->x, m->floor, 0x110, 0);
            m->state = 0;
            m->step = 0;
        } else if ((s < 0xb || !INV(m)) && s != 10) {
            if (INV(m)) tile(w, m->x, m->floor, 0xcc, 0);
        } else {
            tile(w, m->x, m->floor, 0xee, 0);
        }
    }
}

static void in_do_come(world *w, gds *g, mario_t *m) {
    int s = ++m->step;
    switch (m->sub) {
    case 0x18:
        if (s == 0xf) {
            tile(w, m->x, m->floor, 0x154, 0);
            m->sub = 2;
            m->state_group = 0;
            m->state = 0xe;
            g->pad_prev = 0;
            return;
        }
        break;
    case 0x1a:
        if (s == 10) { m->sub = 0x18; return; }
        break;
    case 0x1d:
        if (s == 5) { tile(w, m->x, m->floor, 0x132, 0); m->sub = 0x1a; return; }
        if (INV(m)) tile(w, m->x, m->floor, 0x110, 0);
        return;
    default:
        return;
    }
    if (INV(m)) tile(w, m->x, m->floor, 0x132, 0);
}

/* ------------------------------------------------------------------ group 5: stage cleared */
static void winn_danc(world *w, gds *g, mario_t *m) {
    int y = m->y, cnt = m->anim_count - 1, pic = m->anim, s = m->step + 1;
    (void)g;
    erase(w, m->old_x, y - 1);
    if (cnt < 1) {
        cnt = m->anim_speed;
        if (pic == 0x24) { pic = 0x25; m->x += 2; }
        else { pic = 0x24; m->x -= 2; }
    }
    draw(w, pic, m->x, y);
    m->old_x = m->x;
    if (s == 0x96) {
        m->state = 1;
        pic = 0x26;
        s = 0;
    }
    m->step = (int2)s;
    m->anim_count = (int2)cnt;
    m->anim = (int2)pic;
}

static void winn_prep(world *w, gds *g, mario_t *m) {
    int s = m->step + 1, pic = m->anim;
    (void)g;
    erase(w, m->x, m->y - 1);
    draw(w, pic, m->x, m->y);
    if (s > 0x1d) {
        m->state = 2;
        pic = 0;
        s = 0;
    }
    m->step = (int2)s;
    m->anim = (int2)pic;
}

static void winn_spin(world *w, gds *g, mario_t *m) {   /* spins up off the top of the screen */
    int s = m->step + 1, y;
    (void)g;
    if (s > 4) {
        s = 0;
        if (++m->anim == 4) m->anim = 0;
    }
    erase(w, m->x, m->y);
    y = m->y - 2;
    m->step = (int2)s;
    m->y = (int2)y;
    if (y < 1) {
        m->invincible = 0;
        m->state_group = 6;
        m->state = 5;                       /* (dyin_end: finished) */
        return;
    }
    draw(w, m->anim + 0x27, m->x, y);
}

/* ------------------------------------------------------------------ group 6: dying */
static void dyin_star(world *w, gds *g, mario_t *m) {
    int st = m->state, s = m->step + 1;
    if (m->saved_group == 1 && m->prev_state == 3) tile(w, m->old_x, m->old_floor, 0, 0x21);
    if (m->saved_group == 2 && m->prev_state == 4) tile(w, m->old_x, m->old_floor, 0xcc, 0x21);
    erase(w, m->x, m->y - 1);
    if (g->frame % 6 != 0) draw(w, st + 0x19, m->x, m->y);
    if (s > 0x27) { st = 1; s = 0; }
    hm_afx(7);
    m->step = (int2)s;
    m->state = (int2)st;
}

static void dyin_eyes(world *w, gds *g, mario_t *m) {
    int st = m->state, s = m->step + 1;
    erase(w, m->x, m->y - 1);
    if (s == 0x1d) { s = 0; st = 2; }
    if (g->frame % 6 != 0) draw(w, st + 0x19, m->x, m->y);
    m->step = (int2)s;
    m->state = (int2)st;
}

static void dyin_fall(world *w, gds *g, mario_t *m) {   /* dyin_fa_1/2/3: falls off the screen */
    int st = m->state, s = m->step + 1, y = m->y;
    erase(w, m->x, y);
    y += 2;
    if (y < 0xd9) {
        if (g->frame % 6 != 0) draw(w, st + 0x19, m->x, y);
        if (st == 2 && s == 10) st = 3;
        else if (st == 3 && s == 0x14) st = 4;
        else if (st == 4 && s == 0x1e) { s = 0; st = 2; }
        m->state = (int2)st;
    } else {
        m->invincible = 0;
        m->state = 5;
    }
    m->step = (int2)s;
    m->y = (int2)y;
}

/* ------------------------------------------------------------------ group 7: stage start */
static void dead_init(world *w, gds *g, mario_t *m) {
    int sf = *(int16_t *)(g->stage_info + 0xe), sc = *(int16_t *)(g->stage_info + 0x10);
    if (sf < 0 || sf >= FLOORS) sf = FLOORS - 1;
    if (sc < 0 || sc >= COLS) sc = 0;
    m->anim_speed = HOTEL(g) == 3 ? 3 : 5;
    m->floor = (int2)sf;
    m->old_floor = m->floor;
    m->x = g->door_x[sc];
    m->old_x = m->x;
    m->y = (int2)(g->floor_y[m->floor] - 0x1e);
    m->old_y = m->y;
    m->door = hm_is_door_elev(w, m->floor, m->x);
    if (m->door == NULL) m->door = &w->doors.cell[sf * COLS + sc];
    if ((w->doors.cell[sf * COLS + sc] & 0xf000) == 0xf000) {
        m->state_group = 2;                 /* arrives in the up lift: in_d_e_go_do's arrival */
        m->state = 4;
    } else {
        m->state_group = 1;
        m->state = 3;
    }
    m->step = 0;
    m->saved_group = 0;
    m->prev_state = 0;
    m->sub = 1;
    m->invincible = 0;
    m->fire_delay = 0;
    m->ready = 1;
}

/* ------------------------------------------------------------------ state_ops */
static void state_op(world *w, gds *g, mario_t *m) {
    int st = m->state;
    switch (m->state_group) {
    case 0:
        switch (st) {
        case 0: case 1:   movi_ru(w, g, m, st & 1); break;
        case 2: case 3:   movi_sh_ru(w, g, m, st & 1); break;
        case 4: case 5:   movi_tu(w, m, st & 1); break;
        case 6: case 7:   movi_st(w, g, m, st & 1); break;
        case 8: case 9:   movi_sh_st(w, m, st & 1); break;
        case 10: case 11: movi_sk(w, g, m, st & 1); break;
        case 12: case 13: movi_du(w, g, m, st & 1); break;
        case 14:          movi_stan(w, m); break;
        }
        break;
    case 1:
    case 2: {
        int up = m->state_group == 1;
        switch (st) {
        case 0: in_e_wait(w, g, m, up); break;
        case 1: in_e_go_in(w, g, m, up); break;
        case 2: in_e_come(w, g, m, up); break;
        case 3: if (up) in_e_ride(w, g, m, 1); break;     /* in_u_e_go_up */
        case 4: if (!up) in_e_ride(w, g, m, 0); break;    /* in_d_e_go_do */
        }
        break;
    }
    case 3:
        switch (st) {
        case 0: jump_crou_leap(w, g, m, 1); break;
        case 1: jump_crou_leap(w, g, m, 2); break;
        case 2: jump_rise(w, g, m); break;
        case 3: jump_ar_1(w, g, m); break;
        case 4: jump_ar_2(w, g, m); break;
        case 5: jump_fall_touc(w, g, m, 0); break;
        case 6: jump_fall_touc(w, g, m, 1); break;
        case 7: jump_land(w, g, m); break;
        case 8: jump_boun(w, g, m); break;
        }
        break;
    case 4:
        switch (st) {
        case 0: in_do_wait(w, g, m); break;
        case 1: in_do_go_in(w, g, m); break;
        case 2: in_do_come(w, g, m); break;
        }
        break;
    case 5:
        switch (st) {
        case 0: winn_danc(w, g, m); break;
        case 1: winn_prep(w, g, m); break;
        case 2: winn_spin(w, g, m); break;
        }
        break;
    case 6:
        switch (st) {
        case 0: dyin_star(w, g, m); break;
        case 1: dyin_eyes(w, g, m); break;
        case 2: case 3: case 4: dyin_fall(w, g, m); break;
        case 5: break;                      /* dyin_end */
        }
        break;
    case 7:
        if (st == 0) dead_init(w, g, m);    /* (1: dead_bein does nothing) */
        break;
    }
}

/* ------------------------------------------------------------------ the controller */
static void walk_speed(gds *g, mario_t *m) {         /* the "hotel 3 skids further" choice */
    if (HOTEL(g) == 3) m->anim_count = 0x12;
    else m->anim_count = m->anim_speed;
}

/* the part of mario() that runs every third field: read the pad, pick the next state */
static void controller(world *w, gds *g, mario_t *m, uint32_t held) {
    int px, py;
    uint32_t b;

    m->fcount = 0;
    g->held_b1++;
    g->held_b2++;

    /* the pointing device, as directions: x 1 left / 2 right, y 3 up / 4 down */
    px = 0;
    if ((held & IN_LEFT) && !(held & IN_RIGHT)) px = 1;
    if ((held & IN_RIGHT) && !(held & IN_LEFT)) px = 2;
    py = 0;
    if ((held & IN_DOWN) && (px == 0 || m->state_group == 1 || m->state_group == 2)) py = 4;
    else if ((held & IN_UP) && px == 0) py = 3;
    b = ((held & IN_B1) ? 1 : 0) | ((held & IN_B2) ? 2 : 0);
    g->pad_x = px;
    g->pad_y = py;

    m->both_buttons = 0;
    m->fire = 0;
    m->fire_delay--;
    if (held & IN_ITEM) {
        if (m->fire_delay < 1) {
            m->fire = 1;
            m->fire_delay = 4;
        }
    }
    if (m->jump_held != 0 && (b & 1) == 0) m->jump_held = 0;
    /* buttons count only on the sample where they go down */
    if ((g->pad_prev & 1) == (b & 1)) b &= ~1u; else g->pad_prev ^= 1;
    if ((g->pad_prev & 2) == (b & 2)) b &= ~2u; else g->pad_prev ^= 2;
    g->pad_buttons = b;

    if (m->state_group == 3) {              /* steering in the air */
        if (px == 1) {
            m->jump_drift = ((m->prev_state & 1) == 0 || m->jump_move == 0) ? 1 : 0;
            m->prev_state = 7;
            m->jump_move = 1;
        } else if (px == 2) {
            m->jump_drift = ((m->prev_state & 1) == 0 && m->jump_move != 0) ? 0 : 1;
            m->prev_state = 6;
            m->jump_move = 1;
        } else {
            if (m->jump_move == 1) m->jump_drift = 1;
            m->prev_state = (int2)((m->prev_state & 1) | 6);
            m->jump_move = 0;
        }
        if (m->fire != 0 && PLAYER(g)->power == 2) {     /* a fireball from the air */
            int x;
            if ((m->prev_state & 1) == 0) x = m->x + (m->jump_move == 1 ? 0x20 : 0xe);
            else x = m->jump_move == 1 ? m->x - 0xe : m->x + 4;
            if (fx_throw(w, x, m->y + 0x14, m->floor, m->prev_state & 1)) {
                m->throw_timer = 9;
                return;
            }
        }
    }
    if (m->state_group != 0) return;
    if (m->fire != 0 && PLAYER(g)->power == 2 && m->state != 0xe) {   /* a fireball */
        int x, dir, st;
        if (px == 1)                { st = 3; dir = 1; x = m->x - 0x10; }
        else if (px == 2)           { st = 2; dir = 0; x = m->x + 0x22; }
        else if ((m->state & 1) == 0) { st = 8; dir = 0; x = m->x + 0xc; }
        else                        { st = 9; dir = 1; x = m->x + 6; }
        if (fx_throw(w, x, g->floor_y[m->floor] - 0x1c, m->floor, dir)) {
            /* Dedicated fire can accompany a fresh jump/door press. */
            if ((g->pad_buttons & 3) == 0) {
                m->state = (int2)st;
                m->throw_timer = 9;
                return;
            }
        }
    }

    /* Dedicated jump/door buttons can interrupt the throwing pose. */
    if (g->pad_buttons & 3) {
        if (m->state == 2 || m->state == 3) m->state -= 2;
        else if (m->state == 8 || m->state == 9) m->state -= 2;
    }
    m->prev_state = m->state;
    if (px == 0) {
        switch (m->state) {
        case 0: walk_speed(g, m); m->anim = 0; m->state = 10; break;
        case 1: walk_speed(g, m); m->anim = 0; m->state = 11; break;
        case 2: case 3: return;
        case 4: m->state = 6; break;
        case 5: m->state = 7; break;
        case 0xc: if (py != 4) { m->anim_count = m->anim_speed; m->anim = 0; m->state = 6; } break;
        case 0xd: if (py != 4) { m->anim_count = m->anim_speed; m->anim = 0; m->state = 7; } break;
        }
    } else if (px == 1) {
        switch (m->state) {
        case 0: walk_speed(g, m); m->anim = 0; m->state = 10; break;
        case 2: case 3: return;
        case 4: m->state = 6; break;
        case 6: case 7: case 0xb: case 0xc: case 0xd: case 0xe:
            m->anim = 0;
            m->anim_count = m->anim_speed;
            m->state = 1;
            break;
        default: break;
        }
    } else {
        switch (m->state) {
        case 1: walk_speed(g, m); m->anim = 0; m->state = 11; break;
        case 2: case 3: return;
        case 5: m->state = 7; break;
        case 6: case 7: case 10: case 0xc: case 0xd: case 0xe:
            m->anim_count = m->anim_speed;
            m->anim = 0;
            m->state = 0;
            break;
        default: break;
        }
    }

    if (py == 4) {                          /* ducking */
        m->down_count++;
        if (m->down_count >= 2) {
            m->down_count--;
            switch (m->state) {
            case 2: case 3: case 0xc: case 0xd:
                return;
            case 6:
                if (m->prev_state == 0) { walk_speed(g, m); m->anim = 0; m->state = 10; goto buttons; }
                /* fall through */
            case 0: case 4: case 0xe:
                m->state = 0xc;
                m->anim_count = 0;
                return;
            case 7:
                if (m->prev_state == 1) { walk_speed(g, m); m->anim = 0; m->state = 11; goto buttons; }
                /* fall through */
            case 1: case 5:
                m->state = 0xd;
                m->anim_count = 0;
                return;
            case 10: m->state = 0xc; m->anim_count = 0x1b; return;
            case 11: m->state = 0xd; m->anim_count = 0x1b; return;
            default: break;
            }
        }
    } else {
        m->down_count = 0;
    }

buttons:
    if ((g->pad_buttons & 2) && g->held_b2 >= 4 && m->both_buttons == 0) {
        /* button 2: open / close the door, or go through a special door */
        uint16_t *c = hm_is_door_elev(w, m->floor, m->x);
        m->door = c;
        if (c && (*c & 1) == 0) {
            switch (*c & 0xf000) {
            case 0x1000:
            case 0x2000: {
                int32_t bonus = 0;
                int was_open = (*c & 0xf000) == 0x2000;
                hm_afx(hm_doors_toggle(&w->doors, m->floor, m->door_col, &bonus));
                PLAYER(g)->score += bonus;
                if (was_open) g->unk_550[2 * (m->floor * COLS + m->door_col) + 1] = 1;
                if (was_open && w->doors.open == 1) m->hurt_timer = 0xc;
                break;
            }
            case 0x4000:
                hm_afx(2);
                m->state_group = 4;
                m->state = 1;
                m->sub = 0x16;
                m->step = 0;
                break;
            }
            g->held_b2 = 0;
        }
    } else if ((g->pad_buttons & 1) && g->held_b1 >= 4 && m->both_buttons == 0) {
        /* button 1: jump */
        hm_afx(0);
        m->jump_move = m->state < 6 ? 1 : 0;
        m->jump_held = 1;
        m->jump_hang = 0;
        m->jump_skip = 0;
        m->jump_drift = 0;
        m->anim_count = 0;
        m->prev_state = m->state;
        m->state_group = 3;
        m->state = 0;
        m->step = -1;
        g->held_b1 = 0;
    } else if (py == 3) {
        /* up: into an open door, or into a lift */
        uint16_t *c = hm_is_door_elev(w, m->floor, m->x);
        m->door = c;
        if (c) {
            switch (*c & 0xf000) {
            case 0x2000: m->state_group = 4; m->state = 1; m->sub = 0x16; break;
            case 0xe000: m->state_group = 2; m->state = 1; m->sub = 1; break;
            case 0xf000: m->state_group = 1; m->state = 1; m->sub = 1; break;
            default: return;
            }
            m->step = 0;
        }
    }
}

/* ------------------------------------------------------------------ collisions */
static int send(actor_t *a, gds *g, int cmd) {
    int old = a->cmd, r;
    a->cmd = (int2)cmd;
    r = ((int (*)(actor_t *, gds *))a->handler)(a, g);
    a->cmd = (int2)old;
    return r;
}

static void hurt(gds *g, mario_t *m) {
    if (world_invulnerable(g)) return;
    if (m->hurt_timer >= 1) return;
    if (PLAYER(g)->power == 0) {
        die(m);
    } else {
        m->hurt_timer = 100;
        PLAYER(g)->power--;
        PLAYER(g)->unk_0c = 1;
        hm_afx(10);
    }
}

static void killed_enemy(world *w, mario_t *m, const actor_t *a, int y1) {
    int cx;
    m->hit = 1;
    hm_afx(5);
    if (a->unk_03c == 0) cx = (a->box_x0 + a->box_x1) >> 1;
    else cx = (a->hit_x0 + a->hit_x1 + a->box_x0 + a->box_x1) >> 2;
    fx_kill_star(w, (cx & ~1) - 6, ((a->box_y0 + y1) >> 1) + 6);
}

static void got_item(gds *g, mario_t *m, actor_t *a) {
    player_t *p = PLAYER(g);
    switch (a->type) {
    case 1:                                 /* star */
        hm_afx(0xb);
        m->star_timer = m->cooldown[0] < 1 ? 1000 : 0x1c20;
        m->cooldown[4] = 0xc;
        m->item_flash = 0x24;
        m->invincible = 1;
        m->unk_146 = 0;
        break;
    case 2:                                 /* 1-up */
        hm_afx(8);
        if (p->lives < 99) p->lives++;
        if (m->cooldown[0] > 0) g->item_hit = 0xc;
        m->cooldown[3] = 0xc;
        break;
    case 3:                                 /* fire flower */
        if (m->cooldown[0] < 1) hm_afx(0xb);
        if (m->cooldown[0] >= 1) {
            g->item_hit = 0xb;
            if (p->lives < 99) p->lives++;
        }
        m->cooldown[2] = 0xc;
        m->item_flash = 0x24;
        p->power = 2;
        p->unk_0c = 1;
        break;
    case 4:                                 /* coin */
        if (a->start_mode != 2) hm_afx(6);
        else {
            if (m->cooldown[0] > 0) { p->coins += 10; hm_afx(6); }
            if (m->cooldown[4] > 0) { m->star_timer = 0x1c20; hm_afx(6); }
            if (m->cooldown[1] > 0) { g->item_hit = 10; if (p->lives < 99) p->lives++; }
            if (m->cooldown[2] > 0) { g->item_hit = 0xb; if (p->lives < 99) p->lives++; }
            if (m->cooldown[3] > 0) { g->item_hit = 0xc; if (p->lives < 99) p->lives++; }
            m->cooldown[0] = 0xc;
        }
        break;
    case 7:                                 /* mushroom */
        if (m->cooldown[0] < 1) hm_afx(0xb);
        if (m->cooldown[0] >= 1) {
            g->item_hit = 10;
            if (p->lives < 99) p->lives++;
        }
        m->cooldown[1] = 0xc;
        m->item_flash = 0x24;
        p->power = 1;
        p->unk_0c = 1;
        break;
    default:
        hm_afx(5);
        break;
    }
    send(a, g, 3);
}

static int touching(const actor_t *a, int x0, int x1, int y0, int y1) {
    if (!(a->box_x1 <= x0 || x1 <= a->box_x0 || a->box_y1 <= y0 || y1 <= a->box_y0)) return 1;
    if (a->unk_03c == 0) return 0;
    return !(a->hit_x1 <= x0 || x1 <= a->hit_x0 || a->hit_y1 <= y0 || y1 <= a->hit_y0);
}

/* Mario's feet are above the enemy's top */
static int above(const actor_t *a, int y0) {
    if (a->unk_03c == 0) return y0 + 0x10 <= a->box_y0;
    return y0 + 0x10 <= (a->hit_y0 + a->box_y0) >> 1;
}

static void mario_coll_det(world *w, gds *g, mario_t *m) {
    int i, n = w->room.n_actors;
    int x0 = m->x + 7, x1 = m->x + 0x19, y1 = m->y + 0x1c, y0 = m->y + 5;
    g->unk_496 = 1;
    for (i = 0; i < 5; i++) m->cooldown[i]--;
    if (m->state_group == 0) {
        if (m->state == 0xc || m->state == 0xd) y0 = m->y + 0xe;    /* ducking: lower top */
        for (i = 0; i < n; i++) {
            actor_t *a = &w->room.actors[i];
            if (a->state != 3 || a->unk_04e != 0 || m->floor != a->cur_floor) continue;
            if (!touching(a, x0, x1, y0, y1)) continue;
            if (a->type > 9) {
                if (m->invincible == 1 || (a->unk_018 != 0 && above(a, y0))) {
                    if (send(a, g, 4) == 0) killed_enemy(w, m, a, y1);
                } else {
                    hurt(g, m);
                }
                continue;
            }
            got_item(g, m, a);
        }
    } else if (m->state_group == 3) {
        for (i = 0; i < n; i++) {
            actor_t *a = &w->room.actors[i];
            if (a->state != 3 || a->unk_04e != 0) continue;
            if (m->floor != a->cur_floor) {
                /* jumping up into something on the floor above */
                if (a->cur_floor + 1 != m->floor || !touching(a, x0, x1, y0, y1)) continue;
                if (a->type > 9) {
                    if (m->invincible == 1) { if (send(a, g, 4) == 0) killed_enemy(w, m, a, y1); }
                    else hurt(g, m);
                    continue;
                }
                got_item(g, m, a);
                continue;
            }
            if (!touching(a, x0, x1, y0, y1)) continue;
            if (a->type > 9) {
                int over = above(a, y0);
                if ((m->state < 4 || !over) && m->invincible != 1 && (a->unk_018 == 0 || !over)) {
                    hurt(g, m);
                } else if (m->invincible == 1) {
                    if (send(a, g, 4) == 0) killed_enemy(w, m, a, y1);
                } else {                    /* a stomp */
                    if (m->state > 3) m->state = 8;
                    if (send(a, g, 3) == 0) killed_enemy(w, m, a, y1);
                    else hurt(g, m);
                }
                continue;
            }
            got_item(g, m, a);
        }
    }
    g->unk_496 = 0;
}

/* ------------------------------------------------------------------ sparkle_anim */
static void sparkle_anim(world *w, mario_t *m) {
    int t = --m->star_timer;
    if (t == 0) m->invincible = 0;
    /* near the end it blinks: no sparkle in these windows */
    if (t < 0x104 && (t > 0xf2 || (t < 0xca && (t > 0xb8 || (t < 0x90 && (t > 0x7e ||
        (t < 0x60 && (t > 0x4e || (t < 0x30 && t > 0x1e)))))))))
        return;
    if (--m->unk_142[3] < 1) {              /* (0x144: frame delay, 0x142: picture) */
        m->unk_142[3] = 4;
        if (++m->unk_142[1] == 6) m->unk_142[1] = 0;
    }
    w->sparkle.slot = m->unk_142[1] + 0x2b;
    if (m->unk_146 == 0) { w->sparkle.x = m->x; w->sparkle.y = m->y; }
    else { w->sparkle.x = m->old_x; w->sparkle.y = m->old_y; }
}

/* ------------------------------------------------------------------ mario() */
void mario_field(world *w, uint32_t held) {
    gds *g = &w->g;
    mario_t *m = g->mario;
    w->move_step = held & IN_RUN ? 2 : 1;

    g->frame++;
    m->fcount++;
    w->sparkle.slot = -1;

    if (m->state_group != 6 && m->state_group != 7) {
        if (g->unk_53a == 2) {              /* all doors open / time up: Mario dies */
            m->saved_group = m->state_group;
            m->prev_state = m->state;
            m->state_group = 6;
            m->state = 0;
            m->fell = 1;
            m->step = 0;
        } else if (g->unk_53a == 1) {       /* every door closed: the dance */
            if (m->state_group != 5) {
                m->saved_group = m->state_group;
                m->prev_state = m->state;
                m->state_group = 5;
                m->state = 0;
                m->anim_speed = 10;
                m->anim_count = 10;
                m->anim = 0x24;
                m->step = 0;
            }
        } else if (g->frame % 3 == 0) {
            controller(w, g, m, held);
        }
    }

    state_op(w, g, m);
    /* a boss has caught Mario (group 7 state 1): the original re-blits the door or lift
       background over him, so he isn't seen until the boss lets go */
    if (m->state_group == 7 && m->state == 1) w->msp.vis = 0;

    if (m->item_flash != 0) {
        m->item_flash--;
        if (g->frame % 7 == 0 && (m->state_group == 0 || m->state_group == 3))
            erase(w, m->x, m->y - 1);
    }
    if (m->hurt_timer > -1) {
        int t = m->hurt_timer--;
        if (t > 0x1e && g->frame % 7 == 0 && (m->state_group == 0 || m->state_group == 3))
            erase(w, m->x, m->y - 1);
    }
    if (m->invincible != 0) sparkle_anim(w, m);
    mario_coll_det(w, g, m);
}
