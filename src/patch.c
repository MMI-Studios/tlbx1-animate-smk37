/* SPDX-License-Identifier: GPL-3.0-only */
#include "patch.h"
#include "runtime.h"
#include "baseline/bridge.h"
#include "support/music.h"
#include <string.h>
#include <stddef.h>
#include "project_state.h"
#include "generated/factory_patches.h"
_Static_assert(AF_PATCH_ORIGINALS+AF_FACTORY_SEQ_COUNT==AF_PATCH_FACTORY,"factory entries");
static const unsigned patch_globals[14]={0,1,8,16,17,18,19,20,24,25,26,27,32,33};
static const unsigned patch_fx[7]={33,34,35,36,0,39,38};
typedef struct {
 uint32_t magic,version,size,checksum;
 float base[4][20],global[14],level[10],route[12],vector[2],lfo[2][5];
 uint32_t masks[4],random_masks[4],lfo_masks[2],slot,octave;
 int32_t fx[8];
 uint32_t wave[4];      /* version 2: bank entry held by each wave register */
} patch_file;
#define PATCH_V1_SIZE ((unsigned)offsetof(patch_file,wave))
_Static_assert(sizeof(patch_file)<=1024,"Patch must fit one storage record");
static struct {unsigned revision,saved_revision,status,notice,selection,current,confirm,pending,slot,id,bytes,captured_revision,occupied,restoring;uint32_t default_retained;} patch;
static union {uint32_t align;unsigned char bytes[1024];} patch_io;
static uint32_t patch_hash(const patch_file *p,unsigned size){const unsigned char*b=(const unsigned char*)p;uint32_t h=2166136261u;for(unsigned i=0;i<size;i++)h=(h^(i>=12&&i<16?0:b[i]))*16777619u;return h;}
static int patch_between(float x,float a,float b){return x>=a&&x<=b;}
static int patch_integer(float x,int a,int b){return patch_between(x,(float)a,(float)b)&&x==(float)(int)x;}
static uint32_t patch_retained_hash(void){/* Only on init/factory load, never per audio sample or edit. */
 float values[50];unsigned n=0;for(unsigned i=0;i<14;i++)if(i!=7)values[n++]=af_get(0,0,AC_D_GLOBAL,patch_globals[i]);for(unsigned i=0;i<10;i++)values[n++]=af_get(i,0,AC_D_LEVEL,0);for(unsigned i=0;i<12;i++)if(i!=7)values[n++]=af_get(0,0,AC_D_ROUTE,i);for(unsigned i=0;i<4;i++)values[n++]=(float)af_project_state()->mask[i];values[n++]=(float)af_control_get(16);
 const unsigned char*b=(const unsigned char*)values;uint32_t h=2166136261u;for(unsigned i=0;i<n*sizeof(float);i++)h=(h^b[i])*16777619u;return h;
}
void af_patch_reset(void){memset(&patch,0,sizeof patch);patch.revision=patch.saved_revision=1;patch.default_retained=patch_retained_hash();}
void af_patch_changed(void){if(patch.restoring)return;if(++patch.revision==0)patch.revision=1;patch.status=patch.pending?AF_PATCH_BUSY:AF_PATCH_READY;}
unsigned af_patch_dirty(void){return patch.revision!=patch.saved_revision;}
unsigned af_patch_revision(void){return patch.revision;}
unsigned af_patch_status(void){return patch.status;}
unsigned af_patch_notice(void){return patch.notice;}
unsigned af_patch_selection(void){return patch.selection;}
unsigned af_patch_current(void){return patch.current;}
void af_patch_origin(unsigned index){patch.current=index;}
unsigned af_patch_confirm_kind(void){return patch.confirm;}
unsigned af_patch_occupied(void){return patch.selection>=AF_PATCH_FACTORY?!!(patch.occupied&(1u<<(patch.selection-AF_PATCH_FACTORY))):1;}
void af_patch_slots(unsigned mask){patch.occupied=mask&65535u;}
unsigned af_patch_pending(void){return patch.pending;}
unsigned af_patch_slot(void){return patch.slot;}
unsigned af_patch_request_id(void){return patch.id;}
unsigned af_patch_bytes(void){return patch.bytes;}
unsigned char *af_patch_buffer(void){return patch_io.bytes;}
int af_patch_select(unsigned index){if(index>=AF_PATCH_ENTRIES||patch.pending||patch.confirm)return 0;patch.selection=index;return 1;}
int af_patch_load(unsigned index){if(index>=AF_PATCH_ENTRIES||patch.pending)return 0;patch.selection=index;patch.confirm=AF_PATCH_LOAD;patch.notice=AF_PATCH_NOTICE_CONFIRM;return 1;}
int af_patch_save(unsigned slot){if(slot>=16||patch.pending)return 0;patch.selection=slot+AF_PATCH_FACTORY;patch.confirm=AF_PATCH_SAVE;patch.notice=AF_PATCH_NOTICE_CONFIRM;return 1;}
void af_patch_cancel(void){if(!patch.pending){patch.confirm=0;patch.notice=0;}}
unsigned af_patch_export(void *destination,unsigned capacity){
 if(!destination||capacity<sizeof(patch_file)||af_get(0,0,AC_D_GLOBAL,42)>0)return 0;
 patch_file p;memset(&p,0,sizeof p);p.magic=0x50544e41;p.version=2;p.size=sizeof p;for(unsigned r=0;r<4;r++)p.wave[r]=ab_wave_get(r);
 aps_state*s=af_project_state();memcpy(p.base,s->base,sizeof p.base);memcpy(p.random_masks,s->mask,sizeof p.random_masks);
 for(unsigned i=0;i<14;i++)p.global[i]=af_get(0,0,AC_D_GLOBAL,patch_globals[i]);
 for(unsigned i=0;i<10;i++)p.level[i]=af_get(i,0,AC_D_LEVEL,0);
 for(unsigned i=0;i<12;i++)p.route[i]=i==7?0:af_get(0,0,AC_D_ROUTE,i);
 for(unsigned i=0;i<8;i++)p.fx[i]=(int32_t)af_get(0,0,AC_D_FX,i);
 for(unsigned i=0;i<4;i++)p.masks[i]=ab_slot_parameter_mask(i);
 for(unsigned a=0;a<2;a++){p.vector[a]=af_base_vector(a);p.lfo_masks[a]=ab_vector_lfo_mask(a);for(unsigned i=0;i<5;i++)p.lfo[a][i]=ab_vector_lfo_get(a,i);}
 p.slot=af_control_get(2);p.octave=af_control_get(16);p.checksum=patch_hash(&p,sizeof p);memcpy(destination,&p,sizeof p);return sizeof p;
}
/* Version 1 (original four waves) and version 2 (bank waves) are both accepted. */
static int patch_read(const void *bytes,unsigned size,patch_file *p){
 if(!bytes||(size!=sizeof(patch_file)&&size!=PATCH_V1_SIZE))return 0;memset(p,0,sizeof *p);memcpy(p,bytes,size);
 unsigned version=size==sizeof(patch_file)?2u:1u;
 if(p->magic!=0x50544e41||p->version!=version||p->size!=size||p->checksum!=patch_hash(p,size))return 0;
 if(version==1)for(unsigned r=0;r<4;r++)p->wave[r]=r;
 for(unsigned r=0;r<4;r++)if(p->wave[r]>=ab_wave_count())return 0;
 return 1;}
