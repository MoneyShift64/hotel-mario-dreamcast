/* Native equivalents of puzzle_2/3/4/5/7 and the_game's bonus completion hook.
 * Thresholds, colour sequences and masks come from original stage data/tables. */
#include "hm_rules.h"
#include "world.h"
#include <stdlib.h>
#include <string.h>
static unsigned roll(world *w) {
    return hm_rnd((uint32_t *)&w->g.unk_82a);
}
static int be16(const world *w, unsigned off) {
    if (!w->extra || off + 2 > w->extra_size)
        return 0;
    return (int16_t)((w->extra[off] << 8) | w->extra[off + 1]);
}
static int hiding_at(const world *w, int cell) {
    return cell >= 0 && cell < 35 && w->mario.state_group == 4 && w->mario.state == 0 &&
           w->mario.door == w->doors.cell + cell;
}
static int near_mario(world *w, int cell) {
    mario_t *m = &w->mario;
    int f = cell / COLS, x = hm_door_x[cell % COLS];
    switch (m->state_group) {
    case 1:
    case 2:
        return 0;
    case 0:
        return f == m->floor && abs(x - m->x) < 64;
    case 3:
        return (f == m->floor || f + 1 == m->floor) && abs(x - m->x) < 64;
    case 4:
        return m->door == w->doors.cell + cell;
    default:
        return 1;
    }
}
static void weather_schedule(world *w) {
    int st = w->g.player[w->g.cur_player].stage;
    w->event.flash_at = 80 + (roll(w) & 31) - st * 2;
    w->event.dark_at = w->event.flash_at + 20 + (roll(w) & 3);
    w->event.restart_at = w->event.dark_at + 7 + (roll(w) & 3);
}
void stage_features_start(world *w) {
    player_t *p = &w->g.player[w->g.cur_player];
    w->puzzle_cell = NULL;
    w->event.last_floor = w->mario.floor;
    for (int i = 0; i < 5; i++)
        w->event.choices[i] = -1;
    if (p->unk_10) {
        w->event.completed = 1;
        return;
    }
    if (p->hotel == 2)
        weather_schedule(w);
    if (p->hotel == 5 && (p->stage == 4 || p->stage == 6 || p->stage == 8) && !w->doors.cell[24]) {
        w->doors.cell[24] = 0x4000;
        w->puzzle_cell = w->doors.cell + 24;
        hm_doors_init(&w->doors, w->doors.cell);
    }
    if (p->hotel == 7 && p->stage < 10 && hm_coin_time[p->stage * 4]) {
        int choices[24], n = 0;
        for (int i = 0; i < w->room.n_actors; i++) {
            actor_t *a = &w->room.actors[i];
            if (a->type == 4 && a->start_mode == 0 && a->floor >= 0 && a->floor < 5 &&
                a->col >= 0 && a->col < 7)
                choices[n++] = a->floor * COLS + a->col;
        }
        if (n)
            for (int i = 0; i < 3; i++)
                w->event.choices[i] = choices[roll(w) % n];
    }
}
static void restore_ghosts(world *w, int force) {
    int retry = 0;
    for (int i = 0; i < 5; i++) {
        int cell = w->event.choices[i];
        if (cell < 0)
            continue;
        if (!force && near_mario(w, cell)) {
            retry = 1;
            continue;
        }
        if ((w->doors.cell[cell] & 0xf000) == 0x3000) {
            w->doors.cell[cell] =
                (w->doors.cell[cell] & 0xfff) | (w->event.old_kind[i] ? 0x2000 : 0x1000);
            w->tile[cell / COLS][cell % COLS].on = 0;
            w->doors.anim[cell / COLS][cell % COLS] = w->event.old_kind[i] ? 3 : 0;
        }
        w->event.choices[i] = -1;
    }
    w->event.timer = retry;
}
void stage_event_finish(world *w) {
    int hotel = w->g.player[w->g.cur_player].hotel;
    if (hotel == 5) {
        restore_ghosts(w, 1);
        if ((w->doors.cell[24] & 0xf000) == 0x4000) {
            w->doors.cell[24] = 0;
            w->tile[3][3].on = 0;
        }
    }
    w->event.completed = 1;
    w->event.timer = 0;
    w->event.light = 0;
    w->puzzle_cell = NULL;
}
static void ghost_event(world *w) {
    int opened[35], all[35], no = 0, na = 0, st = w->g.player[w->g.cur_player].stage;
    for (int i = 0; i < 35; i++) {
        unsigned v = w->doors.cell[i], k = v & 0xf000;
        if (near_mario(w, i) || (v & 7) || (k != 0x1000 && k != 0x2000))
            continue;
        all[na++] = i;
        if (k == 0x2000)
            opened[no++] = i;
    }
    int n = hm_ghost_count[st];
    if (n > na)
        n = na;
    for (int i = 0; i < 5; i++)
        w->event.choices[i] = -1;
    if (no && n)
        w->event.choices[0] = opened[roll(w) % no];
    for (int i = (no && n) ? 1 : 0; i < n; i++) {
        int at = (int)(roll(w) % na), cell;
        for (int tries = 0; tries < na; tries++) {
            at = (at + 1) % na;
            cell = all[at];
            if (cell >= 0 && cell != w->event.choices[0])
                break;
        }
        cell = all[at];
        if (cell < 0)
            break;
        w->event.choices[i] = cell;
        all[at] = -1;
    }
    for (int i = 0; i < n; i++) {
        int cell = w->event.choices[i];
        if (cell < 0)
            continue;
        w->event.old_kind[i] = (w->doors.cell[cell] & 0xf000) == 0x2000;
        w->doors.cell[cell] = (w->doors.cell[cell] & 0xfff) | 0x3000;
        w->tile[cell / COLS][cell % COLS].on = 1;
        w->tile[cell / COLS][cell % COLS].sx = 0xaa;
        w->tile[cell / COLS][cell % COLS].sy = 0;
    }
    w->event.timer = 100 + st * 5;
}
void stage_features_tick(world *w) {
    gds *g = &w->g;
    player_t *p = &g->player[g->cur_player];
    if (g->unk_81a && g->unk_53a == 0) {
        stage_event_finish(w);
        p->unk_10 = 1;
        g->unk_81a = 0;
        if (p->hotel == 2 || p->hotel == 4 || p->hotel == 5 || p->hotel == 7) {
            g->time_left = *(int32_t *)(w->stage_info + 8);
            w->time_tick = 0;
            if (p->hotel == 5) {
                w->mario.state_group = 0;
                w->mario.state = 14;
                w->mario.hurt_timer = 99;
            } else {
                w->mario.state_group = 4;
                w->mario.state = 1;
                w->mario.sub = 0x1c;
                w->mario.step = 0;
            }
        }
        return;
    }
    if (w->event.completed || p->unk_10 || g->unk_53a != 0 || p->stage < 0 || p->stage >= 15)
        return;
    if (p->hotel == 2) {
        int tick = ++w->event.tick;
        if (tick == w->event.restart_at) {
            w->event.tick = 0;
            weather_schedule(w);
            int threshold =
                w->event.count < 5 ? be16(w, 180 + p->stage * 10 + w->event.count * 2) : 0;
            if (threshold > 0 && g->time_left < threshold) {
                w->event.pending = 1;
                w->event.flicker = w->event.flash_at - 10;
            }
        }
        if (w->event.pending && w->event.tick == w->event.flicker) {
            w->event.pending = 0;
            w->event.count++;
            w->event.timer = 150;
            if (w->room.n_actors && w->obj_of[0] == OBJ_sparks)
                w->room.actors[0].state = 1;
            w->event.event_count++;
        }
        if (w->event.tick < w->event.flash_at)
            w->event.light = 0;
        else if (w->event.tick < w->event.dark_at) {
            if (--w->event.flash <= 0) {
                w->event.light = roll(w) & 7;
                w->event.flash = roll(w) & 3;
                hm_afx(12);
            }
        } else
            w->event.light = 8;
    } else if (p->hotel == 4 || p->hotel == 7) {
        const uint16_t *times = p->hotel == 4 ? hm_puff_time : hm_coin_time;
        int threshold =
            p->stage < 10 && w->event.count < 3 ? times[p->stage * 4 + w->event.count] : 0;
        if (threshold > 0 && g->time_left == threshold && !w->event.timer) {
            if (p->hotel == 4 && w->room.n_actors && w->obj_of[0] == OBJ_puffs)
                w->room.actors[0].state = 1;
            if (p->hotel == 7) {
                int cell = w->event.choices[w->event.count];
                if (cell >= 0)
                    w->puzzle_cell = w->doors.cell + cell;
                hm_afx(12);
            }
            w->event.count++;
            w->event.timer = 150;
            w->event.event_count++;
        }
    } else if (p->hotel == 5) {
        if (w->puzzle_cell && hiding_at(w, (int)(w->puzzle_cell - w->doors.cell)))
            g->unk_81a = 1;
        int threshold =
            w->event.count < 4 ? (int16_t)hm_ghost_time[p->stage * 4 + w->event.count] : -1;
        if (threshold > 0 && g->time_left == threshold && !w->event.timer) {
            w->event.count++;
            ghost_event(w);
            w->event.event_count++;
        }
        if (w->event.timer && --w->event.timer == 0)
            restore_ghosts(w, 0);
        return;
    }
    if (w->event.timer > 0) {
        w->event.timer--;
        if (w->puzzle_cell && hiding_at(w, (int)(w->puzzle_cell - w->doors.cell)))
            g->unk_81a = 1;
    }
}
/* puzzle_2/3 write matching intensity controls for BOTH planes. Coordinates
 * are full 384x280 display pixels, including the port's 20-line draw offset.
 * These are brightness weights, never sprite-opacity values. */
