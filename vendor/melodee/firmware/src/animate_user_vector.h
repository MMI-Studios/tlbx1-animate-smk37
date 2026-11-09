/* SPDX-License-Identifier: GPL-3.0-only */
/* Owner's Track 1, evidence/melodee-smk-browser-session.json, 2026-10-31.
 * Decoded session SHA256: 5d8fa30e55d1b563c56e3f8e2517a36793dc4a739306e88e3428289638dc5bd7.
 * Pattern notes, other tracks, master mix and transient Solo are not preset data. */
static const int16_t AN_USER_VECTOR_PARAMS[]={
127,5,60,110,55,0,0,0,0,19,0,106,45,-7,51,7,108,0,2,1,64,0,127,0,0,0,0,0,0,16,2,0,64,0,0,0,0,0,0,0,0,0,0,0,40,0,1,1,127,3,9,63,0,0,0,0,0,0,0,0,0,0,0,127,0,127,0,0,127,0,127,0,0,127,0,127,0,0,127,0,127,0,0,1,127,127,127,127,127,127,127,127,0,0,0,0,64,64,43,94,127,127,127,127
};
_Static_assert(sizeof AN_USER_VECTOR_PARAMS==sizeof(((track_t*)0)->p),"Update captured preset mapping if parameter layout changes");
static const animate_config_t AN_USER_VECTOR_CONFIG={
 .selected=1,.ready=1,.slot={
 {0,125,0,100,0,1,3,40,114,40},
 {1,127,0,100,0,1,3,40,110,40},
 {2,127,0,100,0,1,3,40,110,40},
 {3,127,0,100,0,1,3,40,110,40}},
 .xshape=0,.yshape=0,.yrate=24,.ydepth=64
};
static const animate_motion_t AN_USER_VECTOR_MOTION={.slot={
 {0,127,0,0,0},{36,66,0,0,0},{0,127,0,0,0},{0,127,0,0,0}
}};
static void animate_preset_loaded(track_t *t,unsigned preset)
{
 animate_defaults(t);
 if(preset!=5)return;
 unsigned part=(uint32_t)(t-trk)%NPART;
 memcpy(t->p,AN_USER_VECTOR_PARAMS,sizeof t->p);
 animate_config[part]=AN_USER_VECTOR_CONFIG;
 animate_motion[part]=AN_USER_VECTOR_MOTION;
}
