/* GPL-3.0-only. Derived from the preserved eng_animate.c render loop.
 * Explicit float controls activate only this extension path. */
/* Control-derived coefficients are shared by voices; oscillator/envelope/filter
 * history is not. Four exact-key entries per slot also cover common lane locks
 * without allocating a full coefficient bank for every voice. A miss changes
 * only calculation cost, never the parameter precision or rendered result. */
typedef struct {
    float key[11];
    uint32_t mask, valid;
    float pitch, g, k, pole, inverse, attack, decay, release;
    uint32_t position_rate;
} bridge_coefficients_t;
static bridge_coefficients_t bridge_coefficients[4][4];
static uint8_t bridge_coefficients_next[4];
static ANIMATE_HOT const bridge_coefficients_t *bridge_coefficients_get(unsigned slot,
                                                           const float *v,
                                                           uint32_t mask)
{
    const float key[11]={v[1],v[2],v[7],v[8],v[9],v[14],v[15],v[16],v[17],v[19],bridge_tempo};
    const uint32_t relevant=mask&((3u<<1)|(7u<<7)|(7u<<13)|(15u<<16));
#ifndef AB_UNCACHED_COEFFICIENTS
    for(unsigned i=0;i<4;i++){
        const bridge_coefficients_t *c=&bridge_coefficients[slot][i];
        if(c->valid&&c->mask==relevant&&!memcmp(c->key,key,sizeof key))return c;
    }
#endif
    bridge_coefficients_t *c=&bridge_coefficients[slot][bridge_coefficients_next[slot]++&3u];
    memcpy(c->key,key,sizeof key);c->mask=relevant;c->valid=1;
    if(relevant&(3u<<1))c->pitch=av_pow2((v[1]+v[2]/100.f)/12.f);
    if(relevant&(7u<<7)){
        static const float beats[8]={.25f,.5f,1,1.333333333f,2,4,8,16};
        float hz=v[8]?bridge_tempo/(60.f*beats[(unsigned)v[9]]):.01f*av_pow2(v[7]*10.9657843f);
        c->position_rate=(uint32_t)(hz*(4294967296.f/FS));
    }
    if(relevant&(7u<<13)){
        float hz=20.f+v[14]*v[14]*19980.f;hz=bridge_clampf(hz,20.f,FS*.45f);
        float angle=3.14159265f*hz/FS;
        c->g=av_sin(angle)/av_sin(1.57079633f-angle);c->k=2.f-v[15]*1.95f;
        c->pole=1.f-av_exp(-6.2831853f*hz/FS);
        c->inverse=1.f/(1.f+c->g*(c->g+c->k));
    }
    if(relevant&(15u<<16)){
        c->attack=v[16]>0?32767.f/(v[16]*FS):32767.f;
        c->decay=v[17]>0?32767.f/(v[17]*FS):32767.f;
        c->release=v[19]>0?32767.f/(v[19]*FS):32767.f;
    }
    return c;
}
typedef struct {
    int16_t key[5];
    unsigned valid;
    int32_t attack,decay,release;
    tsvf_t filter;
} bridge_native_coefficients_t;
static bridge_native_coefficients_t bridge_native_coefficients[4];
static ANIMATE_HOT const bridge_native_coefficients_t *bridge_native_coefficients_get(unsigned slot,const int16_t *p)
{
    int16_t key[5]={p[AN_ATK-1],p[AN_DEC-1],p[AN_REL-1],p[AN_CUT-1],p[AN_RES-1]};
    bridge_native_coefficients_t *c=&bridge_native_coefficients[slot];
#ifndef AB_UNCACHED_COEFFICIENTS
    if(c->valid&&!memcmp(c->key,key,sizeof key))return c;
#endif
    memcpy(c->key,key,sizeof key);c->valid=1;
    c->attack=32767/(1+(uint32_t)p[AN_ATK-1]*p[AN_ATK-1]*FS/4096u)+1;
    c->decay=32767/(1+(uint32_t)p[AN_DEC-1]*p[AN_DEC-1]*FS/4096u)+1;
    c->release=32767/(1+(uint32_t)p[AN_REL-1]*p[AN_REL-1]*FS/4096u)+1;
    tsvf_coef(&c->filter,p[AN_CUT-1]<<8,p[AN_RES-1]);
    return c;
}
static ANIMATE_HOT uint32_t bridge_vector_rate(unsigned axis)
{
    static struct {float key[4];uint32_t rate,valid;} cache[2];
    const float *p=bridge_vector_lfo[axis];
    const float key[4]={p[1],p[3],p[4],bridge_tempo};
#ifndef AB_UNCACHED_COEFFICIENTS
    if(cache[axis].valid&&!memcmp(cache[axis].key,key,sizeof key))return cache[axis].rate;
#endif
    static const float beats[8]={.25f,.5f,1,1.333333333f,2,4,8,16};
    float hz=p[3]?bridge_tempo/(60.f*beats[(unsigned)p[4]]):p[1];
    cache[axis].rate=(uint32_t)(hz*(4294967296.f/FS));
    memcpy(cache[axis].key,key,sizeof key);cache[axis].valid=1;
    return cache[axis].rate;
}
static ANIMATE_HOT void bridge_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    unsigned vi=(unsigned)(v-t->v);uint32_t masks[4];float values[4][20];
    unsigned changed=0;for(unsigned q=0;q<4;q++){masks[q]=bridge_masks[q]|bridge_lock_masks[vi][q];changed|=masks[q];}
    /* Source selection is native to the preserved kernel. Do not move every
     * active voice onto the float extension path merely to select a table.
     * Apply per-voice sources only for this render, preserving base/lock ownership. */
    if(!(changed&~1u)&&!bridge_vector_override&&!bridge_vector_lfo_masks[0]&&!bridge_vector_lfo_masks[1]){
      animate_config_t*c=animate_config_of(t);int16_t sources[4];
      for(unsigned q=0;q<4;q++){sources[q]=c->slot[q][0];if(masks[q]&1u)c->slot[q][0]=(int16_t)((bridge_lock_masks[vi][q]&1u)?bridge_locks[vi][q][0]:bridge_values[q][0]);}
      animate_render(t,v,out,n,m);
      for(unsigned q=0;q<4;q++)c->slot[q][0]=sources[q];
      animate_part_t*state=animate_part(0);if(state)for(unsigned q=0;q<4;q++)bridge_env[vi][q]=state->slot[vi][q].env;return;
    }
    for(unsigned q=0;q<4;q++)for(unsigned j=0;j<20;j++)values[q][j]=(bridge_lock_masks[vi][q]&(1u<<j))?bridge_locks[vi][q][j]:bridge_values[q][j];
    int16_t slot_params[4][10];memcpy(slot_params,animate_config_of(t)->slot,sizeof slot_params);
    for(unsigned q=0;q<4;q++)if(masks[q]&1u)slot_params[q][0]=(int16_t)values[q][0];
    const bridge_coefficients_t *coefficients[4];
    for(unsigned q=0;q<4;q++)coefficients[q]=bridge_coefficients_get(q,values[q],masks[q]);
    float fg[4],fk[4],fa[4],fi[4];int extended_filter[4];
    for(unsigned q=0;q<4;q++){extended_filter[q]=!(bridge_masks[q]&(1u<<20))&&(masks[q]&((1u<<13)|(1u<<14)|(1u<<15)))!=0;if(!extended_filter[q])continue;
      fg[q]=coefficients[q]->g;fk[q]=coefficients[q]->k;
      fa[q]=coefficients[q]->pole;fi[q]=coefficients[q]->inverse;}
    animate_part_t *part=animate_part((uint32_t)(t-trk));
    if(!part)return;
    animate_config_t *c=animate_config_of(t);
    animate_slot_voice_t *sv=part->slot[v-t->v];
    uint32_t lfo = v->ph[1], lfo_y=v->ph[2];
    /* RATE 0 stops; 127 = 4 Hz. A quadrature sine traces the vector plane. */
    uint32_t rate = (uint32_t)t->p[P_E2] * (uint32_t)(4294967296.0 / (127.0 * FS));
    rate *= 4u;
    uint32_t rate_y=(uint32_t)c->yrate*(uint32_t)(4294967296.0/(127.0*FS))*4u;
    uint32_t vector_rates[2]={rate,rate_y};int32_t vector_depths[2]={t->p[P_E3]*16383/127,c->ydepth*16383/127};
    for(unsigned axis=0;axis<2;axis++)if(bridge_vector_lfo_masks[axis]){
      float*p=bridge_vector_lfo[axis];
      if(bridge_vector_lfo_masks[axis]&26u)vector_rates[axis]=bridge_vector_rate(axis);
      if(bridge_vector_lfo_masks[axis]&4u)vector_depths[axis]=(int32_t)(p[2]*16383.f);
    }
    rate=vector_rates[0];rate_y=vector_rates[1];
    uint32_t inc[4],start[4],end[4],position_rate[4];
    animate_motion_t *motion=&animate_motion[(uint32_t)(t-trk)%NPART];
    int32_t attack[4],decay[4],release[4];tsvf_t filter[4];
    float extended_attack[4],extended_decay[4],extended_release[4];
    for(unsigned s=0;s<4;s++){
      int16_t *p=slot_params[s];inc[s]=cents_inc(m->pitch16+p[AN_TUNE-1]*16,0,v->scale_fine);
      int16_t *w=motion->slot[s];uint32_t len=animate_poc_lengths[p[AN_WAVE-1]];
      start[s]=(uint32_t)w[0]*len/127u;if(start[s]>=len)start[s]=len-1;
      end[s]=(uint32_t)w[1]*len/127u;if(end[s]<=start[s])end[s]=start[s]+1;
      if(masks[s]&((1u<<1)|(1u<<2))){float phase_step=cents_inc(m->pitch16,0,v->scale_fine)*coefficients[s]->pitch;
        /* The phase accumulator is modulo one cycle, even above FS. */
        while(phase_step>=4294967296.f)phase_step-=4294967296.f;inc[s]=(uint32_t)phase_step;}
      if(masks[s]&((1u<<10)|(1u<<11)|(1u<<12))){
        float begin=bridge_looped[vi][s]?values[s][11]:values[s][10];start[s]=(uint32_t)(begin*len);if(start[s]>=len)start[s]=len-1;
        end[s]=(uint32_t)(values[s][12]*len);if(end[s]<=start[s])end[s]=start[s]+1;}
      position_rate[s]=(uint32_t)w[3]*(uint32_t)(4294967296.0/(127.0*FS))*4u;
      if(masks[s]&((1u<<7)|(1u<<8)|(1u<<9))){
       position_rate[s]=coefficients[s]->position_rate;}
      const bridge_native_coefficients_t *native=bridge_native_coefficients_get(s,p);
      attack[s]=native->attack;decay[s]=native->decay;release[s]=native->release;
      filter[s]=native->filter;
      if(masks[s]&((1u<<16)|(1u<<17)|(1u<<18)|(1u<<19))){
        extended_attack[s]=coefficients[s]->attack;
        extended_decay[s]=coefficients[s]->decay;
        extended_release[s]=coefficients[s]->release;
      }
    }
    /* Per-block invariants; exact divide replacements are from eng_animate.c. */
    const int32_t base_x=t->p[P_E0]*32767/127,base_y=t->p[P_E1]*32767/127;
    const int solo=animate_solo[(uint32_t)(t-trk)%NPART]!=0;
    int32_t sustain[4];for(unsigned q=0;q<4;q++)sustain[q]=slot_params[q][AN_SUS-1]*32767/127;
    for (uint32_t i = 0; i < n; i++) {
        int32_t depth = vector_depths[0];
        int32_t lx=c->xshape==1?osc_tri(lfo):c->xshape==2?(int32_t)(lfo>>16)-32768:c->xshape==3?(lfo&0x80000000u?-32767:32767):osc_sine(lfo);
        int32_t ly=c->yshape==1?osc_tri(lfo_y):c->yshape==2?(int32_t)(lfo_y>>16)-32768:c->yshape==3?(lfo_y&0x80000000u?-32767:32767):osc_sine(lfo_y);
        if(bridge_vector_lfo_masks[0]&1u)lx=bridge_lfo(lfo,(unsigned)bridge_vector_lfo[0][0],bridge_vector_noise[vi][0]);
        if(bridge_vector_lfo_masks[1]&1u)ly=bridge_lfo(lfo_y,(unsigned)bridge_vector_lfo[1][0],bridge_vector_noise[vi][1]);
        int32_t x = clamp((bridge_vector_override?(int32_t)(bridge_vector_x*32767):base_x) + mulq15(lx, depth), 0, 32767);
        int32_t y = clamp((bridge_vector_override?(int32_t)(bridge_vector_y*32767):base_y) + mulq15(ly, vector_depths[1]), 0, 32767);
        /* TLBX's 0.999/0.001 smoothing, with at least one Q15 unit of progress. */
        int32_t dx = x - v->s[0], dy = y - v->s[1];
        int32_t qx = ANIMATE_DIV1000(dx), qy = ANIMATE_DIV1000(dy);
        v->s[0] += qx + (dx && qx == 0 ? (dx > 0 ? 1 : -1) : 0);
        v->s[1] += qy + (dy && qy == 0 ? (dy > 0 ? 1 : -1) : 0);
        x = v->s[0]; y = v->s[1];
        int32_t weights[4] = {mulq15(32767-x,32767-y), mulq15(x,32767-y),
                        mulq15(32767-x,y), mulq15(x,y)};
        int32_t sample = 0, pan_left=0,pan_right=0;
        if(solo)
            for(unsigned s=0;s<4;s++)weights[s]=s==(unsigned)c->selected?32767:0;
        for (uint32_t slot = 0; slot < 4; slot++){
            int16_t *p=slot_params[slot];animate_slot_voice_t *s=&sv[slot];
            if(!(masks[slot]&((1u<<16)|(1u<<17)|(1u<<18)|(1u<<19)))){
            if(!v->gate)s->stage=3;
            if(s->stage==0){s->env+=attack[slot];if(s->env>=32767){s->env=32767;s->stage=1;}}
            else if(s->stage==1){s->env-=decay[slot];int32_t sus=sustain[slot];
              if(s->env<=sus){s->env=sus;s->stage=2;}}
            else if(s->stage==2)s->env=sustain[slot];
            else {s->env-=release[slot];if(s->env<0)s->env=0;}
            }
            if(masks[slot]&((1u<<16)|(1u<<17)|(1u<<18)|(1u<<19))){
              float env=bridge_env[vi][slot],sus=values[slot][18]*32767.f;
              if(!v->gate)s->stage=3;
              if(s->stage==0){env+=extended_attack[slot];if(env>=32767.f){env=32767.f;s->stage=1;}}
              else if(s->stage==1){env-=extended_decay[slot];if(env<=sus){env=sus;s->stage=2;}}
              else if(s->stage==2)env=sus;
              else {env-=extended_release[slot];if(env<0)env=0;}
              bridge_env[vi][slot]=env;s->env=(int32_t)env;
            }else bridge_env[vi][slot]=s->env;
            int16_t *w=motion->slot[slot];
            uint32_t max_offset=animate_udiv((end[slot]-start[slot]-1u)*(uint32_t)w[4],0x81020409u,38);
            if(masks[slot]&(1u<<5))max_offset=(uint32_t)((end[slot]-start[slot]-1u)*values[slot][5]);
            unsigned shape=(masks[slot]&(1u<<6))?(unsigned)values[slot][6]:(w[2]==2?3:w[2]==3?2:w[2]);
            uint32_t offset=max_offset?ANIMATE_UDIV65535((uint32_t)(bridge_lfo(s->position_phase,shape,bridge_noise[vi][slot])+32768)*max_offset):0;
            int32_t value=animate_lookup((uint32_t)p[AN_WAVE-1],s->phase,start[slot],end[slot],offset);
            /* Cancel a live control-edit discontinuity, then decay the correction.
             * Unedited/default rendering remains sample-identical to the POC. */
            if(s->window_ready&&(s->last_start!=w[0]||s->last_end!=w[1]||s->last_shape!=w[2]||s->last_rate!=w[3]||s->last_depth!=w[4]))
                s->edit_offset=s->last_output-value;
            s->last_start=w[0];s->last_end=w[1];s->last_shape=w[2];s->last_rate=w[3];s->last_depth=w[4];s->window_ready=1;
            value=clamp(value+s->edit_offset,-32767,32767);s->last_output=value;
            if(s->edit_offset)s->edit_offset-=ANIMATE_SDIV_POW2(s->edit_offset,7)+(s->edit_offset>0?1:-1);
            uint32_t prevpos=s->position_phase;s->position_phase+=position_rate[slot];
            if(s->position_phase<prevpos){uint32_t z=bridge_noise[vi][slot];z^=z<<13;z^=z>>17;z^=z<<5;bridge_noise[vi][slot]=z;}
            if(extended_filter[slot]){
              float x=value/32768.f,*z=bridge_filter[vi][slot],g=fg[slot],k=fk[slot];unsigned type=(unsigned)values[slot][13];
              if(type==4){x=av_tanh(x-values[slot][15]*3.8f*z[3]);for(unsigned j=0;j<4;j++){z[j]+=fa[slot]*(av_tanh(x)-av_tanh(z[j]));x=z[j];}}
              else {float a=fi[slot],v3=x-z[1],bp=a*z[0]+g*a*v3,lp=z[1]+g*a*z[0]+g*g*a*v3;
                z[0]=2*bp-z[0];z[1]=2*lp-z[1];x=type==2?x-k*bp-lp:type==3?bp:lp;
                if(type==0){v3=x-z[3];bp=a*z[2]+g*a*v3;lp=z[3]+g*a*z[2]+g*g*a*v3;z[2]=2*bp-z[2];z[3]=2*lp-z[3];x=lp;}}
              value=(int32_t)(bridge_clampf(x,-1.f,1.f)*32767.f);
            }else if(!(bridge_masks[slot]&(1u<<20))&&!p[AN_BYPASS-1])value=tsvf_lp(&filter[slot],value,&s->ic1,&s->ic2);
            value=clamp(value,-32767,32767);
            int32_t contribution=mulq15(mulq15(value,s->env),weights[slot]);
            contribution=(masks[slot]&(1u<<3))?ANIMATE_DIV127((int32_t)(contribution*values[slot][3])*t->p[P_E4+slot]):ANIMATE_DIV127(ANIMATE_DIV127(contribution*p[AN_LEVEL-1])*t->p[P_E4+slot]);
            sample+=contribution;
            if(masks[slot]&(1u<<4)){float pan=values[slot][4];if(pan>0)pan_left-=(int32_t)(contribution*pan);else pan_right+=(int32_t)(contribution*pan);}
            uint32_t previous=s->phase;s->phase+=inc[slot];if(s->phase<previous){
              bridge_looped[vi][slot]=1;
              if(masks[slot]&((1u<<10)|(1u<<11)|(1u<<12))){unsigned len=animate_poc_lengths[p[0]];
                start[slot]=(uint32_t)(values[slot][11]*len);if(start[slot]>=len)start[slot]=len-1;
                end[slot]=(uint32_t)(values[slot][12]*len);if(end[slot]<=start[slot])end[slot]=start[slot]+1;}
            }
        }
        out[i] += voice_amp(mulq15(sample,v->vel*258), m, i) * 2;
        bridge_pan_l[i]+=voice_amp(mulq15(pan_left,v->vel*258),m,i)*2;
        bridge_pan_r[i]+=voice_amp(mulq15(pan_right,v->vel*258),m,i)*2;
        uint32_t before_x=lfo,before_y=lfo_y;lfo += rate;lfo_y+=rate_y;
        for(unsigned axis=0;axis<2;axis++)if(axis?lfo_y<before_y:lfo<before_x){uint32_t z=bridge_vector_noise[vi][axis];z^=z<<13;z^=z>>17;z^=z<<5;bridge_vector_noise[vi][axis]=z;}
    }
    v->ph[1] = lfo;v->ph[2]=lfo_y;
}


