/* IOSretroMob: synthetic scene replay through the actual MBX semantic rasterizer.
 * MIT. No Apple TA programs, driver bytes or guest execution. */
#include "mbx_vgp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static int fail(const char *m) { fprintf(stderr,"IOSretroMob replay: %s\n",m);return 1; }
int main(int argc,char **argv) {
    if(argc!=2) return fail("expected fresh output PPM path; scene on stdin");
    unsigned width,height,count,textured,linear,r,g,b;
    if(scanf("%u %u %u %u %u %u %u %u",&width,&height,&count,&textured,&linear,&r,&g,&b)!=8 ||
       !width || width>320 || !height || height>480 || (count!=3 && count!=6) || textured>1 || linear>1 || r>255 || g>255 || b>255) return fail("invalid scene header");
    mbx_vgp_scene s={0};s.vertex_count=count;s.texture_linear=linear!=0;
    s.clear_right=width;s.clear_bottom=height;s.clear_bgra=0xff000000u|(r<<16)|(g<<8)|b;
    if(scanf("%lf %lf %lf %lf",&s.centre_x,&s.scale_x,&s.centre_y,&s.scale_y)!=4) return fail("missing viewport");
    for(unsigned i=0;i<16;i++) if(scanf("%lf",&s.matrix[i])!=1) return fail("missing matrix");
    for(unsigned i=0;i<count;i++) {
        for(unsigned k=0;k<3;k++) if(scanf("%lf",&s.vertex[i].xyz[k])!=1)return fail("missing position");
        for(unsigned k=0;k<4;k++) if(scanf("%lf",&s.vertex[i].rgba[k])!=1)return fail("missing colour");
        for(unsigned k=0;k<2;k++) if(scanf("%lf",&s.vertex[i].uv[k])!=1)return fail("missing UV");
    }
    char extra; if(scanf(" %c",&extra)==1)return fail("trailing scene input");
    uint8_t tex[8*8*4];
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
        uint8_t *t=tex+4*(y*8+x);unsigned dim=(x+y)%2?96:255;
        t[0]=(uint8_t)(x<4?dim:24);t[1]=(uint8_t)(y<4?32:dim);t[2]=(uint8_t)(x>=4?dim:48);t[3]=255;
        if(x==0 && y==0){t[0]=255;t[1]=t[2]=0;}
        if(x==7 && y==0){t[1]=255;t[0]=t[2]=0;}
        if(x==0 && y==7){t[2]=255;t[0]=t[1]=0;}
        if(x==7 && y==7){t[0]=t[1]=255;t[2]=0;}
    }
    if(textured){s.texture_width=s.texture_height=8;s.texture_bytes=sizeof tex;s.texture_rgba=tex;}
    uint8_t *pixels=calloc((size_t)width*height,4);if(!pixels)return fail("allocation");
    const char *why=NULL;uint32_t written=0;
    if(!mbx_vgp_raster(&s,pixels,width,height,0,0,width,height,&written,&why)){free(pixels);return fail(why?why:"raster rejected scene");}
    FILE *f=fopen(argv[1],"wbx");if(!f){free(pixels);return fail("output exists or cannot be created");}
    int ok=fprintf(f,"P6\n%u %u\n255\n",width,height)>0;
    for(size_t i=0;i<(size_t)width*height;i++){uint8_t rgb[3]={pixels[4*i+2],pixels[4*i+1],pixels[4*i]};if(fwrite(rgb,1,3,f)!=3)ok=0;}
    if(fclose(f))ok=0;free(pixels);if(!ok)return fail("output write failed");
    printf("{\"synthetic\":true,\"width\":%u,\"height\":%u,\"pixel_writes_including_clear\":%u}\n",width,height,written);return 0;
}
