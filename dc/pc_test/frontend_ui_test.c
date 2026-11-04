/* Exercise actual menu/pause loops with scripted pads and a mocked platform.
 * This verifies state and input handling, not PVR output or hardware audio. */
#define main dreamcast_main
#include "../test_main.c"
#undef main
#include <assert.h>
static const char *data_dir;
static uint32_t pads[4096], palettes[4][256];
static int pad_frame, pad_count, sound_paused,light_updates;
static int mock_card_present,mock_load_result;
static int mock_scores_result=HM_STORE_MISSING,progress_writes,score_writes;
static hm_profile mock_progress_profile,mock_scores_profile;
static int mock_save_result=HM_STORE_NO_DEVICE;
static int backdrop_draws,trace_ui;
static int movie_stage_test,movie_pad_calls,movie_calls,movie_pauses;
static void script(const uint32_t *input, int n) {
    memset(pads, 0, sizeof pads);
    memcpy(pads, input, n * sizeof *input);
    pad_frame = 0;
    pad_count = n;
}
const char *plat_vmu_error_detail(void){return "HOST IO -1";}
int plat_vmu_list(int *ids, int capacity) {
    if(mock_card_present && capacity>0){ids[0]=1;return 1;}
    return 0;
}
int plat_vmu_load(int id, hm_profile *p) {
    (void)id;
    if(mock_card_present&&!mock_load_result)*p=mock_progress_profile;
    return mock_card_present?mock_load_result:HM_STORE_NO_DEVICE;
}
int plat_vmu_save(int id, hm_profile *p) {
    (void)id;
    (void)p;
    progress_writes++;return mock_save_result;
}
int movie_play(const char *name) {
    if(movie_stage_test){assert(!strcmp(name,"movie_bonus_2"));movie_calls++;pad_frame=1;}
    return 1;
}
int movie_play_prepared(const char *name,void (*prepared)(void *),void *ctx) { if(prepared)prepared(ctx);return movie_play(name); }
int plat_vmu_load_scores(int id,hm_profile *p){(void)id;if(mock_card_present&&!mock_scores_result)*p=mock_scores_profile;return mock_card_present?mock_scores_result:HM_STORE_NO_DEVICE;}
int plat_vmu_save_scores(int id,hm_profile *p){(void)id;(void)p;score_writes++;return mock_save_result;}
void plat_init(void) {}
void plat_sound_init(void) {}
void plat_sound_poll(void) {}
void plat_sound_pause(int paused) { sound_paused = paused; if(movie_stage_test&&paused)movie_pauses++; }
uint32_t plat_pad(void) {
    if(movie_stage_test && ++movie_pad_calls==2)gW.g.unk_81a=1;
    assert(pad_frame < pad_count);
    return pads[pad_frame];
}
int plat_soft_reset(void) { return plat_pad() == (PAD_B1 | PAD_B2 | PAD_X | PAD_Y | PAD_START); }
void plat_frame_begin(void) {}
void plat_frame_end(void) {
    pad_frame++;
    assert(pad_frame <= pad_count);
}
void plat_wait_ready(void) {}
void plat_draw_picture(const hm_tex *t) {
    if(t==&gBackdrop){assert(t->parts && t->bank==2);backdrop_draws++;}
    if(trace_ui && t==&gBackdrop)printf("PICTURE continue_%d\n",gBackdropHotel);
    if(trace_ui && t==&gSaveBackdrop)printf("PICTURE continue_%d\n",gCampaign.player[gCampaign.current].hotel);
}
void plat_draw_picture_lit(const hm_tex *t, int y) {
    (void)t;
    (void)y;
}
void plat_draw_tile_lit(const hm_tex *t, int sx, int sy, int w, int h, int x, int y) {
    (void)t;
    (void)sx;
    (void)sy;
    (void)w;
    (void)h;
    (void)x;
    (void)y;
}
void plat_draw_picture_offset(const hm_tex *t, int y) {
    (void)t;
    (void)y;
}
void plat_draw_overlay(const hm_tex *t, unsigned a) {
    (void)t;
    (void)a;
}
void plat_draw_panel(int x, int y, int w, int h, uint32_t c) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)c;
}
void plat_set_light_mask(int (*fn)(void *, int, int), void *ctx) {
    light_updates++;
    (void)fn;
    (void)ctx;
}
void plat_draw_frame_scaled(const hm_sprite_set *s, int f, int x, int y, float z, int scale) {
    (void)s;
    (void)f;
    (void)y;
    (void)z;
    (void)scale;
    assert(x >= 0 && x < 384);
    if(trace_ui && (s==&gFont || s==&gHud))
        printf("SPRITE %s %d %d %d %d\n",s==&gFont?"menu_font":"hud",f,x,y,scale);
}
void plat_draw_frame(const hm_sprite_set *s, int f, int x, int y, float z) {
    plat_draw_frame_scaled(s, f, x, y, z, 1);
}
void plat_draw_frame_masked(const hm_sprite_set *s, int f, int x, int y, float z) {
    plat_draw_frame(s, f, x, y, z);
}
void plat_draw_tile(const hm_tex *t, int sx, int sy, int w, int h, int x, int y) {
    (void)t;
    (void)sx;
    (void)sy;
    (void)w;
    (void)h;
    (void)x;
    (void)y;
}

