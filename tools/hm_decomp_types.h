// Hotel Mario - type information fed to the decompiler (one declaration per line).
// Edit this file to name structure fields, then re-run tools/decompile.py.









struct player_t { int4 score; int2 hotel; int2 stage; int2 lives; int2 coins; int2 unk_0c; int2 power; int2 unk_10; int1 got_bonus; int1 unk_13; int1 unk_14; int1 unk_15; int2 progress; int2 unk_18; int1 unk_1a; uint1 flags; };
struct obj_t { code *handler; int1 unk_04[0x18]; int2 unk_1c; int2 unk_1e; int2 active; int1 unk_22[0x2a]; };
struct mario_t { code *handler; code *slot[57]; int1 unk_0e8[0x1c]; uint2 *door; int2 step; int2 fcount; int2 door_col; int2 x; int2 y; int2 floor; int2 old_x; int2 old_y; int2 old_floor; int2 state_group; int2 saved_group; int2 state; int2 sub; int2 prev_state; int2 anim; int2 anim_speed; int2 anim_count; int2 lift_used; int2 lift_moved; int2 jump_move; int2 unk_130; int2 unk_132; int2 jump_held; int2 jump_hang; int2 jump_skip; int2 jump_drift; int2 drift_toggle; int2 star_timer; int2 invincible; int1 unk_142[4]; int2 unk_146; int2 fire; int2 fire_delay; int2 throw_timer; int2 hurt_timer; int2 item_flash; int2 cooldown[5]; int2 both_buttons; int2 down_count; int2 ready; int2 bumped; int2 fell; int2 hit; };
struct actor_t { code *handler; int2 type; int2 floor; int2 col; int2 x; int2 y; int2 cur_floor; int2 drawn_x1; int2 drawn_x0; int2 drawn_y1; int2 drawn_y0; int2 unk_018; int2 box_x0; int2 box_x1; int2 box_y0; int2 box_y1; int2 cmd; int2 dir; int2 state; int2 step; int2 unk_02a; int2 unk_02c; int2 unk_02e; int2 appear_count; int2 timer; int2 delay; int2 unk_036; int2 start_mode; int2 frame; int2 unk_03c; int2 hit_x0; int2 hit_x1; int2 hit_y0; int2 hit_y1; int2 speed; int2 unk_048; int2 unk_04a; int2 unk_04c; int2 unk_04e; code *frames[64]; code *handlers[25]; code *walk_steps[24]; code *steps_b[16]; int4 x_min; int4 x_max; int1 unk_25c[0x18]; };
struct room_t { int2 n_actors; int2 handler_ids[25]; actor_t actors[24]; };
struct gds { int4 video_path; int4 audio_path; int4 sound_map; int2 unk_00c; int2 unk_00e; int4 afx[17]; int2 unk_054; int4 sound_map_no; int4 fct_a; int4 fct_b; int4 lct_a; int4 lct_b; int2 video_mode; int4 screen_a1; int4 screen_a0; int4 screen_b0; int4 unk_078; int4 draw_a1; int4 draw_a0; int4 draw_screen; int4 unk_088; int4 clean_bg; int4 sprite_gfx; int4 unk_094; int4 clut_a; int4 clut_b; uint1 loading_clut[0x184]; int4 bg; int4 vram_a; int4 vram_b; int4 intro_module; int4 unk_234; int4 unk_238; int4 font; int4 unk_240; uint1 *stage_info; room_t *room; int4 unk_24c; mario_t *mario; obj_t kill_stars[5]; int2 n_fireballs; obj_t fireballs[2]; int2 frame; int2 unk_46c; int2 unk_46e; uint4 pad_buttons; uint4 pad_prev; int4 pad_x; int4 pad_x_last; int4 pad_y; int4 pad_y_last; int4 held_b1; int4 held_b2; int2 unk_490; int2 unk_492; int2 draw_field; int2 unk_496; int2 unk_498; int2 unk_49a; int2 floor_y[5]; int2 door_x[7]; int1 unk_4b4[0x74]; int2 doors_count; int2 doors_open; int4 time_left; int2 unk_530; int2 unk_532; int2 unk_534; int2 unk_536; int2 unk_538; int2 unk_53a; int2 item_hit; int2 item_kind; int4 item_slot; int4 item_slot2; int4 change_handler_a; int4 change_handler_b; int1 unk_550[0x46]; int2 cur_player; int2 two_players; int2 unk_59a; int2 unk_59c; int2 unk_59e; int2 unk_5a0; uint1 cycle_clut[0x200]; player_t player[2]; int1 puzzle_vars[0x40]; int2 unk_81a; int4 unk_81c; int4 unk_820; int2 unk_824; int4 unk_826; int4 unk_82a; int2 unk_82e; int2 unk_830; int2 pause_ok; int4 unk_834; code *svc_cd_anim; code *svc_cd_clut; code *svc_cd_init; code *svc_cd_mul_bkg; code *svc_cd_play; code *svc_cd_smap; code *svc_cd_smq; code *svc_cd_wait; code *svc_ds_abort; code *svc_ds_close; code *svc_ds_dispatch; code *svc_ds_dispatch_execute; code *svc_ds_link_map_module; code *svc_ds_open; int4 svc_unk_870; code *svc_check_doors; code *svc_color_cycle; code *svc_demoHelp_pt_coord; code *svc_elev_switch; code *svc_fade; code *svc_init_doors; code *svc_init_floor_map; code *svc_load_all; code *svc_player_clut_change; code *svc_power_check; code *svc_power_cycle; code *svc_playAFX; code *svc_puzzle; code *svc_scoring; code *svc_set_up_clut; code *svc_sfb_coll_det; code *svc_show_high_score; code *svc_show_restore; code *svc_unfade; int4 svc_myrtf; int4 svc_DsPcb; int2 unk_8c8; };

