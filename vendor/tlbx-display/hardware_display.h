/* SPDX-License-Identifier: GPL-3.0-only
 * Main-loop LCD projection. Exact compact command cache; no audio IRQ locking.
 * State lives in ordinary BSS, not the tightly budgeted audio pool. */
#ifndef TLBX_HARDWARE_DISPLAY_H
#define TLBX_HARDWARE_DISPLAY_H
#include "display.h"
typedef struct {uint8_t type,x,y,w,h;uint16_t color;char text[32];} tlbx_lcd_cached;
typedef void (*tlbx_lcd_blit_fn)(unsigned,unsigned,unsigned,unsigned,const uint16_t*);
static struct {
 tlbx_lcd_cached previous[64];unsigned count,valid;
 uint8_t left[240],right[240];
 uint16_t pixels[240*4];
} tlbx_lcd_state;
static uint16_t tlbx_lcd_wire(unsigned c){return(uint16_t)((c>>8)|((c&255)<<8));}
static void tlbx_hardware_display_reset(void){tlbx_lcd_state.valid=0;}
static void tlbx_lcd_mark(unsigned x,unsigned y,unsigned w,unsigned h){
 if(x>=240||y>=240)return;
 if(w>240-x)w=240-x;
 if(h>240-y)h=240-y;
 for(unsigned row=y;row<y+h;row++){if(x<tlbx_lcd_state.left[row])tlbx_lcd_state.left[row]=(uint8_t)x;if(x+w>tlbx_lcd_state.right[row])tlbx_lcd_state.right[row]=(uint8_t)(x+w);}
}
static int tlbx_lcd_same(const tlbx_lcd_cached*a,const tlbx_draw_command_t*b){
 if(a->type!=b->type||a->x!=b->x||a->y!=b->y||a->w!=b->w||a->h!=b->h||a->color!=b->color)return 0;
 if(b->type==2)return 0; /* Plot pixels vary without changing command geometry. */
 if(b->type==1)for(unsigned i=0;i<32;i++){if(a->text[i]!=b->text[i])return 0;if(!b->text[i])break;}
 return 1;
}
static void tlbx_lcd_cache(tlbx_lcd_cached*a,const tlbx_draw_command_t*b){
 a->type=(uint8_t)b->type;a->x=(uint8_t)b->x;a->y=(uint8_t)b->y;a->w=(uint8_t)b->w;a->h=(uint8_t)b->h;a->color=(uint16_t)b->color;
 for(unsigned i=0;i<32;i++)a->text[i]=b->text[i];
}
static void tlbx_lcd_raster(uint16_t*out,unsigned x0,unsigned y,unsigned width,const tlbx_draw_command_t*commands,unsigned count,const uint8_t*font,const int16_t*data,const uint16_t*bins){
 for(unsigned x=0;x<width;x++)out[x]=tlbx_lcd_wire(0x0841);
 for(unsigned n=0;n<count;n++){
  const tlbx_draw_command_t*c=&commands[n];if(y<c->y||y>=c->y+c->h)continue;
  unsigned left=c->x>x0?c->x:x0,right=c->x+c->w<x0+width?c->x+c->w:x0+width;if(left>=right)continue;
  unsigned py=y-c->y;
  for(unsigned x=left;x<right;x++){
   unsigned px=x-c->x,color=c->color;
   if(c->type==1){unsigned i=px/12,ch=32;int ink=0;if(i<31){unsigned k=0;while(k<i&&c->text[k])k++;if(k==i)ch=(unsigned char)c->text[i];}if(ch>=32&&ch<=127&&px%12<10&&py<14)ink=font[(ch-32)*5+(px%12)/2]&(1u<<(py/2));color=ink?c->color:0x0841;}
   else if(c->type==2){unsigned mode=(unsigned char)c->text[0];int ink=0;
    if(mode==0&&c->w>1){unsigned i=px*127/(c->w-1),j=i?i-1:i;int a=(int)c->h/2-data[i*2]*(int)c->h/65536,b=(int)c->h/2-data[j*2]*(int)c->h/65536;ink=(int)py>=(a<b?a:b)&&(int)py<=(a>b?a:b);}
    else if(mode==1&&c->w>=32){unsigned bin=px*32/c->w;ink=px%(c->w/32)!=0&&py>=c->h-(unsigned)bins[bin]*c->h/32768;}
    color=ink?c->color:(py==c->h/2||px==c->w/2)?0x3186:0x0841;
   }
   out[x-x0]=tlbx_lcd_wire(color);
  }
  if(c->type==2&&(unsigned char)c->text[0]==2)for(unsigned i=0;i<128;i++){int l=data[i*2],r=data[i*2+1];int px=(int)c->w/2+(l-r)*(int)c->w/131072,py2=(int)c->h/2-(l+r)*(int)c->h/131072;int x=(int)c->x+px;if(py2==(int)py&&px>=0&&(unsigned)px<c->w&&x>=(int)left&&x<(int)right)out[x-x0]=tlbx_lcd_wire(c->color);}
 }
}
static void tlbx_hardware_display(const tlbx_draw_command_t*commands,unsigned count,const uint8_t*font,const int16_t*points,const uint16_t*bins,tlbx_lcd_blit_fn blit){
 if(count>64)count=64;
 for(unsigned y=0;y<240;y++){tlbx_lcd_state.left[y]=240;tlbx_lcd_state.right[y]=0;}
 if(!tlbx_lcd_state.valid)tlbx_lcd_mark(0,0,240,240);
 else for(unsigned n=0;n<count||n<tlbx_lcd_state.count;n++){
  if(n<count&&n<tlbx_lcd_state.count&&tlbx_lcd_same(&tlbx_lcd_state.previous[n],&commands[n]))continue;
  if(n<tlbx_lcd_state.count){const tlbx_lcd_cached*c=&tlbx_lcd_state.previous[n];tlbx_lcd_mark(c->x,c->y,c->w,c->h);}
  if(n<count){const tlbx_draw_command_t*c=&commands[n];tlbx_lcd_mark(c->x,c->y,c->w,c->h);}
 }
 for(unsigned y=0;y<240;){unsigned left=tlbx_lcd_state.left[y],right=tlbx_lcd_state.right[y];if(left>=right){y++;continue;}
  unsigned rows=1;while(rows<4&&y+rows<240&&tlbx_lcd_state.left[y+rows]==left&&tlbx_lcd_state.right[y+rows]==right)rows++;
  unsigned width=right-left;for(unsigned row=0;row<rows;row++)tlbx_lcd_raster(tlbx_lcd_state.pixels+row*width,left,y+row,width,commands,count,font,points,bins);
  blit(left,y,width,rows,tlbx_lcd_state.pixels);y+=rows;
 }
 for(unsigned n=0;n<count;n++)tlbx_lcd_cache(&tlbx_lcd_state.previous[n],&commands[n]);
 tlbx_lcd_state.count=count;tlbx_lcd_state.valid=1;
}
#endif
