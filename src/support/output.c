/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted from Melodee firmware/src/fx.c master_out/dc_block and dsp.c softclip.
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments.
 * Table matches tools/gen_tables.py TANH_Q15; no borrowed synthesis engine.
 * TLBX adaptation: explicit state/API, bounded input, no speaker EQ. */
#include "output.h"
#define OUTPUT_THRESHOLD 18000
static const int16_t output_tanh[257]={0,255,511,767,1023,1279,1534,1790,2045,2300,2554,2809,3062,3316,3569,3822,4074,4326,4577,4828,5078,5328,5577,5825,6072,6319,6565,6811,7055,7299,7542,7784,8025,8265,8504,8742,8980,9216,9451,9685,9918,10150,10381,10611,10840,11067,11293,11518,11742,11964,12186,12405,12624,12841,13057,13272,13485,13697,13908,14117,14325,14531,14736,14940,15142,15342,15541,15739,15935,16130,16323,16515,16705,16894,17081,17267,17451,17633,17814,17994,18172,18349,18524,18697,18869,19039,19208,19375,19541,19705,19868,20029,20188,20346,20503,20658,20811,20963,21114,21263,21410,21556,21701,21844,21985,22125,22264,22401,22536,22670,22803,22935,23064,23193,23320,23445,23570,23693,23814,23934,24053,24170,24286,24401,24514,24626,24737,24847,24955,25062,25167,25271,25375,25476,25577,25676,25775,25872,25967,26062,26156,26248,26339,26429,26518,26606,26692,26778,26862,26946,27028,27109,27190,27269,27347,27424,27500,27576,27650,27723,27795,27867,27937,28006,28075,28142,28209,28275,28340,28404,28467,28529,28591,28652,28712,28771,28829,28886,28943,28999,29054,29108,29162,29215,29267,29318,29369,29419,29468,29517,29565,29612,29658,29704,29750,29794,29838,29882,29925,29967,30008,30049,30090,30130,30169,30208,30246,30283,30320,30357,30393,30428,30463,30498,30532,30565,30598,30631,30663,30694,30726,30756,30786,30816,30846,30874,30903,30931,30959,30986,31013,31039,31065,31091,31116,31141,31166,31190,31214,31238,31261,31284,31306,31328,31350,31371,31393,31413,31434,31454,31474,31494,31513,31532,31551,31570,31588};
static int32_t output_bound(int32_t x){return x>1048576?1048576:x< -1048576?-1048576:x;}
static int32_t output_dc(int32_t x,int32_t *dc,int32_t *error){
 int32_t e=x*64-*dc+*error,d=e>>12;*error=e-d*4096;*dc+=d;return x-((*dc+32)>>6);
}
static int32_t output_softclip(int32_t x){
 int32_t a=x<0?-x:x,i=a>>8,y;
 if(i>=256)y=output_tanh[256];else y=output_tanh[i]+((output_tanh[i+1]-output_tanh[i])*(a&255)>>8);
 return x<0?-y:y;
}
void animate_output_init(animate_output*s){s->dc[0]=s->dc[1]=s->error[0]=s->error[1]=0;s->peak=OUTPUT_THRESHOLD;}
void animate_output_process(animate_output*s,int32_t*l,int32_t*r){
 *l=output_dc(output_bound(*l),&s->dc[0],&s->error[0]);*r=output_dc(output_bound(*r),&s->dc[1],&s->error[1]);
 int32_t al=*l<0?-*l:*l,ar=*r<0?-*r:*r,a=al>ar?al:ar;
 if(a>s->peak)s->peak+=(a-s->peak)>>2;else if(s->peak>OUTPUT_THRESHOLD)s->peak-=((s->peak-OUTPUT_THRESHOLD)>>12)+1;
 if(s->peak>OUTPUT_THRESHOLD){int32_t g=(int32_t)(((uint32_t)OUTPUT_THRESHOLD<<15)/(uint32_t)s->peak);*l=((*l>>4)*g)>>11;*r=((*r>>4)*g)>>11;}
 *l=output_softclip(*l);*r=output_softclip(*r);
}
int32_t animate_output_from_float(float x){if(!(x==x))return 0;if(x>=32)return 1048576;if(x<=-32)return -1048576;return(int32_t)(x*32768);}
