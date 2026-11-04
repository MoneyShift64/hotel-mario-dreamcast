/* Regression tests for campaign rules; no renderer or disc required. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "campaign.h"
static world w;

static void tag_is(hm_campaign *c, const char *expected) {
    char tag[8];
    assert(campaign_tag(c, tag, sizeof tag));
    assert(strcmp(tag, expected) == 0);
}

static void test_route(void) {
    hm_campaign c;
    int hotels[] = {1, 2, 3, 4, 5, 7, 8}, count = 0;
    campaign_start(&c, 1);
    for (unsigned h = 0; h < sizeof hotels / sizeof *hotels; h++) {
        int stages = hotels[h] == 8 ? 15 : 10;
        for (int s = 0; s < stages; s++) {
            char expected[8];
            snprintf(expected, sizeof expected, "%d_%02d", hotels[h], s + 1);
            tag_is(&c, expected);
            assert(c.player[0].hotel == hotels[h] && c.player[0].stage == s);
            assert(campaign_advance(&c, 1) == (++count == 75 ? CAMPAIGN_COMPLETE : CAMPAIGN_NEXT));
        }
    }
    assert(c.finished[0]);
    char tag[8];
    assert(!campaign_tag(&c, tag, sizeof tag)); /* Never wrap to hotel 0. */
}

static void test_retry_and_game_over(void) {
    hm_campaign c;
    campaign_start(&c, 1);
    c.player[0].score = 1234; c.player[0].coins = 29; c.player[0].power = 2;
    c.player[0].hotel = 7; c.player[0].stage = 4;
    assert(campaign_advance(&c, 0) == CAMPAIGN_NEXT);
    assert(c.player[0].lives == 2 && c.player[0].power == 0);
    assert(c.player[0].score == 1234 && c.player[0].coins == 29);
    tag_is(&c, "7_05");
    assert(campaign_advance(&c, 0) == CAMPAIGN_NEXT);
    assert(campaign_advance(&c, 0) == CAMPAIGN_GAME_OVER);
    assert(c.player[0].lives == 0 && c.player[0].score == 1234);
}

static void test_two_players(void) {
    hm_campaign c;
    campaign_start(&c, 2);
    c.player[0].score = 500; c.player[0].power = 2;
    assert(campaign_advance(&c, 1) == CAMPAIGN_NEXT);
    assert(c.current == 1);
    tag_is(&c, "1_01");
    assert(c.player[0].stage == 1 && c.player[0].power == 2);
    c.player[1].lives = 1;
    assert(campaign_advance(&c, 0) == CAMPAIGN_PLAYER_OUT);
    assert(c.current == 0 && c.player[1].lives == 0);
    tag_is(&c, "1_02");
    assert(campaign_advance(&c, 1) == CAMPAIGN_NEXT);
    assert(c.current == 0 && c.player[0].score == 500);
    c.player[0].lives = 1;
    assert(campaign_advance(&c, 0) == CAMPAIGN_GAME_OVER);

    campaign_start(&c, 2);
    c.player[0].hotel = 8; c.player[0].stage = 14;
    assert(campaign_advance(&c, 1) == CAMPAIGN_PLAYER_OUT);
    assert(c.finished[0] && c.current == 1);
    c.player[1].hotel = 8; c.player[1].stage = 14;
    assert(campaign_advance(&c, 1) == CAMPAIGN_COMPLETE);
    assert(c.finished[0] && c.finished[1]);
}

static void test_capture_bonus_and_world_restore(void) {
    hm_campaign c;
    campaign_start(&c, 2);
    c.current = 1;
    c.player[1].hotel = 5; c.player[1].stage = 3;
    c.player[1].score = 100; c.player[1].coins = 7; c.player[1].power = 2;
    c.player[1].progress = 7; c.player[1].unk_18 = 2;
    world_start_campaign(&w, NULL, NULL, c.player, c.current, 1);
    assert(w.g.cur_player == 1 && w.g.two_players);
    assert(memcmp(w.g.player, c.player, sizeof c.player) == 0);
    w.status = WORLD_CLEARED; w.g.time_left = 123;
    campaign_capture(&c, &w);
    assert(c.bonus_time == 123 && c.bonus_rate == 50);
    int ticks = 0;
    while (campaign_bonus_tick(&c)) ticks++;
    assert(ticks == 123 && c.player[1].score == 6250);
    assert(c.player[0].score == 0 && c.player[1].coins == 7);
    assert(!campaign_bonus_tick(&c) && c.player[1].score == 6250);
    w.status = WORLD_TIME_UP; w.g.time_left = 25;
    campaign_capture(&c, &w);
    assert(!campaign_bonus_tick(&c) && c.bonus_time == 0);
}

static void test_clear_jingle(void) {
    /* Stage indices are zero-based; hotel 8 has fifteen stages. */
    const int cases[][3] = {{1, 8, 9}, {1, 9, 10}, {8, 10, 9}, {8, 14, 10}};
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; i++) {
        world_start(&w, NULL, NULL, cases[i][0], cases[i][1], 0);
        world_frame(&w, 0); /* Empty door grid triggers exit_check. */
        assert(w.music_req == cases[i][2]);
    }
}

static void test_special_bonus(void) {
    hm_campaign c;
    campaign_start(&c, 1);
    world_start_campaign(&w, NULL, NULL, c.player, 0, 0);
    assert(campaign_special_multiplier(&w) == 100);
    w.g.unk_550[1] = 0;
    assert(campaign_special_multiplier(&w) == 75);
    w.mario.hit = 1;
    assert(campaign_special_multiplier(&w) == 50);
    w.room.n_actors = 1; w.room.actors[0].unk_048 = 1;
    assert(campaign_special_multiplier(&w) == 25);
    w.room.actors[0].unk_04a = 1;
    assert(campaign_special_multiplier(&w) == 1);
    w.g.unk_550[1] = 1;
    assert(campaign_special_multiplier(&w) == 25);
    w.status = WORLD_CLEARED; w.g.time_left = 2;
    campaign_capture(&c, &w);
    assert(c.special_bonus == 5000);
    campaign_finish_bonus(&c);
    assert(!c.special_paid);
    while (campaign_bonus_tick(&c)) {}
    campaign_finish_bonus(&c);
    assert(c.special_paid && c.player[0].score == 5020);
    campaign_finish_bonus(&c);
    assert(c.player[0].score == 5020);
}

int main(void) {
    test_special_bonus();
    test_clear_jingle();
    test_route();
    test_retry_and_game_over();
    test_two_players();
    test_capture_bonus_and_world_restore();
    puts("Campaign checks passed: 75-stage route, retries, game over, two-player turns, time bonus, player restoration.");
    return 0;
}
