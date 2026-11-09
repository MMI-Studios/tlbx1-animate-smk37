/* SPDX-License-Identifier: GPL-3.0-only
 * Scale/chord mapping adapted from Melodee firmware/src/seq.c, microtonal.h,
 * chord.c and tuning.h. Chord code copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Catalog remains the preserved donor's
 * generated scales.h (assets/scales/catalog.json); no engine code is imported.
 * TLBX adaptation returns fractional pitches, independent of track/UI structs. */
#include "music.h"
#include "../../vendor/melodee/firmware/src/scales.h"
static int music_floor(int a,int b){int q=a/b;return q-(a%b<0);}
static unsigned music_scale(const animate_music*m){return m->scale<SCALE_TOTAL?m->scale:0;}
unsigned animate_scale_count(void){return SCALE_TOTAL;}
const char *animate_scale_name(unsigned s,int full){return s<SCALE_TOTAL?(full?SCALE_TITLE[s]:N_SCALE[s]):"";}
unsigned animate_scale_degrees(unsigned s){return s<SCALE_TOTAL?SCALE_DEGREES[s]:0;}
unsigned animate_scale_family(unsigned s){return s<SCALE_TOTAL?SCALE_FAMILY[s]+1u:0;}
const char *animate_family_name(unsigned f,int full){return f<sizeof SCALE_FAMILY_SHORT/sizeof *SCALE_FAMILY_SHORT?(full?SCALE_FAMILY_TITLE[f]:SCALE_FAMILY_SHORT[f]):"";}
int32_t animate_scale_pitch(const animate_music*m,int degree){
 unsigned s=music_scale(m),count=SCALE_DEGREES[s];int cycle=music_floor(degree,(int)count),at=degree-cycle*(int)count;
 int32_t base=(60+(int)(m->root%12)+m->transpose)*10000;
 if(s>=SCALE_LEGACY){const micro_scale_t*micro=&MICRO_SCALE[s-SCALE_LEGACY];return base+cycle*micro->pitch[count]+micro->pitch[at];}
 unsigned mask=SCALE_MASK[s],note=0;for(;note<12;note++)if(mask&(1u<<note)){if(!at)break;at--;}
 return base+cycle*120000+(int)note*10000;
}
static int music_degree_below(const animate_music*m,int32_t pitch){
 /* Binary search over more than the audible MIDI range, including 53 EDO. */
 int lo=-1024,hi=1024;while(lo<hi){int mid=lo+(hi-lo+1)/2;if(animate_scale_pitch(m,mid)<=pitch)lo=mid;else hi=mid-1;}return lo;
}
int32_t animate_music_map(const animate_music*m,unsigned note,int octave){
 if(note>127||octave< -10||octave>10||m->transpose< -127||m->transpose>127)return -1;
 int32_t pitch;
 if(m->map==ANIMATE_MAP_ALL)pitch=animate_scale_pitch(m,(int)note-60+octave*(int)SCALE_DEGREES[music_scale(m)]);
 else if(m->map==ANIMATE_MAP_WHITE){static const int8_t degree[12]={0,-1,1,-1,2,3,-1,4,-1,5,-1,6};if(degree[note%12]<0)return -1;pitch=animate_scale_pitch(m,degree[note%12]+((int)note/12-5)*7+octave*(int)SCALE_DEGREES[music_scale(m)]);}
 else{pitch=((int)note+12*octave+m->transpose)*10000;if(m->map==ANIMATE_MAP_SNAP)pitch=animate_scale_pitch(m,music_degree_below(m,pitch));}
 return pitch<0||pitch>1270000?-1:pitch;
}
unsigned animate_music_chord(const animate_music*m,int32_t root,int32_t out[4]){
 static const int8_t shapes[7][4]={{0,4,7,-1},{0,3,7,-1},{0,4,7,10},{0,4,7,11},{0,3,7,10},{0,5,7,-1},{0,7,12,-1}};
 if(root<0||root>1270000)return 0;
 if(!m->chord||m->chord>ANIMATE_CHORD_POWER){out[0]=root;return 1;}
 animate_music scale=*m;if(scale.scale==0&&m->chord<=ANIMATE_CHORD_DIA7)scale.scale=1;
 unsigned s=music_scale(&scale),n=0;int degree=music_degree_below(&scale,root);int32_t pitches[4];
 int32_t period=s<SCALE_LEGACY?120000:MICRO_SCALE[s-SCALE_LEGACY].pitch[SCALE_DEGREES[s]];
 if(m->chord<=ANIMATE_CHORD_DIA7){n=m->chord==ANIMATE_CHORD_DIA7?4:3;for(unsigned i=0;i<n;i++)pitches[i]=animate_scale_pitch(&scale,degree+(int)i*2);}
 else for(unsigned i=0;i<4&&shapes[m->chord-ANIMATE_CHORD_MAJOR][i]>=0;i++){
  int32_t p=root+shapes[m->chord-ANIMATE_CHORD_MAJOR][i]*10000;
  if(s>=SCALE_LEGACY){int d=music_degree_below(&scale,p);int32_t low=animate_scale_pitch(&scale,d),high=animate_scale_pitch(&scale,d+1);p=p-low<high-p?low:high;}
  pitches[n++]=p;
 }
 if(m->voicing==ANIMATE_VOICE_OPEN&&n>=3)pitches[1]+=period;
 else if(m->voicing==ANIMATE_VOICE_INV1||m->voicing==ANIMATE_VOICE_INV2){unsigned shifts=m->voicing==ANIMATE_VOICE_INV2?2:1;while(shifts--){int32_t p=pitches[0]+period;for(unsigned i=1;i<n;i++)pitches[i-1]=pitches[i];pitches[n-1]=p;}}
 else if(m->voicing==ANIMATE_VOICE_BASS){if(n==4){pitches[2]=pitches[3];n--;}pitches[n++]=pitches[0]-period;}
 for(unsigned i=1;i<n;i++)for(unsigned j=i;j&&pitches[j]<pitches[j-1];j--){int32_t p=pitches[j];pitches[j]=pitches[j-1];pitches[j-1]=p;}
 unsigned written=0;for(unsigned i=0;i<n;i++)if(pitches[i]>=0&&pitches[i]<=1270000&&(!written||pitches[i]!=out[written-1]))out[written++]=pitches[i];return written;
}
float animate_music_hz(int32_t pitch,unsigned a4){
 if(pitch<0||pitch>1270000)return 0;
 if(a4<400)a4=400;
 if(a4>480)a4=480;
 int oct=music_floor(pitch-690000,120000);float x=(pitch-690000-oct*120000)*(0.6931471805599453f/120000);
 float ratio=1+x*(1+x*(.5f+x*(.166666667f+x*(.041666667f+x*(.008333333f+x*(.001388889f+x*.000198413f))))));
 float hz=a4*ratio;while(oct>0){hz*=2;oct--;}while(oct<0){hz*=.5f;oct++;}return hz;
}
