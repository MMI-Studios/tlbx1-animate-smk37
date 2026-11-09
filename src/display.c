/* SPDX-License-Identifier: GPL-3.0-only
 * Original TLBX pixel font and hardware-independent projection. No runtime
 * coupling; unavailable host values remain visibly unavailable. */
#include "display.h"
#include "animate_parameters.h"
static const uint8_t display_font[96][5]={
 ['!'-32]={0,0,95,0,0},['?'-32]={2,1,81,9,6},
 ['A'-32]={126,9,9,9,126},['B'-32]={127,73,73,73,54},
 ['C'-32]={62,65,65,65,34},['D'-32]={127,65,65,34,28},
 ['E'-32]={127,73,73,73,65},['F'-32]={127,9,9,9,1},
 ['G'-32]={62,65,73,73,58},['H'-32]={127,8,8,8,127},
 ['I'-32]={0,65,127,65,0},['J'-32]={32,64,65,63,1},
 ['K'-32]={127,8,20,34,65},['L'-32]={127,64,64,64,64},
 ['M'-32]={127,2,12,2,127},['N'-32]={127,2,4,8,127},
 ['O'-32]={62,65,65,65,62},['P'-32]={127,9,9,9,6},
 ['Q'-32]={62,65,81,33,94},['R'-32]={127,9,25,41,70},
 ['S'-32]={38,73,73,73,50},['T'-32]={1,1,127,1,1},
 ['U'-32]={63,64,64,64,63},['V'-32]={31,32,64,32,31},
 ['W'-32]={127,32,24,32,127},['X'-32]={99,20,8,20,99},
 ['Y'-32]={3,4,120,4,3},['Z'-32]={97,81,73,69,67},
 ['0'-32]={62,81,73,69,62},['1'-32]={0,66,127,64,0},
 ['2'-32]={98,81,73,73,70},['3'-32]={34,65,73,73,54},
 ['4'-32]={24,20,18,127,16},['5'-32]={39,69,69,69,57},
 ['6'-32]={62,73,73,73,50},['7'-32]={1,113,9,5,3},
 ['8'-32]={54,73,73,73,54},['9'-32]={38,73,73,73,62},
 ['-'-32]={8,8,8,8,8},['+'-32]={8,8,62,8,8},
 ['.'-32]={0,96,96,0,0},[':'-32]={0,54,54,0,0},
 ['/'-32]={64,32,8,2,1},['='-32]={20,20,20,20,20},
 ['%'-32]={99,19,8,100,99},['*'-32]={20,8,62,8,20},
 [','-32]={0,80,48,0,0},['<'-32]={8,20,34,65,0},['>'-32]={0,65,34,20,8},
 ['('-32]={0,28,34,65,0},[')'-32]={0,65,34,28,0},['_'-32]={64,64,64,64,64},
 ['#'-32]={20,127,20,127,20}
};
static ac_draw_command commands[64];static unsigned used;
const ac_draw_command *ac_display_data(void){return commands;}
unsigned ac_display_count(void){return used;}
unsigned ac_display_stride(void){return sizeof(ac_draw_command);}
const uint8_t *ac_display_font(void){return &display_font[0][0];}
/* TLBX brand palette (RGB565), from assets/branding: charcoal #1A1A22,
 * purple #C53BFF, deep purple #4A0D7A, orange #E06E26, off-white #F6F1FF.
 * Text cells keep the shared rasterizer's dark background, so colour is
 * carried by ink, accent bars and rules rather than filled text bands. */
