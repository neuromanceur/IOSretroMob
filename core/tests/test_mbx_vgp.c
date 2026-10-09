/* Own synthetic geometry and optional private captured-stream replay. MIT.
 * Captured shader bytes are supplied externally, never embedded in this file.
 */
#include "soc.h"
#include "mbx_vgp.h"
#include "snapshot.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned passes,failures;
#define CHECK(x) do { if(x) passes++;else {failures++;fprintf(stderr,"FAIL %s:%d: %s\n",__func__,__LINE__,#x);} } while(0)
static mbx_vgp_scene scene(void) {
    mbx_vgp_scene s={0};
    s.matrix[0]=s.matrix[5]=s.matrix[15]=1;s.matrix[10]=-1;
    s.centre_x=s.scale_x=32;s.centre_y=24;s.scale_y=-24;
    s.clear_right=64;s.clear_bottom=48;s.clear_bgra=0xff112233;
    double points[3][3]={{-.7,-.6,0},{.7,-.6,0},{0,.7,0}};
    for(unsigned i=0;i<3;i++) {memcpy(s.vertex[i].xyz,points[i],sizeof points[i]);s.vertex[i].rgba[i]=s.vertex[i].rgba[3]=1;}
    return s;
}
static void rotations(void) {
    for(unsigned angle=0;angle<3;angle++) {
        mbx_vgp_scene s=scene();
        if(angle==1) {s.matrix[0]=s.matrix[5]=0;s.matrix[1]=-1;s.matrix[4]=1;}
        if(angle==2) s.matrix[0]=s.matrix[5]=-1;
        uint8_t pixels[64*48*4]={0},reverse[sizeof pixels];uint32_t count=0;const char *why=NULL;
        CHECK(mbx_vgp_raster(&s,pixels,64,48,0,0,64,48,&count,&why));
        unsigned errors=0,interior=0;
        for(unsigned y=0;y<48;y++) for(unsigned x=0;x<64;x++) {
            /* Independent inverse-rotation + analytic weights for this
             * triangle, not the model's forward transform/edge/clip code. */
            double nx=(x+.5)/32-1,ny=1-(y+.5)/24;
            double rx=angle==0?nx:angle==1?ny:-nx;
            double ry=angle==0?ny:angle==1?-nx:-ny;
            double b=(ry+.6)/1.3,g=(rx+.7-.7*b)/1.4,r=1-b-g;
            if(fabs(r)<1e-9 || fabs(g)<1e-9 || fabs(b)<1e-9)continue;
            uint8_t *p=pixels+4*(64*y+x);
            if(r>0 && g>0 && b>0) {
                unsigned rr=(unsigned)floor(255*r+.5),gg=(unsigned)floor(255*g+.5),bb=(unsigned)floor(255*b+.5);
                errors+=abs((int)p[2]-(int)rr)>1 || abs((int)p[1]-(int)gg)>1 || abs((int)p[0]-(int)bb)>1 || p[3]!=255;interior++;
            } else errors+=p[0]!=0x33 || p[1]!=0x22 || p[2]!=0x11 || p[3]!=255;
        }
        CHECK(errors==0);CHECK(interior>600);CHECK(count>64*48);
        mbx_vgp_vertex tmp=s.vertex[1];s.vertex[1]=s.vertex[2];s.vertex[2]=tmp;
        CHECK(mbx_vgp_raster(&s,reverse,64,48,0,0,64,48,&count,&why));
        CHECK(!memcmp(reverse,pixels,sizeof pixels));
    }
}
static void bounds(void) {
    mbx_vgp_scene s=scene();uint8_t p[64*48*4],before[sizeof p];uint32_t count;const char *why=NULL;
    memset(p,0xa5,sizeof p);memcpy(before,p,sizeof p);
    CHECK(mbx_vgp_raster(&s,p,64,48,10,10,30,30,&count,&why));
    unsigned errors=0;
    for(unsigned y=0;y<48;y++)for(unsigned x=0;x<64;x++)
        if(x<10 || x>=30 || y<10 || y>=30)errors+=memcmp(p+4*(64*y+x),before+4*(64*y+x),4)!=0;
    CHECK(!errors);
    /* Clip all vertices out in z: colour clear succeeds, no triangle writes. */
    for(unsigned i=0;i<3;i++)s.vertex[i].xyz[2]=3;
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));CHECK(count==64*48);
    s=scene();s.vertex[2].xyz[0]=3; /* genuine six-plane clipping */
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));CHECK(count>64*48);
    s=scene();s.matrix[12]=.5; /* nonconstant positive homogeneous w */
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));CHECK(count>64*48);
    for(unsigned bad=0;bad<7;bad++) {
        s=scene();
        if(bad==0)s.vertex[2].xyz[1]=NAN;
        if(bad==1)s.matrix[0]=INFINITY;
        if(bad==2)s.vertex[1].rgba[3]=.5;
        if(bad==3)s.clear_right=65;
        if(bad==4)s.matrix[15]=0;
        if(bad==5)s.scale_y=NAN;
        if(bad==6)s.vertex[0].xyz[0]=1e30;
        memset(p,0xa5,sizeof p);
        CHECK(!mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));CHECK(!memcmp(p,before,sizeof p));
    }
    uint32_t w[800]={0};mbx_vgp_scene out=scene(),original=out;
    CHECK(!mbx_vgp_decode(w,767,&out,&why));CHECK(!memcmp(&out,&original,sizeof out));
    CHECK(!mbx_vgp_decode(w,766,&out,&why));CHECK(!mbx_vgp_decode(NULL,767,&out,&why));
}

