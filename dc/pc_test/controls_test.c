#include "world.h"
#include <assert.h>
#include <stdio.h>
static world w;
static void start(int power) {
    world_start(&w,NULL,NULL,1,0,0);
    w.g.player[0].power=power;w.g.frame=0;
    w.g.held_b1=w.g.held_b2=10;
    w.mario.state_group=0;w.mario.state=6;w.mario.floor=2;
    w.mario.x=w.mario.old_x=100;w.mario.y=hm_floor_y[2]-30;
    w.mario.anim_speed=5;w.mario.hurt_timer=-1;
}
static void frames(int n,unsigned input) {while(n--)mario_field(&w,input);}
static int shots(void) {return (w.fireball[0].state==3)+(w.fireball[1].state==3);}
int main(void) {
    start(0);w.mario.state=0;frames(20,IN_RIGHT);int walk=w.mario.x-100;
    start(0);w.mario.state=0;frames(20,IN_RIGHT|IN_RUN);int run=w.mario.x-100;
    assert(walk==20 && run==40 && !shots());
    start(2);frames(3,IN_ITEM);assert(shots()==1 && w.mario.state_group==0);
    start(2);frames(3,IN_ITEM);frames(3,IN_ITEM|IN_B1);
    assert(w.mario.state_group==3 && shots()==1); /* fire pose must not swallow jump */
    start(0);frames(3,IN_ITEM);assert(!shots() && w.mario.state_group==0);
    start(2);frames(3,IN_B1);assert(w.mario.state_group==3 && !shots());
    start(2);frames(3,IN_B1|IN_ITEM);assert(w.mario.state_group==3 && shots()==1);
    start(2);frames(3,IN_B1|IN_B2);assert(!shots()); /* old chord no longer fires */
    start(2);w.mario.state_group=3;w.mario.state=2;w.mario.step=8;
    w.mario.prev_state=6;frames(3,IN_ITEM);assert(shots()==1 && w.mario.state_group==3);
    start(2);frames(30,IN_ITEM);assert(shots()==2); /* original two-shot limit */
    puts("Controls passed: walk/run, dedicated item on ground/in air, power requirement, jump+fire, old chord disabled and shot limit.");
}
