#include "campaign.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static world w;
static uint8_t *inf, *room, *extra;
static unsigned extra_size;
static const char *data;
static uint8_t *load(const char *stem, const char *tag, const char *extension, unsigned *size) {
    char p[512];
    snprintf(p, sizeof p, "%s/%s_%s.%s", data, stem, tag, extension);
    FILE *f = fopen(p, "rb");
    assert(f);
    assert(!fseek(f, 0, SEEK_END));
    long n = ftell(f);
    assert(n > 0 && !fseek(f, 0, SEEK_SET));
    uint8_t *b = malloc(n);
    assert(b && fread(b, 1, n, f) == (size_t)n);
    fclose(f);
    if (size)
        *size = n;
    return b;
}
static void start(int h, int stage, int upper) {
    free(inf);
    free(room);
    free(extra);
    char tag[16];
    snprintf(tag, sizeof tag, "%d_%02d%s", h, stage + 1, upper ? "_u" : "");
    inf = load("stage", tag, "inf", NULL);
    room = load("room", tag, "room", NULL);
    extra = load("stage", tag, "ext", &extra_size);
    hm_campaign c;
    campaign_start(&c, 1);
    c.player[0].hotel = h;
    c.player[0].stage = stage;
    world_start_full(&w, inf, room, c.player, 0, 0, upper, extra, extra_size);
}
static void clear_grid(void) {
    for (int i = 0; i < 35; i++)
        if ((w.doors.cell[i] & 0xf000) == 0x2000)
            w.doors.cell[i] = (w.doors.cell[i] & 0xfff) | 0x1000;
    hm_doors_init(&w.doors, w.doors.cell);
    w.g.doors_open = 0;
}
static void reciprocal(const uint16_t *g) {
    for (int i = 0; i < 35; i++) {
        unsigned v = g[i], k = v & 0xf000;
        if (k != 0xe000 && k != 0xf000)
            continue;
        int f = (v >> 8) & 15, c = (v >> 4) & 15;
        assert(f < 5 && c < 7);
        int j = f * 7 + c;
        assert(((g[j] >> 8) & 15) == (unsigned)(i / 7));
        assert(((g[j] >> 4) & 15) == (unsigned)(i % 7));
    }
}
static void routes_and_data(void) {
    int n = 0, upper_count = 0, lifts = 0;
    for (int h = 0; h <= 8; h++) {
        if (h == 6)
            continue;
        int stages = h == 8 ? 15 : 10;
        for (int st = 0; st < stages; st++)
            for (int upper = 0; upper <= (h >= 7); upper++) {
                start(h, st, upper);
                n++;
                upper_count += upper;
                unsigned objects = room[4] | room[5] << 8;
                unsigned expected = room[6 + objects * 16] | room[7 + objects * 16] << 8;
                assert(w.room.n_actors == (int)expected);
                for (int i = 0; i < w.room.n_actors; i++)
                    assert(w.obj_of[i] >= 0 && w.obj_of[i] < N_OBJ);
                if (w.lift_kind) {
                    reciprocal(w.lift_grid[0]);
                    reciprocal(w.lift_grid[1]);
                    lifts++;
                    w.mario.state_group = 0;
                    w.g.time_left = 170;
                    stage_lifts_tick(&w);
                    assert(w.lift_phase == 1);
                    stage_lifts_tick(&w);
                    assert(w.lift_phase == 1);
                    w.g.time_left = 153;
                    w.mario.state_group = 1;
                    stage_lifts_tick(&w);
                    assert(w.lift_pending && w.lift_phase == 1);
                    w.g.time_left = 152;
                    w.mario.state_group = 0;
                    stage_lifts_tick(&w);
                    assert(!w.lift_pending && w.lift_phase == 0);
                }
                if (h >= 7 && !upper)
                    assert(w.roof_col >= 0 && (w.doors.cell[w.roof_col] & 0xf000) == 0xb000);
            }
    }
    assert(n == 110 && upper_count == 25);
    printf(
        "Data checks: %d rooms, %d upper rooms, %d switching layouts; every actor has a handler.\n",
        n, upper_count, lifts);
}
static void roof_and_retry(void) {
    start(7, 0, 0);
    clear_grid();
    w.mario.state_group = 0;
    w.mario.state = 0;
    w.mario.floor = 4;
    w.mario.x = 44;
    w.mario.y = hm_floor_y[4] - 30;
    assert(world_frame(&w, 0) == WORLD_PLAYING);
    assert(w.g.unk_53a == 3 && w.doors.cell[w.roof_col] == 0xf000);
    w.mario.state_group = 1;
    w.mario.state = 3;
    w.mario.sub = 11;
    w.mario.door = w.doors.cell + w.roof_col;
    assert(world_frame(&w, 0) == WORLD_UPPER);
    start(7, 0, 1);
    assert(w.upper && w.roof_col == -1);
    clear_grid();
    world_frame(&w, 0);
    assert(w.g.unk_53a == 1);
    hm_campaign c;
    campaign_start(&c, 1);
    c.player[0].hotel = 7;
    c.player[0].stage = 0;
    c.player[0].score = 1000;
    c.player[0].power = 2;
    w.status = WORLD_MARIO_DIED;
    w.g.player[0] = c.player[0];
    campaign_capture(&c, &w);
    assert(campaign_advance(&c, 0) == CAMPAIGN_NEXT && c.player[0].lives == 2);
    world_start_full(&w, inf, room, c.player, 0, 0, 0, extra, extra_size);
    assert(!w.upper && w.g.player[0].score == 1000 && w.g.player[0].lives == 2);
}
static void completion_jingles(void) {
    const int cases[][4] = {{1, 8, 0, 9}, {1, 9, 0, 10}, {7, 9, 1, 10},
                            {8, 9, 1, 9}, {8, 14, 1, 10}};
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        start(cases[i][0], cases[i][1], cases[i][2]);
        clear_grid();
        w.tick_count = 0; /* Check the clear boundary before bosses can reopen a door. */
        w.mario.state_group = 0;
        w.mario.state = 0;
        w.mario.floor = 4;
        w.mario.x = 44;
        w.mario.y = hm_floor_y[4] - 30;
        world_frame(&w, 0);
        assert(w.g.unk_53a == 1 && w.music_req == cases[i][3]);
    }
}
static void events(void) {
    start(4, 3, 0);
    w.g.time_left = 150;
    stage_features_tick(&w);
    assert(w.event.count == 1 && w.event.timer == 149 && w.room.actors[0].state == 1);
    w.puzzle_cell = w.doors.cell + 10;
    w.mario.door = w.puzzle_cell;
    w.mario.state_group = 4;
    w.mario.state = 0;
    stage_features_tick(&w);
    assert(w.g.unk_81a);
    stage_features_tick(&w);
    assert(w.event.completed && w.g.player[0].unk_10 &&
           w.g.time_left == *(int32_t *)(w.stage_info + 8));
    start(5, 0, 0);
    w.mario.state_group = 0;
    w.mario.floor = 4;
    w.mario.x = 44;
    w.g.time_left = 130;
    int opened = w.doors.open;
    stage_features_tick(&w);
    assert(w.event.count == 1 && w.event.timer > 0);
    int chosen = w.event.choices[0];
    assert(chosen >= 0 && (w.doors.cell[chosen] & 0xf000) == 0x3000);
    w.mario.state_group = 1;
    while (w.event.timer)
        stage_features_tick(&w);
    assert((w.doors.cell[chosen] & 0xf000) != 0x3000 && w.doors.open == opened);
    start(7, 4, 0);
    w.g.time_left = 180;
    stage_features_tick(&w);
    assert(w.event.count == 1 && w.event.timer == 149 && w.puzzle_cell);
    start(2, 3, 0);
    w.g.time_left = 159;
    int dark = 0, flash = 0;
    for (int i = 0; i < 500; i++) {
        stage_features_tick(&w);
        assert(w.event.light >= 0 && w.event.light <= 8);
        dark |= w.event.light == 8;
        flash |= w.event.light > 0 && w.event.light < 8;
    }
    assert(dark && flash && w.event.event_count);
    uint32_t rgb;
    assert(stage_light_colour(&w, 43, &rgb));
    stage_event_finish(&w);
    assert(!stage_light_colour(&w, 43, &rgb));
}
static void secrets(void) {
    start(1, 5, 0);
    assert(w.secret.mode == 3);
    w.mario.state_group = 4;
    w.mario.state = 0;
    w.mario.door = w.doors.cell + w.secret.floor * 7 + w.secret.col;
    stage_secrets_tick(&w);
    assert(w.status == WORLD_WARP && w.warp_hotel == 3 && w.warp_stage == 2);
    start(1, 0, 0);
    w.secret.mode = 2;
    w.secret.kind = 2;
    w.secret.remaining = 2;
    w.secret.x = w.mario.x + 10;
    w.secret.y = w.mario.y + 10;
    w.secret.cooldown = 0;
    int lives = w.g.player[0].lives;
    stage_secrets_tick(&w);
    assert(w.g.player[0].lives == lives + 1 && w.secret.remaining == 1);
    stage_secrets_tick(&w);
    assert(w.g.player[0].lives == lives + 1);
    w.secret.mode = 1;
    w.secret.cooldown = 0;
    hm_fx f = {0};
    f.bx0 = w.secret.x - 1;
    f.bx1 = w.secret.x + 1;
    f.by0 = w.secret.y - 1;
    f.by1 = w.secret.y + 1;
    stage_secret_fireball(&w, &f);
    stage_secrets_tick(&w);
    assert(w.g.player[0].lives == lives + 2 && w.secret.remaining == 0);
    start(1, 0, 0);
    w.secret.mode = 4;
    w.secret.kind = 4;
    w.secret.remaining = 1;
    w.secret.cooldown = 0;
    w.secret.toggle = 1;
    w.secret.floor = 4;
    w.secret.col = 2;
    w.mario.state_group = 4;
    w.mario.state = 0;
    w.mario.door = w.doors.cell + 30;
    int coins = w.g.player[0].coins;
    stage_secrets_tick(&w);
    assert(w.g.player[0].coins == coins + 1 && w.secret.remaining == 1);
    for (int i = 0; i < 5; i++)
        stage_secrets_tick(&w);
    assert(w.g.player[0].coins == coins + 2 && w.secret.remaining == 0);
}
static void lighting(void) {
    /* Full display coordinates: the original row table already includes +20. */
    for (int st=0;st<10;st++) {
        start(3,st,0);
        int expected=(20-st*2)*255/63;
        for (int active=0;active<5;active++) {
            w.mario.floor=active;
            for (int floor=0;floor<5;floor++) {
                int display_y=hm_floor_y[floor]-15+20;
                assert(stage_plane_light(&w,180,display_y)==(active==floor ? 255 : expected));
            }
            assert(stage_plane_light(&w,180,40)==255); /* HUD */
        }
        w.event.completed=1;
        assert(stage_plane_light(&w,180,113)==255);
        w.event.completed=0;w.g.player[0].unk_10=1;
        assert(stage_plane_light(&w,180,113)==255);
    }
    start(2,0,0);w.event.light=8;
    assert(stage_plane_light(&w,180,100)==6*255/63);
    assert(stage_plane_light(&w,10,100)==255);
    assert(stage_plane_light(&w,370,100)==255);
    assert(stage_plane_light(&w,180,250)==255); /* stop command at line 238 */
    puts("Lighting checks passed: all cave stages/floors, display-space offset, storm boundaries and bonus cleanup.");
}
static void simulation(void) {
    int rooms = 0;
    for (int h = 0; h <= 8; h++) {
        if (h == 6)
            continue;
        for (int st = 0; st < (h == 8 ? 15 : 10); st++)
            for (int u = 0; u <= (h >= 7); u++) {
                start(h, st, u);
                for (int f = 0; f < 6000; f++) {
                    unsigned keys = (f / 97) % 2 ? IN_LEFT : IN_RIGHT;
                    if (f % 83 < 9)
                        keys |= IN_B1;
                    if (f % 109 < 9)
                        keys |= IN_B2;
                    if (f % 173 < 20)
                        keys |= IN_UP;
                    int status = world_frame(&w, keys);
                    assert(w.room.n_actors <= 24 && w.n_spr <= 64);
                    if (status != WORLD_PLAYING) {
                        player_t saved[2];
                        memcpy(saved, w.g.player, sizeof saved);
                        world_start_full(&w, inf, room, saved, 0, 0, u, extra, extra_size);
                    }
                }
                rooms++;
            }
    }
    printf("Feature simulation passed: %d rooms x 6000 frames.\n", rooms);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    data = argv[1];
    routes_and_data();
    roof_and_retry();
    completion_jingles();
    events();
    secrets();
    lighting();
    simulation();
    free(inf);
    free(room);
    free(extra);
    puts("Stage features checks passed.");
}
