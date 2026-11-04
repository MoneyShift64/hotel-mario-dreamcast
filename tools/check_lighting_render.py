#!/usr/bin/env python3
"""Record production PVR overlay geometry, depth and draw cost on the host."""
from pathlib import Path
import os,subprocess
root=Path(__file__).resolve().parents[1];build=root/'dc/build/pc';build.mkdir(parents=True,exist_ok=True)
s=(root/'dc/hm_plat_dc.c').read_text()
fn=s[s.index('static void draw_light_regions(void)'):s.index('void plat_draw_picture_lit(')]
source='''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "light_regions.h"
typedef struct {float x,y;} shz_vec2_t;
static shz_vec2_t to_screen(float x,float y){return (shz_vec2_t){x,y};}
typedef int pvr_poly_cxt_t;
typedef struct {int marker;} pvr_poly_hdr_t;
typedef struct {float x,y,z;uint32_t argb;int flags;} pvr_vertex_t;
#define PVR_LIST_TR_POLY 1
#define PVR_CMD_VERTEX 1
#define PVR_CMD_VERTEX_EOL 2
static hm_light_region sLightRegions[HM_LIGHT_REGION_MAX];
static int sLightRequested,sLightCount,headers,vertices;
static void pvr_poly_cxt_col(pvr_poly_cxt_t*c,int list){assert(list==PVR_LIST_TR_POLY);*c=1;}
static void pvr_poly_compile(pvr_poly_hdr_t*h,const pvr_poly_cxt_t*c){assert(*c==1);h->marker=99;}
static void pvr_prim(const void*data,unsigned bytes) {
    if(bytes==sizeof(pvr_poly_hdr_t)){assert(((const pvr_poly_hdr_t*)data)->marker==99);headers++;return;}
    assert(bytes==sizeof(pvr_vertex_t));const pvr_vertex_t*v=data;
    const hm_light_region*r=&sLightRegions[vertices/4];
    assert(v->z==3.9f); /* under HUD z>=4 and pause UI z>=4.5 */
    assert(v->argb==((uint32_t)r->alpha<<24)); /* black darkens the composed scene */
    int corner=vertices%4;assert(v->x==(corner<2?r->x:r->x+r->w));
    assert(v->y==((corner==0||corner==2)?r->y+r->h:r->y));
    assert(v->flags==(corner==3?PVR_CMD_VERTEX_EOL:PVR_CMD_VERTEX));vertices++;
}
'''+fn+'''
int main(void) {
    sLightCount=2;sLightRegions[0]=(hm_light_region){0,59,384,36,175};
    sLightRegions[1]=(hm_light_region){0,131,384,108,175};
    draw_light_regions();assert(!headers&&!vertices); /* menus do not request lighting */
    sLightRequested=1;draw_light_regions();assert(headers==1&&vertices==8);
    puts("Lighting renderer passed: one shared header, four vertices per cached region, correct brightness/depth, no lighting on menus.");
}
'''
p=build/'lighting_render_test.c';p.write_text(source)
subprocess.run([os.environ.get('CC','clang'),'-g','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(root/'dc'),str(p),'-o',str(build/'lighting_render_test')],check=True)
subprocess.run([str(build/'lighting_render_test')],check=True)
