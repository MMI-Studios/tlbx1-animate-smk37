#include "display.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static float get(void*u,unsigned lane,unsigned slot,unsigned field,unsigned index){(void)u;(void)field;return(float)((lane+slot+index)%128);}
static const char*value(void*u,unsigned lane,unsigned slot,unsigned field,unsigned index){(void)u;(void)lane;(void)slot;(void)index;return field==AC_D_PARAM?"0.25s":0;}
static int label(const char*s){for(unsigned i=0;i<ac_display_count();i++){const ac_draw_command*c=ac_display_data()+i;if(c->type==1&&!strcmp(c->text,s))return 1;}return 0;}
static void mixer_labels(void){ac_controls c;ac_control_init(&c);c.view=AC_VIEW_MIXER;c.slot=3;c.lane=9;for(unsigned bank=0;bank<3;bank++){c.mixer_bank=bank;ac_display_draw(&c,0);const char*expected[]={"K1 A VOL","K2 A PAN","K3 B VOL","K4 B PAN","K5 C VOL","K6 C PAN","K7 D VOL","K8 D PAN"};for(unsigned i=0;i<8;i++)assert(label(expected[i]));}}
int main(void){mixer_labels();ac_controls c;ac_control_init(&c);ac_display_source source={0,get,0,value};assert(ac_display_stride()==56);for(unsigned view=0;view<=AC_VIEW_CONFIRM;view++)for(unsigned bank=0;bank<3;bank++){c.view=(ac_view)view;c.lane=9;c.slot=3;c.mixer_bank=c.random_bank=c.route_bank=c.global_page=c.mod_bank=bank;c.route_bank%=2;c.param_bank=bank%2;ac_display_draw(&c,&source);assert(ac_display_count()<=64);for(unsigned i=0;i<ac_display_count();i++){const ac_draw_command*p=ac_display_data()+i;assert(p->x+p->w<=240&&p->y+p->h<=240);if(p->type==1)assert(strlen(p->text)*12<=p->w);}}c.view=AC_VIEW_PARAM;ac_display_draw(&c,&source);assert(label("0.25S"));ac_display_draw(&c,0);assert(label("--"));puts("display: all mode/bank bounds including Home/Patches/Confirm, fractional labels and unavailable values passed");return 0;}
