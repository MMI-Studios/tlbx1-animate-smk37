/* SPDX-License-Identifier: GPL-3.0-only
 * Browser project transport. Hardware explicitly has no project-store writer. */
#include "runtime.h"
#include "project_state.h"
#include "patch.h"
#include "baseline/bridge.h"
#include "support/music.h"
#include <string.h>
#include <stddef.h>
#ifdef AF_HARDWARE
unsigned af_project_size(void){return 0;}
unsigned char *af_project_buffer(void){return 0;}
unsigned af_project_read(unsigned offset,unsigned count){(void)offset;(void)count;return 0;}
int af_project_begin(unsigned size){(void)size;return 0;}
int af_project_write(unsigned offset,unsigned count){(void)offset;(void)count;return 0;}
int af_project_commit(void){return 0;}
#else
extern aps_state *af_project_state(void);
extern void af_project_restore_prepare(void);
extern float af_base_vector(unsigned axis);
static const unsigned globals[13]={0,1,16,17,18,19,20,24,25,26,27,32,33};
static const unsigned fx_ids[7]={33,34,35,36,0,39,38};
typedef struct {
 uint32_t magic,version,size,crc;
 float global[13],level[10],route[12],vector[2],lfo[2][5];
 int32_t fx[8];uint32_t controls[17],mask[4],lfo_mask[2];
 uint32_t wave[4];      /* version 2: bank entry held by each wave register */
} af_file_header;
#define AF_HEADER_V1 ((unsigned)offsetof(af_file_header,wave))
static af_file_header header;
static unsigned char chunk[1024],incoming[110000];
static unsigned total,incoming_size,incoming_written;
static uint32_t hash(uint32_t h,const void*data,unsigned count){const unsigned char*p=data;while(count--)h=(h^*p++)*16777619u;return h;}
static uint32_t header_hash(const af_file_header*h,unsigned size){af_file_header copy=*h;copy.crc=0;return hash(2166136261u,&copy,size);}
static int between(float x,float low,float high){return x>=low&&x<=high;}
static int integer(float x,int low,int high){return between(x,(float)low,(float)high)&&x==(float)(int)x;}
unsigned char*af_project_buffer(void){return chunk;}
unsigned af_project_size(void){
 if(af_get(0,0,AC_D_GLOBAL,42)>0)return 0;
 aps_state*s=af_project_state();memset(&header,0,sizeof header);
 total=sizeof header+aps_project_size(s);header.magic=0x32464e41;header.version=2;header.size=total;for(unsigned r=0;r<4;r++)header.wave[r]=ab_wave_get(r);
 for(unsigned i=0;i<13;i++)header.global[i]=af_get(0,0,AC_D_GLOBAL,globals[i]);
 for(unsigned i=0;i<10;i++)header.level[i]=af_get(i,0,AC_D_LEVEL,0);
 for(unsigned i=0;i<12;i++)header.route[i]=i==7?0:af_get(0,0,AC_D_ROUTE,i);
 for(unsigned i=0;i<8;i++)header.fx[i]=(int32_t)af_get(0,0,AC_D_FX,i);
 for(unsigned i=0;i<17;i++)if(i<10||i>14)header.controls[i]=af_control_get(i);
 for(unsigned i=0;i<4;i++)header.mask[i]=ab_slot_parameter_mask(i);
 for(unsigned axis=0;axis<2;axis++){header.vector[axis]=af_base_vector(axis);header.lfo_mask[axis]=ab_vector_lfo_mask(axis);for(unsigned i=0;i<5;i++)header.lfo[axis][i]=ab_vector_lfo_get(axis,i);}
 uint32_t crc=header_hash(&header,sizeof header);for(unsigned at=0;at<total-sizeof header;){unsigned n=aps_project_read(s,at,chunk,sizeof chunk);if(!n)return 0;crc=hash(crc,chunk,n);at+=n;}header.crc=crc;return total;
}
unsigned af_project_read(unsigned offset,unsigned count){
 if(!total||offset>=total||count>sizeof chunk)return 0;
 if(count>total-offset)count=total-offset;
 unsigned done=0;if(offset<sizeof header){unsigned n=sizeof header-offset;if(n>count)n=count;memcpy(chunk,(const unsigned char*)&header+offset,n);offset+=n;done=n;}
 if(done<count)done+=aps_project_read(af_project_state(),offset-sizeof header,chunk+done,count-done);
 return done;
}
int af_project_begin(unsigned size){incoming_size=incoming_written=0;if(size<=sizeof header||size>sizeof incoming)return 0;incoming_size=size;return 1;}
int af_project_write(unsigned offset,unsigned count){if(!incoming_size||offset!=incoming_written||count>sizeof chunk||count>incoming_size-offset)return 0;memcpy(incoming+offset,chunk,count);incoming_written+=count;return 1;}
/* Header size by version: 1 has no wave registers (original four waves). */
static unsigned header_read(af_file_header*h){memset(h,0,sizeof *h);if(incoming_size<AF_HEADER_V1)return 0;memcpy(h,incoming,AF_HEADER_V1);
 unsigned size=h->version==2?sizeof *h:h->version==1?AF_HEADER_V1:0;if(!size||incoming_size<=size)return 0;memcpy(h,incoming,size);
 if(h->version==1)for(unsigned r=0;r<4;r++)h->wave[r]=r;return size;}
