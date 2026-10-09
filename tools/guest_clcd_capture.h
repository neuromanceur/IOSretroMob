/* Read-only, opt-in diagnostic live CLCD sampling. S5LBox, MIT.
 * No guest writes, no generated/reference pixels, no stale/off scanout.
 * The caller supplies a new existing private output directory via environment.
 * At most 128 distinct consecutive sampled images; not a frame/FPS counter.
 */
#ifndef S5LBOX_GUEST_CLCD_CAPTURE_H
#define S5LBOX_GUEST_CLCD_CAPTURE_H
#include "soc.h"
#include "sha256.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct {
    bool initialized, enabled, have_previous;
    uint64_t interval, next;
    unsigned count;
    char directory[1024];
    uint8_t previous_digest[32];
} guest_clcd_capture;
static void guest_clcd_capture_init(uint64_t start) {
    if(guest_clcd_capture.initialized)return;
    guest_clcd_capture.initialized=true;
    const char *path=getenv("S5LBOX_CLCD_CAPTURE_DIR");
    const char *period=getenv("S5LBOX_CLCD_CAPTURE_INTERVAL");
    if(!path && !period)return;
    char *end=NULL;errno=0;
    uint64_t interval=period?strtoull(period,&end,10):0;
    if(!path || !path[0] || strlen(path)>=sizeof guest_clcd_capture.directory ||
       !period || !period[0] || period[0]=='-' || errno || !end || *end ||
       interval<1000000 || interval>1000000000 || start>UINT64_MAX-interval) {
        fprintf(stderr,"CLCD_SAMPLE disabled: invalid directory/interval\n");return;
    }
    memcpy(guest_clcd_capture.directory,path,strlen(path)+1);
    guest_clcd_capture.interval=interval;guest_clcd_capture.next=start;
    guest_clcd_capture.enabled=true;
}
static void guest_clcd_capture_sample(const s5l8900_t *m,uint64_t n) {
    if(!guest_clcd_capture.enabled || n<guest_clcd_capture.next)return;
    guest_clcd_capture.next=n+guest_clcd_capture.interval;
    if(guest_clcd_capture.count>=128){guest_clcd_capture.enabled=false;return;}
    if(!s5l_clcd_running(&m->clcd))return;
    unsigned window=s5l_clcd_active_window(&m->clcd);
    if(window==CLCD_WIN_NONE)return;
    uint8_t rgb[320*480*3];uint32_t width=0,height=0;
    if(!s5l_clcd_scanout(&m->clcd,window,m->ram,m->ram_base,m->ram_size,
                       rgb,sizeof rgb,&width,&height))return;
    size_t bytes=(size_t)width*height*3;uint8_t digest[32];ios3_sha256_context_t hash;
    if(!ios3_sha256_init(&hash) || !ios3_sha256_update(&hash,rgb,bytes) ||
       !ios3_sha256_final(&hash,digest))return;
    if(guest_clcd_capture.have_previous && !memcmp(digest,guest_clcd_capture.previous_digest,32))return;
    char path[1152];int length=snprintf(path,sizeof path,"%s/clcd-%04u-at-%" PRIu64 ".ppm",
        guest_clcd_capture.directory,guest_clcd_capture.count,n);
    if(length<0 || (size_t)length>=sizeof path){guest_clcd_capture.enabled=false;return;}
    FILE *out=fopen(path,"wbx");
    if(!out){fprintf(stderr,"CLCD_SAMPLE disabled: create-only output failed (%d)\n",errno);guest_clcd_capture.enabled=false;return;}
    bool ok=fprintf(out,"P6\n%u %u\n255\n",width,height)>0;
    if(ok)ok=fwrite(rgb,1,bytes,out)==bytes;
    if(fclose(out))ok=false;
    if(!ok){fprintf(stderr,"CLCD_SAMPLE disabled: incomplete output %s\n",path);guest_clcd_capture.enabled=false;return;}
    memcpy(guest_clcd_capture.previous_digest,digest,32);guest_clcd_capture.have_previous=true;
    fprintf(stderr,"CLCD_SAMPLE instruction=%" PRIu64 " sample=%u window=%u size=%ux%u rgb_sha256=",
        n,guest_clcd_capture.count,window,width,height);
    for(unsigned i=0;i<32;i++)fprintf(stderr,"%02x",digest[i]);
    fprintf(stderr," path=%s\n",path);guest_clcd_capture.count++;
}
#endif
