/* world.c - one Hotel Mario stage in play. See world.h.
 *
 * the_game's loop: mario() every field (mario.c), the object pass every 6th field,
 * exit_check. Everything goes through the game's own rules: doors via the stage_info
 * grid, objects via their handlers and the cmd 3 / cmd 4 messages of mario_coll_det.
 */
#include <string.h>
#include "world.h"

world *hm_world;

void world_set_cheats(world *w,unsigned flags) {
    w->cheats=flags;
    if(flags&HM_CHEAT_TIME) {
        *(int32_t *)(w->stage_info+8)=500;
        w->g.time_left=500;
    }
}

uint32_t hm_rnd(uint32_t *seed) {             /* rnd() / m_00060: x << 1, feedback 0x20020 */
    uint32_t v = *seed, top = v >> 31;
    v = (v << 1) | top;
    if (top) v ^= 0x20020;
    *seed = v;
    return v;
}

#define X(n) n,
static int (*const obj_entry[N_OBJ])(actor_t *, gds *) = {
#include "obj_list.h"
};
#undef X
#define X(n) #n,
static const char *const obj_name[N_OBJ] = {
#include "obj_list.h"
};
#undef X

void hm_afx(int n) { if (hm_world) hm_world->afx_req = n; }    /* ds_dispatch(playAFX, n) */

const char *world_obj_name(int obj) { return obj >= 0 && obj < N_OBJ ? obj_name[obj] : ""; }