void plat_set_colour(int b, int i, uint32_t c) { palettes[b][i] = 0xff000000 | c; }
void plat_set_transparent(int b, int i) { palettes[b][i] = 0; }
void plat_save_palette(int b, uint32_t out[256]) { memcpy(out, palettes[b], 1024); }
void plat_restore_palette(int b, const uint32_t in[256]) { memcpy(palettes[b], in, 1024); }
void *plat_load_file(const char *name, uint32_t *size) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", data_dir, name);
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    void *d = malloc(n);
    assert(d && fread(d, 1, n, f) == (size_t)n);
    fclose(f);
    if (size)
        *size = n;
    return d;
}
int plat_pal_from_mem(const uint8_t *d, int bank, int first, uint32_t *out, int max) {
    if (!d || memcmp(d, "HMPL", 4))
        return 0;
    int n = d[4] | d[5] << 8;
    for (int i = 0; i < n; i++) {
        uint32_t c = d[8 + i * 3] << 16 | d[9 + i * 3] << 8 | d[10 + i * 3];
        if (out && i < max)
            out[i] = c;
        if (bank >= 0)
            plat_set_colour(bank, first + i, c);
    }
    return n;
}
int plat_load_pal(const char *n, int b, int first, uint32_t *out, int max) {
    uint8_t *d = plat_load_file(n, NULL);
    int ok = plat_pal_from_mem(d, b, first, out, max);
    free(d);
    return ok;
}
int plat_tex_from_mem(const uint8_t *d, int b, hm_tex *t) {
    if (!d || memcmp(d, "HMT2", 4))
        return 0;
    memset(t, 0, sizeof *t);
    t->parts = d[4];
    t->bank = b;
    return 1;
}
int plat_load_tex(const char *n, int b, hm_tex *t) {
    uint8_t *d = plat_load_file(n, NULL);
    int ok = plat_tex_from_mem(d, b, t);
    free(d);
    return ok;
}
void plat_free_tex(hm_tex *t) { memset(t, 0, sizeof *t); }
int plat_load_sprites(const char *t, const char *f, hm_sprite_set *s) {
    (void)t;
    (void)f;
    (void)s;
    return 0;
}
void *plat_thread_start(void *(*fn)(void *), void *arg) {
    fn(arg);
    return NULL;
}
void plat_thread_join(void *t) { (void)t; }
static world before;
static void fixture(void) {
    hm_campaign c;
    campaign_start(&c, 1);
    c.player[0].hotel = 7;
    c.player[0].stage = 0;
    c.player[0].progress = 8;
    c.player[0].unk_18 = 14;
    c.player[0].score = 12345;
    c.player[0].power = 2;
    uint8_t *inf = plat_load_file("stage_7_01.inf", NULL),
            *room = plat_load_file("room_7_01.room", NULL);
    uint32_t n;
    uint8_t *extra = plat_load_file("stage_7_01.ext", &n);
    world_start_full(&gW, inf, room, c.player, 0, 0, 0, extra, n);
    gCampaign = c;
    gW.extra = NULL;
    gW.extra_size = 0;
    free(inf);
    free(room);
    free(extra);
    before = gW;
    for (int b = 0; b < 4; b++)
        for (int i = 0; i < 256; i++)
            palettes[b][i] = b * 1000 + i;
}
static void check_bonus_movie_input(void) {
    campaign_start(&gCampaign,1);gCampaign.player[0].hotel=2;
    gInf=plat_load_file("stage_2_01.inf",NULL);gRoom=plat_load_file("room_2_01.room",NULL);
    gExtra=plat_load_file("stage_2_01.ext",&gExtraSize);
    assert(gInf&&gRoom&&gExtra);
    const uint32_t input[]={0,PAD_START,PAD_START,0,PAD_B1|PAD_B2|PAD_X|PAD_Y|PAD_START};
    script(input,5);movie_stage_test=1;movie_pad_calls=movie_calls=movie_pauses=0;
    assert(stage()==-1);assert(movie_calls==1&&!movie_pauses);
    movie_stage_test=0;free(gInf);free(gRoom);free(gExtra);gInf=gRoom=gExtra=NULL;gExtraSize=0;
}
static void check_morton_start(void) {
    uint32_t size;
    uint8_t *inf=plat_load_file("stage_1_10.inf",NULL),
            *room=plat_load_file("room_1_10.room",NULL),
            *extra=plat_load_file("stage_1_10.ext",&size);
    assert(inf && room && extra);
    static world probe;
    for(int close=0;close<2;close++) {
        player_t players[2]={{0}};
        players[0].hotel=1;players[0].stage=9;players[0].lives=5;
        world_start_full(&probe,inf,room,players,0,0,0,extra,size);
        assert(probe.doors.open==12 && probe.doors.count==13);
        for(int f=0;f<220;f++) {
            unsigned input=(probe.mario.state_group==1 || probe.mario.state_group==2)?IN_DOWN:IN_LEFT;
            if(close && f>=135 && f<151)input|=IN_B2;
            world_frame(&probe,input);
            if(probe.g.unk_53a==2)break;
        }
        if(close) {
            assert(probe.g.unk_53a==0 && !probe.mario.fell && !stage_loss_message(&probe));
        } else {
            assert(probe.end_reason==WORLD_ALL_OPEN && !probe.mario.fell);
            assert(!strcmp(stage_loss_message(&probe),"ALL DOORS OPEN"));
        }
    }
    probe.g.unk_53a=2;probe.end_reason=WORLD_TIME_UP;
    assert(!strcmp(stage_loss_message(&probe),"TIME UP"));
    probe.end_reason=WORLD_MARIO_DIED;assert(!stage_loss_message(&probe));
    free(inf);free(room);free(extra);
    puts("Morton start passed: left-only loses to all doors open; closing a door avoids it; correct loss messages.");
}

