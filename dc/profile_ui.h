/* Original-style five-checkpoint NEW/RESTORE and SAVE/EXIT flows.
 * No autosave slot, options menu or mid-room/campaign-state snapshots. */
#ifndef HM_PROFILE_UI_H
#define HM_PROFILE_UI_H
static const char *store_error(int code) {
    switch (code) {
    case HM_STORE_NO_DEVICE:
        return "NO VMU CONNECTED";
    case HM_STORE_FULL:
        return "VMU IS FULL";
    case HM_STORE_CORRUPT:
        return "SAVE DATA DAMAGED";
    case HM_STORE_FOREIGN:
        return "SAVE FILE IS NOT HOTEL MARIO";
    case HM_STORE_CHANGED:
        return "RESTART TO LOAD THIS VMU";
    default:
        return "VMU READ OR WRITE FAILED";
    }
}
static void profile_page(const char *title) {
    int backdrop=prepare_backdrop(gCampaign.player[gCampaign.current].hotel);
    original_palette(gFontPal,3);
    plat_frame_begin();
    if(backdrop)plat_draw_picture(&gBackdrop);
    original_text(title, 40, 38, 2);
}
static int profile_message(const char *line, const char *detail, int confirm) {
    uint32_t prev = plat_pad();
    original_palette(gFontPal, 3);
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset())
            return 0;
        profile_page(confirm ? "CONFIRM" : "VMU");
        original_text(line, 24, 110, 1);
        if (detail) {
            char lines[96];snprintf(lines,sizeof lines,"%s",detail);
            char *part=lines;int y=140;
            while(part) {
                char *next=strchr(part,'\n');
                if(next)*next++=0;
                original_text(part,24,y,1);y+=20;part=next;
            }
        }
        original_text(confirm ? "A YES   B CANCEL" : "A OR B TO CONTINUE", 48, 230, 1);
        plat_frame_end();
        if (pressed & PAD_B2)
            return 0;
        if (pressed & (PAD_B1 | PAD_START))
            return 1;
        prev = p;
    }
}
static void profile_startup(void) {
    profile_init(&gProfile);gScoreVmuId=-1;
    int ids[24],n=plat_vmu_list(ids,24);
    gVmuId=n?ids[0]:-1;gProfileReadError=HM_STORE_NO_DEVICE;
    for(int i=0;i<n;i++) {
        hm_profile candidate;int status=plat_vmu_load(ids[i],&candidate);
        if(!status){memcpy(gProfile.slots,candidate.slots,sizeof gProfile.slots);gProfile.sequence=candidate.sequence;gVmuId=ids[i];gProfileReadError=0;break;}
        if(i==0)gProfileReadError=status==HM_STORE_MISSING?0:status;
    }
    for(int i=0;i<n;i++) {
        hm_profile candidate;
        if(!plat_vmu_load_scores(ids[i],&candidate)) {
            memcpy(gProfile.scores,candidate.scores,sizeof gProfile.scores);
            gScoreVmuId=ids[i];break;
        }
    }
    if(gScoreVmuId<0 && n)gScoreVmuId=ids[0];
    snprintf(gSaveStatus,sizeof gSaveStatus,"%s",gVmuId>=0?"VMU READY":"NO VMU CONNECTED");
}
static int profile_commit_part(int scores) {
    int *id=scores?&gScoreVmuId:&gVmuId;
    int bit=scores?2:1;
    if (gDemoRunning)
        return HM_STORE_MISSING;
    if (!(gProfileDirty&bit))
        return 0;
    int status=HM_STORE_NO_DEVICE;
    if(*id<0){int ids[24],n=plat_vmu_list(ids,24);if(n)*id=ids[0];}
    if (*id >= 0) {
        profile_page("SAVING");
        char target[16];snprintf(target,sizeof target,"VMU %c%d",'A'+*id/6,*id%6);
        original_text(target,140,110,1);
        original_text("DO NOT REMOVE THE VMU", 44, 140, 1);
        plat_frame_end();
        status = scores?plat_vmu_save_scores(*id,&gProfile):plat_vmu_save(*id,&gProfile);
    }
    if (!status) {
        gProfileDirty &= ~bit;
        gProfileReadError = 0;
        snprintf(gSaveStatus, sizeof gSaveStatus, "SAVED ON VMU %c%d", 'A' + *id / 6,
                 *id % 6);
    } else {
        snprintf(gSaveStatus, sizeof gSaveStatus, "%s", store_error(status));
        char detail[96];
        if(status==HM_STORE_IO)snprintf(detail,sizeof detail,"%s\nDATA RETAINED IN MEMORY",plat_vmu_error_detail());
        else snprintf(detail,sizeof detail,"DATA RETAINED IN MEMORY");
        profile_message(store_error(status),detail,0);
    }
    return status;
}
static int profile_commit(void){return profile_commit_part(0);}
static int profile_initials(char name[4]) {
    int at = 0;
    uint32_t prev = plat_pad();
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset() || (pressed & PAD_B2))
            return 0;
        if (pressed & PAD_LEFT)
            at = (at + 2) % 3;
        if (pressed & PAD_RIGHT)
            at = (at + 1) % 3;
        const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
        const char *letter = strchr(alphabet, name[at]);
        int index = letter ? (int)(letter - alphabet) : 0;
        if (pressed & PAD_UP)
            name[at] = alphabet[(index + 1) % 37];
        if (pressed & PAD_DOWN)
            name[at] = alphabet[(index + 36) % 37];
        profile_page("ENTER NAME");
        for (int i = 0; i < 3; i++) {
            char text[2] = {name[i], 0};
            original_text(text, 146 + i * 30, 120, 2);
        }
        original_text("p", 146 + at * 30, 148, 1);
        original_text("D PAD EDIT   A ACCEPT   B CANCEL", 24, 230, 1);
        plat_frame_end();
        if (pressed & (PAD_B1 | PAD_START)) {
            name[3] = 0;
            return 1;
        }
        prev = p;
    }
}
static void profile_scores(void) {
    uint32_t prev = plat_pad();
    original_palette(gFontPal, 3);
    original_palette(gMenuPal, BANK_PLANE_B);
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset())
            return;
        plat_frame_begin();
        plat_draw_picture(&gMenu);
        /* show_high_score's original title/initials/score coordinates (+20 draw offset). */
        original_text("HIGH SCORES", 87, 140, 2);
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 3; j++) {
                char text[2] = {gProfile.scores[i].name[j], 0};
                original_text(text, 87 + j * 15, 160 + i * 10, 1);
            }
            char text[16];
            if (gProfile.scores[i].score == 0x0fffffffu)
                snprintf(text, sizeof text, "IMMORTAL");
            else
                snprintf(text, sizeof text, "%8lu", (unsigned long)gProfile.scores[i].score);
            original_text(text, 217, 160 + i * 10, 1);
        }
        plat_frame_end();
        if (pressed & (PAD_B1 | PAD_B2 | PAD_START))
            return;
        prev = p;
    }
}
static void profile_submit_score(int player) {
    if (gDemoRunning)
        return;
    uint32_t score = gCampaign.player[player].score;
    if (profile_score_rank(&gProfile, score) < 0)
        return;
    char name[4] = "AAA";
    if (!profile_initials(name))
        return;
    profile_add_score(&gProfile, name, score);
    gProfileDirty|=2;
    profile_commit_part(1);
    profile_scores();
}
/* Original saved-games list: five named hotel/stage entries and NEW GAME/EXIT.
 * Restoring operates on one player, exactly like intro_sub's NEW/RESTORE loop. */