int stage_plane_light(const world *w, int x, int y) {
    const player_t *p = &w->g.player[w->g.cur_player];
    if (w->event.completed || p->unk_10) return 255;
    if (p->hotel == 2 && w->extra_size >= 370) {
        int selected[4] = {-1,-1,-1,-1};
        /* LCT writes persist until another write to that matte register. */
        for (int i=0;i<9;i++) {
            unsigned off=280+i*10;
            int type=be16(w,off+2), line=be16(w,off+6), reg=be16(w,off+8);
            if ((type != 1 && type != 2) || line > y || reg < 0 || reg >= 4) continue;
            if (selected[reg] < 0 || line >= be16(w,280+selected[reg]*10+6)) selected[reg]=i;
        }
        int light=255, last_x=-1;
        for (int reg=0;reg<4;reg++) {
            if (selected[reg]<0) break; /* unused register is a stop command */
            unsigned off=280+selected[reg]*10;
            int at=be16(w,off+4);
            if (at<last_x || x<at) break;
            last_x=at;
            if (be16(w,off+2)==2) break; /* disregard higher registers */
            light=be16(w,off) ? hm_light_icf[w->event.light]*255/63 : 255;
        }
        return light;
    }
    if (p->hotel == 3 && p->stage >= 0 && p->stage < 10 && w->extra_size >= 720) {
        for (int floor=0;floor<5;floor++) {
            if (y < hm_row_start[floor] || y >= hm_row_start[floor+1]) continue;
            int row=y-hm_row_start[floor];
            int left=be16(w,floor*144+row*4), right=be16(w,floor*144+row*4+2);
            if (floor==w->mario.floor && x>=left && x<right) return 255;
            return hm_dark_icf[p->stage]*255/63;
        }
    }
    return 255;
}
int stage_light_colour(const world *w, int colour, uint32_t *rgb) {
    if (w->g.player[w->g.cur_player].hotel != 2 || w->event.completed || !w->extra ||
        w->extra_size < 180 || colour < 43 || colour > 45)
        return 0;
    unsigned off = (colour - 43) * 60 + w->event.light * 6;
    *rgb = ((uint32_t)(be16(w, off) & 255) << 16) | ((be16(w, off + 2) & 255) << 8) |
           (be16(w, off + 4) & 255);
    return 1;
}
