/* Hotel Mario campaign harness, build 15.
 * A/Start selects one/two players and starts the original per-stage card.
 * During play: d-pad, A jump, B doors, X run, Y item, Start pause.
 * Completed stages tally time points; lost lives retry, and two-player turns
 * alternate. Game over/completion returns to the original title menu.
 * A+B+X+Y+Start returns to the campaign menu.
 * Original CD-i cutscenes and scrolling credits use the movie player.
 */
#include <kos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sh4zam/shz_sh4zam.h>
#include "hm_plat.h"
#include "sound.h"
#include "doors.h"
#include "world.h"
#include "campaign.h"
#include "frontend.h"
#include "persistence.h"
#include "vmu_store.h"
#include "demo.h"
#include "movie.h"


#define floor_y hm_floor_y                                    /* gds->floor_y (init_gds) */
#define SCREEN_OFS (20 + gW.shake)                                        /* draw_screen = screen_b0 + 20 lines */

/* Mario's frame slots (as ms_init fills them in) */
enum { WALK_R = 0, WALK_L = 4, RUN_R = 8, RUN_L = 16, FRONT = 24 };   /* slots 0-3 face right */

static hm_tex gTitle, gStage, gDoorSheet, gMenu, gBackdrop;
static uint32_t gBackdropPal[128];
static int gBackdropHotel;
static hm_sprite_set gFont;
static uint32_t gMenuPal[128], gFontPal[128];
static world gW;
static hm_campaign gCampaign;
static unsigned gCheats;
static hm_profile gProfile;
static int gVmuId=-1,gScoreVmuId=-1,gProfileDirty,gDemoRunning,gProfileReadError;
static char gSaveStatus[64];
static int profile_slots(int restoring,int player);
static void original_text(const char *,int,int,int);
static player_t gSessionProgress[2]; /* per-session progress; CD-i saves contain named hotel/stage checkpoints only */
static hm_tex gHotelMap, gMapMask;
static int gMapProgress;
static int pause_menu(void);
static int hotel_map(player_t *player, int paused);

static int continue_game(int player);
static hm_sprite_set gObj[N_OBJ];
static uint8_t *gExtra;
static uint32_t gExtraSize;
static uint8_t *gInf, *gRoom;   /* kept for replays */
static hm_sprite_set gHud, gFx, gMario;
static uint32_t gStagePal[128], gPlayerPal[40], gPowerPal[160];
static struct { int16_t first, last, speed, count, pos, active; } gCycle[5];
static char gStages[100][8];             /* "H_SS" */
static int gNStages, gCur, gLuigi;

/* player colours 0x0b-0x14 (what player_clut_change does on the CD-i) */
static uint32_t gSpritePal[128];        /* plane A in play: loading_clut (intro record 5) */
static int gNSpritePal;

static int gPlayerShown = -1;           /* last colour state written (to skip repeats) */

/* player_clut_change + power_cycle: colours 0x0b-0x14 of plane B are Mario's */
static void set_player_colours(void) {
    int i, set, phase = 0, kind, power = gW.g.player[gW.g.cur_player].power, key;
    set = gLuigi * 2 + (power == 2);
    kind = world_power_colours(&gW, &i, &phase);
    key = set * 1000 + kind * 100 + phase;
    if (key == gPlayerShown) return;
    gPlayerShown = key;
    for (i = 0; i < 10; i++) plat_set_colour(BANK_PLANE_B, 0x0b + i, gPlayerPal[set * 10 + i]);
    if (kind == 1)                                   /* shroomPowerColors: 12 x 5 */
        for (i = 0; i < 5; i++) plat_set_colour(BANK_PLANE_B, 0x10 + i, gPowerPal[phase * 5 + i]);
    else if (kind == 2)                              /* firePowerColors: 10 x 5 per player */
        for (i = 0; i < 5; i++) plat_set_colour(BANK_PLANE_B, 0x10 + i, gPowerPal[60 + gLuigi * 50 + phase * 5 + i]);
}

/* colour cycling from stage_info+0x58 (color_cycle on the CD-i): rotate a range */
static void cycle_colours(void) {
    int c, i;
    for (c = 0; c < 5; c++) {
        int n = gCycle[c].last - gCycle[c].first + 1;
        if (gCycle[c].speed <= 0 || gCycle[c].speed >= 0x7fff || n <= 1 || gCycle[c].first < 0 || gCycle[c].last > 127)
            continue;
        if (--gCycle[c].count > 0) continue;
        gCycle[c].count = gCycle[c].speed;
        gCycle[c].pos = (gCycle[c].pos + 1) % n;
        for (i = 0; i < n; i++)
            plat_set_colour(BANK_PLANE_B, gCycle[c].first + i, gStagePal[gCycle[c].first + (i + gCycle[c].pos) % n]);
    }
}

