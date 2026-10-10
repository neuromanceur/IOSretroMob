/* Synthetic native capture fixtures only. Original IOSretroMob, MIT. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "clcd_multilayer_capture.h"
#include "guest_clcd_capture.h"
#include "guest_clcd_multilayer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void window(s5l8900_t *m,unsigned k,uint32_t fb,uint32_t w,uint32_t h,uint32_t stride,uint32_t pos) {
    m->clcd.win[k]=(s5l_clcd_window_t){stride,0x700,fb,(w<<16)|h,w,pos};
}
int main(int argc,char **argv) {
    s5l8900_t *m=calloc(1,sizeof *m);assert(m);uint8_t ram[96];memset(ram,0xee,sizeof ram);
    const uint8_t p0[]={3,2,1,0, 6,5,4,128, 9,8,7,255};
    const uint8_t p1[]={12,11,10,64, 15,14,13,255, 18,17,16,0};
    memcpy(ram,p0,12);memcpy(ram+16,p1,12);
    const uint8_t p2[]={22,21,20,255, 25,24,23,0};
    const uint8_t p3[]={28,27,26,128, 31,30,29,255};
    memcpy(ram+48,p2,8);memcpy(ram+60,p3,8);
    m->ram=ram;m->ram_base=0x08000000;m->ram_size=sizeof ram;
    m->clcd.scanning=true;m->clcd.enable=1;m->clcd.gate=1;
    m->clcd.ctrl=CLCD_CTRL_ENABLE|CLCD_CTRL_WIN0|CLCD_CTRL_WIN1|CLCD_CTRL_WIN2;
    window(m,0,m->ram_base,3,2,16,0x00010002);
    window(m,1,m->ram_base+32,2,1,8,0);m->clcd.win[1].control=0x200;
    window(m,2,m->ram_base+48,2,2,12,0x00030001);
    clcd_native_view_t v,sentinel;memset(&sentinel,0xa5,sizeof sentinel);v=sentinel;
    assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,28,&v)==CLCD_NATIVE_OK);
    assert(v.bytes==28 && v.pixels==ram && v.x==1 && v.y==2 && v.stride==16);
    assert(v.pixels[3]==0 && v.pixels[7]==128 && v.pixels[11]==255);
    v=sentinel;assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,27,&v)==CLCD_NATIVE_MEMORY);assert(!memcmp(&v,&sentinel,sizeof v));
    assert(clcd_native_view(&m->clcd,1,ram,m->ram_base,96,&v)==CLCD_NATIVE_FORMAT);
    assert(clcd_native_view(&m->clcd,3,ram,m->ram_base,96,&v)==CLCD_NATIVE_DISABLED);
    assert(clcd_native_view(&m->clcd,4,ram,m->ram_base,96,&v)==CLCD_NATIVE_DISABLED);
    s5l_clcd_window_t save=m->clcd.win[0];
    for(unsigned fmt=0;fmt<8;fmt++)if(fmt!=3 && fmt!=7){m->clcd.win[0].control=fmt<<8;assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,96,&v)==CLCD_NATIVE_FORMAT);}
    m->clcd.win[0]=save;m->clcd.win[0].control|=1u<<16;assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,96,&v)==CLCD_NATIVE_FORMAT);
    m->clcd.win[0]=save;m->clcd.win[0].stride=11;assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,96,&v)==CLCD_NATIVE_STRIDE);
    m->clcd.win[0].stride=15;assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,96,&v)==CLCD_NATIVE_STRIDE);
    m->clcd.win[0]=save;m->clcd.win[0].geometry=3u<<16;assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,96,&v)==CLCD_NATIVE_GEOMETRY);
    m->clcd.win[0]=save;m->clcd.win[0].fbaddr=m->ram_base-1;assert(clcd_native_view(&m->clcd,0,ram,m->ram_base,96,&v)==CLCD_NATIVE_MEMORY);
    window(m,0,0xfffffffc,2,1,8,0);assert(clcd_native_view(&m->clcd,0,ram,0xfffffffc,96,&v)==CLCD_NATIVE_MEMORY);
    m->clcd.win[0]=save;uint8_t rgb[18];uint32_t w,h;
    const uint8_t want[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18};
    assert(s5l_clcd_scanout(&m->clcd,0,ram,m->ram_base,96,rgb,18,&w,&h));
    assert(w==3 && h==2 && !memcmp(rgb,want,18));
    uint8_t d0[32],d1[32];assert(clcd_multilayer_digest(m,d0));ram[51]^=128;
    assert(clcd_multilayer_digest(m,d1));assert(memcmp(d0,d1,32));ram[51]^=128;
    m->clcd.win[2].position++;assert(clcd_multilayer_digest(m,d1));assert(memcmp(d0,d1,32));m->clcd.win[2].position--;
    m->clcd.frames++;m->clcd.intstatus++;assert(clcd_multilayer_digest(m,d1));assert(!memcmp(d0,d1,32));
    if(argc==3 && !strcmp(argv[1],"--sampler")) {
        guest_clcd_layers_init();
        for(unsigned i=0;i<130;i++) {
            ram[7]^=1;guest_clcd_layers_sample(m,123u+(uint64_t)i*1000000u);
        }
        printf("SAMPLER count=%u enabled=%u bytes=%" PRIu64 "\n",guest_clcd_layers.count,(unsigned)guest_clcd_layers.enabled,guest_clcd_layers.written);
    }
    if(argc==2) {
        uint64_t written=777;char hash[65];
        assert(!clcd_multilayer_write(m,123,argv[1],0,32767,&written,hash));assert(written==777);
        s5l_clcd_t saved=m->clcd;uint8_t original[96];memcpy(original,ram,96);
        assert(clcd_multilayer_write(m,123,argv[1],0,100000,&written,hash));
        assert(!memcmp(&saved,&m->clcd,sizeof saved) && !memcmp(original,ram,96));
        printf("SYNTHETIC manifest0_sha256=%s bytes=%" PRIu64 "\n",hash,written);
        assert(!clcd_multilayer_write(m,123,argv[1],0,100000,&written,hash));
        guest_clcd_capture.enabled=true;guest_clcd_capture.interval=1000000;
        assert(strlen(argv[1])<sizeof guest_clcd_capture.directory);strcpy(guest_clcd_capture.directory,argv[1]);
        guest_clcd_capture_sample(m,123);
        m->clcd.win[2].control=0x300;m->clcd.win[2].stride=8;
        const uint8_t words[]={0xff,0xff,0x00,0xf8,0xee,0xee,0xee,0xee,0xe0,0x07,0x1f,0x00};memcpy(ram+48,words,12);
        assert(clcd_native_view(&m->clcd,2,ram,m->ram_base,96,&v)==CLCD_NATIVE_OK && v.bytes==12);
        assert(clcd_multilayer_write(m,124,argv[1],1,100000,&written,hash));
        printf("SYNTHETIC manifest1_sha256=%s bytes=%" PRIu64 "\n",hash,written);
    }
    m->ram=NULL;free(m);puts("PASS: synthetic CLCD spans, native alpha, formats, bounds and unchanged legacy RGB");return 0;
}
