/* Manic Karts VMU I/O adapted for Hotel Mario's filename, icon and payload.
 * One file; no alternating banks, binding checks or post-write verification. */
#include "assets/vmu/static/vmu_icon.h"
#include "vmu_store.h"
#include <dc/maple.h>
#include <dc/vmu_pkg.h>
#include <dc/vmufs.h>
#include <kos.h>
#include <kos/mutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char save_name[]="HOTELMARIO";
static mutex_t gVmuLock=MUTEX_INITIALIZER;
typedef struct {maple_device_t *dev;uint8_t *file;int size,status;char name[16];volatile int done;} SaveJob;
static kthread_t *gSaveThread;
static SaveJob gSaveJob;
static char error_detail[64]="VMU IO";
const char *plat_vmu_error_detail(void){return error_detail;}
static maple_device_t *device(int id) {
    if(id<0 || id>=24)return NULL;
    maple_device_t *d=maple_enum_dev(id/6,id%6);
    return d && (d->info.functions&MAPLE_FUNC_MEMCARD)?d:NULL;
}
int plat_vmu_list(int *ids,int capacity) {
    int count=0;
    for(int i=0;i<8 && count<capacity;i++) {
        maple_device_t *d=maple_enum_type(i,MAPLE_FUNC_MEMCARD);
        if(d)ids[count++]=d->port*6+d->unit;
    }
    return count;
}
static void *save_thread(void *arg) {
    SaveJob *j=arg;
    mutex_lock(&gVmuLock);
    int r=vmufs_write(j->dev,j->name,j->file,j->size,VMUFS_OVERWRITE);
    if(r<0){thd_sleep(200);r=vmufs_write(j->dev,j->name,j->file,j->size,VMUFS_OVERWRITE);}
    mutex_unlock(&gVmuLock);
    j->status=r;free(j->file);j->file=NULL;j->done=1;
    return NULL;
}
static void save_wait(void) {
    if(gSaveThread) {
        while(!gSaveJob.done)thd_sleep(20);
        thd_join(gSaveThread,NULL);gSaveThread=NULL;
    }
}
/* Manic Karts locates game data using the package header, then its magic. */
static int save_extract(const uint8_t *h,int got,hm_profile *p) {
    static const int eyecatch[]={0,72*56*2,512+72*56,32+72*56/2};
    if(got>=128) {
        int icons=h[0x40]|h[0x41]<<8,ec=h[0x44]|h[0x45]<<8;
        int at=128+icons*512+(ec>=0 && ec<4?eyecatch[ec]:0);
        if(icons>=1 && icons<=3 && at+HM_PROFILE_BYTES<=got &&
           !memcmp(h+at,"HMSV",4) && !profile_decode(p,h+at,HM_PROFILE_BYTES))return 1;
    }
    for(int at=128;at+HM_PROFILE_BYTES<=got;at+=32)
        if(!memcmp(h+at,"HMSV",4) && !profile_decode(p,h+at,HM_PROFILE_BYTES))return 1;
    return 0;
}
static int save_read(maple_device_t *d,const char *name,hm_profile *p) {
    save_wait();int last=-1;
    for(int tries=0;tries<3;tries++) {
        void *data=NULL;int got=0;
        mutex_lock(&gVmuLock);int r=vmufs_read(d,name,&data,&got);mutex_unlock(&gVmuLock);
        if(r>=0 && data) {
            int ok=save_extract(data,got,p);free(data);
            return ok?0:HM_STORE_CORRUPT;
        }
        free(data);last=r;
        if(r==-2)return HM_STORE_MISSING;
        thd_sleep(60);
    }
    snprintf(error_detail,sizeof error_detail,"%c%d READ FILE N%d",'A'+d->port,d->unit,-last);
    return HM_STORE_IO;
}
static int load_named(int id,hm_profile *p,int scores) {
    maple_device_t *d=device(id);if(!d)return HM_STORE_NO_DEVICE;
    int status=save_read(d,scores?"HMARIO.SYS":save_name,p);
    if(scores)return status;
    if(status!=HM_STORE_MISSING)return status;
    /* Read-only import of old saves. Never write, delete or repair A/B. */
    hm_profile a,b;int sa=save_read(d,"HOTELMARIO_A",&a),sb=save_read(d,"HOTELMARIO_B",&b);
    if(!sa || !sb) {
        *p=sa?b:sb?a:(b.sequence!=a.sequence && b.sequence-a.sequence<0x80000000u)?b:a;
        return 0;
    }
    return sa==HM_STORE_MISSING && sb==HM_STORE_MISSING?HM_STORE_MISSING:HM_STORE_CORRUPT;
}
static int save_named(int id,hm_profile *p,int scores) {
    maple_device_t *d=device(id);if(!d)return HM_STORE_NO_DEVICE;
    hm_profile trial=*p;trial.sequence++;
    uint8_t data[HM_PROFILE_BYTES];int status=profile_encode(&trial,data);if(status)return status;
    vmu_pkg_t pkg;memset(&pkg,0,sizeof pkg);
    strcpy(pkg.desc_short,"HOTEL MARIO");strcpy(pkg.desc_long,"HOTEL MARIO SAVE");strcpy(pkg.app_id,"HOTEL_MARIO_DC");
    pkg.icon_cnt=HM_VMU_ICON_FRAMES;pkg.icon_anim_speed=HM_VMU_ICON_SPEED;pkg.eyecatch_type=VMUPKG_EC_NONE;
    pkg.data_len=sizeof data;pkg.data=data;pkg.icon_data=(uint8_t *)hm_vmu_icon_pixels;
    pkg.eyecatch_data=(uint8_t *)hm_vmu_icon_pixels;
    memcpy(pkg.icon_pal,hm_vmu_icon_palette,sizeof pkg.icon_pal);
    uint8_t *file=NULL;int size=0;
    if(vmu_pkg_build(&pkg,&file,&size)<0 || !file)return HM_STORE_IO;
    /* KOS reads full sectors: keep the package's trailing partial block valid. */
    int padded=(size+511)&~511;uint8_t *blocks=calloc(1,padded);
    if(!blocks){free(file);return HM_STORE_IO;}
    memcpy(blocks,file,size);free(file);
    save_wait();gSaveJob.dev=d;gSaveJob.file=blocks;gSaveJob.size=padded;
    strncpy(gSaveJob.name,scores?"HMARIO.SYS":save_name,15);gSaveJob.done=0;
    gSaveThread=thd_create(0,save_thread,&gSaveJob);
    if(!gSaveThread)save_thread(&gSaveJob);
    /* Wait for the same worker's result to fit Hotel Mario's saving screen.
     * No second file scan/read-back gate is added to the reference write. */
    save_wait();status=gSaveJob.status;
    if(status<0) {
        snprintf(error_detail,sizeof error_detail,"%c%d WRITE FILE N%d",'A'+d->port,d->unit,-status);
        return status==-7?HM_STORE_FULL:HM_STORE_IO;
    }
    p->sequence=trial.sequence;return 0;
}

int plat_vmu_load(int id,hm_profile *p){return load_named(id,p,0);}
int plat_vmu_save(int id,hm_profile *p){return save_named(id,p,0);}
int plat_vmu_load_scores(int id,hm_profile *p){return load_named(id,p,1);}
int plat_vmu_save_scores(int id,hm_profile *p){return save_named(id,p,1);}