static int load_common(void) {
    uint32_t size;
    char *list;
    int i = 0;
    if (!plat_load_sprites("mario.tex", "mario.frm", &gMario)) return 0;
    if (!plat_load_tex("menu_title.tex", BANK_PLANE_B, &gMenu) ||
        !plat_load_sprites("menu_font.tex", "menu_font.frm", &gFont)) return 0;
    if (!plat_load_tex("map_mask.tex", 2, &gMapMask)) return 0;
    gFont.tex.bank = 3; /* Font has a separate palette while pause overlays gameplay. */
    if (!plat_load_pal("menu_title.pal", -1, 0, gMenuPal, 128) ||
        !plat_load_pal("menu_font.pal", -1, 0, gFontPal, 128)) return 0;
    for (i = 0; i < N_OBJ; i++) {                      /* the ported objects: obj_<name>.tex/.frm */
        char t[40], f[40];
        snprintf(t, sizeof t, "obj_%s.tex", world_obj_name(i));
        snprintf(f, sizeof f, "obj_%s.frm", world_obj_name(i));
        if (plat_load_sprites(t, f, &gObj[i])) gObj[i].tex.bank = BANK_PLANE_A;
    }
    if (plat_load_sprites("hud.tex", "hud.frm", &gHud)) gHud.tex.bank = BANK_PLANE_A;
    if (plat_load_sprites("fx.tex", "fx.frm", &gFx)) gFx.tex.bank = BANK_PLANE_A;
    plat_load_pal("players.pal", -1, 0, gPlayerPal, 40);
    plat_load_pal("power.pal", -1, 0, gPowerPal, 160);
    gNSpritePal = plat_load_pal("sprites.pal", -1, 0, gSpritePal, 128);
    list = plat_load_file("stages.txt", &size);
    if (list == NULL) return 0;
    i = 0;
    while (i < (int)size && gNStages < 100) {          /* one "H_SS" per line */
        int n = 0;
        while (i < (int)size && list[i] != '\n' && n < 7) gStages[gNStages][n++] = list[i++];
        gStages[gNStages][n] = 0;
        while (i < (int)size && list[i] != '\n') i++;
        i++;
        if (n) gNStages++;
    }
    free(list);
    return gNStages > 0;
}

/* ---- loading: the title card first, everything else in the background while it shows */
static struct { char tag[8]; uint8_t *stage_tex, *stage_pal, *door_tex, *info, *room, *sfx, *music[11]; uint32_t n; } gLoad;
static uint8_t *gSfx, *gMusic[11];         /* the stage's sounds in use (music by Ln_am record) */
static void *gLoader;

static void *loader_thread(void *arg) {
    char name[40];
    (void)arg;
    snprintf(name, sizeof name, "stage_%s.tex", gLoad.tag); gLoad.stage_tex = plat_load_file(name, &gLoad.n);
    snprintf(name, sizeof name, "stage_%s.pal", gLoad.tag); gLoad.stage_pal = plat_load_file(name, &gLoad.n);
    snprintf(name, sizeof name, "doors_%s.tex", gLoad.tag); gLoad.door_tex = plat_load_file(name, &gLoad.n);
    snprintf(name, sizeof name, "stage_%s.inf", gLoad.tag); gLoad.info = plat_load_file(name, &gLoad.n);
    snprintf(name, sizeof name, "room_%s.room", gLoad.tag); gLoad.room = plat_load_file(name, &gLoad.n);
    snprintf(name, sizeof name, "sfx_%s.xa", gLoad.tag); gLoad.sfx = plat_load_file(name, &gLoad.n);
    {   /* the stage tune (same rule as world_start) and the end-of-stage records 8-10 */
        int hotel = gLoad.tag[0] - '0', stage = atoi(gLoad.tag + 2) - 1, r;
        int tune = (stage == (hotel == 8 ? 14 : 9)) ? 4 : 5 + stage % 3;
        for (r = 4; r <= 10; r++) {
            if (r < 8 && r != tune) continue;
            snprintf(name, sizeof name, "music_%d_%d.xa", hotel, r);
            gLoad.music[r] = plat_load_file(name, &gLoad.n);
        }
    }
    return NULL;
}

static void drop_loaded(void) {
    plat_thread_join(gLoader);
    gLoader = NULL;
    if (gLoad.stage_tex) free(gLoad.stage_tex);
    if (gLoad.stage_pal) free(gLoad.stage_pal);
    if (gLoad.door_tex) free(gLoad.door_tex);
    if (gLoad.info) free(gLoad.info);
    if (gLoad.room) free(gLoad.room);
    if (gLoad.sfx) free(gLoad.sfx);
    { int r; for (r = 0; r < 11; r++) if (gLoad.music[r]) free(gLoad.music[r]); }
    memset(&gLoad, 0, sizeof gLoad);
}

/* the title card for gStages[gCur] now, the rest of the stage in the background */
static void load_title(void) {
    char name[40];
    const char *tag = gStages[gCur];
    drop_loaded();
    plat_free_tex(&gTitle);
    snprintf(name, sizeof name, "title_%s_%c.tex", tag, gLuigi ? 'l' : 'm');
    plat_load_tex(name, BANK_PLANE_A, &gTitle);
    snprintf(name, sizeof name, "title_%s_%c.pal", tag, gLuigi ? 'l' : 'm');
    plat_load_pal(name, BANK_PLANE_A, 0, NULL, 0);
    plat_set_transparent(BANK_PLANE_A, PAL_TRANSPARENT);
    memcpy(gLoad.tag, tag, sizeof gLoad.tag);
    gLoader = plat_thread_start(loader_thread, NULL);
}

