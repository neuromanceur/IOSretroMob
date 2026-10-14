/* Generic MBX format safety fixtures. MIT, IOSretroMob.
 * Synthetic words and pixels only; include the real parser/rasterizer so
 * malformed arithmetic and source bounds are exercised before guest writes. */
#include "../src/soc/mbx.c"
#include <assert.h>
#include "snapshot.h"
static unsigned checks;
#define VERIFY(x) do { assert(x); checks++; } while(0)
static uint32_t fword(float f) { uint32_t u; memcpy(&u,&f,4);return u; }
static void transformed(uint32_t w[74]) {
    memset(w,0,74*4);w[0]=0xa0000003;
    for(unsigned i=0;i<4;i++)w[1+i*5]=fword(1);
    w[17]=0x100001ef;w[28]=fword(160);w[29]=fword(160);
    w[30]=fword(240);w[31]=fword(-240);w[36]=0xf00200c4;
    for(unsigned i=0;i<4;i++) {unsigned b=37+i*9;
        w[b+1]=fword(i&1?1:-1);w[b+2]=fword(i&2?1:-1);
        w[b+4]=0xffffffff;}
    w[73]=3;
}
static void test_extended_ta_retention(void) {
    s5l8900_t *m=calloc(1,sizeof *m),*copy=calloc(1,sizeof *copy);
    VERIFY(m && copy && s5l8900_init(m,0x08000000,0x00200000));
    VERIFY(s5l8900_init(copy,0x08000000,0x00200000));
    m->mbx.reg[0x1000/4]=0x08001000;
    for(unsigned p=0;p<64;p++)m->bus.write32(m->bus.ctx,0x08001400+p*4,0x08010000+p*4096);
    m->mbx.reg[0x800/4]=1;m->mbx.reg[0x83c/4]=0x00100000;
    VERIFY(s5l_mbx_stage_ta_write(&m->mbx,&m->bus,0x800,1));
    for(unsigned i=0;i<33288;i++) {
        uint32_t v=i==0?0x10000010:i==33287?0xf0000000:0x70000000u+(i*4);
        VERIFY(s5l_mbx_stage_ta_write(&m->mbx,&m->bus,S5L_MBX_3D_DATA_FIFO,v));
        VERIFY(m->mbx.ta_fifo[i]==v);
    }
    VERIFY(m->mbx.ta_fifo_words==33288 && m->mbx.ta_fifo_complete);
    VERIFY(m->mbx.status==0); /* retention alone is never render completion */
    uint8_t *wire=NULL;size_t length=0;
    VERIFY(snapshot_save_mem(m,&wire,&length)==SNAP_OK);
    VERIFY(wire[16]==34);
    VERIFY(snapshot_load_mem(copy,wire,length)==SNAP_OK);
    VERIFY(copy->mbx.ta_fifo_words==33288 && copy->mbx.ta_fifo_complete);
    VERIFY(!memcmp(copy->mbx.ta_fifo,m->mbx.ta_fifo,sizeof m->mbx.ta_fifo));
    free(wire);
    VERIFY(s5l_mbx_stage_ta_write(&m->mbx,&m->bus,0x800,1));
    memset(m->ram+0x50000,0xc6,64);
    for(unsigned i=0;i<65535;i++)VERIFY(s5l_mbx_stage_ta_write(&m->mbx,&m->bus,S5L_MBX_3D_DATA_FIFO,0x70000000));
    VERIFY(!s5l_mbx_stage_ta_write(&m->mbx,&m->bus,S5L_MBX_3D_DATA_FIFO,0xf0000000));
    VERIFY(m->mbx.ta_fifo_words==65535 && !m->mbx.ta_fifo_complete && m->mbx.status==0);
    for(unsigned i=0;i<64;i++)VERIFY(m->ram[0x50000+i]==0xc6);
    s5l8900_free(m);s5l8900_free(copy);free(m);free(copy);
}
int main(void) {
    uint32_t w[74];struct mbx_ta_draw d;const char *why=NULL;
    transformed(w);memset(&d,0,sizeof d);
    VERIFY(mbx_ta_parse_vertices_transformed(w,74,17,36,&d,&why)==MBX_TA_DRAW_OK);
    VERIFY(d.x0==0 && d.x1==320 && d.y0==0 && d.y1==480);
    w[16]=0;VERIFY(mbx_ta_parse_vertices_transformed(w,74,17,36,&d,&why)==MBX_TA_DRAW_BAD);
    transformed(w);w[16]=fword(-1);VERIFY(mbx_ta_parse_vertices_transformed(w,74,17,36,&d,&why)==MBX_TA_DRAW_BAD);
    transformed(w);w[38]=fword(INFINITY);VERIFY(mbx_ta_parse_vertices_transformed(w,74,17,36,&d,&why)==MBX_TA_DRAW_BAD);
    transformed(w);w[1]=fword(3e38f);w[38]=fword(3e38f);VERIFY(mbx_ta_parse_vertices_transformed(w,74,17,36,&d,&why)==MBX_TA_DRAW_BAD);
    transformed(w);w[28]=0;VERIFY(mbx_ta_parse_vertices_transformed(w,74,17,36,&d,&why)==MBX_TA_DRAW_BAD);
    transformed(w);VERIFY(mbx_ta_parse_vertices_transformed(w,73,17,36,&d,&why)==MBX_TA_DRAW_BAD);
    s5l8900_t *m=calloc(1,sizeof *m);VERIFY(m && s5l8900_init(m,0x08000000,65536));
    m->mbx.reg[0x1008u/4]=0x08001000;
    m->bus.write32(m->bus.ctx,0x08001000,0x08002000);
    /* Index order y in bit0, x in bit1. Source BGRA red/green/blue/white. */
    const uint32_t tex[4]={0xffff0000,0xff00ff00,0xff0000ff,0xffffffff};
    for(unsigned i=0;i<4;i++)m->bus.write32(m->bus.ctx,0x08002000+4*i,tex[i]);
    uint8_t target[32],before[32];memset(target,0x55,sizeof target);memcpy(before,target,32);
    d=(struct mbx_ta_draw){.textured=true,.is_morton=true,.texel_format=5,.source=0x00800000,
       .texture_width=512,.texture_height=128,.x0=0,.y0=0,.x1=2,.y1=2,.u0=0,.v0=0,.u1=1,.v1=1};
    uint32_t l=4,t=4,r=0,b=0;uint64_t pixels=0;
#define APPLY() mbx_ta_apply_draw(&m->mbx,&m->bus,&d,0x00802000,4,4,32,2,0,0,4,4,target,&l,&t,&r,&b,&pixels,&why)
    VERIFY(!APPLY());VERIFY(!memcmp(target,before,32) && pixels==0);
    d.texture_width=d.texture_height=3;VERIFY(!APPLY());VERIFY(!memcmp(target,before,32));
    d.texture_width=d.texture_height=2;d.u0=NAN;VERIFY(!APPLY());
    d.u0=0;d.x0=NAN;VERIFY(!APPLY());VERIFY(!memcmp(target,before,32));
    d.x0=0;
    /* The proven C7 title form is pixel-format class 5. Any other class shares
     * the twiddle but has an unmeasured texel width: it must be refused with
     * the framebuffer byte-for-byte unchanged and zero pixels, never guessed. */
    d.texel_format=0;VERIFY(!APPLY());VERIFY(!memcmp(target,before,32) && pixels==0);
    d.texel_format=5;VERIFY(APPLY());VERIFY(pixels==4);
    const uint8_t expected[4][2]={{0,0xf8},{0xe0,7},{0x1f,0},{0xff,0xff}};
    VERIFY(!memcmp(target,expected[0],2));VERIFY(!memcmp(target+2,expected[1],2));
    VERIFY(!memcmp(target+8,expected[2],2));VERIFY(!memcmp(target+10,expected[3],2));
    VERIFY(!memcmp(target+4,before+4,4) && !memcmp(target+16,before+16,16));
    d=(struct mbx_ta_draw){.is_clear=true,.colour=0x00ff0000,.x0=-2,.y0=-2,.x1=2,.y1=2};
    memset(target,0x55,32);pixels=0;VERIFY(APPLY());VERIFY(pixels==4);
    VERIFY(target[0]==0 && target[1]==0xf8 && target[14]==0x55);
    s5l8900_free(m);free(m);

    /* Structural Morton recognition in mbx_ta_texture_state: source mode 0x12
     * with square power-of-two dimensions and exponent fields <= 6 is one
     * family whose pixel-format class is captured, not assumed. A non-square
     * or oversize extent under the same source mode must NOT be recognised.
     * mbx_ta_texture_state scans words[start+1 .. control-3] for a (header,
     * source, sampler) triple, so place the descriptor at index 1. */
    {
        struct mbx_ta_draw td;
        /* proven BGRA8 class 5 header (C7 title form). */
        uint32_t proven[6]={ 0, 0xa6618000u, 0x92011c40u, 0x86084610u, 0, 0 };
        uint32_t cls0[6]  ={ 0, 0x06618000u, 0x92014f20u, 0x86084610u, 0, 0 };
        uint32_t nonsq[6] ={ 0, 0x06518000u, 0x92014f20u, 0x86084610u, 0, 0 };
        uint32_t big[6]   ={ 0, 0x07618000u, 0x92014f20u, 0x86084610u, 0, 0 };
        memset(&td,0,sizeof td);
        VERIFY(mbx_ta_texture_state(proven,0u,6u,&td));
        VERIFY(td.is_morton && td.texel_format==5u && td.texture_width==512u);
        memset(&td,0,sizeof td);
        VERIFY(mbx_ta_texture_state(cls0,0u,6u,&td));
        VERIFY(td.is_morton && td.texel_format==0u && td.texture_width==512u);
        memset(&td,0,sizeof td);
        VERIFY(!mbx_ta_texture_state(nonsq,0u,6u,&td)); /* not square */
        memset(&td,0,sizeof td);
        VERIFY(!mbx_ta_texture_state(big,0u,6u,&td));   /* wf=7 > 512 */
    }
    test_extended_ta_retention();
    printf("%u synthetic checks passed; no guest execution\n",checks);return 0;
}
