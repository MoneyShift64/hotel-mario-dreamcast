/* world.h - one Hotel Mario stage in play (no platform code: also builds on a PC).
 *
 * Uses the game's own structures (src/hm_structs.h): gds, mario_t, room_t/actor_t and the
 * stage_info grid, so the code ported from the CD-i (doors, objects) runs unchanged.
 * Every field: mario() (mario.c: controller, state routine, mario_coll_det). Every 6th
 * field the_game's object pass: erase (cmd 1), deferred messages, update (cmd 2),
 * check_doors, power_check. Then exit_check.
 */
#ifndef HM_WORLD_H
#define HM_WORLD_H
#include <stdint.h>
#include "hm_structs.h"
#include "doors.h"

/* input bits (same values as hm_plat.h) */
enum {
    IN_LEFT = 1, IN_RIGHT = 2, IN_UP = 4, IN_DOWN = 8, IN_B1 = 16, IN_B2 = 32, IN_RUN = 64, IN_ITEM = 128
};

enum { WORLD_PLAYING, WORLD_CLEARED, WORLD_ALL_OPEN, WORLD_MARIO_DIED, WORLD_TIME_UP, WORLD_UPPER, WORLD_WARP, WORLD_STATUS_COUNT };

/* things to draw this frame */
typedef struct world_sprite { int obj, frame, x, y; } world_sprite;   /* obj: OBJ_* */
#define X(n) OBJ_##n,
enum {
#include "obj_list.h"
    N_OBJ
};
#undef X

/* a fireball or a kill star (fx.c) */
typedef struct hm_fx {
    int16_t x, y, floor, dir, state, phase, hit, anim, count, bounce, frame;
    int16_t bx0, bx1, by0, by1;          /* fireball box */
} hm_fx;

typedef struct world {
    gds g;
    mario_t mario;
    unsigned cheats; /* Session-only easter eggs; never serialized to VMU. */
    int move_step; /* Dreamcast: walk 1 px/field, hold X for original 2 px/field speed. */
    room_t room;
    uint8_t stage_info[0x94];            /* native copy: +4 seed, +8 time, +0x12 grid, +0x58 cycles */
    hm_doors doors;
    int upper, roof_col, shake;
    const uint8_t *extra;
    unsigned extra_size;
    int lift_kind, lift_n, lift_phase, lift_pending, lift_time;
    uint16_t lift_grid[2][35];
    struct {
        int tick, flash_at, dark_at, restart_at, flicker, flash, light;
        int count, timer, pending, choices[5], old_kind[5];
        int last_floor, completed, event_count;
    } event;
    struct {
        int mode, kind, floor, col, x, y, remaining, toggle, cooldown;
    } secret;
    int warp_hotel, warp_stage;
    int obj_of[24];                      /* actor -> OBJ_* */
    int status, frames;
    int luigi;                           /* show LUIGI in the score bar */
    int afx_req;                         /* playAFX: effect asked for this frame (-1 none) */
    int music_req, music_loop;           /* tune (Ln_am record 4-10) to start, 0 = stop, -1 none */
    int jingle_pending;                  /* exit_check: record to play once Mario has fallen */
    int end_reason;                      /* WORLD_* once exit_check has started the ending */
    int time_tick;                       /* power_check: time_left drops every 6 object ticks */
    int power_phase;                     /* firePowerColors+600: power-up colour cycle step */
    struct { int vis, slot, x, y; } msp; /* Mario's picture on plane B (stays until erased) */
    struct { int slot, x, y; } sparkle;  /* star sparkle this field (slot -1: none) */
    struct { int8_t on; int16_t sx, sy; } tile[FLOORS][COLS];   /* door sheet tiles drawn over doors */
    int tick_count;                      /* unk_46c: objects update every 6th field (10 Hz) */
    world_sprite spr[64];
    int n_spr;
    hm_fx fireball[2], kill_star[5];
    int next_star;                       /* gds->n_fireballs: the next kill star to use */
    uint16_t *puzzle_cell;               /* gds->unk_81c: the door the sparks picked (puzzle_2) */
    world_sprite fx[8];                  /* fireball / kill star pictures (fx.frm slots) */
    int n_fx;
} world;

enum { HM_CHEAT_INVINCIBLE=1, HM_CHEAT_TIME=2 };
void world_set_cheats(world *w,unsigned flags);
extern world *hm_world;                  /* the one in play (object code draws into it) */
static inline int world_invulnerable(const gds *g) {
    return hm_world && &hm_world->g==g && (hm_world->cheats&HM_CHEAT_INVINCIBLE);
}

/* inf = .inf file, room = .room file (both as produced by make_dc_assets.py) */
void world_start(world *w, const uint8_t *inf, const uint8_t *room, int hotel, int stage, int player);
void world_start_campaign(world *w, const uint8_t *inf, const uint8_t *room,
                          const player_t players[2], int current, int two_players);
void world_start_full(world *w, const uint8_t *inf, const uint8_t *room,
                      const player_t players[2], int current, int two_players,
                      int upper, const uint8_t *extra, unsigned extra_size);
void stage_features_start(world *w);
void stage_secrets_start(world *w);
int stage_plane_light(const world *w, int x, int y);
int stage_light_colour(const world *w, int colour, uint32_t *rgb);
void stage_features_tick(world *w);
void stage_secrets_tick(world *w);
void stage_secret_fireball(world *w, const hm_fx *fb);
void stage_event_finish(world *w);
void stage_lifts_start(world *w);
void stage_lifts_tick(world *w);
int  world_frame(world *w, uint32_t held);                     /* WORLD_*; held = IN_* bits */
int  world_hud(const world *w, world_sprite *out, int max);     /* score bar: frame = hud.frm slot */
int  world_power_colours(const world *w, int *set, int *phase); /* 0 none, 1 mushroom, 2 fire */

/* sound requests (the platform plays them after world_frame) */
void hm_afx(int n);

/* mario.c */
void mario_field(world *w, uint32_t held);
uint16_t *hm_is_door_elev(world *w, int floor, int x);

/* fx.c */
void fx_start(world *w);
void fx_erase(world *w);
void fx_update(world *w);
void fx_kill_star(world *w, int x, int y);
int  fx_throw(world *w, int x, int y, int floor, int dir);
uint32_t hm_rnd(uint32_t *seed);                               /* the game's rnd() */

/* object code -> drawing (replaces calling a compiled sprite routine) */
void obj_draw(actor_t *a, int frame, int x, int y);

/* the ported module objects */
#define X(n) int n(actor_t *a, gds *g);
#include "obj_list.h"
#undef X
const char *world_obj_name(int obj);                           /* "goomba" ... (data file names) */

#endif