/* when the player starts: wait for the background load (usually done) and upload it */
static void finish_stage_load(void) {
    plat_thread_join(gLoader);
    gLoader = NULL;
    plat_free_tex(&gStage);
    plat_free_tex(&gDoorSheet);
    plat_tex_from_mem(gLoad.stage_tex, BANK_PLANE_B, &gStage);
    plat_pal_from_mem(gLoad.stage_pal, BANK_PLANE_B, 0, gStagePal, 128);
    plat_tex_from_mem(gLoad.door_tex, BANK_PLANE_B, &gDoorSheet);
    plat_set_transparent(BANK_PLANE_B, PAL_TRANSPARENT);
    gPlayerShown = -1;
    set_player_colours();
    {   /* the_game: set_up_clut(fct_a, loading_clut) - the one palette all objects use */
        int i;
        for (i = 0; i < gNSpritePal && i < 128; i++) plat_set_colour(BANK_PLANE_A, i, gSpritePal[i]);
        plat_set_transparent(BANK_PLANE_A, PAL_TRANSPARENT);
    }
    memset(gCycle, 0, sizeof gCycle);
    if (gLoad.info && memcmp(gLoad.info, "HMSI", 4) == 0)
        memcpy(gCycle, gLoad.info + 4 + 70 + 4, sizeof gCycle);       /* after the time limit */
    {   /* the sounds: stop, then swap in this stage's */
        int r;
        snd_music(NULL, 0);
        snd_sfx_bank(NULL);
        if (gSfx) free(gSfx);
        gSfx = gLoad.sfx; gLoad.sfx = NULL;
        for (r = 0; r < 11; r++) {
            if (gMusic[r]) free(gMusic[r]);
            gMusic[r] = gLoad.music[r]; gLoad.music[r] = NULL;
        }
        snd_sfx_bank(gSfx);
    }
    if (gInf) free(gInf);
    if (gRoom) free(gRoom);
    gInf = gLoad.info;  gLoad.info = NULL;                            /* kept for replays */
    gRoom = gLoad.room; gLoad.room = NULL;
    drop_loaded();
}

static void wait_reset_released(void) {
    while (plat_pad() & (PAD_B1 | PAD_B2 | PAD_X | PAD_Y | PAD_START)) {
        plat_frame_begin();
        plat_frame_end();
    }
}

/* returns 1 to play, 0 if the stage changed (reload), -1 on soft reset */
static int title(void) {
    uint32_t prev = plat_pad();
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset()) return -1;
        plat_frame_begin();
        plat_draw_picture(&gTitle);
        plat_frame_end();
        if(pressed&PAD_B2)return 2;
        if(pressed&PAD_Y)return 3;
        if (pressed & (PAD_B1 | PAD_START)) return 1;
        prev = p;
    }
}

static void draw_doors(void) {
    int f, c, sx, sy;
    for (f = 0; f < FLOORS; f++)
        for (c = 0; c < COLS; c++) {
            if (gW.tile[f][c].on) { sx = gW.tile[f][c].sx; sy = gW.tile[f][c].sy; }   /* Mario's door/lift art */
            else if (!hm_doors_tile(&gW.doors, f, c, &sx, &sy)) continue;            /* dm(door_x, floor_y - 31, ...) */
            plat_draw_tile_lit(&gDoorSheet, sx, sy, 32, 31, hm_door_x[c], floor_y[f] - 31 + SCREEN_OFS);
        }
}

static uint8_t gLightMap[CDI_H][CDI_W];
static int gLightKey=-1;
static int plane_light(void *context, int x, int y) {
    (void)context;
    return x>=0 && x<CDI_W && y>=0 && y<CDI_H ? gLightMap[y][x] : 255;
}
static int update_light_map(void) {
    const player_t *p=&gW.g.player[gW.g.cur_player];
    int key=p->hotel | (p->stage<<4) | (gW.upper<<8) | ((gW.mario.floor+1)<<9) |
            (gW.event.light<<13) | (!!gW.event.completed<<17) | (!!p->unk_10<<18);
    if (key==gLightKey) return 0;
    gLightKey=key;
    for (int y=0;y<CDI_H;y++) for (int x=0;x<CDI_W;x++)
        gLightMap[y][x]=stage_plane_light(&gW,x,y);
    return 1;
}

/* exit_check displays these reasons on the original gameplay screen. */
static const char *stage_loss_message(const world *w) {
    if(w->g.unk_53a!=2)return NULL;
    if(w->end_reason==WORLD_ALL_OPEN)return "ALL DOORS OPEN";
    if(w->end_reason==WORLD_TIME_UP)return "TIME UP";
    return NULL;
}