/* Independent inverse mapping checks nearest texels, shared seam, UV
 * perspective and orientation. These are synthetic, not guest screenshots. */
static void textures(void) {
    uint8_t tex[16]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,0,255};
    mbx_vgp_scene original=scene();original.vertex_count=6;
    original.texture_width=original.texture_height=2;
    original.texture_rgba=tex;original.texture_bytes=16;
    const double xy[6][2]={{-.6,-.6},{.6,-.6},{.6,.6},{-.6,-.6},{.6,.6},{-.6,.6}};
    for(unsigned i=0;i<6;i++) {
        original.vertex[i].xyz[0]=xy[i][0];original.vertex[i].xyz[1]=xy[i][1];original.vertex[i].xyz[2]=0;
        original.vertex[i].uv[0]=(xy[i][0]+.6)/1.2;original.vertex[i].uv[1]=(xy[i][1]+.6)/1.2;
        for(unsigned k=0;k<4;k++)original.vertex[i].rgba[k]=1;
    }
    uint8_t p[64*48*4],before[sizeof p];uint32_t count;const char *why=NULL;
    for(unsigned variant=0;variant<8;variant++) {
        mbx_vgp_scene s=original;s.texture_linear=variant>=4;unsigned orientation=variant%4;
        if(orientation==1){s.matrix[0]=s.matrix[5]=0;s.matrix[1]=-1;s.matrix[4]=1;}
        if(orientation==2)s.matrix[0]=s.matrix[5]=-1;
        if(orientation==3)s.matrix[12]=.4;
        CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));
        unsigned errors=0,inside=0;
        for(unsigned y=0;y<48;y++)for(unsigned x=0;x<64;x++) {
            double nx=(x+.5)/32-1,ny=1-(y+.5)/24;
            double rx=orientation==1?ny:orientation==2?-nx:nx;
            double ry=orientation==1?-nx:orientation==2?-ny:ny;
            if(orientation==3){rx=nx/(1-.4*nx);ry=ny/(1-.4*nx);}
            uint8_t *pixel=p+4*(64*y+x);
            if(fabs(rx)<.6 && fabs(ry)<.6) {
                unsigned tx=rx>=0,ty=ry>=0;uint8_t *c=tex+4*(2*ty+tx);
                if(s.texture_linear) {
                    /* Closed-form four-corner colour, independent of tap loops. */
                    double a=fmax(0,fmin(1,(rx+.6)/1.2*2-.5));
                    double b=fmax(0,fmin(1,(ry+.6)/1.2*2-.5));
                    unsigned r=(unsigned)floor(255*(1-a-b+2*a*b)+.5);
                    unsigned g=(unsigned)floor(255*a+.5),bl=(unsigned)floor(255*(1-a)*b+.5);
                    errors+=abs((int)pixel[2]-(int)r)>1 || abs((int)pixel[1]-(int)g)>1 || abs((int)pixel[0]-(int)bl)>1 || pixel[3]!=255;
                } else errors+=pixel[0]!=c[2]||pixel[1]!=c[1]||pixel[2]!=c[0]||pixel[3]!=255;
                inside++;
            }else errors+=pixel[0]!=0x33||pixel[1]!=0x22||pixel[2]!=0x11||pixel[3]!=255;
        }
        CHECK(!errors);CHECK(inside>900);CHECK(count==64*48+inside);
    }
    for(unsigned bad=0;bad<7;bad++) {
        mbx_vgp_scene s=original;tex[3]=255;
        if(bad==0)s.vertex[5].uv[1]=NAN;
        if(bad==1)s.vertex[5].xyz[2]=INFINITY;
        if(bad==2)s.texture_bytes=15;
        if(bad==3)s.texture_width=513;
        if(bad==4)s.texture_rgba=NULL;
        if(bad==5)tex[3]=254;
        if(bad==6)s.vertex_count=5;
        memset(p,0xa5,sizeof p);memcpy(before,p,sizeof p);
        CHECK(!mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));CHECK(!memcmp(p,before,sizeof p));
    }
    tex[3]=255;
    /* Clamp coordinates, clipping and scissor remain bounded. */
    mbx_vgp_scene s=original;for(unsigned i=0;i<6;i++){s.vertex[i].uv[0]=-2;s.vertex[i].uv[1]=3;}
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));
    CHECK(p[4*(24*64+32)]==255 && p[4*(24*64+32)+2]==0);
    s=original;s.vertex[1].xyz[0]=2;
    CHECK(mbx_vgp_raster(&s,p,64,48,10,10,30,30,&count,&why));
}

