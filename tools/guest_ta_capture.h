/* Bounded, opt-in native TA bus recorder. Original IOSretroMob, MIT.
 * Observation only: no GPU state, guest RAM, IRQ or completion is changed.
 * Separate from snapshot-serialized TA retention; oversized streams remain
 * unsupported by that device model until independently decoded and tested. */
#ifndef IOSRETROMOB_GUEST_TA_CAPTURE_H
#define IOSRETROMOB_GUEST_TA_CAPTURE_H
#include "sha256.h"
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>
#define RTA_WORDS 32768u
#define RTA_SCENES 32u
#define RTA_BYTES (4u*1024u*1024u)
static struct {
    bool enabled,failed; char dir[PATH_MAX]; FILE *index;
    uint64_t start,end,total; uint32_t scene,seen,kept,events;
    uint32_t words[RTA_WORDS];
} RTA;
static void rta_fail(void) {
    RTA.failed=true;
    fprintf(stderr,"TA_CAPTURE_FAILURE: stopping the host run; guest state unchanged\n");
    fflush(stderr);raise(SIGINT);
}
static bool rta_parse(uint64_t steps) {
    const char *dir=getenv("IOSRETROMOB_TA_CAPTURE_DIR");
    const char *start=getenv("IOSRETROMOB_TA_CAPTURE_START");
    const char *end=getenv("IOSRETROMOB_TA_CAPTURE_END");
    if(!dir)return !start&&!end;
    char canonical[PATH_MAX];struct stat st;
    if(*dir!='/'||strlen(dir)>=sizeof RTA.dir||
       !realpath(dir,canonical)||strcmp(dir,canonical)||
       lstat(dir,&st)||!S_ISDIR(st.st_mode))return false;
    /* Every ancestor checked without following a symlink. */
    strcpy(canonical,dir);
    for(char *q=canonical+strlen(canonical);q>canonical;q--)if(*q=='/'||*q==0){
        char saved=*q;*q=0;
        if(*canonical&&(lstat(canonical,&st)||S_ISLNK(st.st_mode))){*q=saved;return false;}
        *q=saved;
    }
    RTA.start=0;RTA.end=steps;
    if((start&&!parse_u64_arg("TA capture start",start,&RTA.start))||
       (end&&!parse_u64_arg("TA capture end",end,&RTA.end))||
       RTA.start>=RTA.end||RTA.end>steps)return false;
    char path[PATH_MAX];if(snprintf(path,sizeof path,"%s/index.jsonl",dir)>=(int)sizeof path)return false;
    RTA.index=fopen(path,"wx");if(!RTA.index)return false;
    strcpy(RTA.dir,dir);RTA.enabled=true;
    fprintf(RTA.index,"{\"kind\":\"config\",\"word_order\":\"little-endian\",\"max_words_per_packet\":%u,\"max_packets\":%u,\"max_raw_bytes\":%u,\"scope\":\"native FIFO observation, not rendered image\"}\n",RTA_WORDS,RTA_SCENES,RTA_BYTES);fflush(RTA.index);
    return true;
}
static void rta_access(uint32_t addr,uint32_t value,unsigned width,bool wr) {
    uint64_t at=G.hot_now;
    if(!RTA.enabled||RTA.failed||!g_mach||!wr||width!=4||at<RTA.start||at>=RTA.end)return;
    uint32_t off=addr-S5L8900_MBX_BASE;
    if(off==0x680u&&RTA.events<512){
        uint32_t *r=g_mach->mbx.reg;
        fprintf(RTA.index,"{\"kind\":\"render_request_before_store\",\"at\":%" PRIu64 ",\"last_captured_packet\":%u,\"value\":%u,\"fbctl\":%u,\"stride_words\":%u,\"fbstart\":%u,\"object\":%u,\"region\":%u,\"xclip\":%u,\"yclip\":%u,\"pixsamp\":%u}\n",at,RTA.scene?RTA.scene-1:UINT32_MAX,value,r[0x650/4],r[0x660/4],r[0x65c/4],r[0x60c/4],r[0x608/4],r[0x654/4],r[0x658/4],r[0x61c/4]);
        RTA.events++;fflush(RTA.index);if(ferror(RTA.index))rta_fail();return;
    }
    if(off!=S5L_MBX_3D_DATA_FIFO||RTA.scene>=RTA_SCENES)return;
    RTA.seen++;if(RTA.kept<RTA_WORDS)RTA.words[RTA.kept++]=value;
    if(value!=S5L_MBX_3D_SUBMIT)return;
    uint8_t data[RTA_WORDS*4u],hash[32];
    size_t bytes=RTA.kept*4u;
    bool complete=RTA.seen==RTA.kept&&RTA.kept>=2&&RTA.words[0]==0x10000010u;
    for(unsigned i=0;i<RTA.kept;i++)for(unsigned b=0;b<4;b++)data[4*i+b]=(uint8_t)(RTA.words[i]>>(8*b));
    if(RTA.total+bytes>RTA_BYTES||!ios3_sha256(data,bytes,hash)){rta_fail();return;}
    char name[96],path[PATH_MAX];snprintf(name,sizeof name,"packet-%02u-%s.bin",RTA.scene,complete?"complete":"PREFIX-TRUNCATED");
    if(snprintf(path,sizeof path,"%s/%s",RTA.dir,name)>=(int)sizeof path){rta_fail();return;}
    FILE *f=fopen(path,"wbx");if(!f){rta_fail();return;}
    bool ok=fwrite(data,1,bytes,f)==bytes;if(fclose(f))ok=false;
    if(!ok){rta_fail();return;}
    fprintf(RTA.index,"{\"kind\":\"packet\",\"at\":%" PRIu64 ",\"scene\":%u,\"seen\":%u,\"kept\":%u,\"complete\":%s,\"bytes\":%zu,\"file\":\"%s\",\"sha256\":\"",at,RTA.scene,RTA.seen,RTA.kept,complete?"true":"false",bytes,name);
    for(unsigned i=0;i<32;i++)fprintf(RTA.index,"%02x",hash[i]);
    fputs("\"}\n",RTA.index);fflush(RTA.index);if(ferror(RTA.index))rta_fail();
    RTA.total+=bytes;RTA.scene++;RTA.seen=RTA.kept=0;
}
static void rta_finish(void) {
    if(!RTA.index)return;
    fprintf(RTA.index,"{\"kind\":\"summary\",\"packets\":%u,\"raw_bytes\":%" PRIu64 ",\"failed\":%s,\"packet_limit_reached\":%s,\"pending_seen\":%u,\"pending_kept\":%u}\n",RTA.scene,RTA.total,RTA.failed?"true":"false",RTA.scene==RTA_SCENES?"true":"false",RTA.seen,RTA.kept);
    if(fclose(RTA.index))rta_fail();RTA.index=NULL;RTA.enabled=false;
}
#endif