static void draw_world(void) {
    int i;
    if (gW.g.player[gW.g.cur_player].hotel==2) for (int c=43;c<=45;c++) {
        uint32_t rgb=gStagePal[c];stage_light_colour(&gW,c,&rgb);
        plat_set_colour(BANK_PLANE_B,c,rgb);
    }
    int masked=!gW.event.completed && !gW.g.player[gW.g.cur_player].unk_10 &&
               (gW.g.player[gW.g.cur_player].hotel==2 || gW.g.player[gW.g.cur_player].hotel==3);
    static int was_masked=-1;
    if(masked) {
        int changed=update_light_map();
        if(changed || was_masked!=1)plat_set_light_mask(plane_light,&gW);
    } else if(was_masked!=0)plat_set_light_mask(NULL,&gW);
    was_masked=masked;
    plat_draw_picture_lit(&gStage,gW.shake);
    draw_doors();
    if (gW.msp.vis && gW.msp.slot >= 0 && gW.msp.slot < gMario.n_slots)
        plat_draw_frame_masked(&gMario, gMario.slot[gW.msp.slot], gW.msp.x, gW.msp.y + SCREEN_OFS, 0.0f);
    if (gW.sparkle.slot >= 0 && gW.sparkle.slot < gMario.n_slots)
        plat_draw_frame_masked(&gMario, gMario.slot[gW.sparkle.slot], gW.sparkle.x, gW.sparkle.y + SCREEN_OFS, 0.0005f);
    for (i = 0; i < gW.n_spr; i++) {              /* plane A: in front of plane B */
        const world_sprite *sp = &gW.spr[i];
        hm_sprite_set *s = &gObj[sp->obj];
        if (s->n_frames && sp->frame < s->n_slots)
            plat_draw_frame_masked(s, s->slot[sp->frame], sp->x, sp->y + SCREEN_OFS, 1.0f + i * 0.001f);
    }
    for (i = 0; i < gW.n_fx; i++)                 /* fireballs, kill stars (plane A) */
        if (gFx.n_frames && gW.fx[i].frame < gFx.n_slots)
            plat_draw_frame_masked(&gFx, gFx.slot[gW.fx[i].frame], gW.fx[i].x, gW.fx[i].y + SCREEN_OFS, 1.5f + i * 0.001f);
    {   /* the score bar (plane A, scoring) */
        world_sprite hud[40];
        int n = world_hud(&gW, hud, 40);
        for (i = 0; i < n; i++)
            if (gHud.n_frames && hud[i].frame < gHud.n_slots)
                plat_draw_frame(&gHud, gHud.slot[hud[i].frame], hud[i].x, hud[i].y + SCREEN_OFS, 2.0f + i * 0.001f);
    }
    const char *loss=stage_loss_message(&gW);
    if(loss)original_text(loss,96,120+SCREEN_OFS,1);
}

/* top(): keep the same attempt/player, scroll to the upper picture, then
 * initialise its distinct module/room. Music uses the original in-RAM files. */
static int enter_upper(void) {
    char name[48];
    uint32_t size, extra_size;
    uint8_t *picture, *palette, *info, *room, *extra;
    hm_tex upper = {0};
    const char *tag = gStages[gCur];
    snd_music(NULL, 0); snd_stop_afx();
    snprintf(name,sizeof name,"stage_%s_u.tex",tag); picture=plat_load_file(name,&size);
    snprintf(name,sizeof name,"stage_%s_u.pal",tag); palette=plat_load_file(name,&size);
    snprintf(name,sizeof name,"stage_%s_u.inf",tag); info=plat_load_file(name,&size);
    snprintf(name,sizeof name,"room_%s_u.room",tag); room=plat_load_file(name,&size);
    snprintf(name,sizeof name,"stage_%s_u.ext",tag); extra=plat_load_file(name,&extra_size);
    if (!picture || !palette || !info || !room || !extra) {
        free(picture);free(palette);free(info);free(room);free(extra);
        return 0;
    }
    plat_wait_ready();
    if (!plat_tex_from_mem(picture,BANK_PLANE_B,&upper)) {
        free(picture);free(palette);free(info);free(room);free(extra);return 0;
    }
    free(picture);
    for (int y=0;y<=280;y++) {
        if (plat_soft_reset()) {
            plat_wait_ready();plat_free_tex(&upper);free(palette);free(info);free(room);free(extra);return 0;
        }
        world_sprite hud[40]; int n=world_hud(&gW,hud,40);
        plat_frame_begin();
        plat_draw_picture_offset(&upper,y-280);
        plat_draw_picture_offset(&gStage,y);
        for (int i=0;i<n;i++) plat_draw_frame(&gHud,gHud.slot[hud[i].frame],hud[i].x,hud[i].y+SCREEN_OFS,2);
        plat_frame_end();
    }
    plat_wait_ready();plat_free_tex(&gStage);gStage=upper;
    plat_pal_from_mem(palette,BANK_PLANE_B,0,gStagePal,128);free(palette);
    free(gInf);free(gRoom);free(gExtra);gInf=info;gRoom=room;gExtra=extra;gExtraSize=extra_size;
    gCampaign.player[gCampaign.current]=gW.g.player[gW.g.cur_player];
    world_start_full(&gW,gInf,gRoom,gCampaign.player,gCampaign.current,gCampaign.players==2,1,gExtra,gExtraSize);
    world_set_cheats(&gW,gCheats);
    gW.luigi=gLuigi;gPlayerShown=-1;
    memcpy(gCycle,gInf+78,sizeof gCycle);
    return 1;
}

