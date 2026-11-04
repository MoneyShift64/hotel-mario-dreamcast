#ifndef HM_VMU_STORE_H
#define HM_VMU_STORE_H
#include "persistence.h"
const char *plat_vmu_error_detail(void);
int plat_vmu_list(int *ids, int capacity);
int plat_vmu_load(int id, hm_profile *profile);
int plat_vmu_save(int id, hm_profile *profile);
int plat_vmu_load_scores(int id,hm_profile *profile);
int plat_vmu_save_scores(int id,hm_profile *profile);
#endif