static int rd16(const uint8_t *p) { return (int16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

static void puzzle(world *w, int p2);
static void call(actor_t *a, gds *g, int cmd) {
    int old = a->cmd;
    a->cmd = cmd;
    ((int (*)(actor_t *, gds *))a->handler)(a, g);
    if (cmd == 3 || cmd == 4) a->cmd = old;
}

static void start(world *w, const uint8_t *inf, const uint8_t *room, int hotel, int stage, int player,
                  const player_t *saved, int two_players, int upper, const uint8_t *extra, unsigned extra_size) {
    gds *g = &w->g;
    int i, n_obj, n, k, f, c;
    const char *names[16];
    char namebuf[16][16];

    memset(w, 0, sizeof *w);
    w->upper = upper;
    w->roof_col = -1;
    if (extra && extra_size >= 12 && memcmp(extra, "HMEX", 4) == 0 &&
        rd16(extra + 4) == 1 && rd32(extra + 8) <= extra_size - 12) {
        w->extra = extra + 12;
        w->extra_size = rd32(extra + 8);
    }
    w->tick_count = 5;                   /* first object tick on the first frame */
    hm_world = w;
    /* stage_info (native copy) */
    if (inf && memcmp(inf, "HMSI", 4) == 0) {
        memcpy(w->stage_info + 0x12, inf + 4, 70);                       /* door grid */
        *(int32_t *)(w->stage_info + 8) = (int32_t)rd32(inf + 74);       /* time limit */
        memcpy(w->stage_info + 0x58, inf + 78, 60);                      /* colour cycling */
        *(uint32_t *)(w->stage_info + 4) = rd32(inf + 138);              /* random seed */
        *(int16_t *)(w->stage_info + 0xe) = (int16_t)rd16(inf + 142);    /* Mario's start floor */
        *(int16_t *)(w->stage_info + 0x10) = (int16_t)rd16(inf + 144);   /* and column */
    } else {
        *(int16_t *)(w->stage_info + 0xe) = 4;
    }
    g->stage_info = w->stage_info;
    g->unk_826 = g->unk_82a = *(int32_t *)(w->stage_info + 4);           /* init_the_game */
    for (f = 0; f < FLOORS; f++) g->floor_y[f] = hm_floor_y[f];
    for (c = 0; c < COLS; c++) g->door_x[c] = hm_door_x[c];
    g->cur_player = player;
    g->player[player].hotel = hotel;
    g->player[player].stage = stage;
    g->player[player].lives = 3;
    if (saved) memcpy(g->player, saved, sizeof g->player);
    g->two_players = two_players;
    g->time_left = *(int32_t *)(w->stage_info + 8);                      /* init_the_game */
    g->mario = &w->mario;
    g->room = &w->room;
    g->draw_field = 1;
    hm_doors_init(&w->doors, (uint16_t *)(w->stage_info + 0x12));
    /* init_doors: door cells start uncredited; empty cells and lifts are
     * already credited for get_the_fxxxxxx_bonus's all-doors test. */
    for (f = 0; f < FLOORS; f++) for (c = 0; c < COLS; c++) {
        unsigned kind = ((uint16_t *)(w->stage_info + 0x12))[f * COLS + c] & 0xf000;
        g->unk_550[2 * (f * COLS + c) + 1] = kind == 0 || kind == 0xe000 || kind == 0xf000;
    }
    if (hotel >= 7 && !upper) {
        for (c = 0; c < COLS; c++)
            if ((w->doors.cell[c] & 0xf000) == 0xf000) {
                w->roof_col = c;
                w->doors.cell[c] = 0xb000; /* init_doors: roof disabled until lower clear */
                break;
            }
    }
    g->doors_count = w->doors.count;
    g->doors_open = w->doors.open;

    /* Mario: init_the_game sets group 7 state 0, so his first field runs dead_init
       (arrive by lift at stage_info +0xe / +0x10) */
    w->mario.state_group = 7;
    w->mario.state = 0;
    w->sparkle.slot = -1;
    w->afx_req = -1;
    /* the stage tune: Ln_am records 5, 6, 7 in turn (sound_map_no), 4 for the boss stage */
    w->music_req = (stage == (hotel == 8 ? 14 : 9)) ? 4 : 5 + stage % 3;
    w->music_loop = 1;

    /* the room: keep the actors whose object is ported (others are left out) */
    if (room && memcmp(room, "HMRM", 4) == 0) {
        n_obj = rd16(room + 4);
        for (i = 0; i < n_obj && i < 16; i++) {
            memcpy(namebuf[i], room + 6 + 16 * i, 16);
            namebuf[i][15] = 0;
            names[i] = namebuf[i];
        }
        k = 6 + 16 * n_obj;
        n = rd16(room + k);
        k += 2;
        for (i = 0; i < n; i++, k += 2 + 76) {
            int oi = rd16(room + k), o, j;
            actor_t *a;
            if (oi < 0 || oi >= n_obj || w->room.n_actors >= 24) continue;
            for (o = 0; o < N_OBJ && strcmp(names[oi], obj_name[o]) != 0; o++) ;
            if (o == N_OBJ) continue;
            a = &w->room.actors[w->room.n_actors];
            w->obj_of[w->room.n_actors] = o;
            w->room.n_actors++;
            /* offsets 0x04..0x4f are all 16-bit fields, stored on the disc as they are */
            for (j = 0; j < 38; j++) (&a->type)[j] = (int2)rd16(room + k + 2 + 2 * j);
            a->handler = (code *)obj_entry[o];
        }
    }
    fx_start(w);
    /* init_the_game: state 0, cmd 0, call each handler once */
    for (i = 0; i < w->room.n_actors; i++) {
        actor_t *a = &w->room.actors[i];
        a->state = 0;
        a->cmd = 0;
        ((int (*)(actor_t *, gds *))a->handler)(a, g);
    }
    stage_lifts_start(w);
    stage_features_start(w);
    stage_secrets_start(w);
    puzzle(w, 0);
}

void world_start(world *w, const uint8_t *inf, const uint8_t *room, int hotel, int stage, int player) {
    start(w, inf, room, hotel, stage, player, NULL, 0, 0, NULL, 0);
}

void world_start_campaign(world *w, const uint8_t *inf, const uint8_t *room,
                          const player_t players[2], int current, int two_players) {
    start(w, inf, room, players[current].hotel, players[current].stage, current, players, two_players, 0, NULL, 0);
}

void world_start_full(world *w, const uint8_t *inf, const uint8_t *room,
                      const player_t players[2], int current, int two_players,
                      int upper, const uint8_t *extra, unsigned extra_size) {
    start(w, inf, room, players[current].hotel, players[current].stage, current,
          players, two_players, upper, extra, extra_size);
}

void obj_draw(actor_t *a, int frame, int x, int y) {
    world *w = hm_world;
    int i = (int)(a - w->room.actors);
    if (frame <= 0 || w->n_spr >= 64 || i < 0 || i >= 24) return;   /* frame 0 = the eraser */
    w->spr[w->n_spr].obj = w->obj_of[i];
    w->spr[w->n_spr].frame = frame;
    w->spr[w->n_spr].x = x;
    w->spr[w->n_spr].y = y;
    w->n_spr++;
}

/* puzzle_8 (hotel 8): wake the big fireballs (type 0x1d) at random times.
   Called with p2 0 at stage start and 1 each object tick, while player.unk_10 == 0.
   puzzle_vars: +0 count, +2 next time_left, +4.. the balls (actor indices here). */
static void puzzle_8(world *w, int p2) {
    gds *g = &w->g;
    int16_t *cnt = (int16_t *)g->puzzle_vars, *when = (int16_t *)(g->puzzle_vars + 2);
    int16_t *idx = (int16_t *)(g->puzzle_vars + 4);
    int i;
    if (p2 == 0) {
        int max = g->player[g->cur_player].stage + 1;
        memset(g->puzzle_vars, 0, 0x40);
        for (i = 0; i < w->room.n_actors && *cnt < 16; i++)
            if (w->room.actors[i].type == 0x1d) idx[(*cnt)++] = (int16_t)i;
        if (max > 5) max = 5;
        if (max < *cnt) *cnt = (int16_t)max;
        { uint32_t r = hm_rnd((uint32_t *)&g->unk_82a);
          *when = (int16_t)(200 - ((r & 0xf) + 0x14)); }
    } else if (p2 == 1 && *when == g->time_left) {
        uint32_t r = hm_rnd((uint32_t *)&g->unk_82a);
        *when = (int16_t)(*when - ((r & 0xf) + 0x14));
        for (i = 0; i < *cnt; i++) {
            actor_t *a = &w->room.actors[idx[i]];
            if (a->state == 1) { a->state = 2; a->floor = (int2)i; }
        }
    }
}

/* puzzle(g, p2): the hotel's bonus/puzzle hook (only hotel 8's is ported so far) */
static void puzzle(world *w, int p2) {
    gds *g = &w->g;
    if (g->player[g->cur_player].unk_10 != 0) return;
    if (g->player[g->cur_player].hotel == 8) puzzle_8(w, p2);
}

/* power_check: time and the 30-coin extra life (once per object tick) */
static void power_check(world *w) {
    gds *g = &w->g;
    player_t *p = &g->player[g->cur_player];
    if (g->unk_53a == 0 || g->unk_53a == 3) {
        if (++w->time_tick == 6) {
            w->time_tick = 0;
            if (g->time_left < 1) g->time_left = 0;
            else g->time_left--;
        }
        if (p->coins > 0x1d) {
            p->coins -= 0x1e;
            hm_afx(8);
            if (++p->lives > 99) p->lives = 99;
        }
    }
}

/* exit_check: start the ending (dance or death), then report it once Mario is done */
static void exit_check(world *w) {
    gds *g = &w->g;
    mario_t *m = &w->mario;
    int i;
    if (g->unk_53a == 3) {
        if (m->fell || g->time_left == 0) {
            g->unk_53a = 0; w->doors.open = g->doors_open = 1;
        } else {
            if (m->state_group == 1 && m->state == 3 && m->sub == 0xb &&
                m->door == &w->doors.cell[w->roof_col]) w->status = WORLD_UPPER;
            if (++w->event.tick % 34 == 0) {
                w->tile[0][w->roof_col].on = 1;
                w->tile[0][w->roof_col].sx = (w->event.tick / 34) & 1 ? 0xaa : 0;
                w->tile[0][w->roof_col].sy = (w->event.tick / 34) & 1 ? 0 : 0x21;
            }
            return;
        }
    }
    if (g->unk_53a == 0) {
        if (w->doors.open != 0) {
            if (m->fell == 0 && g->time_left != 0 && w->doors.count != w->doors.open) return;
            g->unk_53a = 2;
            g->player[g->cur_player].power = 0;
            g->player[g->cur_player].unk_0c = 1;
            w->end_reason = m->fell ? WORLD_MARIO_DIED : g->time_left == 0 ? WORLD_TIME_UP : WORLD_ALL_OPEN;
            w->music_req = 0;                       /* cd_smq: the tune stops */
            w->jingle_pending = 8;                  /* then record 8 once he's falling */
        } else {
            if (w->roof_col >= 0 && !w->upper) {
                for (i = 0; i < w->room.n_actors; i++) call(&w->room.actors[i], g, 4);
                stage_event_finish(w);
                for (i = 0; i < FLOORS * COLS; i++)
                    if ((w->doors.cell[i] & 0xf000) == 0x1000 || (w->doors.cell[i] & 0xf000) == 0x2000)
                        w->doors.cell[i] = 0;
                w->doors.cell[w->roof_col] = 0xf000;
                memset(w->tile, 0, sizeof w->tile);
                hm_doors_init(&w->doors, w->doors.cell);
                g->unk_53a = 3;
                w->event.tick = 0;
                return;
            }
            g->unk_53a = 1;
            stage_event_finish(w);
            for (i = 0; i < w->room.n_actors; i++) call(&w->room.actors[i], g, 4);
            w->end_reason = WORLD_CLEARED;
            player_t *p = &g->player[g->cur_player];
            w->music_req = p->stage == (p->hotel == 8 ? 14 : 9) ? 10 : 9; /* zero-based final stage */
            w->music_loop = 0;
        }
        return;
    }
    if (w->jingle_pending && m->state_group == 6 && m->state >= 2) {
        w->music_req = w->jingle_pending;
        w->music_loop = 0;
        w->jingle_pending = 0;
    }
    if (m->state_group == 6 && m->state == 5) w->status = w->end_reason;
}

int world_frame(world *w, uint32_t held) {
    gds *g = &w->g;
    player_t *p = &g->player[g->cur_player];
    int i;
    hm_world = w;
    w->frames++;
    if (w->status != WORLD_PLAYING) return w->status;

    mario_field(w, held);                           /* (the signal handler) */

    /* mario(): the objects only run every 6th field (the_game waits for unk_490,
       set when unk_46c reaches 6); their sprites stay on screen until the next tick */
    if (++w->tick_count >= 6) {
        w->tick_count = 0;
        w->n_spr = 0;
        fx_erase(w);
        for (i = 0; i < w->room.n_actors; i++) call(&w->room.actors[i], g, 1);   /* erase pass */
        power_check(w);
        for (i = 0; i < w->room.n_actors; i++) {                                  /* deferred */
            actor_t *a = &w->room.actors[i];
            if (a->unk_04e) { call(a, g, a->unk_04e); a->unk_04e = 0; }
        }
        for (i = 0; i < w->room.n_actors; i++) {                                  /* update pass */
            call(&w->room.actors[i], g, 2);
            if (g->unk_054) { hm_afx(g->unk_054); g->unk_054 = 0; }
        }
        stage_lifts_tick(w);
        hm_doors_update(&w->doors);                 /* check_doors */
        g->doors_open = w->doors.open;
        fx_update(w);
        if (p->unk_0c != 0) {                       /* player_clut_change */
            w->power_phase = 0;
            p->unk_0c = 0;
        }
        if (p->power != 0 && w->mario.invincible == 0) w->power_phase++;   /* power_cycle */
        stage_features_tick(w);
        stage_secrets_tick(w);
        puzzle(w, 1);
        g->draw_field = (g->draw_field == 1) ? 2 : 1;
    }
    if (w->status == WORLD_PLAYING) exit_check(w);
    return w->status;
}

/* the colours for plane B 0x10-0x14: power_cycle's table and step */
int world_power_colours(const world *w, int *set, int *phase) {
    const gds *g = &w->g;
    int power = g->player[g->cur_player].power;
    if (power == 0 || w->power_phase == 0) return 0;
    *set = g->cur_player;
    *phase = power == 1 ? w->power_phase % 12 : w->power_phase % 10;
    return power;
}

/* scoring: the score bar, as the slots of hud.frm (positions are draw_a0 offsets / 384) */
static int digits(world_sprite *out, int n, int max, long v, int count, int x, int y, int blank_lead) {
    char d[10];
    int i, lead = 1;
    for (i = count - 1; i >= 0; i--) { d[i] = (char)(v % 10); v /= 10; }
    for (i = 0; i < count && n < max; i++) {
        if (lead && blank_lead > i && d[i] == 0) continue;      /* leading zeros are not drawn */
        lead = 0;
        out[n].obj = -1; out[n].frame = d[i]; out[n].x = x + 10 * i; out[n].y = y;
        n++;
    }
    return n;
}

int world_hud(const world *w, world_sprite *out, int max) {
    const gds *g = &w->g;
    const player_t *p = &g->player[g->cur_player];
    static const struct { int slot, x, y; } fixed[] = {
        { 11, 34, 28 },     /* MARIO (12: LUIGI) at draw_a0 + 0x2a22 */
        { 13, 80, 28 },     /* x       0x2a50 */
        { 14, 126, 28 },    /* TIME    0x2a7e */
        { 15, 210, 27 },    /* coin    0x2952 */
        { 13, 224, 28 },    /* x       0x2ae0 */
        { 17, 34, 20 },     /* STAGE   0x1e22 */
    };
    int i, n = 0;
    for (i = 0; i < 6 && n < max; i++, n++) {
        out[n].obj = -1;
        out[n].frame = fixed[i].slot + (i == 0 && w->luigi);
        out[n].x = fixed[i].x;
        out[n].y = fixed[i].y;
    }
    n = digits(out, n, max, p->stage + 1, 2, 82, 20, 1);                  /* 0x1e52 */
    n = digits(out, n, max, (long)(p->score / 10) * 10, 8, 270, 28, 7);   /* 0x2b0e: tens, units 0 */
    n = digits(out, n, max, g->time_left < 0 ? 0 : g->time_left, 3, 162, 28, 2);   /* 0x2aa2 */
    n = digits(out, n, max, p->coins, 2, 236, 28, 1);                     /* 0x2aec */
    n = digits(out, n, max, p->lives, 2, 92, 28, 1);                      /* 0x2a5c */
    return n;
}
