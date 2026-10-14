/* Optional host tap guard: exact native window hash, no guest modification.
 * Original IOSretroMob, MIT. A mismatch suppresses only the scheduled tap. */
#ifndef IOSRETROMOB_GUEST_TOUCH_GUARD_H
#define IOSRETROMOB_GUEST_TOUCH_GUARD_H
#include "sha256.h"
#include "clcd_multilayer_capture.h"
typedef struct {bool enabled;uint64_t at;unsigned window;uint8_t hash[32];} tap_guard_t;
static tap_guard_t tap_guard, tap_guards[8];
static unsigned tap_guard_count;
static void tap_guard_select(uint64_t at) {
    tap_guard.enabled=false;
    for(unsigned i=0;i<tap_guard_count;i++)if(tap_guards[i].at==at){tap_guard=tap_guards[i];break;}
}
static bool tap_guard_parse_one(const char *s) {
    if(!s)return true;
    char *end=NULL;errno=0;uint64_t at=strtoull(s,&end,10);
    if(errno||end==s||*end!=':'||*s<'0'||*s>'9')return false;
    s=end+1;errno=0;unsigned long window=strtoul(s,&end,10);
    if(errno||end==s||window>=CLCD_WIN_COUNT||*end!=':')return false;
    s=end+1;if(strlen(s)!=64)return false;uint8_t hash[32];
    for(unsigned j=0;j<32;j++){unsigned value=0;
        for(unsigned k=0;k<2;k++){unsigned c=(unsigned char)s[2*j+k];
            if(c>='0'&&c<='9')c-='0';else if(c>='a'&&c<='f')c=c-'a'+10;else return false;
            value=value*16+c;}hash[j]=(uint8_t)value;}
    tap_guard.enabled=true;tap_guard.at=at;tap_guard.window=(unsigned)window;memcpy(tap_guard.hash,hash,32);return true;
}
/* Up to eight independently conditioned taps; transactional parser. */
static bool tap_guard_parse(const char *s) {
    if(!s)return true;
    tap_guard_t parsed[8], saved=tap_guard;unsigned count=0;
    while(*s) {
        const char *z=strchr(s,';');size_t n=z?(size_t)(z-s):strlen(s);
        char one[96];
        if(!n || n>=sizeof one || count>=8)goto invalid;
        memcpy(one,s,n);one[n]=0;
        if(!tap_guard_parse_one(one))goto invalid;
        for(unsigned i=0;i<count;i++)if(parsed[i].at==tap_guard.at)goto invalid;
        parsed[count++]=tap_guard;
        if(!z)break;s=z+1;if(!*s)goto invalid;
    }
    if(!count)goto invalid;
    memcpy(tap_guards,parsed,count*sizeof *parsed);tap_guard_count=count;tap_guard=parsed[0];
    return true;
invalid:
    tap_guard=saved;return false;
}
static bool tap_guard_matches(const s5l8900_t *m) {
    clcd_native_view_t view;
    clcd_native_status_t status=clcd_native_view(&m->clcd,tap_guard.window,m->ram,m->ram_base,m->ram_size,&view);
    if(status!=CLCD_NATIVE_OK&&status!=CLCD_NATIVE_RAW_ORDER)return false;
    uint8_t hash[32];return ios3_sha256(view.pixels,view.bytes,hash)&&!memcmp(hash,tap_guard.hash,32);
}
#endif