/* Returns 1 when cleared, 2 to replay, 3 for map/warp, -1 on soft reset. */
static int stage(void) {
    uint32_t prev = plat_pad(), suppress = 0;
    int pause = 0, result = 0;
    world_start_full(&gW, gInf, gRoom, gCampaign.player,
                     gCampaign.current, gCampaign.players == 2, 0, gExtra, gExtraSize);
    world_set_cheats(&gW,gCheats);
    gPlayerShown = -1;
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        uint32_t in = 0;
        int st;
        if (plat_soft_reset()) return -1;
        if (gDemoRunning && (p || gW.frames>=1800)) return -1;
        if (!gDemoRunning && !pause && gW.status == WORLD_PLAYING &&
            (gW.g.unk_53a == 0 || gW.g.unk_53a == 3) && (pressed & PAD_START)) {
            int action = pause_menu();
            if (!action) return -1;
            if (action == 2) return 3; /* map travel: same player's turn, no tally */
            suppress = plat_pad();
            prev = suppress;
            continue; /* Resume on a fresh field; menu input never reaches Mario. */
        }
        suppress &= p; /* Consume menu-held buttons until each is released. */
        uint32_t game_pad = p & ~suppress;
        if (game_pad & PAD_LEFT) in |= IN_LEFT;
        if (game_pad & PAD_RIGHT) in |= IN_RIGHT;
        if (game_pad & PAD_UP) in |= IN_UP;
        if (game_pad & PAD_DOWN) in |= IN_DOWN;
        if (game_pad & PAD_B1) in |= IN_B1;            /* A: CD-i button 1 (jump) */
        if (game_pad & PAD_B2) in |= IN_B2;            /* B: CD-i button 2 (doors) */
        if (game_pad & PAD_X) in |= IN_RUN;
        if (game_pad & PAD_Y) in |= IN_ITEM;
        if(gDemoRunning)in=demo_input(&gW,gW.frames);
        gW.luigi = gLuigi;
        int bonus_before=gW.g.player[gW.g.cur_player].unk_10;
        st = world_frame(&gW, in);
        if(!gDemoRunning && !bonus_before && gW.g.player[gW.g.cur_player].unk_10 && st==WORLD_PLAYING) {
            int hotel=gW.g.player[gW.g.cur_player].hotel;
            if(hotel==2 || hotel==4 || hotel==5 || hotel==7) {
                char clip[48];snprintf(clip,sizeof clip,"movie_bonus_%d",hotel);
                /* Do not advance the world or the gameplay decoder during a film. */
                if(movie_play(clip)<0)return -1;
                p=plat_pad();suppress=p;prev=p;
            }
        }
        if (gW.afx_req >= 0) { snd_play_afx(gW.afx_req); gW.afx_req = -1; }
        if (gW.music_req >= 0) {
            snd_music(gW.music_req > 0 && gW.music_req < 11 ? gMusic[gW.music_req] : NULL, gW.music_loop);
            gW.music_req = -1;
        }
        if(gDemoRunning && st!=WORLD_PLAYING)return -1;
        if (st == WORLD_UPPER) {
            if (!enter_upper()) return -1;
            prev=p;continue;
        }
        if (st == WORLD_WARP) {
            char clip[48];snprintf(clip,sizeof clip,"movie_bonus_%d",gW.g.player[gW.g.cur_player].hotel);
            if(movie_play(clip)<0)return -1;
            player_t *player=&gCampaign.player[gCampaign.current];
            *player=gW.g.player[gW.g.cur_player];
            player->hotel=gW.warp_hotel;player->stage=gW.warp_stage;
            if (!(player->flags&4)) player->unk_10=0;
            if (player->hotel>player->progress || (player->hotel==player->progress && player->stage>player->unk_18)) {
                player->progress=player->hotel;player->unk_18=player->stage;
            }
            frontend_record_progress(gSessionProgress,gCampaign.player);
            snd_music(NULL,0);snd_stop_afx();return 3;
        }
        if (st != WORLD_PLAYING && !pause) {
            pause = 480;
            result = (st == WORLD_CLEARED) ? 1 : 2;
            campaign_capture(&gCampaign, &gW);
            frontend_record_progress(gSessionProgress,gCampaign.player);
        }
        /* after the end: let the jingle finish (at most ~8 s) */
        if (pause && (--pause == 0 || (pause < 450 && !snd_music_playing()))) {
            snd_music(NULL, 0); snd_stop_afx(); return result;
        }
        cycle_colours();
        set_player_colours();
        plat_frame_begin();
        draw_world();
        if(gDemoRunning)original_text("DEMO   PRESS ANY BUTTON",48,264,1);
        plat_frame_end();
        prev = p;
    }
}

/* Acknowledge status screens using a fresh press, never a held jump button. */


/* alphablit's original font is A..q; digits are encoded as [..d. */
static void original_text(const char *text, int x, int y, int scale) {
    for (; *text; text++) {
        int c = (unsigned char)*text;
        if (c >= '0' && c <= '9') c = '[' + c - '0';
        if (c == ' ') { x += 10 * scale; continue; }
        if (c < 'A' || c > 'q') continue;
        int slot = c - 'A';
        const hm_frame *f = &gFont.frames[gFont.slot[slot]];
        plat_draw_frame_scaled(&gFont, gFont.slot[slot], x, y, 3, scale);
        x += f->w * scale;
    }
}

static void original_palette(const uint32_t *pal, int bank) {
    for (int i = 0; i < 128; i++) plat_set_colour(bank, i, pal[i]);
    plat_set_transparent(bank, PAL_TRANSPARENT);
}

/* Original intro.rtf records progress+7 (DYUV map) and 16 (hotel mask).
 * Original map navigation is in init_the_game, 0x71xx..0x74xx. */
/* Keep the original arrival/ready text readable even when movie loading is fast. */
static void arrival_card(int hotel,int ready) {
    int x;const char *label=frontend_arrival(hotel,&x);
    original_palette(gFontPal,3);
    plat_frame_begin();original_text(label,x,110,1);
    original_text("ARE PREPARING FOR YOUR ARRIVAL",47,120,1);
    if(ready) {
        time_t now=time(NULL);struct tm *date=localtime(&now);
        const char *message=frontend_ready(date?date->tm_mon+1:0,date?date->tm_mday:0,&x);
        original_text(message,x,150,1);
    }
    plat_frame_end();
}
static void arrival_ready(void *hotel){
    /* Three seconds at the port's 60 Hz display rate; no extra confirmation. */
    for(int frame=0;frame<180;frame++) {
        if(plat_soft_reset())break;
        arrival_card(*(int*)hotel,1);
    }
}
static int play_hotel_entry(int hotel) {
    char clip[48];snprintf(clip,sizeof clip,"movie_entry_%d",hotel);
    snd_music(NULL,0);snd_stop_afx();arrival_card(hotel,0);
    return movie_play_prepared(clip,arrival_ready,&hotel);
}

