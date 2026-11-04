#!/usr/bin/env python3
"""Run production saves through actual installed KOS filesystem + VMU package code."""
import os,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];build=root/'dc/build/pc/vmu_fs';build.mkdir(parents=True,exist_ok=True)
kos=Path(os.environ.get('KOS_BASE','/opt/toolchains/dc/kos'));inc=build/'include'
for sub in ['dc/maple','kos']:(inc/sub).mkdir(parents=True,exist_ok=True)
(inc/'kos.h').write_text(r"""
#include <pthread.h>
#include <stdlib.h>
void thd_sleep(unsigned);
extern int test_thread_fail;
typedef pthread_t kthread_t;
static inline kthread_t *thd_create(int detached,void *(*fn)(void *),void *arg) {
    (void)detached;if(test_thread_fail)return NULL;
    kthread_t *t=malloc(sizeof *t);
    if(!t)return NULL;
    if(pthread_create(t,NULL,fn,arg)){free(t);return NULL;}
    return t;
}
static inline int thd_join(kthread_t *t,void **value) {
    int status=pthread_join(*t,value);free(t);return status;
}
""")
(inc/'kos/dbglog.h').write_text('#define dbglog(...) ((void)0)\n')
(inc/'kos/mutex.h').write_text('''#include <assert.h>
typedef int mutex_t;
#define MUTEX_TYPE_NORMAL 0
#define MUTEX_INITIALIZER 0
static inline int mutex_init(mutex_t*m,int type){(void)type;*m=0;return 0;}
static inline int mutex_destroy(mutex_t*m){(void)m;return 0;}
static inline int mutex_lock(mutex_t*m){assert(!*m);*m=1;return 0;}
static inline int mutex_unlock(mutex_t*m){assert(*m);*m=0;return 0;}
''')
(inc/'dc/maple.h').write_text('''#ifndef TEST_MAPLE_H
#define TEST_MAPLE_H
#include <stdint.h>
typedef uint8_t uint8;typedef uint16_t uint16;typedef uint32_t uint32;
#define MAPLE_FUNC_MEMCARD 0x02000000
struct maple_device {int port,unit;struct {uint32_t functions;}info;};
typedef struct maple_device maple_device_t;
maple_device_t*maple_enum_dev(int,int);
maple_device_t*maple_enum_type(int,uint32_t);
#endif
''')
(inc/'dc/maple/vmu.h').write_text('''#include <dc/maple.h>
int vmu_block_read(maple_device_t*,uint16_t,uint8_t*);
int vmu_block_write(maple_device_t*,uint16_t,const uint8_t*);
''')
for name in ['vmu_pkg.h','vmufs.h']:(inc/'dc'/name).write_text('#include <stddef.h>\n'+(kos/'kernel/arch/dreamcast/include/dc'/name).read_text())
s=(kos/'kernel/arch/dreamcast/util/vmu_pkg.c').read_text().split('static unsigned int pal_get_map')[0]
for include in ['#include <kos/fs.h>','#include <kos/regfield.h>']:s=s.replace(include,'')
(build/'vmu_pkg_core.c').write_text(s)
cc=[os.environ.get('CC','clang'),'-g','-O1','-D_arch_dreamcast','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(inc),'-I'+str(root/'dc'),'-I'+str(root/'src')]
cc += [str(root/p) for p in ['dc/pc_test/vmu_filesystem_test.c','dc/vmu_store_dc.c','dc/persistence.c','dc/campaign.c']]
cc += [str(kos/'kernel/arch/dreamcast/fs/vmufs.c'),str(build/'vmu_pkg_core.c'),'-o',str(build/'vmu_filesystem_test')]
subprocess.run(cc,check=True)
p=subprocess.run([str(build/'vmu_filesystem_test')],capture_output=True,text=True)
if '--expect-padding-bug' in sys.argv:
    (build/'reproduced-padding-bug.log').write_text(p.stdout+p.stderr)
    assert p.returncode and 'heap-buffer-overflow' in p.stderr,p.stdout+p.stderr
    print('Reproduced build-20 heap-buffer-overread using the actual KOS block writer.')
else:
    print(p.stdout,end='')
    if p.returncode:print(p.stderr,file=sys.stderr);raise SystemExit(p.returncode)
