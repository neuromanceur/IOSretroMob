/* IOSretroMob, MIT. Synthetic clock-hypothesis tests, not hardware proof. */
#include "soc.h"
#include "snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned failed,checks;
#define CHECK(c) do {checks++;if(!(c)){failed++;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);}}while(0)
static void config(s5l_i2s_t *s) {
    s5l_i2s_write(s,0,1);s5l_i2s_write(s,4,0x01100301);
    s5l_i2s_write(s,8,0);s5l_i2s_write(s,0x3c,1);
}
static void controller(void) {
    s5l_i2s_t s;s5l_gpioic_t g;s5l_gpioic_reset(&g);s5l_i2s_reset(&s);config(&s);
    s5l_i2s_tick(&s,6000000,&g,134);
    CHECK(!s.experimental_hz && !s.clock_running && s.frame_edges==0 && !g.driven[4]);
    CHECK(!s5l_i2s_set_experimental_clock(NULL,44100));
    s5l_i2s_t before=s;CHECK(!s5l_i2s_set_experimental_clock(&s,192001));CHECK(!memcmp(&before,&s,sizeof s));
    CHECK(s5l_i2s_set_experimental_clock(&s,44100));CHECK(s.clock_running);
    CHECK(s5l_i2s_next_frame(&s)==137);
    s5l_i2s_tick(&s,0,&g,134);CHECK(s.frame_edges==0);
    s5l_i2s_tick(&s,136,&g,134);CHECK(s.frame_edges==0 && s.frame_phase==5997600);
    s5l_i2s_tick(&s,1,&g,134);CHECK(s.frame_edges==1 && s.frame_phase==41700);
    CHECK(s5l_gpioic_pending(&g,134) && !(g.raw[4]&(1u<<6)));
    s5l_gpioic_write(&g,GPIOIC_INTSTAT+16,1u<<6);CHECK(!s5l_gpioic_pending(&g,134));
    s5l_i2s_tick(&s,0,&g,134);CHECK(!s5l_gpioic_pending(&g,134));
    CHECK(s5l_i2s_next_frame(&s)==136);s5l_i2s_tick(&s,136,&g,134);
    CHECK(s.frame_edges==2 && s.frame_phase==39300 && s5l_gpioic_pending(&g,134));
    CHECK(s5l_i2s_set_experimental_clock(&s,0));before=s;
    s5l_i2s_tick(&s,UINT32_MAX,&g,134);CHECK(!memcmp(&before,&s,sizeof s));
    CHECK(s5l_i2s_set_experimental_clock(&s,44100));
    uint64_t edges=s.frame_edges;s5l_i2s_write(&s,0,0);CHECK(!s.clock_running && !s5l_i2s_next_frame(&s));
    s5l_i2s_tick(&s,6000000,&g,134);CHECK(s.frame_edges==edges);
    s5l_i2s_write(&s,0,1);CHECK(s.clock_running && s.frame_phase==0);
    s5l_i2s_write(&s,4,0x01000301);CHECK(!s.clock_running);
    s5l_i2s_write(&s,4,0x01100301);CHECK(s.clock_running);
    s5l_i2s_write(&s,8,1);CHECK(!s.clock_running);
    s5l_i2s_write(&s,8,6);CHECK(s.clock_running); /* DMA enable does not create samples */
    s5l_i2s_reset(&s);CHECK(!s.experimental_hz && !s.clock_running && !s.frame_edges && !s.clock_generation);
    /* Exact one-second rate, partition invariance, bounded long advance. */
    s5l_i2s_t a,b;s5l_i2s_reset(&a);s5l_i2s_reset(&b);config(&a);config(&b);
    CHECK(s5l_i2s_set_experimental_clock(&a,44100) && s5l_i2s_set_experimental_clock(&b,44100));
    s5l_i2s_tick(&a,6000000,NULL,134);
    for(unsigned i=0;i<6000;i++)s5l_i2s_tick(&b,1000,NULL,134);
    CHECK(a.frame_edges==44100 && a.frame_phase==0 && a.clock_ticks==6000000);
    CHECK(a.frame_edges==b.frame_edges && a.frame_phase==b.frame_phase && a.clock_ticks==b.clock_ticks);
    s5l_i2s_tick(&a,UINT32_MAX,NULL,134);
    CHECK(a.frame_edges==31612109 && a.frame_phase==3709500);
}
static void machine(void) {
    s5l8900_t m;CHECK(s5l8900_init(&m,0,65536));m.cpu_hz=m.tb_hz=6000000;
    CHECK(s5l_i2s_set_experimental_clock(&m.i2s[0],44100));
    m.bus.write32(m.bus.ctx,S5L8900_I2S0_BASE,1);
    m.bus.write32(m.bus.ctx,S5L8900_I2S0_BASE+4,0x01100301);
    m.bus.write32(m.bus.ctx,S5L8900_I2S0_BASE+8,0);
    s5l8900_tick(&m,137);CHECK(m.i2s[0].frame_edges==1 && !m.cpu.irq_line);
    CHECK(m.gpioic.stat[4]&(1u<<6));
    s5l_gpioic_write(&m.gpioic,GPIOIC_INTSTAT+16,1u<<6);
    s5l_gpioic_write(&m.gpioic,GPIOIC_INTEN+16,1u<<6);
    s5l8900_tick(&m,136);CHECK(s5l_gpioic_group_irq(&m.gpioic,4) && !m.cpu.irq_line);
    s5l_vic_write(&m.vic[0],VIC_INTENABLE,1u<<2);s5l8900_tick(&m,0);
    CHECK(m.cpu.irq_line && s5l_vic_vectaddr(&m.vic[0],0)==0x80000002);
    /* CPU IRQ mask belongs to ARM; a real unmasked core vectors normally. */
    m.cpu.cpsr=ARM_MODE_SVC;m.cpu.r[15]=0x100;arm_step(&m.cpu);CHECK(m.cpu.r[15]==0x18);
    s5l_gpioic_write(&m.gpioic,GPIOIC_INTSTAT+16,1u<<6);s5l8900_tick(&m,0);CHECK(!m.cpu.irq_line);
    CHECK(m.dmac[0].config==0 && m.dmac[0].transfers==0 && m.dmac[0].bytes_moved==0);
    const s5l_wake_source_t *src;unsigned n=s5l8900_wake_sources(&src);uint32_t at=0;
    CHECK(s5l8900_next_wake(&m,src,n,&at)==S5L_WAKE_AT && at==136);
    s5l_gpioic_write(&m.gpioic,GPIOIC_INTEN+16,0);
    const s5l_wake_source_t *iis=NULL;for(unsigned j=0;j<n;j++)if(!strcmp(src[j].name,"experimental-iis0-frame"))iis=&src[j];
    CHECK(iis && s5l8900_next_wake(&m,iis,1,&at)==S5L_WAKE_NEVER);
    uint8_t *buf=NULL;size_t len=0;CHECK(snapshot_save_mem(&m,&buf,&len)==SNAP_ERR_CORRUPT && !buf && !len);
    s5l_i2s_reset(&m.i2s[0]);CHECK(s5l_i2s_set_experimental_clock(&m.i2s[0],0));
    CHECK(snapshot_save_mem(&m,&buf,&len)==SNAP_OK);free(buf);s5l8900_free(&m);
}
/* Independent transfer reference: 64 ordered bytes, two bytes per source
 * transfer. Eight assumed stereo frames fit the queue; later frames drain
 * four bytes each. Terminal count must wait for the remaining eight frames. */
