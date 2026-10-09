/* S5LBox measured opaque fixed-function VGP triangles and textures. MIT.
 *
 * Semantic specialization, NOT a guessed instruction interpreter: recognize
 * the entire measured shader/state envelope with SHA-256, excluding only
 * independently decoded vertex inputs, uniform-0 MVP, viewport and colour
 * clear rectangle, UV coordinates and a texture resource address. Program uploads, selection, ancillary clear/setup and all
 * remaining state must match exactly. Six measured envelopes are allowed.
 * No process name, CPU PC, GPU address or diagnostic vertex/angle is tested.
 * Unknown programs/states and nonopaque colours remain unsupported.
 *
 * Evidence and remaining ISA limitations: reports/unity-compatibility/
 * mbx-triangle-2026-10-09/VGP_DECODER_REPORT.md and
 * mbx-textures-2026-10-09/TEXTURE_DECODER.md.
 */
#include "mbx_vgp.h"
#include "../../../tools/sha256.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static bool reject(const char **why, const char *message) {
    if (why) *why = message;
    return false;
}
static double number(uint32_t word) {
    float f;
    memcpy(&f, &word, sizeof f);
    return f;
}
static bool bounded(double value) {
    return isfinite(value) && fabs(value) <= 1000000.0;
}
static bool integer(double value, uint32_t *out) {
    if (!isfinite(value) || value < 0 || value > 4096 || floor(value) != value)
        return false;
    *out = (uint32_t)value;
    return true;
}
bool mbx_vgp_decode(const uint32_t *words, uint32_t count,
                    mbx_vgp_scene *scene, const char **why) {
    if (!words || !scene || (count != 767 && count != 783 && count != 773 && count != 781 && count != 803))
        return reject(why, "VGP program/state envelope length is unsupported");
    const bool textured=count==773 || count==781 || count==803;
    const uint32_t shift=textured?(count==781?8:0):count-767;
    const uint32_t vertices=count==803?6:3, stride=textured?10:8;
    const uint32_t clear = (textured?310:302) + shift, uniform = 601 + shift;
    const uint32_t draw = 721 + shift, control = 740 + shift;
    uint32_t normalized[803];
    memcpy(normalized, words, count * sizeof *words);
    for (uint32_t v = 0; v < 4; v++) {
        uint32_t base = clear + 7 + v * 5;
        normalized[base + 1] = normalized[base + 2] = 0;
        normalized[base + 4] = 0;
    }
    for (uint32_t i = uniform + 1; i < uniform + 17; i++) normalized[i] = 0;
    for (uint32_t i = draw + 11; i < draw + 15; i++) normalized[i] = 0;
    for (uint32_t i = control + 1; i < control + 1 + vertices*stride; i++) normalized[i] = 0;
    if(textured)normalized[draw+4]&=~0x0003ffffu;
    const bool linear=textured && count==803 && (words[draw+4]&0x80000000u);
    static const uint8_t signatures[6][32] = {
        {0xc0,0x85,0x8d,0x38,0x69,0x11,0xa6,0xcd,0xb2,0xcf,0x4f,0x43,0x5e,0x14,0xaa,0xf1,
         0xdb,0x3b,0x62,0xa7,0x6c,0xa3,0xd6,0x5c,0x79,0xdc,0x85,0x2e,0x32,0xc4,0x15,0xc9},
        {0x65,0x22,0x50,0x78,0x33,0x44,0x54,0x39,0x2d,0xe0,0x57,0x97,0xb8,0xbb,0x0c,0x1f,
         0x2b,0xc5,0xa0,0x52,0x60,0x1e,0xdf,0x23,0xff,0xd2,0x70,0xe1,0x5c,0xfd,0xd5,0x88},
        {0x54,0x77,0x15,0x99,0x5c,0x2d,0x20,0xcd,0x21,0x16,0x98,0x49,0x1e,0x2f,0x17,0x7d,
         0x92,0x43,0x6b,0x98,0x10,0x7b,0xcd,0xf8,0xbd,0x86,0xae,0xd2,0x56,0xbd,0x28,0xb6},
        {0xdf,0x05,0xec,0x0e,0x49,0x2e,0xb0,0x89,0xb6,0x07,0x84,0x1d,0xbe,0x40,0xe4,0xc5,
         0x39,0xe9,0x3a,0xcb,0xc1,0xa3,0xa4,0x68,0x93,0x84,0xee,0x31,0x42,0x29,0x97,0x98},
        {0xa8,0x26,0xeb,0x82,0xa4,0x7d,0x97,0x80,0xc2,0x28,0x0a,0x03,0xcd,0x0a,0x1f,0x79,
         0x9b,0x2c,0xf4,0x01,0x32,0x36,0xb7,0x9a,0x55,0x83,0x4e,0xbf,0x70,0x2d,0x5c,0xbf},
        {0x89,0x3e,0x43,0x4a,0xeb,0xc8,0x66,0xc6,0x46,0xb4,0x56,0xbc,0xbe,0x7b,0x2e,0x09,0x45,0x49,0x55,0x11,0x03,0x0d,0x04,0x00,0x5d,0xd4,0x4e,0xd7,0x34,0x0a,0x4f,0x7e},
    };
    /* Hash explicit little-endian bytes, independent of host endianness. */
    ios3_sha256_context_t hash;
    uint8_t digest[32];
    if (!ios3_sha256_init(&hash)) return reject(why, "VGP hash init failed");
    for (uint32_t i = 0; i < count; i++) {
        uint32_t w = normalized[i];
        uint8_t bytes[4] = {(uint8_t)w,(uint8_t)(w>>8),(uint8_t)(w>>16),(uint8_t)(w>>24)};
        if (!ios3_sha256_update(&hash, bytes, 4)) return reject(why, "VGP hash update failed");
    }
    if (!ios3_sha256_final(&hash, digest) ||
        memcmp(digest, signatures[linear?5:textured?(count==773?2:count==781?3:4):(shift?1:0)], sizeof digest))
        return reject(why, "VGP program or fixed render state is unknown");
    mbx_vgp_scene out = {0};
    out.vertex_count=vertices;
    if(textured) {
        /* Actual glTexImage2D RGBA8 8x8 upload is opaque BGRA8 Morton,
         * y in even bits. 0x12 differs from sprites' linear 0x0e family.
         * All header/sampler/format bits remain part of the exact signature. */
        out.texture_source=(words[draw+4]&0x3ffffu)<<7;
        out.texture_width=out.texture_height=8;
        out.texture_linear=linear;
        if(!out.texture_source || out.texture_source>0x02000000u-256u)
            return reject(why,"VGP texture address is invalid");
    }
    for (uint32_t i = 0; i < 16; i++) {
        out.matrix[i] = number(words[uniform + 1 + i]);
        if (!bounded(out.matrix[i])) return reject(why, "VGP matrix is nonfinite or unbounded");
    }
    out.centre_x = number(words[draw + 11]); out.scale_x = number(words[draw + 12]);
    out.centre_y = number(words[draw + 13]); out.scale_y = number(words[draw + 14]);
    if (!bounded(out.centre_x) || !bounded(out.centre_y) ||
        !bounded(out.scale_x) || !bounded(out.scale_y) ||
        out.scale_x <= 0 || out.scale_y >= 0 ||
        fabs(out.centre_x) > 4096 || fabs(out.centre_y) > 4096 ||
        out.scale_x > 4096 || out.scale_y < -4096)
        return reject(why, "VGP viewport is unsupported");
    for (uint32_t v = 0; v < vertices; v++) {
        uint32_t base = control + 1 + stride * v;
        if (words[base] != 0) return reject(why, "VGP vertex prefix is unknown");
        for (uint32_t k = 0; k < 3; k++) {
            out.vertex[v].xyz[k] = number(words[base + 1 + k]);
            if (!bounded(out.vertex[v].xyz[k])) return reject(why, "VGP position is nonfinite or unbounded");
        }
        for (uint32_t k = 0; k < 4; k++) {
            double f = number(words[base + 4 + k]);
            if (!isfinite(f) || f < 0 || f > 1 || (k == 3 && f != 1))
                return reject(why, "VGP nonopaque or invalid colour is unsupported");
            out.vertex[v].rgba[k] = f;
        }
    }
    if(textured)for(uint32_t v=0;v<vertices;v++)for(uint32_t k=0;k<2;k++) {
        out.vertex[v].uv[k]=number(words[control+1+stride*v+8+k]);
        if(!bounded(out.vertex[v].uv[k]))return reject(why,"VGP UV is nonfinite or unbounded");
    }
    uint32_t cx[4], cy[4];
    /* b00e4000 clear supplies packed RGBA (R in the low byte), whereas
     * the target is BGRA8. glClearColor(.05,.05,.12,1) measures ff1e0c0c. */
    uint32_t clear_rgba = words[clear + 11];
    if ((clear_rgba >> 24) != 255) return reject(why, "VGP clear must be opaque");
    out.clear_bgra = (clear_rgba & 0xff00ff00u) |
        ((clear_rgba & 0xffu) << 16) | ((clear_rgba >> 16) & 0xffu);
    for (uint32_t v = 0; v < 4; v++) {
        uint32_t base = clear + 7 + v * 5;
        if (!integer(number(words[base + 1]), &cx[v]) ||
            !integer(number(words[base + 2]), &cy[v]) ||
            words[base + 4] != clear_rgba)
            return reject(why, "VGP clear rectangle is invalid");
    }
    if (cx[0] != cx[1] || cx[2] != cx[3] || cy[0] != cy[2] ||
        cy[1] != cy[3] || cx[0] >= cx[2] || cy[0] >= cy[1])
        return reject(why, "VGP clear corners are unsupported");
    out.clear_left = cx[0]; out.clear_right = cx[2];
    out.clear_top = cy[0]; out.clear_bottom = cy[1];
    *scene = out;
    return true;
}

