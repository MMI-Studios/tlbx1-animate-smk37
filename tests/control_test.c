#include "control.h"
#include "animate_parameters.h"
#include <assert.h>
#include <stdio.h>
static ac_action last;static unsigned calls;static void emit(void*u,const ac_action*a){(void)u;last=*a;calls++;}
static void button(ac_controls*c,unsigned b,uint32_t ms){ac_control_button(c,b,1,ms,emit,0);ac_control_button(c,b,0,ms,emit,0);}
static void pad(ac_controls*c,unsigned p){ac_control_pad(c,p,1,emit,0);ac_control_pad(c,p,0,emit,0);}
static void mode(ac_controls*c,unsigned p){ac_control_button(c,AC_SHIFT,1,10,emit,0);pad(c,p);ac_control_button(c,AC_SHIFT,0,11,emit,0);}
int main(void){ac_controls c;ac_control_init(&c);mode(&c,0);pad(&c,9);assert(c.lane==9&&c.view==AC_VIEW_PARAM);c.param_bank=0;c.shift=1;last.type=AC_LANE_SELECT;ac_control_encoder(&c,0,1,emit,0);assert(last.type==AC_PARAM_ADJUST&&last.index==0);last.type=AC_LANE_SELECT;ac_control_encoder(&c,1,1,emit,0);assert(last.type==AC_LANE_SELECT);c.shift=0;for(unsigned bank=0;bank<2;bank++){c.param_bank=bank;for(unsigned k=0;k<8;k++){ac_control_encoder(&c,k,1,emit,0);assert(last.type==AC_PARAM_ADJUST&&last.index==(bank==0&&k==0?20u:bank*8+k)&&last.lane==9);}}for(unsigned f=0;f<4;f++){ac_control_fader(&c,f,127,emit,0);assert(last.type==AC_PARAM_SET&&last.index==16+f);}pad(&c,3);assert(c.slot==3);mode(&c,4);assert(c.slot==0&&c.view==AC_VIEW_PARAM);
 mode(&c,3);button(&c,AC_LEFT,0);assert(c.mixer_bank==2);ac_control_fader(&c,1,50,emit,0);assert(last.lane==9&&last.type==AC_LEVEL_SET);unsigned before=calls;ac_control_fader(&c,2,50,emit,0);assert(calls==before);
 for(unsigned bank=0;bank<3;bank++){c.mixer_bank=bank;for(unsigned k=0;k<8;k++){ac_control_encoder(&c,k,-2,emit,0);assert(last.type==AC_PARAM_ADJUST&&last.slot==k/2&&last.index==(k%2?ANIMATE_PAN:ANIMATE_LEVEL)&&last.value==-2&&last.lane==9);}}
 mode(&c,1);c.step_page=7;ac_control_pad(&c,15,1,emit,0);c.lane=0;c.step_page=0;ac_control_encoder(&c,2,-1,emit,0);assert(last.type==AC_STEP_EDIT&&last.lane==9&&last.index==127&&last.slot==2);ac_control_pad(&c,15,0,emit,0);
 mode(&c,6);button(&c,AC_RIGHT,0);assert(c.step_page==1);ac_control_button(&c,AC_SHIFT,1,100,emit,0);button(&c,AC_RIGHT,101);assert(c.route_bank==1&&c.step_page==1);ac_control_button(&c,AC_SHIFT,0,102,emit,0);ac_control_encoder(&c,1,1,emit,0);assert(last.type==AC_RANDOM_ROUTE_ADJUST&&last.index==9);pad(&c,15);assert(last.type==AC_RANDOM_STEP_TOGGLE&&last.index==31);
 mode(&c,5);button(&c,AC_LEFT,0);assert(c.random_bank==2);before=calls;ac_control_encoder(&c,3,1,emit,0);assert(calls==before);ac_control_encoder(&c,2,1,emit,0);assert(last.index==18&&last.type==AC_RANDOM_MASK_SET);
 mode(&c,7);assert(c.view==AC_VIEW_MOD);button(&c,AC_RIGHT,0);ac_control_encoder(&c,0,1,emit,0);assert(last.type==AC_MOD_ADJUST&&last.index==8);
 button(&c,AC_GLOBE,0);assert(c.view==AC_VIEW_GLOBAL);button(&c,AC_LEFT,0);assert(c.global_page==5);button(&c,AC_GLOBE,0);assert(c.view==AC_VIEW_MOD);
 ac_control_button(&c,AC_SHIFT,1,200,emit,0);before=calls;button(&c,AC_REC,699);assert(calls==before);button(&c,AC_REC,700);assert(last.type==AC_STEP_CLEAR);button(&c,AC_REPEAT,701);assert(last.type==AC_HOLD_TOGGLE);ac_control_button(&c,AC_SHIFT,0,702,emit,0);button(&c,AC_REPEAT,703);assert(last.type==AC_REPEAT_TOGGLE);
 ac_control_init(&c);assert(c.view==AC_VIEW_HOME);pad(&c,3);assert(c.view==AC_VIEW_PARAM&&c.slot==3);
 ac_control_button(&c,AC_SHIFT,1,800,emit,0);button(&c,AC_GLOBE,801);ac_control_button(&c,AC_SHIFT,0,802,emit,0);assert(c.view==AC_VIEW_HOME);pad(&c,5);assert(c.view==AC_VIEW_MOD&&c.mod_bank==1);
 button(&c,AC_PATCH,803);assert(last.type==AC_PATCH_BROWSE);c.view=AC_VIEW_PATCHES;button(&c,AC_RIGHT,804);assert(last.type==AC_PATCH_SELECT&&last.value==1);button(&c,AC_PATCH,805);assert(last.type==AC_PATCH_LOAD_REQUEST);
 c.view=AC_VIEW_CONFIRM;before=calls;ac_control_encoder(&c,0,1,emit,0);ac_control_fader(&c,0,127,emit,0);button(&c,AC_PATCH,806);button(&c,AC_ARP,807);pad(&c,5);assert(calls==before);pad(&c,0);assert(last.type==AC_PATCH_CONFIRM);pad(&c,1);assert(last.type==AC_PATCH_CANCEL);button(&c,AC_PLAY,808);assert(last.type==AC_PLAY_TOGGLE);
 puts("control: existing controls, Home navigation, guarded patch requests and confirmation isolation passed");return 0;}