static void paced_dma(void) {
    s5l8900_t m;CHECK(s5l8900_init(&m,0,65536));m.cpu_hz=m.tb_hz=6000000;
    s5l_i2s_t *s=&m.i2s[0];CHECK(s5l_i2s_set_experimental_clock(s,44100));config(s);
    for(unsigned i=0;i<64;i++)m.ram[0x100+i]=(uint8_t)(0xa0+i);
    s5l_pl080_write(&m.dmac[0],0x30,1);
    s5l_pl080_write(&m.dmac[0],0x100,0x100);
    s5l_pl080_write(&m.dmac[0],0x104,0x3ca00010);
    s5l_pl080_write(&m.dmac[0],0x108,0);
    s5l_pl080_write(&m.dmac[0],0x10c,0x84240020);
    s5l_pl080_write(&m.dmac[0],0x110,0x8801);
    s5l_vic_write(&m.vic[0],0x10,1u<<16);
    s5l8900_tick(&m,137);
    CHECK(m.dmac[0].bytes_moved==0 && s->tx_level==0 && !m.dmac[0].raw_tc);
    CHECK(!s5l_i2s_tx_ready(s,2));
    s5l_i2s_write(s,8,4);CHECK(!s5l_i2s_tx_ready(s,2));
    s5l_i2s_write(s,8,2);CHECK(!s5l_i2s_tx_ready(s,2));
    s5l_i2s_write(s,8,6);CHECK(s5l_i2s_tx_ready(s,2));
    s5l8900_tick(&m,0);
    /* Four-byte watermark stalls after 30 bytes for width two. */
    CHECK(s->tx_level==30 && s->tx_bytes_in==30 && m.dmac[0].bytes_moved==30);
    CHECK((m.dmac[0].ch[0].ctrl&0xfff)==17 && (m.dmac[0].ch[0].cfg&1) && !m.cpu.irq_line);
    for(unsigned j=0;j<30;j++)CHECK(s->tx_fifo[(s->tx_head+j)%32]==(uint8_t)(0xa0+j));
    const s5l_wake_source_t *src;unsigned n=s5l8900_wake_sources(&src);uint32_t at=0;
    CHECK(s5l8900_next_wake(&m,src,n,&at)==S5L_WAKE_UNKNOWN);
    uint64_t moved=m.dmac[0].bytes_moved;s5l8900_tick(&m,0);
    CHECK(m.dmac[0].bytes_moved==moved && !m.dmac[0].raw_tc);
    s5l_i2s_write(s,0,0);s5l8900_tick(&m,6000000);
    CHECK(s->tx_bytes_out==0 && s->tx_bytes_in==30);
    s5l_i2s_write(s,0,1);
    for(unsigned frame=0;frame<9;frame++) {
        unsigned expected_out=4*(frame+1);
        s5l8900_tick(&m,frame==0?137:136);
        CHECK(s->tx_bytes_out==expected_out);
        unsigned expected_in=30+expected_out;if(expected_in>64)expected_in=64;
        CHECK(s->tx_bytes_in==expected_in && !s->tx_refused);
        for(unsigned j=0;j<s->tx_level;j++)
            CHECK(s->tx_fifo[(s->tx_head+j)%32]==(uint8_t)(0xa0+expected_out+j));
        if(frame<8)CHECK(!m.dmac[0].raw_tc && !m.cpu.irq_line);
    }
    CHECK(m.dmac[0].bytes_moved==64 && m.dmac[0].raw_tc==1 && m.cpu.irq_line);
    CHECK(!(m.dmac[0].ch[0].cfg&1) && s->tx_level==28);
    s5l_pl080_write(&m.dmac[0],8,1);s5l8900_tick(&m,0);CHECK(!m.cpu.irq_line);
    s5l8900_tick(&m,1000);CHECK(s->tx_level==0 && s->tx_bytes_out==64 && !s->tx_refused);
    CHECK(!s5l_i2s_tx_ready(s,3));
    CHECK(!s5l_i2s_tx_write(s,1,3) && s->tx_refused==1);
    CHECK(s5l_i2s_tx_write(s,0x44332211,4));
    CHECK(s->tx_fifo[s->tx_head]==0x11 && s->tx_fifo[(s->tx_head+3)%32]==0x44);
    s5l_i2s_write(s,8,2);moved=s->tx_bytes_out;s5l_i2s_tick(s,6000000,NULL,134);
    CHECK(s->tx_level==4 && s->tx_bytes_out==moved); /* interface off */
    CHECK(s5l_i2s_set_experimental_clock(s,0) && s->tx_level==0 && s->tx_head==0);
    s5l_i2s_reset(s);CHECK(!s->tx_bytes_in && !s->tx_bytes_out && !s->tx_underflow_frames);
    /* Unequal-width DMA drains each staged word losslessly at byte width. */
    CHECK(s5l_i2s_set_experimental_clock(s,44100));config(s);s5l_i2s_write(s,8,6);
    s5l_pl080_write(&m.dmac[0],0x100,0x100);s5l_pl080_write(&m.dmac[0],0x10c,0x84080010);
    s5l_pl080_write(&m.dmac[0],0x110,0x8801);s5l8900_tick(&m,0);
    CHECK(s->tx_level==32 && s->tx_bytes_in==32 && !s->tx_refused);
    for(unsigned j=0;j<32;j++)CHECK(s->tx_fifo[j]==(uint8_t)(0xa0+j));
    for(unsigned i=0;i<8;i++)s5l8900_tick(&m,i==0?137:136);
    CHECK(s->tx_bytes_in==64 && s->tx_bytes_out==32 && s->tx_level==32 && !s->tx_refused);
    s5l8900_free(&m);
}
int main(void){controller();machine();paced_dma();printf("IIS experimental hypothesis: %u checks, %u failures\n",checks,failed);return failed?1:0;}