enum {AC_BG=0x0841,AC_INK=0xF79F,AC_PURPLE=0xC1DF,AC_DEEP=0x486F,AC_ORANGE=0xE364,AC_MUTED=0xACD8,AC_DIM=0x5AAD,AC_SLATE=0x2946,AC_BEAT=0x3A0B};
enum {AC_ROW0=30,AC_ROWH=18,AC_LEGEND=186,AC_LEGEND2=204,AC_FOOT=224};
static void ac_rect(unsigned x,unsigned y,unsigned w,unsigned h,unsigned color){if(used>=64||x>=240||y>=240||!w||!h)return;ac_draw_command*c=&commands[used++];c->type=0;c->x=x;c->y=y;c->w=w>240-x?240-x:w;c->h=h>240-y?240-y:h;c->color=color;c->text[0]=0;}
static void ac_textc(unsigned x,unsigned y,unsigned w,unsigned color,const char*s){if(used>=64||x>=240||y>226)return;ac_draw_command*c=&commands[used++];c->type=1;c->x=x;c->y=y;c->w=w>240-x?240-x:w;c->h=14;c->color=color;unsigned i=0;while(s&&*s&&i<31&&i<c->w/12){unsigned ch=(unsigned char)*s++;if(ch>='a'&&ch<='z')ch-=32;c->text[i++]=(char)(ch<32||ch>127?'?':ch);}c->text[i]=0;}
static unsigned ac_len(const char*s,unsigned max){unsigned n=0;while(s&&s[n]&&n<max)n++;return n;}
/* Right-aligned text ending at xr; never wider than max pixels. */
static void ac_text_right(unsigned xr,unsigned y,unsigned max,unsigned color,const char*s){unsigned n=ac_len(s,max/12<31?max/12:31);if(!n)return;ac_textc(xr-n*12,y,n*12,color,s);}
static void ac_format(char*text,int n){char reverse[12];unsigned k=0,j=0,v=n<0?(unsigned)(-(n+1))+1:(unsigned)n;if(n<0)text[k++]='-';do{reverse[j++]=(char)('0'+v%10);v/=10;}while(v);while(j)text[k++]=reverse[--j];text[k]=0;}
static void ac_numberc(unsigned x,unsigned y,unsigned w,unsigned color,int n){char text[16];ac_format(text,n);ac_textc(x,y,w,color,text);}
static float ac_get(const ac_display_source*s,unsigned lane,unsigned slot,unsigned field,unsigned index){return s&&s->get?s->get(s->user,lane,slot,field,index):0;}
static const char*ac_value(const ac_display_source*s,unsigned lane,unsigned slot,unsigned field,unsigned index){return s&&s->value?s->value(s->user,lane,slot,field,index):0;}
/* Thin position bar under a parameter row, using the descriptive UI range.
 * Bipolar ranges fill from the centre. Unknown/placeholder ranges draw none. */
