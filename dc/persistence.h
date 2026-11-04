#ifndef HM_PERSISTENCE_H
#define HM_PERSISTENCE_H
#include "campaign.h"
#define HM_SAVE_SLOTS 5 /* original CD-i named hotel/stage checkpoints */
#define HM_HIGH_SCORES 5
#define HM_PROFILE_BYTES 78
#define HM_STORE_MISSING 1
#define HM_STORE_IO -1
#define HM_STORE_CORRUPT -2
#define HM_STORE_NO_DEVICE -3
#define HM_STORE_FULL -4
#define HM_STORE_FOREIGN -5
#define HM_STORE_CHANGED -6
typedef struct {
    char name[4];
    uint32_t score;
} hm_high_score;
typedef struct {
    int valid;
    char name[4];
    int hotel, stage;
} hm_save_slot;
typedef struct {
    uint32_t sequence;
    hm_high_score scores[HM_HIGH_SCORES];
    hm_save_slot slots[HM_SAVE_SLOTS];
} hm_profile;
void profile_init(hm_profile *);
int profile_encode(const hm_profile *, uint8_t[HM_PROFILE_BYTES]);
int profile_decode(hm_profile *, const uint8_t *, unsigned);
int profile_capture(hm_profile *, int, const char *, const hm_campaign *);
int profile_restore_player(const hm_profile *, int, player_t *);
int profile_restore(const hm_profile *, int, hm_campaign *);
int profile_score_rank(const hm_profile *, uint32_t);
int profile_add_score(hm_profile *, const char *, uint32_t);
#endif
