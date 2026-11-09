/* Shared by hardware and synthetic-flash protocol tests. No autonomous writes. */
#define AF_BACKUP_BYTES (32u*4096u+4096u)
static struct {uint32_t nonce,offset,crc,last_offset,last_count;unsigned ready;} af_backup;
static uint32_t af_u7(const uint8_t*p){uint32_t v=0;for(unsigned i=0;i<5;i++)v|=(uint32_t)(p[i]&127)<<(i*7);return v;}
static void af_put7(uint8_t*p,uint32_t v){for(unsigned i=0;i<5;i++){p[i]=v&127;v>>=7;}}
static int af_backup_read(unsigned offset,void*out,unsigned bytes){if(offset>AF_BACKUP_BYTES||bytes>AF_BACKUP_BYTES-offset)return -1;uint8_t*p=out;while(bytes){unsigned before=offset<32u*4096u;unsigned n=before?32u*4096u-offset:AF_BACKUP_BYTES-offset;if(n>bytes)n=bytes;unsigned a=before?AF_STORE_FIRST+offset:AF_STORE_MARKER+offset-32u*4096u;if(af_flash_read(0,a,p,n))return -1;offset+=n;p+=n;bytes-=n;}return 0;}
static int af_store_reply(const uint8_t*request,unsigned status,const uint32_t*values,unsigned count,const uint8_t*data,unsigned bytes){uint8_t wire[320];unsigned at=0;wire[at++]=0xf0;for(unsigned i=0;i<7;i++)wire[at++]=i==4?request[i]|0x40:request[i];wire[at++]=status;for(unsigned i=0;i<count;i++){af_put7(wire+at,values[i]);at+=5;}for(unsigned i=0;i<bytes;i++){wire[at++]=data[i]&127;wire[at++]=data[i]>>7;}wire[at++]=0xf7;return ota_wire_send(wire,at);}
static int af_store_sysex(const uint8_t*p,unsigned n){if(n<7||p[0]!=0x7d||p[1]!=0x41||p[2]!=0x46||p[3]!=1)return 0;uint8_t req[32],data[128];if(n>sizeof req){ota_frame_done();return 1;}memcpy(req,p,n);ota_frame_done();unsigned cmd=req[4],status=0,count=0,bytes=0;uint32_t v[7]={0};
 for(unsigned at=11;at<n;at+=5)if(req[at]>15){(void)af_store_reply(req,3,0,0,0,0);return 1;}
 if(cmd==1&&n==7){v[0]=af_store_device;v[1]=af_store.mounted;v[2]=af_store.unit;v[3]=af_backup.nonce;v[4]=AF_BACKUP_BYTES;v[5]=af_backup.offset;v[6]=~af_backup.crc;count=7;}
 else if(!af_store_device)status=1;
 else if(af_patch_pending())status=2;
 else if(cmd==2&&n==7){af_backup.nonce=fm1_ticks()^af_store.unit^(af_backup.nonce+1);if(!af_backup.nonce)af_backup.nonce=1;af_backup.offset=0;af_backup.crc=~0u;af_backup.last_count=0;af_backup.ready=1;v[0]=af_backup.nonce;v[1]=AF_BACKUP_BYTES;v[2]=0;count=3;}
 else if(cmd==3&&n==22){uint32_t nonce=af_u7(req+7),off=af_u7(req+12),size=af_u7(req+17);if(!af_backup.ready||nonce!=af_backup.nonce||!size||size>128||off>AF_BACKUP_BYTES||size>AF_BACKUP_BYTES-off||(off!=af_backup.offset&&(off!=af_backup.last_offset||size!=af_backup.last_count)))status=3;
 else if(af_backup_read(off,data,size))status=4;else{v[0]=nonce;v[1]=off;v[2]=size;count=3;bytes=size;if(off==af_backup.offset){af_backup.crc=af_store_crc_step(af_backup.crc,data,size);af_backup.last_offset=off;af_backup.last_count=size;af_backup.offset+=size;}}}
 else if(cmd==4&&n==27){uint32_t nonce=af_u7(req+7),crc=af_u7(req+12),size=af_u7(req+17),confirm=af_u7(req+22);if(!af_backup.ready||nonce!=af_backup.nonce||af_backup.offset!=AF_BACKUP_BYTES||size!=AF_BACKUP_BYTES||crc!=~af_backup.crc||confirm!=0x434f4e46)status=3;else{uint32_t check=~0u;for(unsigned off=0;off<AF_BACKUP_BYTES;off+=128){if(af_backup_read(off,data,128)){status=4;break;}check=af_store_crc_step(check,data,128);}if(!status&&~check!=crc)status=5;if(!status){af_store_pause();af_store_claiming=1;int rc=af_storage_initialize(&af_store,af_store.unit);af_store_claiming=0;af_store_resume();status=rc?4:0;af_patch_slots(af_storage_slots(&af_store));}af_backup.ready=0;}}
 else status=6;
 (void)af_store_reply(req,status,v,count,data,bytes);return 1;}
