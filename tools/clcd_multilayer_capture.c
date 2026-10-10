/* Generic native CLCD capture. Original IOSretroMob addition, MIT.
 * Device model and register provenance: S5LBox, j0shua-SYSON. */
#include "clcd_multilayer_capture.h"
#include "sha256.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

clcd_native_status_t clcd_native_view(const s5l_clcd_t *c, unsigned k,
    const uint8_t *ram, uint32_t base, size_t length, clcd_native_view_t *out) {
    clcd_native_view_t v;
    memset(&v, 0, sizeof v);
    uint32_t fb;
    if (!c || k >= CLCD_WIN_COUNT ||
        !s5l_clcd_window(c,k,&fb,&v.width,&v.height,&v.stride,&v.format,&v.order))
        return CLCD_NATIVE_DISABLED;
    if ((v.format != 7u && v.format != 3u) || v.order != 0u)
        return CLCD_NATIVE_FORMAT;
    if (!v.width || !v.height) return CLCD_NATIVE_GEOMETRY;
    v.bytes_per_pixel = v.format == 7u ? 4u : 2u;
    const uint64_t row = (uint64_t)v.width * v.bytes_per_pixel;
    if (v.stride < row || v.stride % v.bytes_per_pixel)
        return CLCD_NATIVE_STRIDE;
    const uint64_t need = (uint64_t)(v.height-1u)*v.stride + row;
    if (!ram || fb < base || need > UINT64_C(0x100000000)-fb ||
        need > SIZE_MAX || (uint64_t)(fb-base) > length ||
        need > (uint64_t)length-(fb-base)) return CLCD_NATIVE_MEMORY;
    v.pixels=ram+(fb-base);v.bytes=(size_t)need;
    v.registers=c->win[k];v.x=v.registers.position>>16;v.y=v.registers.position&0xffffu;
    if (out) *out=v;
    return CLCD_NATIVE_OK;
}
const char *clcd_native_status_name(clcd_native_status_t s) {
    static const char *const names[]={"ok","disabled","unsupported-format-or-order",
        "empty-geometry","invalid-stride","outside-guest-memory"};
    return (unsigned)s < sizeof names/sizeof names[0] ? names[s] : "invalid-status";
}
static bool add_word(ios3_sha256_context_t *h,uint32_t v) {
    uint8_t b[4]={(uint8_t)v,(uint8_t)(v>>8),(uint8_t)(v>>16),(uint8_t)(v>>24)};
    return ios3_sha256_update(h,b,4);
}
static bool add_controls(ios3_sha256_context_t *h,const s5l_clcd_t *c) {
    const uint32_t words[]={c->enable,c->disable,c->ctrl,c->fifo,c->intmask,
        c->reg1c,c->preenable,c->backdrop,c->update,c->update2,c->gate};
    for(unsigned i=0;i<sizeof words/sizeof words[0];i++) if(!add_word(h,words[i]))return false;
    for(unsigned i=0;i<sizeof c->video/sizeof c->video[0];i++)if(!add_word(h,c->video[i]))return false;
    for(unsigned i=0;i<5;i++)if(!add_word(h,c->wincfg_aux[i]))return false;
    for(unsigned i=0;i<sizeof c->csc/sizeof c->csc[0];i++)if(!add_word(h,c->csc[i]))return false;
    for(unsigned i=0;i<sizeof c->opaque/sizeof c->opaque[0];i++)if(!add_word(h,c->opaque[i]))return false;
    for(unsigned k=0;k<3;k++)for(unsigned i=0;i<256;i++)if(!add_word(h,c->gamma[k][i]))return false;
    return true;
}
bool clcd_multilayer_digest(const s5l8900_t *m,uint8_t digest[32]) {
    if(!m || !digest)return false;
    ios3_sha256_context_t h;
    if(!ios3_sha256_init(&h)||!add_controls(&h,&m->clcd))return false;
    for(unsigned k=0;k<CLCD_WIN_COUNT;k++) {
        const s5l_clcd_window_t *w=&m->clcd.win[k];
        const uint32_t a[]={w->stride,w->control,w->fbaddr,w->geometry,w->linewords,w->position};
        for(unsigned j=0;j<6;j++)if(!add_word(&h,a[j]))return false;
        clcd_native_view_t v;
        clcd_native_status_t s=clcd_native_view(&m->clcd,k,m->ram,m->ram_base,m->ram_size,&v);
        if(!add_word(&h,(uint32_t)s))return false;
        if(s==CLCD_NATIVE_OK && !ios3_sha256_update(&h,v.pixels,v.bytes))return false;
    }
    return ios3_sha256_final(&h,digest);
}
static bool hex_hash(const void *b,size_t n,char hex[65]) {
    ios3_sha256_context_t h;uint8_t d[32];
    if(!ios3_sha256_init(&h)||!ios3_sha256_update(&h,b,n)||!ios3_sha256_final(&h,d))return false;
    for(unsigned i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",d[i]);
    return true;
}
static bool write_file(const char *directory,const char *name,const void *b,size_t n) {
    char p[1200];int z=snprintf(p,sizeof p,"%s/%s",directory,name);
    if(z<0 || (size_t)z>=sizeof p)return false;
    FILE *f=fopen(p,"wbx");if(!f)return false;
    bool ok=fwrite(b,1,n,f)==n;if(fclose(f))ok=false;return ok;
}
static void array(FILE *f,const uint32_t *a,size_t n) {
    fputc('[',f);for(size_t i=0;i<n;i++)fprintf(f,"%s%" PRIu32,i?",":"",a[i]);fputc(']',f);
}
bool clcd_multilayer_write(const s5l8900_t *m,uint64_t n,const char *directory,
    unsigned sample,uint64_t reserve,uint64_t *written,char manifest_hash[65]) {
    if(!m || !directory || !written || !manifest_hash || !s5l_clcd_running(&m->clcd))return false;
    clcd_native_view_t views[4];clcd_native_status_t status[4];char hashes[4][65]={{0}},names[4][128]={{0}};
    uint64_t bytes=0;
    for(unsigned k=0;k<4;k++) {
        status[k]=clcd_native_view(&m->clcd,k,m->ram,m->ram_base,m->ram_size,&views[k]);
        if(status[k]==CLCD_NATIVE_OK) {
            if(views[k].bytes>8u*1024u*1024u)return false;
            bytes+=views[k].bytes;
            if(!hex_hash(views[k].pixels,views[k].bytes,hashes[k]))return false;
            int z=snprintf(names[k],sizeof names[k],"layers-%04u-at-%" PRIu64 "-win%u.raw",sample,n,k);
            if(z<0 || (size_t)z>=sizeof names[k])return false;
        }
    }
    /* Worst-case numeric global registers and all gamma words fit in 32 KiB. */
    if(reserve<32768 || bytes>reserve-32768)return false;
    for(unsigned k=0;k<4;k++)if(status[k]==CLCD_NATIVE_OK &&
        !write_file(directory,names[k],views[k].pixels,views[k].bytes))return false;
    char name[128],path[1200];int z=snprintf(name,sizeof name,"layers-%04u-at-%" PRIu64 ".json",sample,n);
    if(z<0 || (size_t)z>=sizeof name)return false;
    z=snprintf(path,sizeof path,"%s/%s",directory,name);if(z<0 || (size_t)z>=sizeof path)return false;
    FILE *f=fopen(path,"wbx");if(!f)return false;const s5l_clcd_t *c=&m->clcd;
    fprintf(f,"{\"schema\":1,\"scope\":\"native-window-observation;not-a-composite\",\"instruction\":%" PRIu64 ",\"sample\":%u,\"running\":true,\"frames\":%" PRIu64 ",\"updates\":%" PRIu64 ",\"registers\":{",n,sample,c->frames,c->updates);
    fprintf(f,"\"enable\":%u,\"disable\":%u,\"ctrl\":%u,\"fifo\":%u,\"intmask\":%u,\"intstatus\":%u,\"reg1c\":%u,\"preenable\":%u,\"backdrop\":%u,\"update\":%u,\"update2\":%u,\"gate\":%u,",c->enable,c->disable,c->ctrl,c->fifo,c->intmask,c->intstatus,c->reg1c,c->preenable,c->backdrop,c->update,c->update2,c->gate);
    fprintf(f,"\"video\":");array(f,c->video,sizeof c->video/sizeof c->video[0]);
    fprintf(f,",\"window_auxiliary\":");array(f,c->wincfg_aux,5);
    fprintf(f,",\"csc\":");array(f,c->csc,sizeof c->csc/sizeof c->csc[0]);
    fprintf(f,",\"vidcon1_and_timing\":");array(f,c->opaque,sizeof c->opaque/sizeof c->opaque[0]);
    fprintf(f,",\"gamma\":[");for(unsigned k=0;k<3;k++){if(k)fputc(',',f);array(f,c->gamma[k],256);}fprintf(f,"]},\"windows\":[");
    for(unsigned k=0;k<4;k++) {
        const s5l_clcd_window_t *w=&c->win[k];
        fprintf(f,"%s{\"index\":%u,\"status\":\"%s\",\"registers\":{\"stride\":%u,\"control\":%u,\"fbaddr\":%u,\"geometry\":%u,\"linewords\":%u,\"position\":%u}",k?",":"",k,clcd_native_status_name(status[k]),w->stride,w->control,w->fbaddr,w->geometry,w->linewords,w->position);
        if(status[k]==CLCD_NATIVE_OK) {
            const clcd_native_view_t *v=&views[k];
            fprintf(f,",\"width\":%u,\"height\":%u,\"x\":%u,\"y\":%u,\"stride\":%u,\"format_code\":%u,\"order_code\":%u,\"layout\":\"%s\",\"alpha\":\"%s\",\"span_bytes\":%zu,\"row_bytes\":%u,\"padding\":\"inter-row-preserved;final-row-excluded\",\"file\":\"%s\",\"sha256\":\"%s\"",v->width,v->height,v->x,v->y,v->stride,v->format,v->order,v->format==7?"BGRA8-native":"RGB565-little-endian",v->format==7?"native-byte-preserved;blend-semantics-unresolved":"no-native-alpha",v->bytes,v->width*v->bytes_per_pixel,names[k],hashes[k]);
        }
        fputc('}',f);
    }
    fprintf(f,"]}\n");bool ok=!ferror(f);long metadata=ftell(f);if(fclose(f))ok=false;
    if(!ok || metadata<0 || metadata>32768)return false;
    f=fopen(path,"rb");if(!f)return false;ios3_sha256_context_t h;uint8_t d[32],b[4096];size_t got;
    ok=ios3_sha256_init(&h);while(ok && (got=fread(b,1,sizeof b,f))!=0)ok=ios3_sha256_update(&h,b,got);
    if(ferror(f))ok=false;
    if(fclose(f))ok=false;
    if(!ok || !ios3_sha256_final(&h,d))return false;
    for(unsigned j=0;j<32;j++)snprintf(manifest_hash+2*j,3,"%02x",d[j]);
    *written=bytes+(uint64_t)metadata;return true;
}
