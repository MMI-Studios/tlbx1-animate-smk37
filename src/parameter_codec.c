#include "parameter_codec.h"
#include <string.h>
typedef struct {uint16_t offset,maximum;uint8_t bits;float low,high;} ap_field;
static const ap_field ap_fields[20]={
 {0,3,3,0,3},{3,960,10,-48,48},{13,400,9,-100,100},
 {22,100,7,0,1},{29,200,8,-1,1},{37,100,7,0,1},
 {44,4,3,0,4},{47,1000,10,0,1},{57,1,2,0,1},
 {59,7,4,0,7},{63,200,8,0,1},{71,65534,16,0,1},
 {87,200,8,0,1},{95,4,3,0,4},{98,999,10,.001f,1},
 {108,95,7,0,.95f},{115,500,9,0,5},{124,1000,10,0,10},
 {134,100,7,0,1},{141,1000,10,0,10}
};
static unsigned ap_code(const ap_lock*l,const ap_field*f){unsigned value=0;for(unsigned b=0;b<f->bits;b++){unsigned at=f->offset+b;value|=((l->bytes[at/8]>>(at%8))&1u)<<b;}return value;}
static void ap_store(ap_lock*l,const ap_field*f,unsigned value){for(unsigned b=0;b<f->bits;b++){unsigned at=f->offset+b,mask=1u<<(at%8);l->bytes[at/8]=(uint8_t)((l->bytes[at/8]&~mask)|(((value>>b)&1u)<<(at%8)));}}
void ap_lock_clear(ap_lock*l){memset(l->bytes,255,sizeof l->bytes);l->bytes[18]&=127;}
void ap_lock_unset(ap_lock*l,unsigned p){if(p<20)ap_store(l,&ap_fields[p],(1u<<ap_fields[p].bits)-1);}
int ap_lock_set(ap_lock*l,unsigned p,float value){if(p>=20)return 0;const ap_field*f=&ap_fields[p];if(!(value>=f->low&&value<=f->high))return 0;unsigned code=(unsigned)((value-f->low)*f->maximum/(f->high-f->low)+.5f);if(code>f->maximum)code=f->maximum;ap_store(l,f,code);return 1;}
int ap_lock_get(const ap_lock*l,unsigned p,float*value){if(p>=20||!value)return 0;const ap_field*f=&ap_fields[p];unsigned code=ap_code(l,f);if(code>f->maximum)return 0;*value=f->low+(f->high-f->low)*code/f->maximum;return 1;}
int ap_lock_valid(const ap_lock*l){if(l->bytes[18]&128)return 0;for(unsigned p=0;p<20;p++){const ap_field*f=&ap_fields[p];unsigned code=ap_code(l,f);if(code>f->maximum&&code!=((1u<<f->bits)-1))return 0;}return 1;}
float ap_parameter_resolution(unsigned p){return p<20?(ap_fields[p].high-ap_fields[p].low)/ap_fields[p].maximum:0;}
