/* SPDX-License-Identifier: GPL-3.0-only
 * See splash.h. Geometry follows icon-concept_v2_b.svg in its 256-unit space:
 * tile 8..248 r44 #1A1A22 with #C53BFF outer glow (sigma 3), orange 35.5..220.5
 * r20 #E06E26, deep purple 54.31..201.69 r20 #4A0D7A, and a soft #C53BFF orb.
 * No libm: freestanding builds use local sqrt/exponential approximations. */
#include "splash.h"
#include "display.h"
#define S_SCALE 0.5f            /* 256-unit icon drawn at 128 px */
#define S_LX 56.f               /* icon box origin on the 240x240 panel */
#define S_LY 18.f
typedef struct {float r,g,b;} s_rgb;
static const s_rgb S_BG={8.f/255,8.f/255,8.f/255},S_TILE={26.f/255,26.f/255,34.f/255},
 S_ORANGE={224.f/255,110.f/255,38.f/255},S_DEEP={74.f/255,13.f/255,122.f/255},
 S_PURPLE={197.f/255,59.f/255,255.f/255},S_CORE={226.f/255,150.f/255,255.f/255},
 S_INK={246.f/255,241.f/255,255.f/255},S_SLATE={42.f/255,42.f/255,54.f/255},S_DIM={90.f/255,84.f/255,104.f/255};
static float s_clamp(float v){return v<0?0:v>1?1:v;}
static float s_sqrt(float v){if(v<=0)return 0;union{float f;uint32_t i;}u={v};u.i=0x1fbd1df5u+(u.i>>1);float x=u.f;x=.5f*(x+v/x);return .5f*(x+v/x);}
/* exp(-x) for x>=0, accurate to ~1e-3 and monotonic: enough for 8-bit glow. */
static float s_expn(float x){if(x>12)return 0;float t=1+x/16;t*=t;t*=t;t*=t;t*=t;return 1/t;}
static float s_ease(float t){t=s_clamp(t);float u=1-t;return 1-u*u*u;}
static float s_phase(unsigned f,float a,float b){return s_clamp(((float)f-a)/(b-a));}
static s_rgb s_mix(s_rgb a,s_rgb b,float t){s_rgb o={a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t};return o;}
/* Signed distance (icon units) to a centred rounded square of half-size h. */
static float s_round(float u,float v,float c,float h,float r){float qx=(u<c?c-u:u-c)-h+r,qy=(v<c?c-v:v-c)-h+r;float ox=qx>0?qx:0,oy=qy>0?qy:0;float in=qx>qy?qx:qy;return s_sqrt(ox*ox+oy*oy)+(in<0?in:0)-r;}
static uint16_t s_565(s_rgb c){unsigned r=(unsigned)(s_clamp(c.r)*31+.5f),g=(unsigned)(s_clamp(c.g)*63+.5f),b=(unsigned)(s_clamp(c.b)*31+.5f);return(uint16_t)(r<<11|g<<5|b);}
static float s_cover(float d_units){return s_clamp(.5f-d_units*S_SCALE);}
/* Per-frame animation state. */
typedef struct {float tile,orange,deep,orb,pulse,word,sub,bar;} s_anim;
static s_anim s_at(unsigned f){s_anim a;a.tile=s_ease(s_phase(f,0,5));a.orange=s_ease(s_phase(f,3,9));a.deep=s_ease(s_phase(f,5,11));
 a.orb=s_ease(s_phase(f,8,16));float p=s_phase(f,16,40);float tri=p*4;tri-=(float)(int)tri;tri=tri<.5f?tri*2:2-tri*2;a.pulse=f>=16?tri:0;
 a.word=s_phase(f,12,19);a.sub=s_ease(s_phase(f,17,23));a.bar=s_clamp(((float)f+1)/(float)AF_SPLASH_FRAMES);return a;}
