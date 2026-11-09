#include <assert.h>
#include <stdio.h>
#include "../src/overload_capture.h"
int main(void){
 af_overload_capture c={0};
 af_overload_record(&c,0,72000,48000,6000,12000,4,59184,24,300);
 assert(!c.valid);
 af_overload_record(&c,1,72000,48000,6000,12000,4,59184,24,300);
 assert(c.valid&&c.value[0]==3000&&c.value[1]==2000);
 assert(c.value[2]==250&&c.value[3]==500&&c.value[4]==250);
 assert(c.value[5]==4&&c.value[6]==2466&&c.value[7]==300);
 af_overload_record(&c,1,24000,20000,2000,2000,1,59184,24,99);
 assert(c.value[0]==3000&&c.value[5]==4&&c.value[7]==300);
 puts("PASS: first triggering buffer retained; stages, voice count and scanner IRQ count preserved");
}