//@global gds* gdsP

int4 after_the_game(gds *g,int4 p2);
void bonus(gds *g,int4 p2,int2 p3,int2 p4);
void check_doors(gds *g,int4 p2);
void color_cycle(gds *g,int4 p2);
void display_bonus_data(gds *g,int4 p2,int4 p3);
void do_switch(gds *g,int4 p2);
void eegg_coll_det(gds *g,int4 p2);
int4 elev_switch(gds *g,int4 p2);
int4 exit_check(gds *g,int2 p2);
int4 get_the_fxxxxxx_bonus(gds *g);
void init_doors(gds *g,int4 p2);
int4 init_the_game(gds *g,int4 p2);
int4 jump_coll_det(gds *g,uint2 p2,int2 p3);
void luigi_hat_spit(gds *g,int4 p2);
void mario(gds *g);
void player_clut_change(gds *g,uint4 p2);
void power_cycle(gds *g,int4 p2);
void puzzle(gds *g,int4 p2);
int4 puzzle_2(gds *g,int4 p2);
int4 puzzle_3(gds *g,int2 p2);
int4 puzzle_4(gds *g,int2 p2);
int4 puzzle_5(gds *g,int4 p2);
int4 puzzle_7(gds *g,int4 p2);
int4 puzzle_8(gds *g,int4 p2);
int4 room_coll_det(gds *g,uint2 p2,uint2 p3,int2 p4);
int4 sfb_eegg_det(gds *g,int4 p2,uint2 p3,int2 p4);
int4 sparkle_color_cycle(gds *g,int4 p2);
int4 switch_needed(gds *g);
void the_game(gds *g);
int4 tooNearMario(gds *g,int4 p2,int2 p3,int2 p4);
void movi_ru_fo(int4 p1,gds *g);
void movi_ru_ba(int4 p1,gds *g);
void movi_sh_ru_fo(int4 p1,gds *g);
void movi_sh_ru_ba(int4 p1,gds *g);
void movi_tu_fo(int4 p1,gds *g);
void movi_tu_ba(int4 p1,gds *g);
void movi_st_fo(int4 p1,gds *g);
void movi_st_ba(int4 p1,gds *g);
void movi_sh_st_fo(int4 p1,gds *g);
void movi_sh_st_ba(int4 p1,gds *g);
void movi_sk_fo(int4 p1,gds *g);
void movi_sk_ba(int4 p1,gds *g);
void movi_du_fo(int4 p1,gds *g);
void movi_du_ba(int4 p1,gds *g);
void movi_stan(int4 p1,gds *g);
int4 in_u_e_wait(int4 p1,gds *g);
void in_u_e_go_in(int4 p1,gds *g);
void in_u_e_come(int4 p1,gds *g);
void in_u_e_go_up(int4 p1,gds *g);
int4 in_d_e_wait(int4 p1,gds *g);
void in_d_e_go_in(int4 p1,gds *g);
void in_d_e_come(int4 p1,gds *g);
void in_d_e_go_do(int4 p1,gds *g);
void jump_crou(int4 p1,gds *g);
void jump_leap(int4 p1,gds *g);
void jump_rise(int4 p1,gds *g);
void jump_ar_1(int4 p1,gds *g);
void jump_ar_2(int4 p1,gds *g);
void jump_fall(int4 p1,gds *g);
void jump_touc(int4 p1,gds *g);
void jump_land(int4 p1,gds *g);
void jump_boun(int4 p1,gds *g);
int4 in_do_wait(int4 p1,gds *g);
void in_do_go_in(int4 p1,gds *g);
void in_do_come(int4 p1,gds *g);
void winn_danc(int4 p1,gds *g);
void winn_prep(int4 p1,gds *g);
int4 winn_spin(int4 p1,gds *g);
void dyin_star(int4 p1,gds *g);
void dyin_eyes(int4 p1,gds *g);
int4 dyin_fa_1(int4 p1,gds *g);
int4 dyin_fa_2(int4 p1,gds *g);
int4 dyin_fa_3(int4 p1,gds *g);
void dead_init(int4 p1,gds *g);
int4 kill_star(int4 p1,gds *g);
int4 sm_f_ball(int4 p1,gds *g);
void mario_coll_det(int4 p1,gds *g);
void sfb_coll_det(int4 p1,gds *g);
int4 sparkle_anim(int4 p1,gds *g);