static int validate(const af_file_header*h,unsigned hsize){
 if(h->magic!=0x32464e41||h->size!=incoming_size)return 0;
 if(h->crc!=hash(header_hash(h,hsize),incoming+hsize,incoming_size-hsize))return 0;
 for(unsigned r=0;r<4;r++)if(h->wave[r]>=ab_wave_count())return 0;
 if(!between(h->global[0],0,1)||!integer(h->global[1],1,16))return 0;
 for(unsigned i=2;i<=5;i++)if(!integer(h->global[i],0,1))return 0;
 if(h->global[3]&&h->global[4])return 0;
 if(!integer(h->global[6],0,5)||!integer(h->global[7],0,(int)animate_scale_count()-1)||!integer(h->global[8],0,11)||!integer(h->global[9],0,3)||!integer(h->global[10],-36,36)||!integer(h->global[11],0,9)||!integer(h->global[12],0,4))return 0;
 for(unsigned i=0;i<10;i++)if(!between(h->level[i],0,1))return 0;
 if(!integer(h->route[0],0,2)||!integer(h->route[1],0,3)||(!h->route[1]&&h->route[0]!=APS_PARAMS))return 0;
 for(unsigned i=2;i<=6;i++)if(!integer(h->route[i],0,1))return 0;
 if(h->route[7]!=0||!integer(h->route[8],0,11)||!integer(h->route[9],0,5)||!between(h->route[10],0,1)||!between(h->route[11],0,1))return 0;
 for(unsigned i=0;i<7;i++)if(!ab_param_valid(fx_ids[i],h->fx[i]))return 0;
 if(h->fx[7]<0||h->fx[7]>1)return 0;
 static const unsigned max_control[17]={12,9,3,7,1,2,1,4,2,2,1,5,16383,127,1,2,6};
 for(unsigned i=0;i<17;i++)if(h->controls[i]>max_control[i])return 0;
 for(unsigned i=0;i<4;i++)if(h->mask[i]&~0x1fffffu)return 0;
 for(unsigned axis=0;axis<2;axis++){const float*v=h->lfo[axis];if(!between(h->vector[axis],0,1)||h->lfo_mask[axis]>31||!integer(v[0],0,4)||!between(v[1],0,20)||!between(v[2],0,1)||!integer(v[3],0,1)||!integer(v[4],0,7))return 0;}
 return aps_project_validate(incoming+hsize,incoming_size-hsize);
}
int af_project_commit(void){
 if(!incoming_size||incoming_written!=incoming_size)return 0;
 af_file_header h;unsigned hsize=header_read(&h);if(!hsize||!validate(&h,hsize))return 0;
 af_project_restore_prepare();
 af_set(0,0,AC_D_GLOBAL,20,h.global[6]);
 aps_state*s=af_project_state();if(!aps_project_import(s,incoming+hsize,incoming_size-hsize))return 0;
 for(unsigned r=0;r<4;r++)ab_wave(r,h.wave[r]);
 for(unsigned slot=0;slot<4;slot++){for(unsigned p=0;p<20;p++)ab_slot_parameter(slot,p,s->base[slot][p]);ab_slot_parameter_mask_set(slot,h.mask[slot]);}
 for(unsigned i=0;i<13;i++)if(i!=3&&i!=4&&i!=6)af_set(0,0,AC_D_GLOBAL,globals[i],h.global[i]);
 if(h.global[3])af_set(0,0,AC_D_GLOBAL,17,1);
 else af_set(0,0,AC_D_GLOBAL,18,h.global[4]);
 for(unsigned i=0;i<10;i++)af_set(i,0,AC_D_LEVEL,0,h.level[i]);
 af_set(0,0,AC_D_ROUTE,1,h.route[1]);for(unsigned i=0;i<12;i++)if(i!=1&&i!=7)af_set(0,0,AC_D_ROUTE,i,h.route[i]);
 for(unsigned i=0;i<8;i++)af_set(0,0,AC_D_FX,i,(float)h.fx[i]);
 for(unsigned axis=0;axis<2;axis++){af_set(0,0,AC_D_MOD,axis*8,h.vector[axis]);for(unsigned i=0;i<5;i++)ab_vector_lfo(axis,i,h.lfo[axis][i]);ab_vector_lfo_mask_set(axis,h.lfo_mask[axis]);}
 for(unsigned i=0;i<17;i++)af_control_set(i,h.controls[i]);
 ab_tempo(s->sequence.bpm);af_patch_origin(AF_PATCH_SESSION);af_patch_changed();incoming_size=incoming_written=total=0;return 1;
}
#endif