/* Closed-form centre pixel tests: alpha is As*As + Ad*(1-As),
 * unlike the existing premultiplied CoreAnimation sprite path. */
static void blending(void) {
    mbx_vgp_scene s=scene();s.vertex_count=6;s.colour_alpha=true;s.blend_source_alpha=true;
    for(unsigned i=0;i<6;i++) {
        s.vertex[i]=s.vertex[i%3];
        for(unsigned k=0;k<4;k++)s.vertex[i].rgba[k]=0;
        s.vertex[i].rgba[i<3?0:1]=1;s.vertex[i].rgba[3]=.5;
    }
    uint8_t p[64*48*4],before[sizeof p];uint32_t count;const char *why=NULL;
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));
    CHECK(!memcmp(p+4*(24*64+32),(uint8_t[]){13,136,68,159},4));
    for(unsigned i=0;i<3;i++){mbx_vgp_vertex v=s.vertex[i];s.vertex[i]=s.vertex[i+3];s.vertex[i+3]=v;}
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));
    CHECK(!memcmp(p+4*(24*64+32),(uint8_t[]){13,73,132,159},4));
    for(unsigned i=0;i<6;i++)s.vertex[i].rgba[3]=0;
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));
    CHECK(!memcmp(p+4*(24*64+32),(uint8_t[]){51,34,17,255},4));
    s.blend_source_alpha=false;for(unsigned i=0;i<6;i++)s.vertex[i].rgba[3]=.5;
    CHECK(mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));
    CHECK(!memcmp(p+4*(24*64+32),(uint8_t[]){0,0,255,128},4));
    for(unsigned bad=0;bad<3;bad++) {
        s.blend_source_alpha=true;s.colour_alpha=true;s.vertex[5].rgba[3]=.5;
        if(bad==0)s.vertex[5].rgba[3]=NAN;
        if(bad==1)s.vertex[5].rgba[3]=1.1;
        if(bad==2)s.colour_alpha=false;
        memset(p,0xa5,sizeof p);memcpy(before,p,sizeof p);
        CHECK(!mbx_vgp_raster(&s,p,64,48,0,0,64,48,&count,&why));CHECK(!memcmp(p,before,sizeof p));
    }
}

