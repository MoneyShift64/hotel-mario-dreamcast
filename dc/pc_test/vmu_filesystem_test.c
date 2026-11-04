/* Actual KOS filesystem/package code on a sector-addressed 128 KiB VMU image. */
#include "vmu_store.h"
#include <assert.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <dc/vmu_pkg.h>
#include <dc/vmufs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t image[256][512];
static maple_device_t card;
int test_thread_fail;
static int discard_fat;
static int intermittent, permanent_block=-1, block_attempt[256];
static int online = 1, busy_reads, busy_after_write, fail_after_directory, fail_data;
maple_device_t *maple_enum_dev(int port, int unit) {
    if (!online || port != 0 || unit != 1)
        return NULL;
    card.port = 0;
    card.unit = 1;
    card.info.functions = MAPLE_FUNC_MEMCARD;
    return &card;
}
maple_device_t *maple_enum_type(int i,uint32_t function) {
    return i==0 && function==MAPLE_FUNC_MEMCARD?maple_enum_dev(0,1):NULL;
}
void thd_sleep(unsigned ms) { (void)ms; }
int vmu_block_read(maple_device_t *d, uint16_t block, uint8_t *bytes) {
    (void)d;
    if (!online)
        return -1;
    if (busy_reads) {
        busy_reads--;
        return -1;
    }
    assert(block < 256);
    if (block<200) {
        if (block==permanent_block) return -5;
        if (intermittent && (block_attempt[block]++ % 2)==0) return -2;
    }
    memcpy(bytes, image[block], 512);
    return 0;
}
int vmu_block_write(maple_device_t *d, uint16_t block, const uint8_t *bytes) {
    (void)d;
    if (!online)
        return -1;
    assert(block < 256);
    if(fail_data && block<200)return -1;
    if(discard_fat && block==254)return 0;
    memcpy(image[block], bytes, 512);
    if (block >= 241 && block <= 253) {
        busy_reads = busy_after_write;
        if (fail_after_directory)
            return -1; /* committed sector, lost completion response */
    }
    return 0;
}
static void format(void) {
    assert(sizeof(vmu_root_t) == 512 && sizeof(vmu_dir_t) == 32);
    memset(image, 0, sizeof image);
    vmu_root_t root = {0};
    memset(root.magic, 0x55, 16);
    root.fat_loc = 254;
    root.fat_size = 1;
    root.dir_loc = 253;
    root.dir_size = 13;
    root.blk_cnt = 200;
    memcpy(image[255], &root, sizeof root);
    uint16_t fat[256];
    for (int i = 0; i < 256; i++)
        fat[i] = i < 200 ? 0xfffc : 0xfffa;
    memcpy(image[254], fat, sizeof fat);
}
int main(void) {
    format();vmufs_init();hm_profile p,loaded;profile_init(&p);
    assert(plat_vmu_load(1,&loaded)==HM_STORE_MISSING);
    hm_campaign c;campaign_start(&c,1);c.player[0].hotel=c.player[0].progress=3;
    c.player[0].stage=c.player[0].unk_18=2;
    assert(!profile_capture(&p,0,"SAV",&c));assert(!plat_vmu_save(1,&p));
    assert(vmufs_free_blocks(&card)==198);
    profile_add_score(&p,"MIO",45678);assert(!plat_vmu_save(1,&p));
    assert(!plat_vmu_load(1,&loaded) && loaded.scores[0].score==45678);
    assert(loaded.slots[0].hotel==3 && loaded.slots[0].stage==2);
    void *raw=NULL;int size=0;vmu_pkg_t pkg;
    assert(!vmufs_read(&card,"HOTELMARIO",&raw,&size) && size==1024);
    assert(!vmu_pkg_parse(raw,size,&pkg) && pkg.icon_cnt==1 && pkg.icon_anim_speed==0);
    for(int i=128+512+HM_PROFILE_BYTES;i<size;i++)assert(!((uint8_t *)raw)[i]);
    /* Existing A imports even when old B has the exact reported broken chain. */
    assert(!vmufs_write(&card,"HOTELMARIO_A",raw,size,0));
    assert(!vmufs_write(&card,"HOTELMARIO_B",raw,size,0));free(raw);
    assert(!vmufs_delete(&card,"HOTELMARIO"));
    vmu_root_t *root=(vmu_root_t *)image[255];vmu_dir_t *dir=calloc(root->dir_size,512);
    assert(!vmufs_dir_read(&card,root,dir));int b=vmufs_dir_find(root,dir,"HOTELMARIO_B");assert(b>=0);
    ((uint16_t *)image[254])[dir[b].firstblk]=0xfffa;free(dir);
    uint8_t *snapshot=malloc(sizeof image);memcpy(snapshot,image,sizeof image);
    assert(!plat_vmu_load(1,&loaded) && loaded.scores[0].score==45678 && loaded.slots[0].hotel==3);
    assert(!memcmp(snapshot,image,sizeof image));
    profile_add_score(&loaded,"NEW",50000);assert(!plat_vmu_save(1,&loaded));
    assert(!plat_vmu_load(1,&p) && p.scores[0].score==50000 && p.slots[0].stage==2);
    assert(!vmufs_read(&card,"HOTELMARIO_A",&raw,&size));
    assert(!vmu_pkg_parse(raw,size,&pkg));hm_profile legacy;
    assert(!profile_decode(&legacy,pkg.data,HM_PROFILE_BYTES)&&legacy.scores[0].score==45678);free(raw);
    /* Legacy B entry and FAT link have not been deleted or repaired. */
    dir=calloc(root->dir_size,512);assert(!vmufs_dir_read(&card,root,dir));
    b=vmufs_dir_find(root,dir,"HOTELMARIO_B");assert(b>=0);
    assert(((uint16_t *)image[254])[dir[b].firstblk]==0xfffa);free(dir);free(snapshot);
    test_thread_fail=1;assert(!plat_vmu_save(1,&p));test_thread_fail=0; /* reference fallback */
    fail_data=1;unsigned sequence=p.sequence;
    assert(plat_vmu_save(1,&p)==HM_STORE_IO && p.sequence==sequence);fail_data=0;
    online=0;assert(plat_vmu_save(1,&p)==HM_STORE_NO_DEVICE);online=1;
    format();profile_init(&p);uint8_t other[512];memset(other,0x5a,sizeof other);
    assert(!vmufs_write(&card,"OTHERGAME",other,512,0));assert(!plat_vmu_save(1,&p));
    assert(!vmufs_read(&card,"OTHERGAME",&raw,&size)&&size==512&&!memcmp(raw,other,512));free(raw);
    assert(vmufs_free_blocks(&card)==197);
    /* Scores use the very same Build-28 package/codec/worker, separate filename.
     * A score update must leave progress bytes untouched, and vice versa. */
    hm_campaign campaign;campaign_start(&campaign,1);
    campaign.player[0].hotel=8;campaign.player[0].stage=14;
    assert(!profile_capture(&p,2,"END",&campaign));
    assert(!plat_vmu_save(1,&p));
    void *progress_before;int progress_size;
    assert(!vmufs_read(&card,"HOTELMARIO",&progress_before,&progress_size));
    profile_add_score(&p,"TOP",765432);
    assert(!plat_vmu_save_scores(1,&p));
    hm_profile scores;profile_init(&scores);
    assert(!plat_vmu_load_scores(1,&scores) && scores.scores[0].score==765432);
    assert(!vmufs_read(&card,"HOTELMARIO",&raw,&size));
    assert(size==progress_size && !memcmp(raw,progress_before,size));free(raw);free(progress_before);
    void *score_before;int score_size;
    assert(!vmufs_read(&card,"HMARIO.SYS",&score_before,&score_size));
    campaign.player[0].stage=7;assert(!profile_capture(&p,2,"MID",&campaign));
    assert(!plat_vmu_save(1,&p));
    assert(!vmufs_read(&card,"HMARIO.SYS",&raw,&size));
    assert(size==score_size && !memcmp(raw,score_before,size));free(raw);free(score_before);
    memset(&loaded,0,sizeof loaded);assert(!plat_vmu_load(1,&loaded));
    assert(loaded.slots[2].hotel==8 && loaded.slots[2].stage==7);
    memset(&scores,0,sizeof scores);assert(!plat_vmu_load_scores(1,&scores));
    assert(scores.scores[0].score==765432);
    puts("Independent Build-28 path passed: separate scores, progress bytes preserved, score bytes preserved, cold profile loads.");
    puts("Manic Karts save path passed: single-file checkpoint/score reload, approved static icon, worker fallback, errors, legacy A import with broken B untouched, and unrelated-file preservation.");
}