static int load_hotel_map(int progress) {
    char name[40];
    if (gMapProgress == progress && gHotelMap.parts) return 1;
    plat_wait_ready();
    plat_free_tex(&gHotelMap);
    gMapProgress = 0;
    snprintf(name, sizeof name, "map_%d.tex", progress);
    if (!plat_load_tex(name, BANK_PLANE_B, &gHotelMap)) return 0;
    gMapProgress = progress;
    return 1;
}

/* The original clean_bg remains behind bonus/gh_save foreground text. */
static int prepare_backdrop(int hotel) {
    if(hotel<1 || hotel>8 || hotel==6)hotel=1;
    if(hotel!=gBackdropHotel) {
        char name[40];plat_wait_ready();plat_free_tex(&gBackdrop);gBackdropHotel=0;
        snprintf(name,sizeof name,"continue_%d.tex",hotel);
        if(!plat_load_tex(name,2,&gBackdrop))return 0;
        snprintf(name,sizeof name,"continue_%d.pal",hotel);
        if(!plat_load_pal(name,-1,0,gBackdropPal,128))return 0;
        gBackdropHotel=hotel;
    }
    original_palette(gBackdropPal,2); /* map-mask use may have changed this bank */
    return 1;
}

/* Returns 1 for travel, 2 for resume, 0 for back, -1 for reset/error. */
static int hotel_map(player_t *player, int paused) {
    uint32_t saved[2][256];
    char name[40], text[64];
    int hotel = player->hotel, progress = player->progress, result = 0, blink = 0;
    if (progress < 1 || progress > 8 || progress == 6) progress = 1;
    if (hotel < 1 || hotel > progress || hotel == 6) hotel = 1;
    plat_save_palette(0, saved[0]); plat_save_palette(1, saved[1]);
    if (!load_hotel_map(progress)) return -1;
    snprintf(name,sizeof name,"map_%d.pal",progress);
    if (!plat_load_pal(name, BANK_PLANE_B, 0, NULL, 0)) return -1;
    original_palette(gFontPal, 3);
    uint32_t prev = plat_pad();
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset()) { result = -1; break; }
        if (pressed & (PAD_LEFT | PAD_UP)) hotel = frontend_next_hotel(hotel,-1,progress);
        if (pressed & (PAD_RIGHT | PAD_DOWN)) hotel = frontend_next_hotel(hotel,1,progress);
        for (int i=0;i<256;i++) plat_set_transparent(2,i);
        plat_set_colour(2,hotel,(++blink/20)&1 ? 0xffffff : 0xcccccc);
        plat_frame_begin();
        plat_draw_picture(&gHotelMap);
        plat_draw_overlay(&gMapMask,96);
        snprintf(text,sizeof text,"%s HOTEL %d",paused ? "PAUSED" :
                 player == &gCampaign.player[1] ? "LUIGI" : "MARIO",hotel);
        original_text(text,96,5,1);
        original_text(paused ? "A TRAVEL   B BACK   START RESUME" : "A START   B BACK   LEFT RIGHT SELECT",24,265,1);
        plat_frame_end();
        if (pressed & PAD_B2) break;
        if (paused && (pressed & PAD_START)) { result = 2; break; }
        if (pressed & (PAD_B1 | PAD_START)) {
            frontend_choose_hotel(player,hotel); result = 1; break;
        }
        prev = p;
    }
    plat_wait_ready();
    plat_restore_palette(0,saved[0]); plat_restore_palette(1,saved[1]);
    return result;
}

static int pause_menu(void) {
    uint32_t saved[256];
    int row = 0, result = 1;
    plat_sound_pause(1);
    plat_save_palette(BANK_PLANE_A,saved);
    original_palette(gFontPal,3);
    uint32_t prev = plat_pad();
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset()) { result = 0; break; }
        if (pressed & PAD_UP) row = (row + 2) % 3;
        if (pressed & PAD_DOWN) row = (row + 1) % 3;
        plat_frame_begin();
        draw_world();
        plat_draw_panel(76,84,232,116,0xe0000000);
        original_text("PAUSED",143,98,2);
        original_text("RESUME",126,132,1);
        original_text("HOTEL MAP",126,152,1);
        original_text("RETURN TO TITLE",126,172,1);
        original_text("p",106,132+row*20,1);
        plat_frame_end();
        if (pressed & (PAD_START | PAD_B2)) break;
        if (pressed & PAD_B1) {
            if (row == 0) break;
            if (row == 2) {
                frontend_record_progress(gSessionProgress,gW.g.player);
                result = 0; break;
            }
            player_t destination = gW.g.player[gW.g.cur_player];
            int travel = hotel_map(&destination,1);
            if (travel < 0) { result = 0; break; }
            if (travel == 2) break;
            if (travel == 1) {
                memcpy(gCampaign.player,gW.g.player,sizeof gCampaign.player);
                gCampaign.player[gCampaign.current] = destination;
                frontend_record_progress(gSessionProgress,gCampaign.player);
                snd_music(NULL,0); snd_stop_afx(); result = 2; break;
            }
            prev = plat_pad();
            continue;
        }
        prev = p;
    }
    plat_wait_ready();
    plat_restore_palette(BANK_PLANE_A,saved);
    plat_sound_pause(0);
    return result;
}

/* gh_save's original CONTINUE/YES/NO text positions and hotel clean_bg art.
 * The original saved-games and high-score flows follow NO. */