typedef struct { double p[4], c[4], uv[2]; } clip_vertex;
typedef struct { double x,y,iw,c[4],uv[2]; } screen_vertex;
static double distance(const clip_vertex *v, unsigned plane) {
    return v->p[3] + (plane & 1 ? -v->p[plane/2] : v->p[plane/2]);
}
static double edge(const screen_vertex *a, const screen_vertex *b, double x,double y) {
    return (b->x-a->x)*(y-a->y) - (b->y-a->y)*(x-a->x);
}
static bool inclusive(const screen_vertex *a, const screen_vertex *b) {
    return b->y < a->y || (b->y == a->y && b->x > a->x);
}
static void store(uint8_t *p, uint32_t colour) {
    p[0]=(uint8_t)colour; p[1]=(uint8_t)(colour>>8);
    p[2]=(uint8_t)(colour>>16); p[3]=(uint8_t)(colour>>24);
}
static bool prepare_triangle(const mbx_vgp_scene *s, unsigned first,
                             screen_vertex screen[16], unsigned *vertices,
                             const char **why) {
    clip_vertex a[16], b[16];
    for (unsigned k=0;k<16;k++)
        if (!bounded(s->matrix[k])) return reject(why, "VGP matrix is invalid");
    for (unsigned v=0;v<3;v++) {
        for(unsigned k=0;k<2;k++) {
            a[v].uv[k]=s->vertex[first+v].uv[k];
            if(s->texture_rgba && !bounded(a[v].uv[k]))
                return reject(why,"VGP texture coordinate is invalid");
        }
        double input[4]={s->vertex[first+v].xyz[0],s->vertex[first+v].xyz[1],s->vertex[first+v].xyz[2],1};
        for (unsigned k=0;k<3;k++)
            if (!bounded(input[k])) return reject(why, "VGP vertex is invalid");
        for (unsigned k=0;k<4;k++) {
            double c=s->vertex[first+v].rgba[k];
            if (!isfinite(c) || c<0 || c>1 || (k==3 && c!=1 && !s->colour_alpha)) return reject(why, "VGP colour is invalid");
            a[v].c[k]=c;
            a[v].p[k]=0;
            for(unsigned j=0;j<4;j++) a[v].p[k]+=s->matrix[4*k+j]*input[j];
            if (!isfinite(a[v].p[k])) return reject(why, "VGP transform overflow");
        }
    }
    unsigned n=3;
    /* Homogeneous six-plane clipping, then divide; never divide by w first. */
    for (unsigned plane=0;plane<6 && n;plane++) {
        unsigned m=0;
        for (unsigned i=0;i<n;i++) {
            clip_vertex *v=&a[i], *prev=&a[(i+n-1)%n];
            double d=distance(v,plane), pd=distance(prev,plane);
            if ((d>=0)!=(pd>=0)) {
                if (m>=16) return reject(why,"VGP clipping capacity exceeded");
                double t=pd/(pd-d);
                for (unsigned k=0;k<4;k++) {
                    b[m].p[k]=prev->p[k]+t*(v->p[k]-prev->p[k]);
                    b[m].c[k]=prev->c[k]+t*(v->c[k]-prev->c[k]);
                }
                for(unsigned k=0;k<2;k++) b[m].uv[k]=prev->uv[k]+t*(v->uv[k]-prev->uv[k]);
                m++;
            }
            if (d>=0) {
                if (m>=16) return reject(why,"VGP clipping capacity exceeded");
                b[m++]=*v;
            }
        }
        n=m; memcpy(a,b,n*sizeof *a);
    }
    for (unsigned v=0;v<n;v++) {
        if (a[v].p[3] <= 1e-8) return reject(why,"VGP projection has nonpositive or tiny w");
        screen[v].iw=1/a[v].p[3];
        screen[v].x=s->centre_x+s->scale_x*a[v].p[0]*screen[v].iw;
        screen[v].y=s->centre_y+s->scale_y*a[v].p[1]*screen[v].iw;
        if (!isfinite(screen[v].x) || !isfinite(screen[v].y)) return reject(why,"VGP projection is not finite");
        for (unsigned k=0;k<4;k++) screen[v].c[k]=a[v].c[k];
        for (unsigned k=0;k<2;k++) screen[v].uv[k]=a[v].uv[k];
    }
    *vertices=n;
    return true;
}

