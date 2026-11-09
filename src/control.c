#include "control.h"
#include "animate_parameters.h"
#include <string.h>
static void ac_send(ac_controls*c,ac_action_type type,unsigned index,int value,ac_emit emit,void*user){if(value>32767)value=32767;if(value< -32768)value=-32768;ac_action a={type,c->lane,c->slot,(uint8_t)index,(int16_t)value};if(emit)emit(user,&a);}
static void ac_view_set(ac_controls*c,ac_view view,ac_emit emit,void*u){c->view=view;c->edit_pads=0;ac_send(c,AC_VIEW_CHANGE,0,view,emit,u);}
void ac_control_init(ac_controls*c){memset(c,0,sizeof *c);c->view=AC_VIEW_HOME;}
void ac_control_button(ac_controls*c,unsigned b,int down,uint32_t ms,ac_emit emit,void*u){
 if(b>=AC_BUTTON_COUNT)return;
 uint32_t bit=1u<<b;int old=!!(c->buttons&bit);if(down)c->buttons|=bit;else c->buttons&=~bit;
 if(b==AC_SHIFT){if(down&&!old)c->shift_since=ms;c->shift=!!down;return;}if(!down||old)return;
 if(c->view==AC_VIEW_CONFIRM&&b!=AC_PLAY){if(b==AC_PARA&&!c->shift)ac_send(c,AC_PATCH_CONFIRM,0,0,emit,u);else if(b==AC_LEFT||(b==AC_GLOBE&&c->shift)){ac_send(c,AC_PATCH_CANCEL,0,0,emit,u);if(b==AC_GLOBE)ac_view_set(c,AC_VIEW_HOME,emit,u);}return;}
 switch(b){
 case AC_PLAY:ac_send(c,AC_PLAY_TOGGLE,0,0,emit,u);break;
 case AC_REC:if(!c->shift)ac_send(c,AC_RECORD_TOGGLE,0,0,emit,u);else if((uint32_t)(ms-c->shift_since)>=500)ac_send(c,AC_STEP_CLEAR,c->step_page,0,emit,u);break;
 case AC_ARP:ac_send(c,c->shift?AC_RANDOM_APPLY:AC_ARP_TOGGLE,0,0,emit,u);break;
 case AC_REPEAT:ac_send(c,c->shift?AC_HOLD_TOGGLE:AC_REPEAT_TOGGLE,0,0,emit,u);break;
 case AC_SCALE:if(!c->shift)ac_send(c,AC_SCALE_TOGGLE,0,0,emit,u);break;
 case AC_CHORD:ac_send(c,c->shift?AC_XY_TOGGLE:AC_CHORD_TOGGLE,0,0,emit,u);break;
 case AC_GLOBE:if(c->shift){ac_view_set(c,AC_VIEW_HOME,emit,u);break;}if(c->view==AC_VIEW_GLOBAL)ac_view_set(c,c->return_view,emit,u);else{c->return_view=c->view==AC_VIEW_LANES?AC_VIEW_PARAM:c->view;c->global_page=0;ac_view_set(c,AC_VIEW_GLOBAL,emit,u);}break;
 case AC_PARA:if(c->shift){c->mod_bank=0;ac_view_set(c,AC_VIEW_MOD,emit,u);}else ac_view_set(c,AC_VIEW_PARAM,emit,u);break;case AC_SEQ:ac_view_set(c,AC_VIEW_SEQ,emit,u);break;case AC_FX:ac_view_set(c,AC_VIEW_FX,emit,u);break;
 case AC_PATCH:ac_send(c,c->shift?AC_PATCH_SAVE_REQUEST:c->view==AC_VIEW_PATCHES?AC_PATCH_LOAD_REQUEST:AC_PATCH_BROWSE,0,0,emit,u);break;
 case AC_OCT_DOWN:case AC_OCT_UP:ac_send(c,c->shift?AC_LENGTH_ADJUST:AC_OCTAVE_ADJUST,0,(b==AC_OCT_UP?1:-1)*(c->shift?16:1),emit,u);break;
 case AC_LEFT:case AC_RIGHT:{int d=b==AC_RIGHT?1:-1;
  if(c->view==AC_VIEW_PATCHES)ac_send(c,AC_PATCH_SELECT,0,d,emit,u);
  else if(c->view==AC_VIEW_ROUTE&&c->shift)c->route_bank^=1;
  else if(c->view==AC_VIEW_SEQ||c->view==AC_VIEW_ROUTE)c->step_page=(uint8_t)((c->step_page+8+d)%8);
  else if(c->view==AC_VIEW_PARAM)c->param_bank^=1;
  else if(c->view==AC_VIEW_MIXER)c->mixer_bank=(uint8_t)((c->mixer_bank+3+d)%3);
  else if(c->view==AC_VIEW_MASK)c->random_bank=(uint8_t)((c->random_bank+3+d)%3);
  else if(c->view==AC_VIEW_MOD)c->mod_bank=(uint8_t)((c->mod_bank+3+d)%3);
  else if(c->view==AC_VIEW_VIS)c->visualizer=(uint8_t)((c->visualizer+3+d)%3);
  else if(c->view==AC_VIEW_GLOBAL)c->global_page=(uint8_t)((c->global_page+6+d)%6);
  break;}
 default:break;}
}
void ac_control_pad(ac_controls*c,unsigned p,int down,ac_emit emit,void*u){
 if(p>=16)return;
 uint16_t bit=(uint16_t)(1u<<p);int old=!!(c->pads&bit);if(down)c->pads|=bit;else{c->pads&=(uint16_t)~bit;c->edit_pads&=(uint16_t)~bit;}if(!down||old)return;
 if(c->view==AC_VIEW_CONFIRM){if(!c->shift&&p<2)ac_send(c,p==0?AC_PATCH_CONFIRM:AC_PATCH_CANCEL,0,0,emit,u);return;}
 if(!c->shift&&c->view==AC_VIEW_PATCHES){if(p<3)ac_send(c,p==0?AC_PATCH_LOAD_REQUEST:p==1?AC_PATCH_CANCEL:AC_PATCH_SAVE_REQUEST,0,0,emit,u);return;}
 if(c->shift){if(p==4){c->slot=(c->slot+1)%4;ac_send(c,AC_SLOT_SELECT,0,c->slot,emit,u);ac_view_set(c,AC_VIEW_PARAM,emit,u);return;}static const ac_view modes[8]={AC_VIEW_LANES,AC_VIEW_SEQ,AC_VIEW_PARAM,AC_VIEW_MIXER,AC_VIEW_PARAM,AC_VIEW_MASK,AC_VIEW_ROUTE,AC_VIEW_MOD};if(p==7)c->mod_bank=0;if(p<8)ac_view_set(c,modes[p],emit,u);else if(p==15)ac_view_set(c,AC_VIEW_VIS,emit,u);return;}
 if(c->view==AC_VIEW_HOME){if(p<4){c->slot=(uint8_t)p;ac_send(c,AC_SLOT_SELECT,0,p,emit,u);ac_view_set(c,AC_VIEW_PARAM,emit,u);}else if(p<6){c->mod_bank=(uint8_t)(p-4);ac_view_set(c,AC_VIEW_MOD,emit,u);}else if(p==8)ac_send(c,AC_PATCH_BROWSE,0,0,emit,u);else if(p==14)ac_send(c,AC_PATCH_SAVE_REQUEST,0,0,emit,u);else if(p==6)ac_view_set(c,AC_VIEW_MIXER,emit,u);else if(p==7)ac_view_set(c,AC_VIEW_SEQ,emit,u);else if(p==9){c->return_view=AC_VIEW_HOME;c->global_page=0;ac_view_set(c,AC_VIEW_GLOBAL,emit,u);}else if(p==10)ac_view_set(c,AC_VIEW_MASK,emit,u);else if(p==11)ac_view_set(c,AC_VIEW_ROUTE,emit,u);else if(p==12)ac_view_set(c,AC_VIEW_FX,emit,u);else if(p==15)ac_view_set(c,AC_VIEW_VIS,emit,u);}
 else if(c->view==AC_VIEW_LANES&&p<10){c->lane=(uint8_t)p;ac_send(c,AC_LANE_SELECT,0,p,emit,u);ac_view_set(c,AC_VIEW_PARAM,emit,u);}
 else if((c->view==AC_VIEW_PARAM||c->view==AC_VIEW_MASK||c->view==AC_VIEW_MOD)&&p<6){if(p<4){c->slot=(uint8_t)p;ac_send(c,AC_SLOT_SELECT,0,p,emit,u);if(c->view==AC_VIEW_MOD)ac_view_set(c,AC_VIEW_PARAM,emit,u);}else{c->mod_bank=(uint8_t)(p-4);ac_view_set(c,AC_VIEW_MOD,emit,u);}}
 else if(c->view==AC_VIEW_SEQ){c->edit_pads|=bit;c->edit_lane[p]=c->lane;c->edit_step[p]=c->step_page*16+p;ac_send(c,AC_STEP_TOGGLE,c->edit_step[p],0,emit,u);}
 else if(c->view==AC_VIEW_ROUTE)ac_send(c,AC_RANDOM_STEP_TOGGLE,c->step_page*16+p,0,emit,u);
 else if(c->view==AC_VIEW_MIXER&&p<4&&c->mixer_bank*4+p<10){ac_action a={AC_MUTE_TOGGLE,(uint8_t)(c->mixer_bank*4+p),c->slot,0,0};if(emit)emit(u,&a);}
}
void ac_control_encoder(ac_controls*c,unsigned k,int d,ac_emit emit,void*u){
 if(k>=8||!d)return;
 if(c->view==AC_VIEW_SEQ&&c->edit_pads&&!c->shift){if(k<4){unsigned p=0;while(!(c->edit_pads&(1u<<p)))p++;ac_action a={AC_STEP_EDIT,c->edit_lane[p],(uint8_t)k,c->edit_step[p],(int16_t)(d>32767?32767:d< -32768?-32768:d)};if(emit)emit(u,&a);}return;}
 if(c->shift){if(c->view==AC_VIEW_PARAM&&!c->param_bank&&k==0)ac_send(c,AC_PARAM_ADJUST,0,d,emit,u);return;}
 switch(c->view){
 case AC_VIEW_PATCHES:if(k==0)ac_send(c,AC_PATCH_SELECT,0,d,emit,u);break;
 case AC_VIEW_PARAM:ac_send(c,AC_PARAM_ADJUST,!c->param_bank&&k==0?20:c->param_bank*8+k,d,emit,u);break;
 case AC_VIEW_MASK:if(c->random_bank*8+k<19)ac_send(c,AC_RANDOM_MASK_SET,c->random_bank*8+k,d>0,emit,u);break;
 case AC_VIEW_ROUTE:if(!c->route_bank&&k<7)ac_send(c,AC_RANDOM_ROUTE_ADJUST,k,k>=2?d>0:d,emit,u);else if(c->route_bank&&k<4)ac_send(c,AC_RANDOM_ROUTE_ADJUST,8+k,d,emit,u);break;
 case AC_VIEW_MIXER:{ac_action a={AC_PARAM_ADJUST,c->lane,(uint8_t)(k/2),(uint8_t)(k%2?ANIMATE_PAN:ANIMATE_LEVEL),(int16_t)(d>32767?32767:d< -32768?-32768:d)};if(emit)emit(u,&a);break;}
 case AC_VIEW_GLOBAL:if(c->global_page<5)ac_send(c,AC_GLOBAL_ADJUST,c->global_page*8+k,d,emit,u);break;
 case AC_VIEW_MOD:ac_send(c,AC_MOD_ADJUST,c->mod_bank*8+k,d,emit,u);break;
 case AC_VIEW_FX:ac_send(c,AC_FX_ADJUST,k,d,emit,u);break;
 default:break;}
}
void ac_control_fader(ac_controls*c,unsigned f,unsigned value,ac_emit emit,void*u){if(f>=4)return;if(value>127)value=127;if(c->view==AC_VIEW_PARAM)ac_send(c,AC_PARAM_SET,16+f,(int)value,emit,u);else if(c->view==AC_VIEW_MIXER&&c->mixer_bank*4+f<10){ac_action a={AC_LEVEL_SET,(uint8_t)(c->mixer_bank*4+f),c->slot,0,(int16_t)value};if(emit)emit(u,&a);}}
