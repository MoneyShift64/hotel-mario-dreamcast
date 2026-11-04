#include "campaign.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static world w;
static int collected;
static int item(actor_t *a,gds *g){(void)g;if(a->cmd==3){collected++;a->state=4;}return 0;}
static int enemy_hits[5];
static int enemy(actor_t *a,gds *g){
 if(a->cmd==3 || a->cmd==4){enemy_hits[a->cmd]++;a->state=4;g->player[g->cur_player].score+=100;}
 return 0;
}
static void start(unsigned cheats,int group){
 world_start(&w,NULL,NULL,1,0,0);world_set_cheats(&w,cheats);
 w.g.time_left=500;w.mario.state_group=group;w.mario.state=group==3?2:6;
 w.mario.floor=2;w.mario.x=w.mario.old_x=100;w.mario.y=hm_floor_y[2]-30;
 w.mario.anim_speed=5;w.mario.hurt_timer=-1;
}
static void overlap(int type){
 w.room.n_actors=1;actor_t *a=&w.room.actors[0];memset(a,0,sizeof *a);
 a->type=type;a->state=3;a->cur_floor=2;a->box_x0=90;a->box_x1=140;
 a->box_y0=w.mario.y-10;a->box_y1=w.mario.y+40;a->handler=(void*)item;
}
int main(int argc,char **argv){
 assert(argc==2);
 /* All enemy/projectile types, including both boxes and airborne contacts. */
 for(int group=0;group<=3;group+=3)for(int type=10;type<=0x3e;type++){
  start(HM_CHEAT_INVINCIBLE,group);overlap(type);
  int state=w.mario.state_group;
  mario_field(&w,0);
  assert(w.mario.state_group==state && !w.mario.fell && w.g.player[0].power==0);
  assert(w.room.actors[0].state==3 && w.mario.invincible==0);
 }
 start(0,0);overlap(10);mario_field(&w,0);assert(w.mario.state_group==6);
 /* Invulnerability must not suppress normal stomps or fire-flower attacks. */
 for(int cheat=0;cheat<2;cheat++) {
  start(cheat?HM_CHEAT_INVINCIBLE:0,3);overlap(10);
  memset(enemy_hits,0,sizeof enemy_hits);
  w.mario.state=4;w.mario.step=0;w.mario.y-=40;
  actor_t *a=&w.room.actors[0];a->handler=(void*)enemy;
  a->box_y0=w.mario.y+23;a->box_y1=w.mario.y+50;
  mario_field(&w,0);
  assert(enemy_hits[3]==1 && a->state==4 && w.g.player[0].score==100);
  assert(w.mario.state_group==3 && !w.mario.fell);
  start(cheat?HM_CHEAT_INVINCIBLE:0,0);overlap(10);w.g.player[0].power=2;
  a=&w.room.actors[0];a->handler=(void*)enemy;a->box_x0=128;a->box_x1=200;
  memset(enemy_hits,0,sizeof enemy_hits);
  for(int i=0;i<3;i++)mario_field(&w,IN_ITEM);
  assert(w.fireball[0].state==3 || w.fireball[1].state==3);
  fx_update(&w);
  assert(enemy_hits[4]==1 && a->state==4 && w.g.player[0].score==100);
  assert(w.g.player[0].power==2 && w.mario.state_group!=6);
 }
 /* Bowser also has a direct door/lift capture outside actor collision damage. */
 for(int cheat=0;cheat<2;cheat++){
  start(cheat?HM_CHEAT_INVINCIBLE:0,4);
  w.room.n_actors=1;actor_t *boss=&w.room.actors[0];memset(boss,0,sizeof *boss);
  boss->type=0x3b;bowser(boss,&w.g);
  boss->state=3;boss->cmd=2;boss->step=0;boss->dir=0;
  boss->x=52;boss->y=50;boss->x_min=100;boss->x_max=2;
  bowser(boss,&w.g);
  assert(w.mario.state_group==(cheat?4:7));
  assert(w.g.unk_53a==(cheat?0:2));
 }
 /* Items retain normal pickup handling, even with the harmless-pass-through cheat. */
 for(int type=1;type<=9;type++){
  start(HM_CHEAT_INVINCIBLE,0);overlap(type);collected=0;mario_field(&w,0);
  assert(collected==1);
  if(type==1)assert(w.mario.invincible==1);
  if(type==2)assert(w.g.player[0].lives==4);
  if(type==3)assert(w.g.player[0].power==2);
  if(type==7)assert(w.g.player[0].power==1);
 }
 /* Every real room accepts 500. Timer HUD supports three digits and tally handles
    all 500 ticks using the original hotel-specific rate (not a 200 cap). */
 char path[512],tag[16];snprintf(path,sizeof path,"%s/stages.txt",argv[1]);FILE *list=fopen(path,"r");assert(list);
 int stages=0;
 while(fgets(tag,sizeof tag,list)){
  tag[strcspn(tag,"\r\n")]=0;
  for(int upper=0;upper<2;upper++){
   snprintf(path,sizeof path,"%s/stage_%s%s.inf",argv[1],tag,upper?"_u":"");FILE *f=fopen(path,"rb");if(!f)continue;
   unsigned char inf[2048];size_t n=fread(inf,1,sizeof inf,f);fclose(f);assert(n>=146);
   world_start(&w,inf,NULL,tag[0]-'0',atoi(tag+2)-1,0);int normal=w.g.time_left;
   assert(normal==199 || normal==200);
   world_set_cheats(&w,HM_CHEAT_TIME);
   assert(w.g.time_left==500 && *(int32_t*)(w.stage_info+8)==500);
   world_sprite hud[40];assert(world_hud(&w,hud,40)<=40);
   hm_campaign c;campaign_start(&c,1);w.status=WORLD_CLEARED;w.g.player[0].score=1000;
   campaign_capture(&c,&w);assert(c.bonus_time==500);
   int ticks=0;while(campaign_bonus_tick(&c))ticks++;
   assert(ticks==500 && c.player[0].score==1000+500*(tag[0]-'0')*10);
   uint32_t before=c.player[0].score;campaign_finish_bonus(&c);
   assert(c.player[0].score==before+c.special_bonus);
   campaign_finish_bonus(&c);assert(c.player[0].score==before+c.special_bonus);
   stages++;
  }
 }
 fclose(list);assert(stages==110);
 puts("Cheats passed: enemy/projectile damage blocked, stomps/fireballs retained, items collected, normal damage retained, 110 room timers/HUD/500-tick scoring checked.");
}