bool mbx_vgp_raster(const mbx_vgp_scene *s, uint8_t *pixels,
                    uint32_t width, uint32_t height,
                    uint32_t left, uint32_t top, uint32_t right, uint32_t bottom,
                    uint32_t *written, const char **why) {
    if (!s || !pixels || !written || !width || width > 320 || !height || height > 480 ||
        left >= right || right > width || top >= bottom || bottom > height ||
        s->clear_left >= s->clear_right || s->clear_right > width ||
        s->clear_top >= s->clear_bottom || s->clear_bottom > height ||
        (s->clear_bgra >> 24) != 255 ||
        !isfinite(s->centre_x) || !isfinite(s->centre_y) ||
        !isfinite(s->scale_x) || !isfinite(s->scale_y) ||
        fabs(s->centre_x) > 4096 || fabs(s->centre_y) > 4096 ||
        s->scale_x <= 0 || s->scale_x > 4096 || s->scale_y >= 0 || s->scale_y < -4096)
        return reject(why, "VGP target, clear or viewport bounds are invalid");
    if((s->blend_source_alpha && !s->colour_alpha) ||
       (s->colour_alpha && (s->texture_rgba || s->texture_source)))
        return reject(why,"VGP blend/texture state is unsupported");
    uint32_t count=s->vertex_count?s->vertex_count:3;
    if(count!=3 && count!=6) return reject(why,"VGP primitive count is unsupported");
    if(s->texture_rgba) {
        if(!s->texture_width || s->texture_width>512 ||
           !s->texture_height || s->texture_height>512 ||
           s->texture_bytes!=s->texture_width*s->texture_height*4u)
            return reject(why,"VGP canonical texture allocation is invalid");
        for(uint32_t i=3;i<s->texture_bytes;i+=4)
            if(s->texture_rgba[i]!=255)
                return reject(why,"VGP nonopaque texture is unsupported");
    } else if(s->texture_bytes || s->texture_source || s->texture_width || s->texture_height || s->texture_linear) return reject(why,"VGP texture resource is missing");
    screen_vertex prepared[2][16];unsigned sizes[2];
    for(unsigned i=0;i<count/3;i++)
        if(!prepare_triangle(s,3*i,prepared[i],&sizes[i],why))return false;
    /* All rejection paths precede the first write. Nothing below can fail. */
    uint32_t total=0;
    uint32_t cl=s->clear_left>left?s->clear_left:left;
    uint32_t cr=s->clear_right<right?s->clear_right:right;
    uint32_t ct=s->clear_top>top?s->clear_top:top;
    uint32_t cb=s->clear_bottom<bottom?s->clear_bottom:bottom;
    for (uint32_t y=ct;y<cb;y++) for (uint32_t x=cl;x<cr;x++) {
        store(pixels+4*(y*width+x),s->clear_bgra);total++;
    }
    for(unsigned primitive=0;primitive<count/3;primitive++) {
      unsigned n=sizes[primitive];screen_vertex *screen=prepared[primitive];
      for (unsigned t=1;t+1<n;t++) {
        screen_vertex v[3]={screen[0],screen[t],screen[t+1]};
        double area=edge(&v[0],&v[1],v[2].x,v[2].y);
        if (!area) continue;
        if (area<0) {screen_vertex tmp=v[1];v[1]=v[2];v[2]=tmp;area=-area;}
        double minx=fmin(v[0].x,fmin(v[1].x,v[2].x));
        double maxx=fmax(v[0].x,fmax(v[1].x,v[2].x));
        double miny=fmin(v[0].y,fmin(v[1].y,v[2].y));
        double maxy=fmax(v[0].y,fmax(v[1].y,v[2].y));
        int x0=(int)fmax(left,ceil(minx-0.5)), x1=(int)fmin(right,ceil(maxx-0.5));
        int y0=(int)fmax(top,ceil(miny-0.5)), y1=(int)fmin(bottom,ceil(maxy-0.5));
        bool inc[3]={inclusive(&v[1],&v[2]),inclusive(&v[2],&v[0]),inclusive(&v[0],&v[1])};
        for(int y=y0;y<y1;y++) for(int x=x0;x<x1;x++) {
            double e[3]={edge(&v[1],&v[2],x+0.5,y+0.5),edge(&v[2],&v[0],x+0.5,y+0.5),edge(&v[0],&v[1],x+0.5,y+0.5)};
            if (e[0]<0 || (e[0]==0&&!inc[0]) || e[1]<0 || (e[1]==0&&!inc[1]) || e[2]<0 || (e[2]==0&&!inc[2])) continue;
            double weight[3]={e[0]*v[0].iw,e[1]*v[1].iw,e[2]*v[2].iw};
            double denominator=weight[0]+weight[1]+weight[2];
            uint32_t rgb[3];
            for(unsigned k=0;k<3;k++) {
                double value=(weight[0]*v[0].c[k]+weight[1]*v[1].c[k]+weight[2]*v[2].c[k])/denominator;
                value=fmax(0,fmin(1,value));rgb[k]=(uint32_t)floor(value*255+0.5);
            }
            if(s->texture_rgba) {
                double uv[2];
                uint32_t coordinate[2];
                for(unsigned k=0;k<2;k++) {
                    double value=(weight[0]*v[0].uv[k]+weight[1]*v[1].uv[k]+weight[2]*v[2].uv[k])/denominator;
                    uint32_t extent=k?s->texture_height:s->texture_width;
                    uv[k]=fmax(0,fmin(1,value));
                    coordinate[k]=uv[k]>=1?extent-1:(uint32_t)floor(uv[k]*extent);
                }
                double sampled[3];
                if(s->texture_linear) {
                    /* GL_ES 1.1 equations 3.18: half-texel offset, four taps,
                     * clamp each neighbour. No CA-specific fixed-point guess. */
                    uint32_t tap[2][2];double fraction[2];
                    for(unsigned k=0;k<2;k++) {
                        uint32_t extent=k?s->texture_height:s->texture_width;
                        double position=uv[k]*extent-.5;
                        int low=(int)floor(position);fraction[k]=position-low;
                        for(unsigned j=0;j<2;j++) {
                            int index=low+(int)j;
                            tap[k][j]=index<0?0:(uint32_t)index>=extent?extent-1:(uint32_t)index;
                        }
                    }
                    for(unsigned k=0;k<3;k++) {
                        sampled[k]=0;
                        for(unsigned y=0;y<2;y++)for(unsigned x=0;x<2;x++) {
                            double f=(x?fraction[0]:1-fraction[0])*(y?fraction[1]:1-fraction[1]);
                            sampled[k]+=f*s->texture_rgba[4*(tap[1][y]*s->texture_width+tap[0][x])+k];
                        }
                    }
                } else {
                    const uint8_t *texel=s->texture_rgba+4*(coordinate[1]*s->texture_width+coordinate[0]);
                    for(unsigned k=0;k<3;k++)sampled[k]=texel[k];
                }
                for(unsigned k=0;k<3;k++) {
                    double colour=(weight[0]*v[0].c[k]+weight[1]*v[1].c[k]+weight[2]*v[2].c[k])/denominator;
                    rgb[k]=(uint32_t)floor(fmax(0,fmin(1,colour))*sampled[k]+.5);
                }
            }
            uint8_t *destination=pixels+4*((uint32_t)y*width+(uint32_t)x);
            if(s->colour_alpha) {
                double fragment[4];
                for(unsigned k=0;k<4;k++) fragment[k]=fmax(0,fmin(1,
                    (weight[0]*v[0].c[k]+weight[1]*v[1].c[k]+weight[2]*v[2].c[k])/denominator));
                const unsigned order[4]={2,1,0,3}; /* RGBA -> target BGRA. */
                for(unsigned k=0;k<4;k++) {
                    double result=fragment[k]*255;
                    if(s->blend_source_alpha)
                        result=fragment[3]*result+(1-fragment[3])*destination[order[k]];
                    destination[order[k]]=(uint8_t)floor(fmax(0,fmin(255,result))+.5);
                }
            } else store(destination,0xff000000u|(rgb[0]<<16)|(rgb[1]<<8)|rgb[2]);
            total++;
        }
    }
    }
    *written=total;
    return true;
}
