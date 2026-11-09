/* GPL-3.0-only: oracle uses unchanged preserved host, never a shipping target. */
#define main donor_main
#include "../vendor/melodee/tests/hostsim.c"
#undef main
static void fixture_init(unsigned preset){host_tracks_init();host_preset(trk,ENGI_ANIMATE,preset);for(unsigned i=1;i<NPART;i++)host_preset(&trk[i],ENGI_ANIMATE,0);}
#include "bridge_fixture.h"
