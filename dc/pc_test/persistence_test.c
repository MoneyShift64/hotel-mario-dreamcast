#include "persistence.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static hm_profile original, decoded, attempt;
static void sample(void) {
    profile_init(&original);
    hm_campaign c;
    campaign_start(&c, 2);
    c.current = 1;
    for (int i = 0; i < 2; i++) {
        c.player[i].hotel = c.player[i].progress = i ? 8 : 7;
        c.player[i].stage = c.player[i].unk_18 = i ? 14 : 9;
        c.player[i].score = 12345 + i;
        c.player[i].lives = 7 - i;
        c.player[i].coins = 17 + i;
        c.player[i].power = 2;
    }
    for (int i = 0; i < 5; i++)
        assert(!profile_capture(&original, i, i ? "MIO" : "AUT", &c));
    assert(profile_add_score(&original, "AAA", 100) == 0);
    assert(profile_add_score(&original, "BBB", 200) == 0);
    assert(profile_add_score(&original, "CCC", 100) == 2);
}
int main(void) {
    sample();
    uint8_t data[HM_PROFILE_BYTES], check[HM_PROFILE_BYTES];
    assert(!profile_encode(&original, data));
    assert(!profile_decode(&decoded, data, sizeof data));
    assert(!profile_encode(&decoded, check) && !memcmp(data, check, sizeof data));
    for (int i = 0; i < 5; i++) {
        hm_campaign c;
        assert(!profile_restore(&decoded, i, &c));
        assert(c.players == 1 && c.current == 0 && c.player[0].hotel == 8 &&
               c.player[0].stage == 14);
        assert(c.player[0].lives == 5 && !c.player[0].score && !c.player[0].coins &&
               !c.player[0].power);
        assert(!c.bonus_time);
    }
    assert(data[16 + 52] == 15 && data[16 + 53] == 8);
    hm_campaign independently;
    campaign_start(&independently, 2);
    independently.current = 1;
    independently.player[0].score = 4567;
    assert(!profile_restore_player(&decoded, 0, &independently.player[1]));
    assert(independently.current == 1 && independently.players == 2 &&
           independently.player[0].score == 4567);
    assert(independently.player[1].lives == 5 && !independently.player[1].score);
    hm_profile sentinel;
    memset(&sentinel, 0x5a, sizeof sentinel);
    for (unsigned i = 0; i < sizeof data; i++)
        for (int b = 0; b < 8; b++) {
            memcpy(check, data, sizeof data);
            check[i] ^= 1 << b;
            decoded = sentinel;
            assert(profile_decode(&decoded, check, sizeof check) == HM_STORE_CORRUPT);
            assert(!memcmp(&decoded, &sentinel, sizeof decoded));
        }
    for (unsigned n = 0; n < sizeof data; n++)
        assert(profile_decode(&decoded, data, n) == HM_STORE_CORRUPT);
    attempt = original;
    attempt.slots[1].hotel = 6;
    assert(profile_encode(&attempt, check) == HM_STORE_CORRUPT);
    attempt = original;
    attempt.scores[0].name[0] = '!';
    assert(profile_encode(&attempt, check) == HM_STORE_CORRUPT);
    puts("Persistence passed: wire format, corruption/truncation, five checkpoints, names, scores and restore rules.");
}