static void setup(s5l8900_t *m) {
    CHECK(s5l8900_init(m,0x08000000,0x00200000));
    if(!m->ram)return;
    m->bus.write32(m->bus.ctx,0x3b001000,0x08003000);
    m->bus.write32(m->bus.ctx,0x3b001008,0x08004000);
    /* Deliberately relocated, unlike the measured guest's object/target. */
    for(unsigned i=0;i<2;i++)m->bus.write32(m->bus.ctx,0x08003000+(0x120+i)*4,0x08010000+i*4096);
    for(unsigned i=0;i<144;i++)m->bus.write32(m->bus.ctx,0x08004000+(0x40+i)*4,0x08080000+i*4096);
    const uint32_t registers[][2]={{0x83c,0x00120000},{0x844,0x00060000},{0x608,0x00060000},{0x60c,0x00120000},
        {0x610,0x30000},{0x674,0},{0x61c,0x20007},{0x650,6},{0x654,0x013f0000},{0x658,0x01cb0000},{0x65c,0x00840000},{0x660,320}};
    for(unsigned i=0;i<sizeof registers/sizeof registers[0];i++)m->bus.write32(m->bus.ctx,0x3b000000+registers[i][0],registers[i][1]);
}
static void submit(s5l8900_t *m,uint32_t *words,uint32_t n) {
    m->bus.write32(m->bus.ctx,0x3b000134,0xffffffff);
    m->bus.write32(m->bus.ctx,0x3b000800,1);
    for(unsigned i=0;i<n;i++)m->bus.write32(m->bus.ctx,0x3b800000,words[i]);
    m->bus.write32(m->bus.ctx,0x3b000134,0x10);
    m->bus.write32(m->bus.ctx,0x3b000680,1);
}
static void replay(const char *path,const char *ppm) {
    FILE *f=fopen(path,"rb");CHECK(f!=NULL);if(!f)return;
    uint8_t raw[4096];size_t bytes=fread(raw,1,sizeof raw,f);CHECK(feof(f));fclose(f);
    CHECK(bytes==767*4 || bytes==783*4 || bytes==791*4 || bytes==807*4);if(bytes!=767*4 && bytes!=783*4 && bytes!=791*4 && bytes!=807*4)return;
    unsigned n=(unsigned)bytes/4;uint32_t words[1024];
    for(unsigned i=0;i<n;i++)words[i]=raw[4*i]|((uint32_t)raw[4*i+1]<<8)|((uint32_t)raw[4*i+2]<<16)|((uint32_t)raw[4*i+3]<<24);
    mbx_vgp_scene s;const char *why=NULL;bool decoded=mbx_vgp_decode(words,n,&s,&why);
    if(!decoded)fprintf(stderr,"Decode refusal: %s\n",why?why:"unspecified");
    CHECK(decoded);
    s5l8900_t m;setup(&m);if(!m.ram)return;
    m.bus.write32(m.bus.ctx,0x3b000800,1);
    for(unsigned i=0;i<n;i++)m.bus.write32(m.bus.ctx,0x3b800000,words[i]);
    CHECK(m.mbx.ta_fifo_complete && m.mbx.ta_fifo_words==n);
    /* Simulate the driver's measured post-TA reuse of the object header.
     * These guest writes must not change the consumed shader input. */
    const unsigned overwritten[]={1,2,7,10,21,25};
    for(unsigned i=0;i<sizeof overwritten/sizeof overwritten[0];i++)
        m.bus.write32(m.bus.ctx,0x08010000+4*overwritten[i],0x12345678+i);
    CHECK(!memcmp(m.mbx.ta_fifo,words,n*4));
    uint8_t *saved=NULL;size_t saved_bytes=0;
    CHECK(snapshot_save_mem(&m,&saved,&saved_bytes)==SNAP_OK);
    if(saved) {
        memset(m.mbx.ta_fifo,0,sizeof m.mbx.ta_fifo);m.mbx.ta_fifo_words=0;m.mbx.ta_fifo_complete=false;
        CHECK(snapshot_load_mem(&m,saved,saved_bytes)==SNAP_OK);
        CHECK(m.mbx.ta_fifo_complete && m.mbx.ta_fifo_words==n);
        CHECK(!memcmp(m.mbx.ta_fifo,words,n*4));free(saved);
    }
    m.bus.write32(m.bus.ctx,0x3b000134,0x10);
    m.bus.write32(m.bus.ctx,0x3b000680,1);
    CHECK(m.mbx_telemetry.completed_3d==1);CHECK(m.mbx_telemetry.rejected_3d==0);
    CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0x4c);
    uint8_t *target=m.ram+0x80000;
    uint8_t *copy=malloc(320*460*4);CHECK(copy!=NULL);if(!copy){s5l8900_free(&m);return;}
    memcpy(copy,target,320*460*4);
    unsigned coloured=0;
    for(unsigned i=0;i<320*460;i++)coloured+=memcmp(target+4*i,(uint8_t[]){0x1e,0x0c,0x0c,0xff},4)!=0;
    if(!s.colour_alpha)CHECK(coloured>10000);CHECK(coloured<80000);
    if(ppm) {
        FILE *o=fopen(ppm,"wbx");CHECK(o!=NULL);
        if(o){fprintf(o,"P6\n320 460\n255\n");for(unsigned i=0;i<320*460;i++){uint8_t rgb[]={target[i*4+2],target[i*4+1],target[i*4]};fwrite(rgb,1,3,o);}fclose(o);}
    }
    if(ppm && s.colour_alpha) {
        size_t len=strlen(ppm);char *name=malloc(len+6);CHECK(name!=NULL);
        if(name){memcpy(name,ppm,len);memcpy(name+len,".bgra",6);FILE *o=fopen(name,"wbx");CHECK(o!=NULL);
            if(o){CHECK(fwrite(target,1,320*460*4,o)==320*460*4);fclose(o);}free(name);}
    }
    /* Every immutable word is checked: mutate all of them, not just an
     * example opcode. Dynamic fields have separate domain/refusal tests. */
    unsigned shift=(n==783||n==807)?16:0,vertices=(n==791||n==807)?6:3;
    unsigned clear=302+shift, uniform=601+shift, draw=721+shift, control=740+shift;
    for(unsigned i=0;i<n;i++) {
        bool variable=(i>uniform && i<uniform+17)||(i>=draw+11 && i<draw+15)||(i>control && i<control+1+vertices*8);
        for(unsigned v=0;v<4;v++)variable|=i==clear+8+5*v||i==clear+9+5*v||i==clear+11+5*v;
        if(variable)continue;
        words[i]^=1;CHECK(!mbx_vgp_decode(words,n,&s,&why));words[i]^=1;
    }
    unsigned mutate[]={5,control+2,uniform+1,control+8,control+1,clear+11};
    for(unsigned i=0;i<sizeof mutate/sizeof mutate[0];i++) {
        unsigned at=mutate[i];uint32_t old=words[at];words[at]=i==4?1:i==5?0x7f123456:0x7fc00000;
        submit(&m,words,n);CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0);
        CHECK(!memcmp(target,copy,320*460*4));words[at]=old;
    }
    /* A known colour shader must never silently ignore external Z state. */
    for(unsigned bad=0;bad<3;bad++) {
        m.bus.write32(m.bus.ctx,0x3b000610,bad==0?0x1382:bad==1?0x1383:0x30000);
        m.bus.write32(m.bus.ctx,0x3b000674,bad==2?0x840000:0);
        submit(&m,words,n);CHECK(!memcmp(target,copy,320*460*4));CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0);
    }
    m.bus.write32(m.bus.ctx,0x3b000610,0x30000);m.bus.write32(m.bus.ctx,0x3b000674,0);
    /* Incomplete scene, unknown FBCTL and a late absent GART page. */
    uint32_t last=words[n-2];words[n-2]=0xf0000000;
    submit(&m,words,n-1);CHECK(!memcmp(target,copy,320*460*4));CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0);words[n-2]=last;
    m.bus.write32(m.bus.ctx,0x3b000650,14);submit(&m,words,n);
    CHECK(!memcmp(target,copy,320*460*4));CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0);m.bus.write32(m.bus.ctx,0x3b000650,6);
    m.bus.write32(m.bus.ctx,0x08004000+(0x40+143)*4,0);submit(&m,words,n);
    CHECK(!memcmp(target,copy,320*460*4));CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0);
    CHECK(m.mbx_telemetry.completed_3d==1);
    free(copy);s5l8900_free(&m);
}
static void replay_texture(const char *path,const char *resource,const char *ppm) {
    FILE *f=fopen(path,"rb");CHECK(f!=NULL);if(!f)return;
    uint8_t raw[4096];size_t bytes=fread(raw,1,sizeof raw,f);CHECK(feof(f));fclose(f);
    unsigned n=(unsigned)bytes/4;
    CHECK(bytes%4==0 && (n==773||n==781||n==803));if(n!=773 && n!=781 && n!=803)return;
    uint32_t words[1024];for(unsigned i=0;i<n;i++)words[i]=raw[4*i]|((uint32_t)raw[4*i+1]<<8)|((uint32_t)raw[4*i+2]<<16)|((uint32_t)raw[4*i+3]<<24);
    mbx_vgp_scene scene;const char *why=NULL;CHECK(mbx_vgp_decode(words,n,&scene,&why));
    CHECK(scene.texture_width==8 && scene.texture_height==8 && scene.vertex_count==(n==803?6u:3u));
    unsigned shift=n==781?8:0,source=725+shift;
    words[source]=(words[source]&0xfffc0000u)|(0x01001000u>>7); /* Relocated texture. */
    s5l8900_t m;setup(&m);if(!m.ram)return;
    m.bus.write32(m.bus.ctx,0x3b001010,0x08005000);
    m.bus.write32(m.bus.ctx,0x08005004,0x08050000);
    f=fopen(resource,"rb");CHECK(f!=NULL);if(!f){s5l8900_free(&m);return;}
    CHECK(fread(m.ram+0x50000,1,256,f)==256);CHECK(fgetc(f)==EOF);fclose(f);
    uint8_t *target=m.ram+0x80000;memset(target,0xa5,320*460*4);
    /* Texture resource, mapping and the complete consumed FIFO must survive
     * v33 save/load. No new serialized state is introduced by the decoder. */
    m.bus.write32(m.bus.ctx,0x3b000800,1);
    for(unsigned i=0;i<n;i++)m.bus.write32(m.bus.ctx,0x3b800000,words[i]);
    uint8_t *saved=NULL;size_t saved_bytes=0;
    CHECK(snapshot_save_mem(&m,&saved,&saved_bytes)==SNAP_OK);
    if(saved) {
        memset(m.mbx.ta_fifo,0,sizeof m.mbx.ta_fifo);m.mbx.ta_fifo_words=0;
        memset(m.ram+0x50000,0,256);
        CHECK(snapshot_load_mem(&m,saved,saved_bytes)==SNAP_OK);
        CHECK(m.mbx.ta_fifo_complete && m.mbx.ta_fifo_words==n);
        CHECK(!memcmp(m.mbx.ta_fifo,words,n*4));
        free(saved);
    }
    submit(&m,words,n);CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0x4c);
    CHECK(m.mbx_telemetry.completed_3d==1);
    if(ppm) {
        f=fopen(ppm,"wbx");CHECK(f!=NULL);
        if(f){fprintf(f,"P6\n320 460\n255\n");for(unsigned i=0;i<320*460;i++){uint8_t c[3]={target[4*i+2],target[4*i+1],target[4*i]};fwrite(c,1,3,f);}fclose(f);}
    }
    uint8_t *before=malloc(320*460*4);CHECK(before!=NULL);if(!before){s5l8900_free(&m);return;}
    memcpy(before,target,320*460*4);
    /* A different valid GPU resource must drive the result: no diagnostic
     * checkerboard is embedded in the decoder or sampler. */
    for(unsigned i=0;i<64;i++)memcpy(m.ram+0x50000+4*i,(uint8_t[]){17,93,201,255},4);
    submit(&m,words,n);CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0x4c);
    CHECK(m.mbx_telemetry.completed_3d==2);
    unsigned changed=0,errors=0;
    for(unsigned i=0;i<320*460;i++) {
        bool background=!memcmp(before+4*i,(uint8_t[]){30,12,12,255},4);
        errors+=memcmp(target+4*i,background?(uint8_t[]){30,12,12,255}:(uint8_t[]){17,93,201,255},4)!=0;
        changed+=!background;
    }
    CHECK(errors==0 && changed>10000);
    memcpy(before,target,320*460*4);
    /* Every immutable program/state bit still rejects; no fallback path. */
    for(unsigned i=0;i<n;i++) {
        uint32_t original=words[i];words[i]^=1;
        bool mutable=(i>=602+shift && i<618+shift)||(i>=732+shift && i<736+shift)||
            (i>=741+shift && i<741+shift+scene.vertex_count*10)||i==source;
        for(unsigned v=0;v<4;v++){unsigned base=317+shift+v*5;mutable|=i==base+1||i==base+2||i==base+4;}
        if(!mutable)CHECK(!mbx_vgp_decode(words,n,&scene,&why));
        words[i]=original;
    }
    for(unsigned bad=0;bad<15;bad++) {
        uint32_t saved=words[source];uint32_t vertex=741+shift,uv=vertex+8;
        uint32_t saveduv=words[uv],sampler=words[source+1];uint32_t format=words[source]&0xfffc0000u;uint8_t alpha=m.ram[0x500ff];
        if(bad==0)words[source]^=0x04000000u; /* Unknown layout/format. */
        if(bad==1)words[uv]=0x7fc00000u;
        if(bad==2)m.ram[0x500ff]=254; /* Last texel invalid: atomic. */
        if(bad==3)m.bus.write32(m.bus.ctx,0x08005004,0);
        if(bad==4)words[source]=format|(0x00840000u>>7); /* VA alias. */
        if(bad==5)m.bus.write32(m.bus.ctx,0x08005004,0x08080000); /* PA alias. */
        if(bad==6)m.bus.write32(m.bus.ctx,0x08004000+(0x40+143)*4,0);
        if(bad==7)m.bus.write32(m.bus.ctx,0x3b000650,14);
        if(bad==8)words[source+1]^=1; /* Unknown sampler. */
        if(bad==9)words[source]=format; /* Null source. */
        if(bad==10)words[source]=format|0x3ffffu; /* Address+256 overflows GPU window. */
        if(bad==11)words[source]=format|(0x01001f80u>>7); /* Unsupported source page span. */
        if(bad==12)m.bus.write32(m.bus.ctx,0x3b000610,0x1382);
        if(bad==13)m.bus.write32(m.bus.ctx,0x3b000610,0x1383);
        if(bad==14)m.bus.write32(m.bus.ctx,0x3b000674,0x840000);
        submit(&m,words,n);CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0);CHECK(!memcmp(before,target,320*460*4));
        words[source]=saved;words[uv]=saveduv;words[source+1]=sampler;m.ram[0x500ff]=alpha;
        m.bus.write32(m.bus.ctx,0x08005004,0x08050000);
        m.bus.write32(m.bus.ctx,0x08004000+(0x40+143)*4,0x08080000+143*4096);
        m.bus.write32(m.bus.ctx,0x3b000650,6);
        m.bus.write32(m.bus.ctx,0x3b000610,0x30000);m.bus.write32(m.bus.ctx,0x3b000674,0);
    }
    submit(&m,words,n-1);CHECK(m.bus.read32(m.bus.ctx,0x3b00012c)==0);CHECK(!memcmp(before,target,320*460*4));
    free(before);s5l8900_free(&m);
}
int main(int argc,char **argv) {
    rotations();bounds();textures();blending();
    if(argc>3)replay_texture(argv[1],argv[2],argv[3]);
    else if(argc>1)replay(argv[1],argc>2?argv[2]:NULL);
    printf("MBX VGP: %u checks passed, %u failed\n",passes,failures);
    return failures?1:0;
}
