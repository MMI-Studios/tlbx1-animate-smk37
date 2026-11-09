/* Renders fixed scenarios through the bridge. Built twice: with
 * -DAB_REFERENCE_KERNEL (original per-sample renderer) and without (unified
 * block-rate kernel). tools/kernel_compare.py compares the two PCM streams. */
#include "baseline/bridge.c"
#include <stdio.h>
#include <stdlib.h>
static FILE *out;
static void render(unsigned blocks){int32_t pcm[256];for(unsigned b=0;b<blocks;b++){ab_render(pcm,128);fwrite(pcm,4,256,out);}}
static uint32_t chord(const unsigned*notes,unsigned count,unsigned vel,uint32_t*h){for(unsigned i=0;i<count;i++)h[i]=ab_note_on(notes[i],vel+i*7);return count;}
static void release(uint32_t*h,unsigned count){for(unsigned i=0;i<count;i++)ab_note_off(h[i]);}
static void begin(unsigned preset){ab_init();ab_preset(preset);}
static const unsigned C4[4]={60,64,67,71},C8[8]={48,55,60,64,67,71,74,79};
static void scenario(unsigned id){
 uint32_t h[8];
 fprintf(out,"%c",0);fseek(out,-1,SEEK_CUR);
 switch(id){
 case 0: case 1: case 2: case 3: case 4: case 5:   /* each preset: 4-note chord, release */
  begin(id);chord(C4,4,90,h);render(520);release(h,4);render(345);break;
 case 6: /* native filter on every slot */
  begin(0);for(unsigned s=0;s<4;s++){ab_slot(s,AN_BYPASS,0);ab_slot(s,AN_CUT,50+s*15);ab_slot(s,AN_RES,30+s*25);}
  chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 7: case 8: case 9: case 10: case 11: /* extended filter types 0..4 */
  begin(0);for(unsigned s=0;s<4;s++){ab_slot_parameter(s,13,(float)(id-7));ab_slot_parameter(s,14,.35f+s*.1f);ab_slot_parameter(s,15,.4f);}
  chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 12: /* pan, level, coarse/fine pitch */
  begin(0);ab_slot_parameter(0,4,-.8f);ab_slot_parameter(1,4,.6f);ab_slot_parameter(2,3,.5f);ab_slot_parameter(3,1,7);ab_slot_parameter(3,2,25);
  chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 13: /* windows + position LFO, native and extended */
  begin(0);for(unsigned s=0;s<2;s++){ab_slot(s,AN_START,20);ab_slot(s,AN_END,100);ab_slot(s,AN_POS_SHAPE,s);ab_slot(s,AN_POS_RATE,60);ab_slot(s,AN_POS_DEPTH,90);}
  ab_slot_parameter(2,10,.1f);ab_slot_parameter(2,11,.3f);ab_slot_parameter(2,12,.8f);ab_slot_parameter(3,5,.6f);ab_slot_parameter(3,6,4);ab_slot_parameter(3,7,.5f);
  chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 14: /* extended envelopes */
  begin(0);for(unsigned s=0;s<4;s++){ab_slot_parameter(s,16,.05f+s*.1f);ab_slot_parameter(s,17,.3f);ab_slot_parameter(s,18,.5f);ab_slot_parameter(s,19,.4f+s*.2f);}
  chord(C4,4,100,h);render(520);release(h,4);render(600);break;
 case 15: /* vector LFO edits, then solo */
  begin(5);ab_vector_lfo(0,0,4);ab_vector_lfo(0,1,6);ab_vector_lfo(0,2,.8f);ab_vector_lfo(1,0,2);ab_vector_lfo(1,1,1.5f);
  chord(C4,4,100,h);render(400);ab_slot(2,AN_SLOT,2);ab_slot(0,AN_SOLO,1);render(120);release(h,4);render(345);break;
 case 16: /* live window edits while held */
  begin(3);chord(C4,4,100,h);render(150);for(unsigned k=0;k<8;k++){ab_slot(k%4,AN_SLOT,k%4);ab_slot(k%4,AN_START,10+k*9);ab_slot(k%4,AN_END,127-k*6);render(40);}
  release(h,4);render(345);break;
 case 18: /* native window only */
  begin(1);ab_slot(0,AN_START,20);ab_slot(0,AN_END,100);chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 19: /* native position LFO, sine */
  begin(1);ab_slot(0,AN_POS_SHAPE,0);ab_slot(0,AN_POS_RATE,60);ab_slot(0,AN_POS_DEPTH,90);chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 20: /* extended loop window */
  begin(0);ab_slot_parameter(2,10,.1f);ab_slot_parameter(2,11,.3f);ab_slot_parameter(2,12,.8f);chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 21: /* extended S&H position */
  begin(0);ab_slot_parameter(3,5,.6f);ab_slot_parameter(3,6,4);ab_slot_parameter(3,7,.5f);chord(C4,4,100,h);render(520);release(h,4);render(345);break;
 case 17: /* eight voices, staggered */
  begin(0);for(unsigned i=0;i<8;i++){h[i]=ab_note_on(C8[i],70+i*7);render(30);}render(300);release(h,8);render(345);break;
 }
}
int main(int argc,char**argv){
 if(argc<3)return 2;out=fopen(argv[2],"wb");scenario((unsigned)atoi(argv[1]));fclose(out);return 0;
}
