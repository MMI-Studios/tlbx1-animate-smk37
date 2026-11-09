#include "animate_parameters.h"
#define RANGE ANIMATE_PARAMETER_RANGE_DISAGREEMENT
#define BEHAVIOR ANIMATE_PARAMETER_BEHAVIOR_DISAGREEMENT
static const animate_parameter_metadata parameters[ANIMATE_SLOT_PARAMETER_COUNT]={
 {ANIMATE_SOURCE,"source","Source","catalogue ID",-1,-1,-1,-1,1,0,0,ANIMATE_PARAMETER_DYNAMIC_CATALOGUE},
 {ANIMATE_COARSE,"coarse","Coarse","semitones",-24,24,-48,48,.1f,0,1,RANGE},
 {ANIMATE_FINE,"fine","Fine","cents",-100,100,-100,100,.5f,0,2,0},
 {ANIMATE_LEVEL,"level","Level","linear",0,1,0,1,.01f,1,3,0},
 {ANIMATE_PAN,"pan","Pan","balance",-1,1,-1,1,.01f,0,4,0},
 {ANIMATE_WT_AMOUNT,"wt_lfo_amount","WT LFO amount","normalized",0,1,0,1,.01f,0,5,0},
 {ANIMATE_WT_SHAPE,"wt_lfo_shape","WT LFO shape","enum",0,4,0,4,1,0,6,0},
 {ANIMATE_WT_RATE,"wt_lfo_rate","WT LFO rate","normalized logarithmic",0,1,0,1,.001f,.5f,7,BEHAVIOR},
 {ANIMATE_WT_SYNC,"wt_lfo_sync","WT LFO sync","boolean",0,1,0,1,1,0,8,0},
 {ANIMATE_WT_DIVISION,"wt_lfo_division","WT LFO division","enum",0,7,0,7,1,0,9,0},
 {ANIMATE_SAMPLE_START,"sample_start","Sample start","normalized",0,1,0,1,.005f,0,10,0},
 {ANIMATE_LOOP_START,"loop_start","Loop start","normalized",0,1,0,1,0,0,11,BEHAVIOR},
 {ANIMATE_LOOP_END,"loop_end","Loop end","normalized",0,1,0,1,.005f,1,11,BEHAVIOR},
 {ANIMATE_FILTER_TYPE,"filter_type","Filter type","enum",0,4,0,4,1,0,12,BEHAVIOR},
 {ANIMATE_FILTER_CUTOFF,"filter_cutoff","Cutoff","normalized quadratic",0,1,.001f,1,.01f,.5f,13,RANGE},
 {ANIMATE_FILTER_RESONANCE,"filter_resonance","Resonance","normalized",0,1,0,.95f,.01f,0,14,RANGE},
 {ANIMATE_ATTACK,"attack","Attack","seconds time constant",0,5,0,5,.01f,.01f,15,0},
 {ANIMATE_DECAY,"decay","Decay","seconds time constant",0,10,0,10,.01f,.1f,16,0},
 {ANIMATE_SUSTAIN,"sustain","Sustain","linear",0,1,0,1,.01f,.8f,17,0},
 {ANIMATE_RELEASE,"release","Release","seconds time constant",0,10,0,10,.01f,.3f,18,0}
};
const animate_parameter_metadata *animate_parameter_info(unsigned id){return id<ANIMATE_SLOT_PARAMETER_COUNT?&parameters[id]:NULL;}
const char *const animate_filter_labels[5]={"Lowpass 24dB","Lowpass 12dB","Highpass","Bandpass","Moog"};
const char *const animate_lfo_shape_labels[5]={"Sine","Triangle","Square","Saw","Sample & Hold"};
const char *const animate_lfo_division_labels[8]={"1/16","1/8","1/4","1/3","1/2","1","2","4"};
const float animate_lfo_division_beats[8]={.25f,.5f,1,1.333333333f,2,4,8,16};