static int continue_game(int player) {
    char name[40]; hm_tex backdrop = {0};
    const player_t *p = &gCampaign.player[player];
    gW.g.cur_player=player;gW.luigi=player;
    gW.g.player[player] = *p; /* The ended world's HUD must show the lost final life. */
    snprintf(name,sizeof name,"continue_%d.tex",p->hotel);
    plat_wait_ready();
    if (!plat_load_tex(name,BANK_PLANE_B,&backdrop)) return 0;
    snprintf(name,sizeof name,"continue_%d.pal",p->hotel);
    plat_load_pal(name,BANK_PLANE_B,0,NULL,0);
    original_palette(gFontPal,3);
    original_palette(gSpritePal,BANK_PLANE_A);
    uint32_t prev = plat_pad(); int yes = 1, result = 0;
    for (;;) {
        uint32_t pad=plat_pad(), pressed=pad & ~prev;
        if (plat_soft_reset()) { result=-1; break; }
        if (pressed & (PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT)) yes ^= 1;
        plat_frame_begin(); plat_draw_picture(&backdrop);
        world_sprite hud[40];
        int n=world_hud(&gW,hud,40);
        for (int i=0;i<n;i++) plat_draw_frame(&gHud,gHud.slot[hud[i].frame],hud[i].x,hud[i].y+20,2);
        original_text("CONTINUEm",101,120,2);
        original_text("YES",130,150,2);
        original_text("NO",130,180,2);
        original_text("p",110,yes ? 150 : 180,2);
        plat_frame_end();
        if (pressed & PAD_B2) break;
        if (pressed & (PAD_B1|PAD_START)) { result=yes; break; }
        prev=pad;
    }
    plat_wait_ready();plat_free_tex(&backdrop);
    return result;
}

#include "profile_ui.h"
static int choose_players(void) {
    gCheats=0;
    int row=0,idle=0;
    uint32_t prev=plat_pad();
    for(;;) {
        uint32_t p=plat_pad(),pressed=p&~prev;int rows=4;
        if(p)idle=0;else idle++;
        if(idle>=1800)return 4;
        if(pressed&PAD_UP)row=(row+rows-1)%rows;
        if(pressed&PAD_DOWN)row=(row+1)%rows;
        original_palette(gMenuPal,BANK_PLANE_B);original_palette(gFontPal,3);
        plat_frame_begin();plat_draw_picture(&gMenu);
        const char*labels[]={"1 PLAYER","2 PLAYER","HI SCORES","DEMO"};
        for(int i=0;i<4;i++)original_text(labels[i],152,140+i*20,1);
        original_text("p",132,140+row*20,1);plat_frame_end();
        if(pressed&(PAD_B1|PAD_START)) {
            if(row<2) {
                if(row==0)gCheats=((p&PAD_L)?HM_CHEAT_INVINCIBLE:0)|((p&PAD_R)?HM_CHEAT_TIME:0);
                return row+1;
            }
            if(row==2)profile_scores();
            else if(row==3)return 4;
            prev=plat_pad();continue;
        }
        prev=p;
    }
}

static int find_campaign_stage(void) {
    int i;
    char tag[8];
    if (!campaign_tag(&gCampaign, tag, sizeof tag)) return -1;
    for (i = 0; i < gNStages; i++) if (strcmp(gStages[i], tag) == 0) return i;
    return -1;
}

static void draw_completion(void) {
    char number[24];
    world_sprite hud[40];
    int current = gCampaign.current;
    gW.g.player[current].score = gCampaign.player[current].score;
    int backdrop=prepare_backdrop(gCampaign.player[current].hotel);
    original_palette(gFontPal,3);
    original_palette(gSpritePal,BANK_PLANE_A);
    plat_frame_begin();
    if(backdrop)plat_draw_picture(&gBackdrop);
    int n = world_hud(&gW, hud, 40);
    for (int i = 0; i < n; i++)
        plat_draw_frame(&gHud, gHud.slot[hud[i].frame], hud[i].x, hud[i].y + SCREEN_OFS, 2);
    /* bonus.c and display_bonus_data.c offsets, plus the port's 20-line draw offset. */
    original_text(current ? "LUIGI" : "MARIO", current ? 152 : 148, 70 + SCREEN_OFS, 2);
    original_text("STAGE COMPLETED", 50, 95 + SCREEN_OFS, 2);
    plat_draw_frame(&gHud, gHud.slot[16], 24, 135 + SCREEN_OFS, 2);
    snprintf(number, sizeof number, "%3d", gCampaign.bonus_time);
    original_text(number, 52, 135 + SCREEN_OFS, 2);
    original_text("n", 116, 135 + SCREEN_OFS, 2);
    snprintf(number, sizeof number, "%3d o", gCampaign.bonus_rate);
    original_text(number, 138, 135 + SCREEN_OFS, 2);
    snprintf(number, sizeof number, "%6ld", (long)gCampaign.bonus_time * gCampaign.bonus_rate);
    original_text(number, 232, 135 + SCREEN_OFS, 2);
    if (gCampaign.special_paid && gCampaign.special_bonus > 0) {
        original_text("BONUS g", 28, 155 + SCREEN_OFS, 2);
        snprintf(number, sizeof number, "%6d", gCampaign.special_bonus);
        original_text(number, 228, 155 + SCREEN_OFS, 2);
    }
    plat_frame_end();
}

