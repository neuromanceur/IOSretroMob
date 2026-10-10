/* Bounded, opt-in multi-window sampler. Original IOSretroMob, MIT. */
#ifndef IOSRETROMOB_GUEST_CLCD_MULTILAYER_H
#define IOSRETROMOB_GUEST_CLCD_MULTILAYER_H
#include "clcd_multilayer_capture.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct {
    bool initialized,enabled,have_previous;
    uint64_t start,end,interval,next,max_bytes,written;
    unsigned count;
    uint8_t previous[32];
    char directory[1024];
} guest_clcd_layers;
static bool guest_clcd_layers_number(const char *key,uint64_t *out) {
    const char *s=getenv(key);if(!s || s[0]<'0' || s[0]>'9')return false;
    char *end=NULL;errno=0;uint64_t v=strtoull(s,&end,10);
    if(errno || !end || *end)return false;
    *out=v;return true;
}
static void guest_clcd_layers_init(void) {
    if(guest_clcd_layers.initialized)return;
    guest_clcd_layers.initialized=true;
    const char *p=getenv("IOSRETROMOB_CLCD_LAYERS_DIR");if(!p)return;
    if(!p[0] || strlen(p)>=sizeof guest_clcd_layers.directory ||
       !guest_clcd_layers_number("IOSRETROMOB_CLCD_LAYERS_START",&guest_clcd_layers.start) ||
       !guest_clcd_layers_number("IOSRETROMOB_CLCD_LAYERS_END",&guest_clcd_layers.end) ||
       !guest_clcd_layers_number("IOSRETROMOB_CLCD_LAYERS_INTERVAL",&guest_clcd_layers.interval) ||
       !guest_clcd_layers_number("IOSRETROMOB_CLCD_LAYERS_MAX_BYTES",&guest_clcd_layers.max_bytes) ||
       guest_clcd_layers.start>guest_clcd_layers.end ||
       guest_clcd_layers.interval<1000000 || guest_clcd_layers.interval>1000000000 ||
       guest_clcd_layers.end>UINT64_MAX-guest_clcd_layers.interval ||
       guest_clcd_layers.max_bytes<32768 || guest_clcd_layers.max_bytes>600000000) {
        fprintf(stderr,"CLCD_LAYERS disabled: invalid bounded configuration\n");return;
    }
    strcpy(guest_clcd_layers.directory,p);guest_clcd_layers.next=guest_clcd_layers.start;
    guest_clcd_layers.enabled=true;
}
static void guest_clcd_layers_sample(const s5l8900_t *m,uint64_t n) {
    if(!guest_clcd_layers.enabled || n<guest_clcd_layers.next)return;
    if(n>guest_clcd_layers.end || guest_clcd_layers.count>=128) {
        guest_clcd_layers.enabled=false;return;
    }
    guest_clcd_layers.next=n+guest_clcd_layers.interval;
    if(!s5l_clcd_running(&m->clcd))return;
    uint8_t digest[32];
    if(!clcd_multilayer_digest(m,digest)) {
        fprintf(stderr,"CLCD_LAYERS disabled: digest failure\n");guest_clcd_layers.enabled=false;return;
    }
    if(guest_clcd_layers.have_previous && !memcmp(digest,guest_clcd_layers.previous,32))return;
    uint64_t written=0;char hash[65];
    if(!clcd_multilayer_write(m,n,guest_clcd_layers.directory,guest_clcd_layers.count,
       guest_clcd_layers.max_bytes-guest_clcd_layers.written,&written,hash)) {
        fprintf(stderr,"CLCD_LAYERS disabled: export failed or byte budget exhausted; partial outputs retained\n");
        guest_clcd_layers.enabled=false;return;
    }
    guest_clcd_layers.written+=written;
    fprintf(stderr,"CLCD_LAYERS instruction=%" PRIu64 " sample=%u bytes=%" PRIu64 " total_bytes=%" PRIu64 " manifest_sha256=%s\n",n,guest_clcd_layers.count,written,guest_clcd_layers.written,hash);
    memcpy(guest_clcd_layers.previous,digest,32);guest_clcd_layers.have_previous=true;guest_clcd_layers.count++;
}
#endif
