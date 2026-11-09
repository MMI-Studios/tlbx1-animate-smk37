/* Boot splash: every pixel that changes between frames lies inside a reported
 * region (hardware redraws only those), bounds hold, and the sequence ends. */
#include "splash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Every pixel differing between consecutive frames must lie inside a reported rect. */
static uint16_t prev[240*240];
int main(void){
 for(unsigned f=0;f<AF_SPLASH_FRAMES;f++){const uint16_t*p=af_splash_render(f);unsigned r[AF_SPLASH_RECTS][4];unsigned n=af_splash_regions(f,r);
  for(unsigned k=0;k<n;k++)assert(r[k][0]+r[k][2]<=240&&r[k][1]+r[k][3]<=240);
  if(f){unsigned missed=0;for(unsigned y=0;y<240;y++)for(unsigned x=0;x<240;x++)if(p[y*240+x]!=prev[y*240+x]){int in=0;for(unsigned k=0;k<n;k++)if(x>=r[k][0]&&x<r[k][0]+r[k][2]&&y>=r[k][1]&&y<r[k][1]+r[k][3])in=1;if(!in){missed++;if(missed<4)fprintf(stderr,"frame %u miss %u,%u\n",f,x,y);}}assert(!missed);}
  memcpy(prev,p,sizeof prev);}
 assert(af_splash_regions(AF_SPLASH_FRAMES,0)==0&&af_splash_pads(AF_SPLASH_FRAMES)==0);
 puts("splash: frame-difference coverage, bounds and end state passed");return 0;}
