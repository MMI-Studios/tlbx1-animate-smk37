/* Same command sequence for preserved host and extracted host. */
static void fixture_render(unsigned blocks,uint32_t*hash,uint64_t*energy){
 int32_t out[CTL*2];while(blocks--){mix_block(out,CTL);for(unsigned i=0;i<CTL*2;i++){*hash=(*hash^(uint32_t)out[i])*16777619u;*energy+=(int64_t)out[i]*out[i];}}
}
int main(int argc,char**argv){
 unsigned preset=argc>1?(unsigned)atoi(argv[1]):0;fixture_init(preset);
 uint32_t hash=2166136261u;uint64_t energy=0;
 fixture_render(30,&hash,&energy);trk_note_on(trk,60,110);fixture_render(400,&hash,&energy);
 trk_note_on(trk,64,100);trk_note_on(trk,67,90);fixture_render(200,&hash,&energy);
 trk_note_off(trk,60);trk_note_off(trk,64);trk_note_off(trk,67);fixture_render(4000,&hash,&energy);
 printf("%u %u %llu\n",preset,hash,(unsigned long long)energy);return 0;
}