static s_rgb s_logo(float px,float py,const s_anim*a,s_rgb c){
 float u=(px-S_LX)/S_SCALE,v=(py-S_LY)/S_SCALE;
 if(u<-20||u>276||v<-20||v>276)return c;
 /* Tile glow then tile. */
 float d=s_round(u,v,128,120,44);
 if(a->tile>0){float dp=d*S_SCALE;if(dp>0){float g=dp/1.5f;c=s_mix(c,S_PURPLE,.85f*a->tile*s_expn(.5f*g*g));}c=s_mix(c,S_TILE,s_cover(d)*a->tile);}
 if(a->orange>0){float k=.82f+.18f*a->orange;d=s_round(128+(u-128)/k,128+(v-128)/k,128,92.5f,20)*k;c=s_mix(c,S_ORANGE,s_cover(d)*a->orange);}
 if(a->deep>0){float k=.82f+.18f*a->deep;d=s_round(128+(u-128)/k,128+(v-128)/k,128,73.69f,20)*k;c=s_mix(c,S_DEEP,s_cover(d)*a->deep);}
 if(a->orb>0){float du=u-128,dv=v-128,r=s_sqrt(du*du+dv*dv),R=(30+16*a->orb)*(1+.03f*a->pulse);
  float halo=R+18+6*a->pulse;if(r<halo&&r>R-2){float t=1-(r-R)/(halo-R);if(t>1)t=1;c=s_mix(c,S_PURPLE,(.38f+.12f*a->pulse)*a->orb*t*t);}
  float cov=s_clamp((R-r)*S_SCALE+.5f);if(cov>0){float t=s_clamp(r/R);s_rgb core=s_mix(S_CORE,S_PURPLE,s_clamp(.25f+t*t));c=s_mix(c,core,cov*a->orb);}}
 return c;
}
/* Pixel-font text: scale s, pitch per glyph, revealed up to clip_x. */
static int s_glyph(const uint8_t*font,const char*text,float x0,float y0,unsigned s,unsigned pitch,float px,float py){
 if(py<y0||py>=y0+7*s||px<x0)return 0;
 unsigned i=(unsigned)((px-x0)/pitch),gx=(unsigned)(px-x0)-i*pitch,gy=(unsigned)(py-y0);
 for(unsigned k=0;k<i;k++)if(!text[k])return 0;
 unsigned ch=(unsigned char)text[i];if(!ch||ch<32||ch>127||gx>=5*s)return 0;
 return(font[(ch-32)*5+gx/s]>>(gy/s))&1;
}
#define W1_TEXT "TLBX-1"
#define W1_S 3u
#define W1_P 18u
#define W1_X 69.f
#define W1_Y 158.f
#define W2_TEXT "ANIMATE"
#define W2_S 2u
#define W2_P 14u
#define W2_X 72.f
#define W2_Y 188.f
#define BAR_X 70u
#define BAR_W 100u
#define BAR_Y 214u
#define W3_TEXT "SMK-37 PRO"
#define W3_X 90.f
#define W3_Y 224.f
static s_rgb s_pixel(unsigned f,unsigned x,unsigned y,const s_anim*a){
 const uint8_t*font=ac_display_font();float px=(float)x+.5f,py=(float)y+.5f;s_rgb c=S_BG;
 if(y<156)return s_logo(px,py,a,c);
 if(y>=(unsigned)W1_Y&&y<(unsigned)W1_Y+21){float clip=W1_X+a->word*(6*W1_P);if(px<clip&&s_glyph(font,W1_TEXT,W1_X,W1_Y,W1_S,W1_P,px,py)){float edge=a->word>=1?1:s_clamp((clip-px)/10);c=s_mix(S_PURPLE,S_INK,edge);}return c;}
 if(y>=(unsigned)W2_Y&&y<(unsigned)W2_Y+14){if(s_glyph(font,W2_TEXT,W2_X,W2_Y,W2_S,W2_P,px,py))c=s_mix(c,S_PURPLE,a->sub);return c;}
 if(y>=BAR_Y&&y<BAR_Y+3&&x>=BAR_X&&x<BAR_X+BAR_W){unsigned fill=(unsigned)(a->bar*BAR_W+.5f);return x-BAR_X<fill?S_ORANGE:S_SLATE;}
 if(y>=(unsigned)W3_Y&&y<(unsigned)W3_Y+7){if(s_glyph(font,W3_TEXT,W3_X,W3_Y,1,6,px,py))c=s_mix(c,S_DIM,a->sub);return c;}
 (void)f;return c;
}
void af_splash_row(unsigned frame,unsigned y,unsigned x0,unsigned w,uint16_t*out){
 if(frame>=AF_SPLASH_FRAMES)frame=AF_SPLASH_FRAMES-1;
 s_anim a=s_at(frame);
 for(unsigned i=0;i<w&&x0+i<240;i++)out[i]=y<240?s_565(s_pixel(frame,x0+i,y,&a)):0;
}
static unsigned s_add(unsigned(*r)[4],unsigned n,unsigned x,unsigned y,unsigned w,unsigned h){if(n<AF_SPLASH_RECTS){r[n][0]=x;r[n][1]=y;r[n][2]=w;r[n][3]=h;n++;}return n;}
unsigned af_splash_regions(unsigned frame,unsigned rects[AF_SPLASH_RECTS][4]){
 unsigned n=0;
 if(!frame)return s_add(rects,0,0,0,240,240);
 if(frame>=AF_SPLASH_FRAMES)return 0;
 if(frame<=11)n=s_add(rects,n,50,12,140,144);                         /* tile, squares, glow */
 else n=s_add(rects,n,80,42,80,80);                                     /* orb, halo, pulse */
 if(frame>=12&&frame<=19)n=s_add(rects,n,(unsigned)W1_X,(unsigned)W1_Y,6*W1_P,21);
 if(frame>=17&&frame<=23){n=s_add(rects,n,(unsigned)W2_X,(unsigned)W2_Y,7*W2_P,14);n=s_add(rects,n,(unsigned)W3_X,(unsigned)W3_Y,60,7);}
 n=s_add(rects,n,BAR_X,BAR_Y,BAR_W,3);
 return n;
}
uint16_t af_splash_pads(unsigned frame){
 if(frame>=AF_SPLASH_FRAMES)return 0;
 if(frame>=AF_SPLASH_FRAMES-6)return 0xffffu;   /* settle: all pads, then off */
 unsigned c=(frame/2)%8;return(uint16_t)((1u<<c)|(1u<<(c+8)));
}
static uint16_t s_frame[240*240];
const uint16_t*af_splash_render(unsigned frame){for(unsigned y=0;y<240;y++)af_splash_row(frame,y,0,240,s_frame+y*240);return s_frame;}