static hm_tex gSaveBackdrop;
static int profile_slots(int restoring, int player) {
    if (!restoring && gDemoRunning)
        return 0;
    const hm_tex *background = &gMenu;
    if (!restoring) {
        gW.g.cur_player=player;gW.luigi=player;gW.g.player[player]=gCampaign.player[player];
        original_palette(gSpritePal,BANK_PLANE_A);
        char file[40];
        plat_wait_ready();
        plat_free_tex(&gSaveBackdrop);
        snprintf(file, sizeof file, "continue_%d.tex", gCampaign.player[player].hotel);
        if (plat_load_tex(file, BANK_PLANE_B, &gSaveBackdrop)) {
            snprintf(file, sizeof file, "continue_%d.pal", gCampaign.player[player].hotel);
            plat_load_pal(file, BANK_PLANE_B, 0, NULL, 0);
            background = &gSaveBackdrop;
        }
    }
    int row = 0;
    uint32_t prev = plat_pad();
    original_palette(gFontPal, 3);
    for (;;) {
        uint32_t p = plat_pad(), pressed = p & ~prev;
        if (plat_soft_reset() || (pressed & PAD_B2))
            return 0;
        if (pressed & PAD_UP)
            row = (row + 5) % 6;
        if (pressed & PAD_DOWN)
            row = (row + 1) % 6;
        plat_frame_begin();
        plat_draw_picture(background);
        if(!restoring) {
            world_sprite hud[40];int n=world_hud(&gW,hud,40);
            for(int i=0;i<n;i++)if(gHud.n_frames && hud[i].frame<gHud.n_slots)
                plat_draw_frame(&gHud,gHud.slot[hud[i].frame],hud[i].x,hud[i].y+20,2);
        }
        original_text("       SAVED GAMES", 72, 140, 1);
        original_text("NAME         HOTEL STAGE", 72, 150, 1);
        for (int i = 0; i < 5; i++) {
            const hm_save_slot *slot = &gProfile.slots[i];
            for (int j = 0; j < 3; j++) {
                char text[2] = {slot->valid ? slot->name[j] : ' ', 0};
                original_text(text, 72 + j * 15, 160 + i * 10, 1);
            }
            if (slot->valid) {
                char text[8];
                snprintf(text, sizeof text, "%2d", slot->hotel > 5 ? slot->hotel - 1 : slot->hotel);
                original_text(text, 232, 160 + i * 10, 1);
                snprintf(text, sizeof text, "%2d", slot->stage + 1);
                original_text(text, 292, 160 + i * 10, 1);
            }
        }
        original_text(restoring ? "NEW GAME" : "EXIT", 72, 210, 1);

        original_text("p", 57, 160 + row * 10, 1);
        plat_frame_end();
        if (pressed & (PAD_B1 | PAD_START)) {
            if (row == 5)
                return restoring ? 2 : 0;
            if (restoring) {
                if (!profile_restore_player(&gProfile, row, &gCampaign.player[player]))
                    return 1;
                /* The original treats an empty restore slot as NEW. */
                return 2;
            }
            if (gProfile.slots[row].valid && !profile_message("REPLACE THIS SAVED GAME", NULL, 1)) {
                prev = plat_pad();
                continue;
            }
            char name[4] = "AAA";
            if (gProfile.slots[row].valid)
                memcpy(name, gProfile.slots[row].name, 4);
            if (profile_initials(name)) {
                hm_campaign checkpoint = gCampaign;
                checkpoint.current = player;
                if (!profile_capture(&gProfile, row, name, &checkpoint)) {
                    gProfileDirty|=1;
                    profile_commit_part(0);
                    gCampaign.player[player].unk_13 = name[0];
                    gCampaign.player[player].unk_14 = name[1];
                    gCampaign.player[player].unk_15 = name[2];
                    return 1;
                }
            }
            prev = plat_pad();
            continue;
        }
        prev = p;
    }
}
static int profile_begin_game(int players) {
    campaign_start(&gCampaign, players);
    for (int player = 0; player < players; player++) {
        gCampaign.player[player].lives = 5;
        int row = 0;
        uint32_t prev = plat_pad();
        for (;;) {
            uint32_t p = plat_pad(), pressed = p & ~prev;
            if (plat_soft_reset() || (pressed & PAD_B2))
                return 0;
            if (pressed & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT))
                row ^= 1;
            original_palette(gMenuPal, BANK_PLANE_B);
            original_palette(gFontPal, 3);
            plat_frame_begin();
            plat_draw_picture(&gMenu);
            char text[16];
            snprintf(text, sizeof text, "PLAYER %d", player + 1);
            original_text(text, 132, 140, 1);
            original_text("NEW", 152, 160, 1);
            original_text("RESTORE", 152, 180, 1);
            original_text("p", 132, 160 + row * 20, 1);
            plat_frame_end();
            if (pressed & (PAD_B1 | PAD_START)) {
                if (!row)
                    break;
                int result = profile_slots(1, player);
                if (result)
                    break;
                prev = plat_pad();
                continue;
            }
            prev = p;
        }
    }
    memcpy(gSessionProgress, gCampaign.player, sizeof gSessionProgress);
    return 1;
}
#endif
