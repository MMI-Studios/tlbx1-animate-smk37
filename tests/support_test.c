#include "support/output.h"
#include "support/music.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
int main(void){
 animate_output state;animate_output_init(&state);int32_t l=0,r=0;animate_output_process(&state,&l,&r);assert(!l&&!r);
 assert(animate_output_from_float(1)==32768&&animate_output_from_float(-1)==-32768&&animate_output_from_float(NAN)==0);
 for(unsigned i=0;i<44100;i++){l=1000;r=-1000;animate_output_process(&state,&l,&r);}assert(abs(l)<=1&&abs(r)<=1);
 animate_output_init(&state);for(unsigned i=0;i<44100;i++){l=(i&1)?INT_MAX:INT_MIN;r=l;animate_output_process(&state,&l,&r);assert(l==r&&l>=-32768&&l<=32767);}assert(state.peak>18000);
 for(unsigned i=0;i<44100*2;i++){l=r=0;animate_output_process(&state,&l,&r);}assert(abs(l)<=1&&abs(r)<=1&&state.peak==18000);
 assert(animate_scale_count()==70);animate_music m={0};unsigned quarter=0,bp=0;
 for(unsigned s=0;s<70;s++){m.scale=s;assert(*animate_scale_name(s,0)&&*animate_scale_name(s,1)&&animate_scale_degrees(s));assert(*animate_family_name(animate_scale_family(s),1));int32_t last=animate_scale_pitch(&m,-100);for(int degree=-99;degree<100;degree++){int32_t p=animate_scale_pitch(&m,degree);assert(p>last);last=p;}if(!strcmp(animate_scale_name(s,0),"24EDO"))quarter=s;if(!strcmp(animate_scale_name(s,0),"BP-ET"))bp=s;}
 m.scale=quarter;assert(animate_scale_pitch(&m,1)==605000);m.map=ANIMATE_MAP_ALL;assert(animate_music_map(&m,61,0)==605000);
 m.scale=bp;assert(animate_scale_pitch(&m,13)-animate_scale_pitch(&m,0)>190000);
 m.scale=1;m.map=ANIMATE_MAP_SNAP;assert(animate_music_map(&m,61,0)==600000);m.map=ANIMATE_MAP_WHITE;assert(animate_music_map(&m,61,0)==-1);assert(animate_music_map(&m,62,0)==620000);
 int32_t notes[4];m.chord=ANIMATE_CHORD_DIA7;assert(animate_music_chord(&m,600000,notes)==4&&notes[0]==600000&&notes[3]==710000);
 m.chord=ANIMATE_CHORD_MINOR;m.voicing=ANIMATE_VOICE_INV1;assert(animate_music_chord(&m,600000,notes)==3&&notes[0]==630000&&notes[2]==720000);
 for(unsigned s=0;s<70;s++)for(unsigned chord=0;chord<=9;chord++)for(unsigned voice=0;voice<5;voice++){m.scale=s;m.chord=chord;m.voicing=voice;unsigned n=animate_music_chord(&m,600000,notes);assert(n&&n<=4);for(unsigned i=0;i<n;i++){assert(notes[i]>=0&&notes[i]<=1270000);if(i)assert(notes[i]>notes[i-1]);}}
 assert(animate_music_hz(690000,440)==440);for(int pitch=0;pitch<=1270000;pitch+=4999){float ref=440*exp2f((pitch-690000)/120000.f),actual=animate_music_hz(pitch,440);assert(fabsf(actual-ref)/ref<.00001f);}
 puts("support: bounded Q15/DC/linked limiter, all70 scale catalogs, fractional/non-octave pitches, chord families/voicings and A4 tuning passed");return 0;
}
