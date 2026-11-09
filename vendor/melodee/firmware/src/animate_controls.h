/* SPDX-License-Identifier: GPL-3.0-only */
enum { AN_SLOT, AN_WAVE, AN_LEVEL, AN_TUNE, AN_CUT, AN_RES, AN_BYPASS,
       AN_ATK, AN_DEC, AN_SUS, AN_REL, AN_XSHAPE, AN_XRATE, AN_XDEPTH,
       AN_YSHAPE, AN_YRATE, AN_YDEPTH, AN_START, AN_END,
       AN_POS_SHAPE, AN_POS_RATE, AN_POS_DEPTH, AN_SOLO, AN_COUNT };
static const char *const AN_SLOTS[]={"A","B","C","D"};
static const char *const AN_WAVES[]={"SINE","SAW","BELL","VOICE"};
static const char *const AN_SHAPES[]={"SINE","TRI","SAW","SQUARE"};
static const char *const AN_SWITCH[]={"FILTER","BYPASS"};
static const char *const AN_SOLO_NAMES[]={"OFF","SOLO"};
static int16_t animate_solo[NPART]; /* Temporary per-track audition, not preset data. */
static const param_desc_t AN_DESC[AN_COUNT]={
 {"SLOT",F_ENUM,0,3,0,AN_SLOTS,0},{"SOURCE",F_ENUM,0,3,0,AN_WAVES,0},
 {"LEVEL",F_PCT,0,127,127,0,0},{"TUNE",F_SEMI,-24,24,0,0,0},
 {"CUT",F_CUTOFF,0,127,100,0,0},{"RES",F_PCT,0,127,0,0,0},
 {"FILTER",F_ENUM,0,1,1,AN_SWITCH,0},
 {"ATTACK",F_INT,0,127,3,0,0},{"DECAY",F_INT,0,127,40,0,0},
 {"SUSTAIN",F_PCT,0,127,110,0,0},{"RELEASE",F_INT,0,127,40,0,0},
 {"SHAPE",F_ENUM,0,3,0,AN_SHAPES,0},{"RATE X",F_INT,0,127,24,0,0},
 {"DEPTH X",F_PCT,0,127,64,0,0},
 {"SHAPE",F_ENUM,0,3,0,AN_SHAPES,0},{"RATE Y",F_INT,0,127,31,0,0},
 {"DEPTH Y",F_PCT,0,127,64,0,0},
 {"START",F_PCT,0,127,0,0,0},{"END",F_PCT,0,127,127,0,0},
 {"SHAPE",F_ENUM,0,3,0,AN_SHAPES,0},{"RATE",F_INT,0,127,0,0,0},
 {"DEPTH",F_PCT,0,127,0,0,0},
 {"SOLO",F_ENUM,0,1,0,AN_SOLO_NAMES,0},
};
typedef struct { int16_t selected, ready, slot[4][10], xshape, yshape, yrate, ydepth; } animate_config_t;
static animate_config_t animate_config[NPART];
/* Kept separate to preserve the version-3 browser settings layout. */
typedef struct { int16_t slot[4][5]; } animate_motion_t;
static animate_motion_t animate_motion[NPART];
#ifndef ANIMATE_HOT
#define ANIMATE_HOT
#endif
static ANIMATE_HOT void animate_defaults(track_t *t)
{
 animate_config_t *c=&animate_config[(uint32_t)(t-trk)%NPART];
 memset(c,0,sizeof *c);c->ready=1;c->yrate=t->p[P_E2];c->ydepth=t->p[P_E3];
 animate_solo[(uint32_t)(t-trk)%NPART]=0;
 animate_motion_t *motion=&animate_motion[(uint32_t)(t-trk)%NPART];
 for(unsigned s=0;s<4;s++)for(unsigned j=AN_START;j<AN_SOLO;j++)
  motion->slot[s][j-AN_START]=AN_DESC[j].def;
 for(unsigned s=0;s<4;s++)for(unsigned j=AN_WAVE;j<=AN_REL;j++)
  c->slot[s][j-AN_WAVE]=j==AN_WAVE?(int16_t)s:AN_DESC[j].def;
}
static ANIMATE_HOT animate_config_t *animate_config_of(track_t *t)
{
 animate_config_t *c=&animate_config[(uint32_t)(t-trk)%NPART];
 if(!c->ready)animate_defaults(t);
 return c;
}
static int16_t *animate_value(track_t *t,unsigned id)
{
 animate_config_t *c=animate_config_of(t);
 if(id==AN_SLOT)return &c->selected;
 if(id==AN_SOLO)return &animate_solo[(uint32_t)(t-trk)%NPART];
 if(id>=AN_START&&id<AN_SOLO)return &animate_motion[(uint32_t)(t-trk)%NPART].slot[clamp(c->selected,0,3)][id-AN_START];
 if(id>=AN_WAVE&&id<=AN_REL)return &c->slot[clamp(c->selected,0,3)][id-AN_WAVE];
 switch(id){case AN_XSHAPE:return &c->xshape;case AN_XRATE:return &t->p[P_E2];
 case AN_XDEPTH:return &t->p[P_E3];case AN_YSHAPE:return &c->yshape;
 case AN_YRATE:return &c->yrate;case AN_YDEPTH:return &c->ydepth;default:return 0;}
}
static int animate_config_valid(const animate_config_t *c)
{
 if(c->ready<0||c->ready>1||c->selected<0||c->selected>3||c->xshape<0||c->xshape>3||
 c->yshape<0||c->yshape>3||c->yrate<0||c->yrate>127||c->ydepth<0||c->ydepth>127)return 0;
 for(unsigned s=0;s<4;s++)for(unsigned j=AN_WAVE;j<=AN_REL;j++)
  if(c->slot[s][j-AN_WAVE]<AN_DESC[j].min||c->slot[s][j-AN_WAVE]>AN_DESC[j].max)return 0;
 return 1;
}
typedef struct { uint32_t phase, position_phase; int32_t env, stage, ic1, ic2, last_output, edit_offset;
 int16_t last_start, last_end, last_shape, last_rate, last_depth; uint8_t window_ready; } animate_slot_voice_t;
typedef struct { animate_slot_voice_t slot[NVOICE][4]; } animate_part_t;
static animate_part_t *animate_part(uint32_t part);
