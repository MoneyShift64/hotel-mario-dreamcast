/* Versioned byte format: never dump native structs/pointers to VMU storage. */
#include "persistence.h"
#include <string.h>
static int hotel(int n) { return n >= 1 && n <= 8 && n != 6; }
static int last(int h) { return h == 8 ? 14 : 9; }
static uint32_t crc32(const uint8_t *p, unsigned n) {
    uint32_t c = ~0u;
    for (unsigned i = 0; i < n; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++)
            c = (c >> 1) ^ ((0u - (c & 1)) & 0xedb88320u);
    }
    return ~c;
}
static void put16(uint8_t *p, unsigned n) {
    p[0] = n;
    p[1] = n >> 8;
}
static unsigned get16(const uint8_t *p) { return p[0] | p[1] << 8; }
static void put32(uint8_t *p, uint32_t n) {
    for (int i = 0; i < 4; i++)
        p[i] = n >> (i * 8);
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int name_ok(const char *s) {
    for (int i = 0; i < 3; i++)
        if (s[i] != ' ' && !(s[i] >= 'A' && s[i] <= 'Z') && !(s[i] >= '0' && s[i] <= '9'))
            return 0;
    return s[3] == 0;
}
static void copy_name(char out[4], const char *in) {
    memset(out, ' ', 3);
    out[3] = 0;
    for (int i = 0; i < 3 && in && in[i]; i++)
        if ((in[i] >= 'A' && in[i] <= 'Z') || (in[i] >= '0' && in[i] <= '9'))
            out[i] = in[i];
}
void profile_init(hm_profile *p) {
    memset(p, 0, sizeof *p);
    for (int i = 0; i < HM_HIGH_SCORES; i++)
        copy_name(p->scores[i].name, "   ");
}
int profile_encode(const hm_profile *p, uint8_t b[HM_PROFILE_BYTES]) {
    memset(b, 0, HM_PROFILE_BYTES);
    memcpy(b, "HMSV", 4);
    put16(b + 4, 1);
    put16(b + 6, HM_PROFILE_BYTES);
    put32(b + 8, p->sequence);
    uint8_t *nv = b + 16;
    for (int i = 0; i < 5; i++) {
        if (!name_ok(p->scores[i].name) || p->scores[i].score > 0x0fffffffu ||
            (i && p->scores[i].score > p->scores[i - 1].score))
            return HM_STORE_CORRUPT;
        memcpy(nv + i * 3, p->scores[i].name, 3);
        put32(nv + 16 + i * 4, p->scores[i].score);
        const hm_save_slot *slot = &p->slots[i];
        if (slot->valid) {
            if (!name_ok(slot->name) || !hotel(slot->hotel) || slot->stage < 0 ||
                slot->stage > last(slot->hotel))
                return HM_STORE_CORRUPT;
            memcpy(nv + 36 + i * 3, slot->name, 3);
            put16(nv + 52 + i * 2, (slot->hotel << 8) | (slot->stage + 1));
        }
    }
    put32(b + 12, crc32(nv, 62) ^ p->sequence ^ 0x484d5356u);
    return 0;
}
int profile_decode(hm_profile *p, const uint8_t *b, unsigned size) {
    if (size != HM_PROFILE_BYTES || memcmp(b, "HMSV", 4) || get16(b + 4) != 1 ||
        get16(b + 6) != size || get32(b + 12) != (crc32(b + 16, 62) ^ get32(b + 8) ^ 0x484d5356u))
        return HM_STORE_CORRUPT;
    hm_profile trial;
    profile_init(&trial);
    trial.sequence = get32(b + 8);
    const uint8_t *nv = b + 16;
    for (int i = 0; i < 5; i++) {
        memcpy(trial.scores[i].name, nv + i * 3, 3);
        trial.scores[i].score = get32(nv + 16 + i * 4);
        unsigned code = get16(nv + 52 + i * 2);
        if (code) {
            hm_save_slot *slot = &trial.slots[i];
            slot->valid = 1;
            memcpy(slot->name, nv + 36 + i * 3, 3);
            slot->name[3] = 0;
            slot->hotel = code >> 8;
            slot->stage = (code & 255) - 1;
        }
    }
    uint8_t check[HM_PROFILE_BYTES];
    if (profile_encode(&trial, check) || memcmp(check, b, size))
        return HM_STORE_CORRUPT;
    *p = trial;
    return 0;
}
int profile_capture(hm_profile *p, int slot, const char *name, const hm_campaign *c) {
    if (slot < 0 || slot >= 5 || c->current < 0 || c->current >= 2)
        return HM_STORE_CORRUPT;
    const player_t *player = &c->player[c->current];
    if (!hotel(player->hotel) || player->stage < 0 || player->stage > last(player->hotel) + 1)
        return HM_STORE_CORRUPT;
    p->slots[slot].valid = 1;
    p->slots[slot].hotel = player->hotel;
    p->slots[slot].stage =
        player->stage > last(player->hotel) ? last(player->hotel) : player->stage;
    copy_name(p->slots[slot].name, name);
    return 0;
}
int profile_restore_player(const hm_profile *p, int slot, player_t *player) {
    if (slot < 0 || slot >= 5 || !p->slots[slot].valid || !hotel(p->slots[slot].hotel) ||
        p->slots[slot].stage < 0 || p->slots[slot].stage > last(p->slots[slot].hotel))
        return HM_STORE_MISSING;
    const hm_save_slot *s = &p->slots[slot];
    memset(player, 0, sizeof *player);
    player->hotel = player->progress = s->hotel;
    player->stage = player->unk_18 = s->stage;
    player->lives = 5;
    player->got_bonus = 1;
    player->unk_13 = s->name[0];
    player->unk_14 = s->name[1];
    player->unk_15 = s->name[2];
    return 0;
}
int profile_restore(const hm_profile *p, int slot, hm_campaign *c) {
    player_t player;
    int status = profile_restore_player(p, slot, &player);
    if (status)
        return status;
    campaign_start(c, 1);
    c->player[0] = player;
    return 0;
}
int profile_score_rank(const hm_profile *p, uint32_t score) {
    if (!score || score > 0x0fffffffu)
        return -1;
    for (int i = 0; i < 5; i++)
        if (score > p->scores[i].score)
            return i;
    return -1;
}
int profile_add_score(hm_profile *p, const char *name, uint32_t score) {
    int rank = profile_score_rank(p, score);
    if (rank < 0)
        return rank;
    for (int i = 4; i > rank; i--)
        p->scores[i] = p->scores[i - 1];
    p->scores[rank].score = score;
    copy_name(p->scores[rank].name, name);
    return rank;
}