static void ac_bar(unsigned y,float v,const animate_parameter_metadata*m){
 if(!m||!(m->ui_max>m->ui_min)||!(v==v))return;
 float t=(v-m->ui_min)/(m->ui_max-m->ui_min);
 if(t<0)t=0;
 if(t>1)t=1;
 const unsigned x=8,w=224;ac_rect(x,y,w,2,AC_SLATE);
 if(m->ui_min<0&&m->ui_max>0){float z=(0-m->ui_min)/(m->ui_max-m->ui_min);unsigned a=x+(unsigned)(z*w+.5f),b=x+(unsigned)(t*w+.5f);
  if(b>a)ac_rect(a,y,b-a,2,AC_PURPLE);
  else if(a>b)ac_rect(b,y,a-b,2,AC_PURPLE);
  ac_rect(a,y-1,1,4,AC_MUTED);}
 else{unsigned f=(unsigned)(t*w+.5f);if(f)ac_rect(x,y,f,2,AC_PURPLE);}
}
static void ac_row(const ac_display_source*s,unsigned lane,unsigned slot,unsigned row,unsigned field,unsigned index,const char*fallback){
 unsigned y=AC_ROW0+row*AC_ROWH;char number[16];
 const char*value=ac_value(s,lane,slot,field,index);
 if(!value){if(!s||!s->get)value="--";else{float v=ac_get(s,lane,slot,field,index);if(!(v==v)||v>99999||v< -9999)value="--";else{ac_format(number,(int)v);value=number;}}}
 unsigned vn=ac_len(value,field==AC_D_PARAM&&index==20?12:6);
 const char*label=s&&s->label?s->label(s->user,lane,slot,field,index):0;if(!label||!*label)label=fallback;
 unsigned room=224-vn*12;if(room>12)room-=12;ac_textc(8,y,room,AC_MUTED,label);
 ac_text_right(232,y,vn>6?vn*12:72,AC_INK,value);
 if(field==AC_D_PARAM&&index==20&&s&&s->get){animate_parameter_metadata m=*animate_parameter_info(0);m.ui_min=0;m.ui_max=ac_get(s,lane,slot,field,21)-1;ac_bar(y+15,ac_get(s,lane,slot,field,index),&m);}
 else if(field==AC_D_PARAM&&s&&s->get)ac_bar(y+15,ac_get(s,lane,slot,field,index),animate_parameter_info(index));
}
static void ac_header(const ac_controls*c,const ac_display_source*s,const char*title,unsigned title_color){
 ac_rect(0,0,4,20,AC_ORANGE);ac_textc(10,3,128,title_color,title);
 if(ac_get(s,c->lane,c->slot,AC_D_STATUS,0))ac_textc(140,3,12,AC_ORANGE,"*");
 char lane[4]={'L',0,0,0};char digits[4];ac_format(digits,c->lane+1);lane[1]=digits[0];lane[2]=digits[1];
 ac_textc(156,3,36,AC_ORANGE,lane);char slot[2]={(char)('A'+c->slot%4),0};ac_textc(194,3,12,AC_PURPLE,slot);
 char page[3]={'P',(char)('1'+c->step_page%8),0};ac_textc(212,3,24,AC_MUTED,page);
 ac_rect(0,20,240,1,AC_PURPLE);ac_rect(0,21,240,1,AC_DEEP);
}
static void ac_steps(const ac_controls*c,const ac_display_source*s,unsigned field,unsigned on){
 int playing=ac_get(s,c->lane,c->slot,AC_D_PLAYING,0)!=0;unsigned head=(unsigned)ac_get(s,c->lane,c->slot,AC_D_PLAYHEAD,0);
 for(unsigned k=0;k<16;k++){unsigned step=c->step_page*16+k;unsigned color=k%4?AC_SLATE:AC_BEAT;
  if(ac_get(s,c->lane,c->slot,field,step))color=on;
  if(field==AC_D_STEP&&playing&&step==head)color=AC_ORANGE;
  ac_rect(5+k%8*29,AC_LEGEND+k/8*15,25,10,color);}
}
void ac_display_draw(const ac_controls*c,const ac_display_source*s){
 static const char*views[]={"SEQUENCER","PARAM","MIXER","LANES","RANDOM MASK","RANDOM ROUTE","VISUALIZER","GLOBAL","MODULATION","EFFECTS","ANIMATE","PATCHES","CONFIRM"};
 static const char*globals[]={"AUDIO","TIMING","ENGINE","SCALE","CHORD","OVERLOAD"};static const char*mods[]={"VECTOR X","VECTOR Y","SLOT WT LFO"};
 used=0;ac_rect(0,0,240,240,AC_BG);
 char title[12]="SLOT A";if(c->view==AC_VIEW_PARAM)title[5]=(char)('A'+c->slot%4);
 const char*name=c->shift?"SHIFT":c->view==AC_VIEW_PARAM?title:c->view==AC_VIEW_MOD?mods[c->mod_bank%3]:views[c->view<=AC_VIEW_CONFIRM?c->view:0];
 ac_header(c,s,name,c->shift?AC_ORANGE:AC_INK);
 if(c->shift){
  /* Paired shortcuts in two columns, then full-width long actions. */
  static const char*pk[10]={"GLOBE","PATCH","P1","P2","P3","P4","P6","P7","P8","P16"};
  static const char*pa[10]={"HOME","SAVE","LANES","SEQ","PARAM","MIXER","MASK","ROUTE","MOD","VIS"};
  static const char*lk[4]={"P5","ARP","REPEAT","REC"};static const char*la[4]={"SLOT ENVELOPE","APPLY RANDOM","HOLD","HELD: CLEAR"};
  for(unsigned i=0;i<10;i++){unsigned x=i%2?126:8,y=AC_ROW0+(i/2)*AC_ROWH,kn=ac_len(pk[i],6);ac_textc(x,y,kn*12,AC_ORANGE,pk[i]);ac_textc(x+kn*12+6,y,108-kn*12,AC_INK,pa[i]);}
  for(unsigned i=0;i<4;i++){unsigned y=AC_ROW0+(5+i)*AC_ROWH,kn=ac_len(lk[i],6);ac_textc(8,y,kn*12,AC_ORANGE,lk[i]);ac_textc(8+kn*12+6,y,220-kn*12,AC_INK,la[i]);}
 }
 else if(c->view==AC_VIEW_HOME){
  const char*patch=ac_value(s,c->lane,c->slot,AC_D_STATUS,6);ac_textc(8,AC_ROW0,224,AC_INK,patch?patch:"ANIMATE PATCH");
  int hold=ac_get(s,c->lane,c->slot,AC_D_HOLD,0)!=0,arp=ac_get(s,c->lane,c->slot,AC_D_GLOBAL,17)!=0,rep=ac_get(s,c->lane,c->slot,AC_D_GLOBAL,18)!=0;
  ac_textc(8,AC_ROW0+20,108,hold?AC_PURPLE:AC_DIM,hold?"HOLD ON":"HOLD OFF");ac_textc(124,AC_ROW0+20,108,arp||rep?AC_ORANGE:AC_DIM,arp?"ARP ON":rep?"REPEAT ON":"ARP OFF");
  ac_rect(8,AC_ROW0+39,224,1,AC_SLATE);
  static const char*menu[16]={"SLOT A","SLOT B","SLOT C","SLOT D","X MOD","Y MOD","MIX","SEQ","PATCH","GLOBAL","MASK","ROUTE","FX","","SAVE","VIS"};
  for(unsigned i=0;i<16;i++){if(!*menu[i])continue;unsigned col=i%2,y=78+(i/2)*17,x=col?124:8;char n[4];ac_format(n,(int)i+1);ac_text_right(x+24,y,24,AC_ORANGE,n);ac_textc(x+30,y,84,AC_INK,menu[i]);}
 }
 else if(c->view==AC_VIEW_PATCHES){
  const char*patch=ac_value(s,c->lane,c->slot,AC_D_STATUS,1);ac_textc(8,AC_ROW0+4,224,AC_INK,patch?patch:"SELECT PATCH");
  int factory=ac_get(s,c->lane,c->slot,AC_D_STATUS,1)<22,saved=ac_get(s,c->lane,c->slot,AC_D_STATUS,2)!=0;
  ac_textc(8,AC_ROW0+24,224,factory?AC_MUTED:saved?AC_PURPLE:AC_DIM,factory?"FACTORY PATCH":saved?"USER SLOT: SAVED":"USER SLOT: EMPTY");
  ac_rect(8,AC_ROW0+46,224,1,AC_SLATE);
  ac_textc(8,92,228,AC_MUTED,"ARROWS / K1 BROWSE");
  ac_textc(8,118,60,AC_ORANGE,"PAD 1");ac_textc(80,118,152,AC_INK,"LOAD");
  ac_textc(8,138,60,AC_ORANGE,"PAD 2");ac_textc(80,138,152,AC_INK,"HOME");
  ac_textc(8,158,60,AC_ORANGE,"PAD 3");ac_textc(80,158,152,AC_INK,"SAVE");
  ac_textc(8,AC_LEGEND,228,AC_DIM,"BROWSE KEEPS SOUND");
 }
 else if(c->view==AC_VIEW_CONFIRM){
  int save=ac_get(s,c->lane,c->slot,AC_D_STATUS,3)==2;ac_textc(8,AC_ROW0+4,228,AC_ORANGE,save?"SAVE SOUND PATCH?":"LOAD SOUND PATCH?");
  const char*patch=ac_value(s,c->lane,c->slot,AC_D_STATUS,1);ac_textc(8,AC_ROW0+28,228,AC_INK,patch?patch:"PATCH");
  ac_textc(8,AC_ROW0+56,228,AC_MUTED,save?(ac_get(s,c->lane,c->slot,AC_D_STATUS,2)?"OVERWRITE THIS SLOT":"SAVE TO EMPTY SLOT"):(ac_get(s,c->lane,c->slot,AC_D_STATUS,0)?"REPLACE UNSAVED EDIT":"REPLACE SOUND"));
  ac_rect(8,AC_ROW0+80,224,1,AC_SLATE);
  ac_textc(8,128,228,AC_PURPLE,"PAD 1 / PARA: YES");ac_textc(8,152,228,AC_DIM,"PAD 2 / LEFT: NO");
 }
 else if(c->view==AC_VIEW_LANES){ac_textc(8,AC_ROW0+20,228,AC_INK,"PADS 1-10 SELECT");ac_textc(8,AC_ROW0+44,228,AC_MUTED,"TEN ANIMATE LANES");
  for(unsigned k=0;k<10;k++)ac_rect(8+k*23,AC_ROW0+80,19,19,k==c->lane?AC_ORANGE:AC_SLATE);}
 else if(c->view==AC_VIEW_PARAM){for(unsigned k=0;k<8;k++){unsigned p=c->param_bank*8+k;if(p==0)p=20;const animate_parameter_metadata*m=animate_parameter_info(p);ac_row(s,c->lane,c->slot,k,AC_D_PARAM,p,m?m->label:p==20?"WAVE":"PARAM");}
  ac_textc(8,AC_LEGEND,228,AC_MUTED,"FADERS A D S R");
  for(unsigned k=0;k<4;k++){const char*v=ac_value(s,c->lane,c->slot,AC_D_PARAM,16+k);if(v)ac_textc(8+k*58,AC_LEGEND2,56,AC_PURPLE,v);else if(s&&s->get)ac_numberc(8+k*58,AC_LEGEND2,56,AC_PURPLE,(int)ac_get(s,c->lane,c->slot,AC_D_PARAM,16+k));else ac_textc(8+k*58,AC_LEGEND2,56,AC_PURPLE,"--");}}
 else if(c->view==AC_VIEW_MIXER){for(unsigned k=0;k<8;k++){unsigned slot=k/2,param=k%2?ANIMATE_PAN:ANIMATE_LEVEL;char label[]="K1 A VOL";label[1]=(char)('1'+k);label[3]=(char)('A'+slot);if(k%2){label[5]='P';label[6]='A';label[7]='N';}ac_display_source row=s?*s:(ac_display_source){0};row.label=0;ac_row(&row,c->lane,slot,k,AC_D_PARAM,param,label);}
  char range[8],hi[4];ac_format(range,c->mixer_bank*4+1);ac_format(hi,c->mixer_bank==2?10:c->mixer_bank*4+4);unsigned n=ac_len(range,4);range[n++]='-';for(unsigned i=0;hi[i]&&n<7;i++)range[n++]=hi[i];range[n]=0;
  ac_textc(8,AC_LEGEND,144,AC_MUTED,"FADER LANES");ac_text_right(232,AC_LEGEND,72,AC_ORANGE,range);}
 else if(c->view==AC_VIEW_MASK){for(unsigned k=0;k<8;k++){unsigned p=c->random_bank*8+k;if(p<19)ac_row(s,c->lane,c->slot,k,AC_D_RANDOM_MASK,p,"MASK CATEGORY");}ac_textc(8,AC_LEGEND,144,AC_MUTED,"PADS A-D  BANK");ac_numberc(186,AC_LEGEND,36,AC_ORANGE,c->random_bank+1);}
 else if(c->view==AC_VIEW_ROUTE){static const char*route[]={"MODE","SCOPE","NOTE","VELOCITY","GATE","PROBABILITY","STEP ON/OFF"};static const char*targets[]={"LANE TARGET","SLOT TARGET","NOTE AMOUNT","PARAM AMOUNT"};
  for(unsigned k=0;k<(c->route_bank?4u:7u);k++)ac_row(s,c->lane,c->slot,k,AC_D_ROUTE,c->route_bank?8+k:k,c->route_bank?targets[k]:route[k]);
  if(!c->route_bank)ac_steps(c,s,AC_D_SELECTED,AC_PURPLE);else ac_textc(8,AC_LEGEND,228,AC_MUTED,"SHIFT ARROWS: ROUTE");}
 else if(c->view==AC_VIEW_GLOBAL||c->view==AC_VIEW_MOD||c->view==AC_VIEW_FX){unsigned field=c->view==AC_VIEW_GLOBAL?AC_D_GLOBAL:c->view==AC_VIEW_MOD?AC_D_MOD:AC_D_FX,bank=c->view==AC_VIEW_GLOBAL?c->global_page:c->view==AC_VIEW_MOD?c->mod_bank:0;
  for(unsigned k=0;k<8;k++)ac_row(s,c->lane,c->slot,k,field,(c->view==AC_VIEW_GLOBAL&&bank==5?48:bank*8)+k,"");
  if(c->view==AC_VIEW_GLOBAL){ac_textc(8,AC_LEGEND,144,AC_MUTED,globals[bank%6]);for(unsigned k=0;k<6;k++)ac_rect(164+k*12,AC_LEGEND+4,8,6,k==bank%6?AC_ORANGE:AC_SLATE);}
  else ac_textc(8,AC_LEGEND,228,AC_MUTED,c->view==AC_VIEW_MOD?mods[bank%3]:"TRACK FX");}
 else if(c->view==AC_VIEW_VIS){static const char*names[]={"OSCILLOSCOPE","SPECTRUM","VECTORSCOPE"};ac_textc(8,AC_ROW0,228,AC_MUTED,names[c->visualizer%3]);for(unsigned k=0;k<3;k++)ac_rect(200+k*12,AC_ROW0+4,8,6,k==c->visualizer%3?AC_ORANGE:AC_SLATE);
  if(used<64){ac_draw_command*p=&commands[used++];p->type=2;p->x=8;p->y=52;p->w=224;p->h=160;p->color=AC_PURPLE;p->text[0]=(char)c->visualizer;}}
 else if(c->view==AC_VIEW_SEQ){if(c->edit_pads){unsigned p=0;while(!(c->edit_pads&(1u<<p)))p++;static const char*fields[]={"NOTE","VELOCITY","GATE","PROBABILITY"};for(unsigned k=0;k<4;k++)ac_row(s,c->edit_lane[p],c->slot,k,AC_D_STEP_FIELD,c->edit_step[p]*4+k,fields[k]);ac_textc(8,AC_ROW0+4*AC_ROWH+12,144,AC_ORANGE,"EDIT STEP");ac_numberc(150,AC_ROW0+4*AC_ROWH+12,48,AC_INK,c->edit_step[p]+1);}
  else{ac_textc(8,AC_ROW0+20,228,AC_INK,"PADS TOGGLE STEPS");ac_textc(8,AC_ROW0+40,228,AC_MUTED,"HOLD PAD + K1-4");ac_textc(8,AC_ROW0+76,228,AC_MUTED,"ARROWS: STEP PAGE");
   for(unsigned k=0;k<8;k++)ac_rect(8+k*28,AC_ROW0+110,22,4,k==c->step_page%8?AC_ORANGE:AC_SLATE);}
  ac_steps(c,s,AC_D_STEP,AC_PURPLE);}
 ac_rect(0,AC_FOOT-5,240,1,AC_SLATE);
 const char*notice=ac_value(s,c->lane,c->slot,AC_D_STATUS,5);
 int playing=ac_get(s,c->lane,c->slot,AC_D_PLAYING,0)!=0,held=ac_get(s,c->lane,c->slot,AC_D_HOLD,0)!=0;
 if(notice&&*notice)ac_textc(8,AC_FOOT,228,AC_ORANGE,notice);
 else if(c->view==AC_VIEW_HOME){int edited=ac_get(s,c->lane,c->slot,AC_D_STATUS,0)!=0;ac_textc(8,AC_FOOT,228,edited?AC_ORANGE:playing?AC_PURPLE:AC_MUTED,edited?"EDITED - NOT SAVED":playing?"PLAY - SOUND CLEAN":"STOP - SOUND CLEAN");}
 else if(!c->shift&&(c->view==AC_VIEW_PARAM||c->view==AC_VIEW_MOD)&&!held&&!playing)ac_textc(8,AC_FOOT,228,AC_DIM,"1-4:A-D 5:X 6:Y");
 else if(c->view==AC_VIEW_MASK||c->view==AC_VIEW_ROUTE)ac_textc(8,AC_FOOT,228,AC_MUTED,"SHIFT ARP: APPLY");
 else ac_textc(8,AC_FOOT,228,playing?AC_ORANGE:held?AC_PURPLE:AC_MUTED,held?(playing?"HOLD  PLAY":"HOLD  STOP"):playing?"PLAY":"STOP");
}