static int time_bonus(void) {
    int frames = 0;
    uint32_t prev = plat_pad();
    while (gCampaign.bonus_time > 0 || frames < 60) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset()) return 0;
        if (pressed & (PAD_B1 | PAD_START)) {
            while (campaign_bonus_tick(&gCampaign)) {}
            frames = 60;
        } else if (++frames > 60 && frames % 5 == 0) campaign_bonus_tick(&gCampaign);
        draw_completion();
        prev = p;
    }
    campaign_finish_bonus(&gCampaign);
    /* Original tally flows straight into the next card; no placeholder prompt. */
    for (int i = 0; i < 120; i++) {
        if (plat_soft_reset()) return 0;
        draw_completion();
    }
    return 1;
}

static void run_demo(void) {
    hm_campaign saved=gCampaign;
    gDemoRunning=1;gCheats=0;
    campaign_start(&gCampaign,1);gCur=find_campaign_stage();gLuigi=0;
    if(gCur>=0) {
        load_title();uint32_t prev=plat_pad();int cancelled=0;
        for(int i=0;i<120;i++) {
            uint32_t p=plat_pad();
            if((p&~prev)||plat_soft_reset()){cancelled=1;break;}
            plat_frame_begin();plat_draw_picture(&gTitle);original_text("DEMO   PRESS ANY BUTTON",48,264,1);plat_frame_end();prev=p;
        }
        if(!cancelled) {
            finish_stage_load();free(gExtra);char name[40];
            snprintf(name,sizeof name,"stage_%s.ext",gStages[gCur]);gExtra=plat_load_file(name,&gExtraSize);
            stage();
        }
    }
    snd_music(NULL,0);snd_stop_afx();drop_loaded();gCampaign=saved;
    gDemoRunning=0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    plat_init();
    plat_sound_init();
    if (!load_common()) {
        fprintf(stderr,"Missing required game data\n");
        return 1;
    }
    profile_startup();
    movie_play("movie_intro");
    for (;;) {
        int running = 1, seen_hotel[2]={0,0};
        int selection=choose_players();
        if(selection==4){run_demo();continue;}
        {
            if(!profile_begin_game(selection))continue;
            for(int i=0;i<gCampaign.players;i++)if(hotel_map(&gCampaign.player[i],0)<=0){running=0;break;}
            if(!running)continue;
        }
        while (running) {
            int r, outcome, old_player;
            char text[256];
            gCur = find_campaign_stage();
            gLuigi = gCampaign.current;
            int hotel=gCampaign.player[gCampaign.current].hotel;
            if(seen_hotel[gCampaign.current]!=hotel) {
                if(play_hotel_entry(hotel)<0)break;
                seen_hotel[gCampaign.current]=hotel;
            }
            if (gCur < 0) {
                fprintf(stderr,"Missing campaign stage\n");
                break;
            }
            for(;;) {
                load_title();r=title();
                if(r==3) {
                    if(hotel_map(&gCampaign.player[gCampaign.current],0)<0){r=-1;break;}
                    gCur=find_campaign_stage();
                    int h=gCampaign.player[gCampaign.current].hotel;
                    if(seen_hotel[gCampaign.current]!=h) {
                        drop_loaded();
                        if(play_hotel_entry(h)<0){r=-1;break;}
                        seen_hotel[gCampaign.current]=h;
                    }
                    continue;
                }
                if(r!=2)break;
                int player=gCampaign.current;
                profile_slots(0,player);profile_submit_score(player);
                int answer=continue_game(player);
                if(answer<0){r=-1;break;}
                if(answer)continue; /* gh_save(1): continue preserves current counters */
                gCampaign.player[player].lives=0;
                int other=player^1;
                if(gCampaign.players==2&&gCampaign.player[other].lives>0&&!gCampaign.finished[other]) {
                    gCampaign.current=other;gCur=find_campaign_stage();gLuigi=other;continue;
                }
                drop_loaded();movie_play("movie_credits");
                r=-1;break;
            }
            if (r < 0) break;
            finish_stage_load();
            free(gExtra);
            snprintf(text,sizeof text,"stage_%s.ext",gStages[gCur]);
            gExtra=plat_load_file(text,&gExtraSize);
            r = stage();
            if (r < 0) break;
            if (r == 3) {continue;} /* map/restore/warp: no turn swap */
            if (r == 1 && !time_bonus()) break;
            old_player = gCampaign.current;
            outcome = campaign_advance(&gCampaign, r == 1);
            if(gCampaign.finished[old_player] && movie_play("movie_ending")<0)break;
            frontend_record_progress(gSessionProgress,gCampaign.player);
            if (!gCampaign.finished[old_player] && gCampaign.player[old_player].lives == 0) {
                int accepted = continue_game(old_player);
                if (accepted < 0) break;
                if (accepted) {
                    frontend_continue_player(&gCampaign.player[old_player]);
                    outcome = CAMPAIGN_NEXT;
                } else {profile_slots(0,old_player);profile_submit_score(old_player);}
            } else if(gCampaign.finished[old_player]) {profile_slots(0,old_player);profile_submit_score(old_player);}
            if (outcome == CAMPAIGN_GAME_OVER || outcome == CAMPAIGN_COMPLETE) {
                snd_music(NULL,0);snd_stop_afx();
                movie_play("movie_credits");
                running = 0;
            }
            /* The next original MARIO/LUIGI stage card announces the new turn. */
        }
        snd_music(NULL, 0); snd_stop_afx();
        drop_loaded();
        wait_reset_released();
    }
    return 0;
}