static void demo_movement_test(void) {
    uint32_t size;
    uint8_t *inf=plat_load_file("stage_1_01.inf",&size);
    uint8_t *room=plat_load_file("room_1_01.room",&size);
    static world w;
    world_start(&w,inf,room,1,0,0);
    int walked=0,rode=0,closed=0,start_open=w.doors.open;
    for(unsigned frame=0;frame<1800;frame++) {
        world_frame(&w,demo_input(&w,frame));
        if(w.mario.state_group==0 && (w.mario.state==0 || w.mario.state==1))walked=1;
        if(w.mario.floor<4)rode=1;
        if(w.doors.open<start_open)closed=1;
        if(w.status!=WORLD_PLAYING)break;
    }
    assert(walked && closed && rode);
    free(inf);free(room);
    puts("Demo simulation passed: leaves start lift, walks, closes doors and changes floor.");
}

int main(int argc, char **argv) {
    assert(argc == 2);
    data_dir = argv[1];
    demo_movement_test();
    const uint32_t arrival_input[181]={0};script(arrival_input,181);
    assert(play_hotel_entry(1)==1 && pad_frame==181);
    /* Independent boot loads must not overwrite the other file's fields. */
    profile_init(&mock_progress_profile);profile_init(&mock_scores_profile);
    mock_progress_profile.slots[0]=(hm_save_slot){1,"SAV",3,2};
    profile_add_score(&mock_progress_profile,"OLD",1111);
    profile_add_score(&mock_scores_profile,"TOP",765432);
    mock_scores_profile.slots[0]=(hm_save_slot){1,"BAD",8,14};
    mock_card_present=1;mock_load_result=mock_scores_result=0;profile_startup();
    assert(gProfile.scores[0].score==765432&&gProfile.slots[0].hotel==3);
    mock_load_result=HM_STORE_CORRUPT;profile_startup();
    assert(gProfileReadError==HM_STORE_CORRUPT&&!gProfile.slots[0].valid&&gProfile.scores[0].score==765432);
    mock_load_result=0;mock_scores_result=HM_STORE_CORRUPT;profile_startup();
    mock_scores_result=0;profile_startup();gProfileDirty=3;mock_save_result=0;
    const uint32_t commit_input[]={0,0};script(commit_input,2);
    int pw=progress_writes,sw=score_writes;
    assert(!profile_commit()&&gProfileDirty==2&&progress_writes==pw+1&&score_writes==sw);
    assert(!profile_commit_part(1)&&!gProfileDirty&&progress_writes==pw+1&&score_writes==sw+1);
    mock_scores_result=HM_STORE_MISSING;mock_save_result=HM_STORE_NO_DEVICE;mock_card_present=0;
    check_bonus_movie_input();
    check_morton_start();
    mock_card_present=1;mock_load_result=HM_STORE_CORRUPT;
    profile_startup();assert(gVmuId==1 && gProfileReadError==HM_STORE_CORRUPT);
    mock_save_result=0;gProfileDirty=1;
    const uint32_t acknowledge_error[]={0,PAD_B1};script(acknowledge_error,2);
    assert(!profile_commit() && !gProfileDirty);
    mock_load_result=HM_STORE_IO;
    profile_startup();assert(gVmuId==1 && gProfileReadError==HM_STORE_IO);
    mock_card_present=0;mock_save_result=HM_STORE_NO_DEVICE;gProfileDirty=0;profile_startup();
    profile_init(&gProfile);
    for (int i = 0; i < 49; i++) {
        gFont.slot[i] = i;
        gFont.frames[i].w = 7;
    }
    const uint32_t backdrop_input[]={0,0};script(backdrop_input,2);
    gCampaign.current=0;gCampaign.player[0].hotel=1;
    int draws=backdrop_draws;
    profile_page("SAVING");assert(backdrop_draws==draws+1 && gBackdropHotel==1);
    draw_completion();assert(backdrop_draws==draws+2);
    assert(prepare_backdrop(7) && gBackdropHotel==7);
    for(int i=0;i<128;i++)plat_set_transparent(2,i);
    assert(prepare_backdrop(7) && palettes[2][0]==(0xff000000|gBackdropPal[0]));
    const uint32_t resume[] = {PAD_START, 0, PAD_START};
    fixture();
    script(resume, 3);
    assert(pause_menu() == 1 && !sound_paused && memcmp(&gW, &before, sizeof gW) == 0);
    uint32_t saved[2][256];
    plat_save_palette(0, saved[0]);
    plat_save_palette(1, saved[1]);
    const uint32_t map_back[] = {PAD_START, 0, PAD_DOWN, 0, PAD_B1, 0, PAD_START};
    script(map_back, 7);
    assert(pause_menu() == 1 && !sound_paused);
    assert(memcmp(&gW, &before, sizeof gW) == 0 && !memcmp(saved, palettes, sizeof saved));
    const uint32_t travel[] = {PAD_START, 0, PAD_DOWN, 0, PAD_B1, 0, PAD_RIGHT, 0, PAD_B1};
    script(travel, 9);
    assert(pause_menu() == 2 && !sound_paused);
    assert(gCampaign.current == 0 && gCampaign.player[0].hotel == 8 &&
           gCampaign.player[0].stage == 14);
    assert(gCampaign.player[0].score == 12345 && gCampaign.player[0].lives == 3 &&
           gCampaign.player[0].power == 2);
    assert(memcmp(&gW, &before, sizeof gW) == 0);
    const uint32_t reset[] = {PAD_START, 0, PAD_B1 | PAD_B2 | PAD_X | PAD_Y | PAD_START};
    script(reset, 3);
    assert(pause_menu() == 0 && !sound_paused);
    const uint32_t normal[] = {0, PAD_B1};
    script(normal, 2);
    assert(choose_players() == 1);
    /* Four rows only: Up from the first row reaches original DEMO. */
    const uint32_t original_demo[]={0,PAD_UP,0,PAD_B1};
    script(original_demo,4);assert(choose_players()==4);
    const uint32_t cheat_buttons[]={PAD_L,PAD_R,PAD_L|PAD_R};
    const unsigned cheat_masks[]={HM_CHEAT_INVINCIBLE,HM_CHEAT_TIME,HM_CHEAT_INVINCIBLE|HM_CHEAT_TIME};
    for(int i=0;i<3;i++){
        uint32_t activate[]={cheat_buttons[i],cheat_buttons[i]|PAD_B1};
        script(activate,2);assert(choose_players()==1 && gCheats==cheat_masks[i]);
        const uint32_t new_game[]={0,PAD_B1};script(new_game,2);
        assert(profile_begin_game(1) && gCheats==cheat_masks[i]);
    }
    profile_init(&gProfile);gProfile.slots[0]=(hm_save_slot){1,"END",8,14};
    for(int i=0;i<3;i++){
        uint32_t activate[]={cheat_buttons[i],cheat_buttons[i]|PAD_B1};script(activate,2);
        assert(choose_players()==1);
        const uint32_t restore_cheat[]={0,PAD_DOWN,0,PAD_B1,0,PAD_B1};script(restore_cheat,6);
        assert(profile_begin_game(1) && gCheats==cheat_masks[i]);
        assert(gCampaign.player[0].hotel==8 && gCampaign.player[0].stage==14);
    }
    const uint32_t two_cheat[]={PAD_L|PAD_R,PAD_L|PAD_R|PAD_DOWN,PAD_L|PAD_R,PAD_L|PAD_R|PAD_B1};
    script(two_cheat,4);assert(choose_players()==2 && !gCheats);
    fixture();
    uint32_t light_size;
    uint8_t *light_extra = plat_load_file("stage_3_01.ext", &light_size);
    gW.extra = light_extra + 12;
    gW.extra_size = light_size - 12;
    gW.g.player[0].hotel = 3;
    gW.g.player[0].stage = 0;
    gW.mario.floor = 0;
    gLightKey = -1;
    update_light_map();
    assert(plane_light(NULL, 180, 77) == 255);
    assert(plane_light(NULL, 180, 113) == 20 * 255 / 63);
    int before_lights=light_updates;draw_world();assert(light_updates==before_lights+1);
    draw_world();assert(light_updates==before_lights+1);
    gW.mario.floor=1;draw_world();assert(light_updates==before_lights+2);
    free(light_extra);
    gW.extra = NULL;
    gW.extra_size = 0;
    fixture();
    gCampaign.player[0].lives = 0;
    const uint32_t yes[] = {0, PAD_B1};
    script(yes, 2);
    assert(continue_game(0) == 1);
    const uint32_t no[] = {0, PAD_DOWN, 0, PAD_B1};
    script(no, 4);
    assert(continue_game(0) == 0);
    script(reset, 3);
    assert(continue_game(0) == -1);
    gDemoRunning = 0;
    profile_init(&gProfile);
    const uint32_t new_game[] = {0, PAD_B1};
    script(new_game, 2);
    assert(profile_begin_game(1) && gCampaign.player[0].lives == 5);
    hm_campaign checkpoint = gCampaign;
    checkpoint.player[0].hotel = checkpoint.player[0].progress = 3;
    checkpoint.player[0].stage = checkpoint.player[0].unk_18 = 2;
    checkpoint.player[0].score = 12345;
    checkpoint.player[0].coins = 17;
    checkpoint.player[0].power = 2;
    assert(!profile_capture(&gProfile, 0, "MIO", &checkpoint));
    const uint32_t restore[] = {0, PAD_DOWN, 0, PAD_B1, 0, PAD_B1};
    script(restore, 6);
    assert(profile_begin_game(1));
    assert(gCampaign.player[0].hotel == 3 && gCampaign.player[0].stage == 2 &&
           gCampaign.player[0].lives == 5);
    assert(!gCampaign.player[0].score && !gCampaign.player[0].coins && !gCampaign.player[0].power);
    const uint32_t two_players[] = {0, PAD_B1, 0, PAD_DOWN, 0, PAD_B1, 0, PAD_B1};
    script(two_players, 8);
    assert(profile_begin_game(2));
    assert(gCampaign.player[0].hotel == 1 && gCampaign.player[1].hotel == 3 &&
           gCampaign.players == 2);
    gCampaign = checkpoint;
    profile_init(&gProfile);
    gVmuId = -1;
    const uint32_t save[] = {0, PAD_B1, 0, PAD_B1, 0, PAD_B1};
    script(save, 6);
    assert(profile_slots(0, 0) == 1 && gProfile.slots[0].hotel == 3 &&
           gProfile.slots[0].stage == 2);
    assert(gCampaign.player[0].score == 12345 && gCampaign.player[0].power == 2);
    const uint32_t card_save[] = {0, PAD_B2};
    script(card_save, 2);
    assert(title() == 2);
    const uint32_t card_map[] = {0, PAD_Y};
    script(card_map, 2);
    assert(title() == 3);
    const uint32_t highscore[] = {0, PAD_B1, 0, PAD_B1, 0, PAD_B2};
    script(highscore, 6);
    profile_submit_score(0);
    assert(gProfile.scores[0].score == 12345);
    hm_profile held = gProfile;
    gDemoRunning = 1;
    profile_submit_score(0);
    assert(!memcmp(&held, &gProfile, sizeof held));
    gDemoRunning = 0;
    static const uint32_t idle[1800] = {0};
    script(idle, 1800);
    assert(choose_players() == 4);
    assert(!memcmp(&held, &gProfile, sizeof held));
    strcpy(gStages[0], "1_01");
    gNStages = 1;
    uint32_t demo_pads[300] = {0};
    demo_pads[250] = PAD_B2;
    script(demo_pads, 300);
    hm_campaign held_campaign = gCampaign;
    player_t held_progress[2];
    memcpy(held_progress, gSessionProgress, sizeof held_progress);
    run_demo();
    assert(pad_frame >= 120 && !gDemoRunning);
    assert(!memcmp(&held, &gProfile, sizeof held) &&
           !memcmp(&held_campaign, &gCampaign, sizeof gCampaign));
    assert(!memcmp(held_progress, gSessionProgress, sizeof held_progress));
    puts("UI save flows passed: original NEW/RESTORE, separate two-player checkpoints, five-slot "
         "SAVE/EXIT, stage-card actions, no-VMU feedback, attract timer and demo isolation.");
    puts("UI checks passed: pause freezes world, resume/map/reset input, palette restoration, "
         "same-turn travel, original four-row title menu.");
    if(getenv("HM_UI_TRACE")) {
        hm_sprite_set *sets[]={&gFont,&gHud};const char *names[]={"menu_font.frm","hud.frm"};
        for(int i=0;i<2;i++) {
            uint8_t *d=plat_load_file(names[i],NULL);assert(d);
            int nf=d[4]|d[5]<<8,ns=d[6]|d[7]<<8;
            sets[i]->n_frames=nf;sets[i]->n_slots=ns;
            memcpy(sets[i]->frames,d+8,nf*sizeof(hm_frame));
            memcpy(sets[i]->slot,d+8+nf*sizeof(hm_frame),ns*2);free(d);
        }
        gCampaign.current=0;gCampaign.player[0].hotel=1;
        gCampaign.player[0].score=18720;gCampaign.bonus_time=32;gCampaign.bonus_rate=10;
        gCampaign.special_paid=0;
        gW.g.cur_player=0;gW.g.player[0]=gCampaign.player[0];
        gW.g.player[0].stage=2;gW.g.player[0].lives=6;gW.g.player[0].coins=0;
        gW.g.time_left=125;gW.shake=0;
        script(backdrop_input,2);trace_ui=1;
        puts("PAGE completion");draw_completion();
        puts("PAGE save");
        gDemoRunning=0;gProfileReadError=0;
        profile_init(&gProfile);gProfile.slots[0].valid=1;
        strcpy(gProfile.slots[0].name,"MIO");
        gProfile.slots[0].hotel=1;gProfile.slots[0].stage=2;
        const uint32_t leave_slots[]={0,PAD_B2};script(leave_slots,2);
        profile_slots(0,0);
        trace_ui=0;
    }

}
