#include "campaign.h"
#include "frontend.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    int x;
    assert(!strcmp(frontend_arrival(1,&x),"MORTON AND HIS NUTTY GROUP")&&x==62);
    assert(!strcmp(frontend_arrival(8,&x),"BOWSER AND COMPANY")&&x==101);
    assert(!strcmp(frontend_ready(11,3,&x),"HERE WE GOg")&&x==139);
    assert(!strcmp(frontend_ready(11,17,&x),"HAPPY TURKEY DAYg GOBBLE GOBBLEg")&&x==37);
    assert(!strcmp(frontend_ready(11,26,&x),"HERE WE GOg"));
    assert(!strcmp(frontend_ready(12,26,&x),"HAPPY HOLIDAYSg"));
    assert(!strcmp(frontend_ready(12,27,&x),"HAPPY NEW YEARg"));
    assert(!strcmp(frontend_ready(1,6,&x),"HERE WE GOg"));
    player_t p = {0};
    p.hotel = p.progress = 8;
    p.stage = p.unk_18 = 14;
    p.score = 3456;
    p.lives = 2;
    p.power = 2;
    p.unk_10 = 1;
    frontend_choose_hotel(&p, 7);
    assert(p.hotel == 7 && p.stage == 0 && !p.unk_10);
    assert(p.lives == 2 && p.score == 3456 && p.power == 2 && p.progress == 8);
    frontend_choose_hotel(&p, 8);
    assert(p.stage == 14);
    frontend_choose_hotel(&p, 6);
    assert(p.hotel == 8);
    p.progress = 3;
    frontend_choose_hotel(&p, 8);
    assert(p.progress == 3);
    for (int progress = 1; progress <= 8; progress++) {
        if (progress == 6)
            continue;
        for (int h = 1; h <= progress; h++) {
            if (h == 6)
                continue;
            int n = frontend_next_hotel(h, 1, progress);
            assert(n >= 1 && n <= progress && n != 6);
            assert(frontend_next_hotel(n, -1, progress) == h);
        }
    }
    assert(frontend_next_hotel(1, -1, 8) == 8);
    assert(frontend_next_hotel(5, 1, 8) == 7);
    player_t saved[2] = {{0}}, active[2] = {{0}};
    active[0].progress = 7;
    active[0].unk_18 = 9;
    active[1].progress = 2;
    active[1].unk_18 = 3;
    frontend_record_progress(saved, active);
    active[0].progress = 1;
    frontend_record_progress(saved, active);
    assert(saved[0].progress == 7 && saved[0].unk_18 == 9 && saved[1].progress == 2);
    p.hotel = 8;
    p.stage = 14;
    p.progress = 8;
    p.unk_18 = 14;
    p.lives = 0;
    p.coins = 29;
    p.score = 54321;
    p.power = 2;
    frontend_continue_player(&p);
    assert(p.lives == 5 && !p.coins && !p.score && !p.power);
    assert(p.hotel == 8 && p.stage == 14 && p.progress == 8 && p.unk_18 == 14);
    active[0].progress = 8;
    active[0].unk_18 = 15; /* Completed campaign is one past the last stage. */
    frontend_record_progress(saved, active);
    assert(saved[0].progress == 8 && saved[0].unk_18 == 14);
    hm_campaign campaign;
    campaign_start(&campaign, 2);
    campaign.player[0].lives = 1;
    assert(campaign_advance(&campaign, 0) == CAMPAIGN_PLAYER_OUT && campaign.current == 1);
    frontend_continue_player(&campaign.player[0]);
    assert(campaign.current == 1 && campaign.player[0].lives == 5);
    assert(campaign_advance(&campaign, 1) == CAMPAIGN_NEXT && campaign.current == 0);
    puts("Frontend checks passed: unlocked hotels, stage restoration, independent progress, state "
         "retention.");
}