int af_patch_validate(const void *bytes,unsigned size){
 patch_file p;if(!patch_read(bytes,size,&p))return 0;
 for(unsigned s=0;s<4;s++){if(p.masks[s]&~0x1fffffu||p.random_masks[s]&~0x7ffffu)return 0;for(unsigned i=0;i<20;i++){const animate_parameter_metadata*m=animate_parameter_info(i);float value=p.base[s][i];if(!patch_between(value,i?m->stored_min:0,i?m->stored_max:3))return 0;if((i==0||i==6||i==8||i==9||i==13)&&value!=(float)(int)value)return 0;}}
 if(!patch_between(p.global[0],0,1)||!patch_integer(p.global[1],1,16)||!patch_integer(p.global[2],20,240))return 0;
 for(unsigned i=3;i<=6;i++)if(!patch_integer(p.global[i],0,1))return 0;
 if(p.global[4]&&p.global[5])return 0;
 if(!patch_integer(p.global[7],0,5)||!patch_integer(p.global[8],0,(int)animate_scale_count()-1)||!patch_integer(p.global[9],0,11)||!patch_integer(p.global[10],0,3)||!patch_integer(p.global[11],-36,36)||!patch_integer(p.global[12],0,9)||!patch_integer(p.global[13],0,4))return 0;
 for(unsigned i=0;i<10;i++)if(!patch_between(p.level[i],0,1))return 0;
 if(!patch_integer(p.route[0],0,2)||!patch_integer(p.route[1],0,3)||(!p.route[1]&&p.route[0]!=APS_PARAMS))return 0;
 for(unsigned i=2;i<=6;i++)if(!patch_integer(p.route[i],0,1))return 0;
 if(p.route[7]!=0||!patch_integer(p.route[8],0,11)||!patch_integer(p.route[9],0,5)||!patch_between(p.route[10],0,1)||!patch_between(p.route[11],0,1))return 0;
 for(unsigned i=0;i<7;i++)if(!ab_param_valid(patch_fx[i],p.fx[i]))return 0;
 if(p.fx[7]<0||p.fx[7]>1||p.slot>3||p.octave>6)return 0;
 for(unsigned a=0;a<2;a++){const float*v=p.lfo[a];if(!patch_between(p.vector[a],0,1)||p.lfo_masks[a]>31||!patch_integer(v[0],0,4)||!patch_between(v[1],0,20)||!patch_between(v[2],0,1)||!patch_integer(v[3],0,1)||!patch_integer(v[4],0,7))return 0;}
 return 1;
}
int af_patch_import(const void *bytes,unsigned size){
 if(!af_patch_validate(bytes,size))return 0;patch_file p;patch_read(bytes,size,&p);patch.restoring=1;
 af_patch_restore_prepare();af_set(0,0,AC_D_GLOBAL,19,0);af_wheel(0,0);af_wheel(1,0);af_set(0,0,AC_D_GLOBAL,20,p.global[7]);
 for(unsigned r=0;r<4;r++)ab_wave(r,p.wave[r]);
 for(unsigned s=0;s<4;s++){for(unsigned i=0;i<20;i++)af_set(0,s,AC_D_PARAM,i,p.base[s][i]);ab_slot_parameter_mask_set(s,p.masks[s]);af_project_state()->mask[s]=p.random_masks[s];}
 for(unsigned i=0;i<14;i++)if(i!=4&&i!=5&&i!=6&&i!=7)af_set(0,0,AC_D_GLOBAL,patch_globals[i],p.global[i]);
 if(p.global[4])af_set(0,0,AC_D_GLOBAL,17,1);else af_set(0,0,AC_D_GLOBAL,18,p.global[5]);
 for(unsigned i=0;i<10;i++)af_set(i,0,AC_D_LEVEL,0,p.level[i]);
 af_set(0,0,AC_D_ROUTE,1,p.route[1]);for(unsigned i=0;i<12;i++)if(i!=1&&i!=7)af_set(0,0,AC_D_ROUTE,i,p.route[i]);
 for(unsigned i=0;i<8;i++)af_set(0,0,AC_D_FX,i,(float)p.fx[i]);
 for(unsigned a=0;a<2;a++){af_set(0,0,AC_D_MOD,a*8,p.vector[a]);for(unsigned i=0;i<5;i++)ab_vector_lfo(a,i,p.lfo[a][i]);ab_vector_lfo_mask_set(a,p.lfo_masks[a]);}
 af_set(0,0,AC_D_GLOBAL,19,p.global[6]);af_control_set(2,p.slot);af_control_set(16,p.octave);
 patch.restoring=0;af_patch_changed();patch.saved_revision=patch.revision;patch.current=AF_PATCH_IMPORTED;return 1;
}
const char *af_patch_factory_name(unsigned index){static const char*originals[AF_PATCH_ORIGINALS]={"VECTOR","SINE","SAW","BELL","VOICE","USER VECTOR"};
 return index<AF_PATCH_ORIGINALS?originals[index]:index<AF_PATCH_FACTORY?afp_patches[index-AF_PATCH_ORIGINALS].name:"";}
