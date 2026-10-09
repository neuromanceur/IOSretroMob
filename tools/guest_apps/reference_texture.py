#!/usr/bin/env python3
"""Read-only independent nearest/bilinear reference for our own affine UV diagnostic.

Input is decoded scene metadata, not an emulator-generated reference image.
Uses analytic barycentric inversion and the intended upload's texel colours.
Does not read shader code, GART, or modify images. MIT; S5LBox j0shua-SYSON.
"""
import argparse,hashlib,json,math
from pathlib import Path

def texel(x,y):
    # Intended glTexImage2D RGBA8 upload, independent of GPU storage ordering.
    if (x,y)==(0,0):return (255,0,0)
    if (x,y)==(7,0):return (0,255,0)
    if (x,y)==(0,7):return (0,0,255)
    if (x,y)==(7,7):return (255,255,0)
    dim=96 if (x+y)%2 else 255
    return (dim if x<4 else 24,32 if y<4 else dim,dim if x>=4 else 48)

def compare(scene,ppm):
    data=Path(ppm).read_bytes();header,payload=data.split(b'\n255\n',1)
    tokens=header.split();assert tokens[0]==b'P6' and len(tokens)==3
    width,height=map(int,tokens[1:]);assert 0<width<=320 and 0<height<=480 and len(payload)==width*height*3
    assert scene['filter']in('nearest','linear') and scene['texture_size']==[8,8]
    assert len(scene['vertices'])in(3,6)
    m=scene['matrix'];cx,sx,cy,sy=scene['viewport'];triangles=[]
    for start in range(0,len(scene['vertices']),3):
        points=[]
        for v in scene['vertices'][start:start+3]:
            p=[*v['xyz'],1];q=[sum(m[4*i+j]*p[j]for j in range(4))for i in range(4)]
            assert q[3]==1 and abs(q[0])<=1.01 and abs(q[1])<=1.01 and abs(q[2])<=1
            points.append((q[0],q[1]))
        (ax,ay),(bx,by),(dx,dy)=points;det=(bx-ax)*(dy-ay)-(by-ay)*(dx-ax);assert abs(det)>1e-8
        triangles.append((ax,ay,bx,by,dx,dy,det,scene['vertices'][start:start+3]))
    bad=inside=excluded=max_error=0;texels=set()
    for y in range(height):
        for x in range(width):
            nx=(x+.5-cx)/sx;ny=(y+.5-cy)/sy;expected=tuple(scene['background_rgb']);tie=False
            for ax,ay,bx,by,dx,dy,det,vertices in triangles:
                beta=((nx-ax)*(dy-ay)-(ny-ay)*(dx-ax))/det
                gamma=((bx-ax)*(ny-ay)-(by-ay)*(nx-ax))/det;alpha=1-beta-gamma
                if min(alpha,beta,gamma)<-1e-9:continue
                if min(abs(alpha),abs(beta),abs(gamma))<1e-8:tie=True
                weights=(alpha,beta,gamma)
                uv=[sum(weights[i]*vertices[i]['uv'][k]for i in range(3))for k in range(2)]
                # At exact nearest boundaries, a hardware fixed-point detail
                # may matter. Count exclusions explicitly; never conceal them.
                if scene['filter']=='nearest' and any(abs(t*8-round(t*8))<1e-8 for t in uv):tie=True
                tx,ty=[max(0,min(7,math.floor(t*8)))for t in uv]
                rgb=texel(tx,ty);texels.add((tx,ty))
                if scene['filter']=='linear':
                    # Independent separable interpolation of the intended image.
                    px,py=[max(0,min(7,t*8-.5))for t in uv]
                    ix,iy=math.floor(px),math.floor(py);ax,ay=px-ix,py-iy
                    c00=texel(ix,iy);c10=texel(min(7,ix+1),iy)
                    c01=texel(ix,min(7,iy+1));c11=texel(min(7,ix+1),min(7,iy+1))
                    rgb=tuple((c00[k]+ax*(c10[k]-c00[k]))+ay*((c01[k]+ax*(c11[k]-c01[k]))-(c00[k]+ax*(c10[k]-c00[k])))for k in range(3))
                colour=[sum(weights[i]*vertices[i]['rgba'][k]for i in range(3))for k in range(3)]
                expected=tuple(math.floor(rgb[k]*colour[k]+.5)for k in range(3));inside+=1;break
            if tie:excluded+=1;continue
            actual=payload[3*(y*width+x):3*(y*width+x)+3];error=max(abs(actual[k]-expected[k])for k in range(3));bad+=error>1;max_error=max(max_error,error)
    return {'ppm':str(Path(ppm).resolve()),'ppm_sha256':hashlib.sha256(data).hexdigest(),'width':width,'height':height,'covered_pixels':inside,'texels_observed':len(texels),'excluded_boundary_ties':excluded,'mismatched_pixels_tolerance_1':bad,'max_channel_error':max_error,'filter':scene['filter'],'scope':'Own affine w=1 geometry; intended RGBA8 8x8 upload; normalized clamp-to-edge; whole scanout compared, analytic inversion independent of C rasterizer.'}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('scene',type=Path);p.add_argument('ppm',type=Path);p.add_argument('out',type=Path);a=p.parse_args();r=compare(json.loads(a.scene.read_text()),a.ppm)
    with a.out.open('x')as f:json.dump(r,f,indent=2);f.write('\n')
    print(json.dumps(r,indent=2));raise SystemExit(bool(r['mismatched_pixels_tolerance_1']or not r['covered_pixels']))