/* Factory sound with sequence: VECTOR base, four bank waves, listed control
 * changes, then the whole ten-lane sequence (steps and locks) is replaced. */
static void patch_factory(unsigned k){const afp_patch*f=&afp_patches[k];
 af_patch_restore_prepare();af_set(0,0,AC_D_GLOBAL,19,0);af_wheel(0,0);af_wheel(1,0);af_set(0,0,AC_D_GLOBAL,20,0);
 for(unsigned r=0;r<4;r++)ab_wave(r,f->wave[r]);
 af_set(0,0,AC_D_GLOBAL,16,0);af_set(0,0,AC_D_GLOBAL,18,0);af_set(0,0,AC_D_GLOBAL,17,0);
 for(unsigned l=0;l<10;l++)af_set(l,0,AC_D_LEVEL,0,1);
 for(unsigned i=0;i<f->nops;i++){const afp_op*o=&f->ops[i];af_set(o->lane,o->slot,o->field,o->index,o->value);}
 aps_state*s=af_project_state();for(unsigned l=0;l<AS_LANES;l++)aps_clear(s,l,0,AS_STEPS,1,1);
 for(unsigned i=0;i<f->nsteps;i++){const afp_step*t=&f->steps[i];as_edit(&s->sequence,t->lane,t->step,(as_step){1,t->note,t->velocity,t->probability,t->gate});}
 for(unsigned i=0;i<f->nlocks;i++){const afp_lock*l=&f->locks[i];aps_lock_set(s,l->lane,l->step,l->slot,l->parameter,l->value);}
}
int af_patch_confirm(void){
 if(!patch.confirm||patch.pending)return 0;
 if(af_get(0,0,AC_D_GLOBAL,42)>0){patch.notice=AF_PATCH_NOTICE_RANDOM;return 0;}
 if(patch.confirm==AF_PATCH_LOAD&&patch.selection<6){patch.restoring=1;af_patch_restore_prepare();af_set(0,0,AC_D_GLOBAL,20,(float)patch.selection);patch.restoring=0;af_patch_changed();if(patch_retained_hash()==patch.default_retained)patch.saved_revision=patch.revision;patch.status=AF_PATCH_LOADED;patch.confirm=patch.notice=0;return 1;}
 if(patch.confirm==AF_PATCH_LOAD&&patch.selection<AF_PATCH_FACTORY){unsigned k=patch.selection;patch.restoring=1;patch_factory(k-AF_PATCH_ORIGINALS);patch.restoring=0;af_patch_changed();patch.saved_revision=patch.revision;patch.current=k;patch.status=AF_PATCH_LOADED;patch.confirm=patch.notice=0;return 1;}
 patch.slot=patch.selection-AF_PATCH_FACTORY;patch.bytes=0;
 if(patch.confirm==AF_PATCH_SAVE){patch.bytes=af_patch_export(patch_io.bytes,sizeof patch_io.bytes);if(!patch.bytes){patch.status=AF_PATCH_ERROR;patch.notice=AF_PATCH_NOTICE_INVALID;return 0;}}
 patch.captured_revision=patch.revision;patch.pending=patch.confirm;patch.confirm=0;patch.notice=0;patch.status=AF_PATCH_BUSY;if(++patch.id==0)patch.id=1;return 1;
}
int af_patch_complete(unsigned id,int success,unsigned bytes){
 if(!patch.pending||id!=patch.id)return 0;unsigned operation=patch.pending;patch.pending=0;
 if(!success){patch.status=AF_PATCH_ERROR;patch.notice=AF_PATCH_NOTICE_STORAGE;return 1;}
 if(operation==AF_PATCH_LOAD){if(!af_patch_import(patch_io.bytes,bytes)){patch.status=AF_PATCH_ERROR;patch.notice=AF_PATCH_NOTICE_INVALID;return 1;}patch.status=AF_PATCH_LOADED;patch.current=patch.slot+AF_PATCH_FACTORY;}
 else{patch.occupied|=1u<<patch.slot;if(patch.revision==patch.captured_revision)patch.saved_revision=patch.revision;patch.status=AF_PATCH_SAVED;patch.current=patch.slot+AF_PATCH_FACTORY;}
 patch.notice=0;return 1;
}
