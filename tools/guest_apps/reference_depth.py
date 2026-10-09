#!/usr/bin/env python3
"""Independent analytic oracle for our own two-triangle ARMv6 depth diagnostic.

No emulator shader decoder or rasterizer is imported. Geometry is the intended
application input, rounded to its actual float32 ABI. Reference images are test
artifacts, never guest captures. MIT; S5LBox credits j0shua-SYSON retained.
"""
import argparse, hashlib, json, math, struct
from pathlib import Path

def f32(x): return struct.unpack('<f', struct.pack('<f', x))[0]

def geometry(case):
    assert case in range(7)
    shapes = [(-.75,-.65,.45,-.65,-.15,.65),(-.45,-.45,.75,-.45,.15,.85)]
    triangles=[]
    for face in ([1,0] if case==2 else [0,1]):
        points=[]
        for index in range(3):
            x,y=map(f32,shapes[face][2*index:2*index+2]);z=f32(.3 if face==0 or case==4 else -.3)
            if case==6:
                # Modelview Ry(23 degrees) * Rx(13 degrees), then GL ortho.
                a,b=math.radians(13),math.radians(23)
                y,z=math.cos(a)*y-math.sin(a)*z,math.sin(a)*y+math.cos(a)*z
                x,z=math.cos(b)*x+math.sin(b)*z,-math.sin(b)*x+math.cos(b)*z
            points.append((160+160*x,230-230*y,(1-z)/2))
        triangles.append((points,(255,0,0) if face==0 else (0,255,0)))
    return triangles

def expected(case, quantized=True):
    # Clear conversion measured separately in the preceding colour campaign.
    background=(12,12,30);rgb=bytearray(bytes(background)*(320*460))
    clear=.25 if case==5 else 1
    depth=[math.floor(clear*16777215+.5) if quantized else clear]*(320*460)
    covered=occluded=ties=0
    for points,colour in geometry(case):
        (ax,ay,az),(bx,by,bz),(cx,cy,cz)=points
        determinant=(bx-ax)*(cy-ay)-(by-ay)*(cx-ax)
        for y in range(max(0,math.floor(min(ay,by,cy))),min(460,math.ceil(max(ay,by,cy)))):
            for x in range(max(0,math.floor(min(ax,bx,cx))),min(320,math.ceil(max(ax,bx,cx)))):
                px,py=x+.5,y+.5
                beta=((px-ax)*(cy-ay)-(py-ay)*(cx-ax))/determinant
                gamma=((bx-ax)*(py-ay)-(by-ay)*(px-ax))/determinant
                alpha=1-beta-gamma
                if min(alpha,beta,gamma)<-1e-10:continue
                if min(abs(alpha),abs(beta),abs(gamma))<1e-10:ties+=1
                z=az+beta*(bz-az)+gamma*(cz-az)
                value=math.floor(max(0,min(1,z))*16777215+.5) if quantized else z
                pos=y*320+x
                if case and value>=depth[pos]:occluded+=1;continue
                rgb[3*pos:3*pos+3]=bytes(colour);covered+=1
                if case and case!=3:depth[pos]=value
    return bytes(rgb),{'colour_writes':covered,'depth_rejected_fragments':occluded,'boundary_ties':ties,'quantization':'model nearest uint24' if quantized else 'ideal continuous depth'}

def compare(case,ppm):
    data=Path(ppm).read_bytes();head,actual=data.split(b'\n255\n',1)
    assert head.split()==[b'P6',b'320',b'460'] and len(actual)==320*460*3
    reference,stats=expected(case)
    ideal,_=expected(case,False)
    assert reference==ideal, 'This test must not imply a physically measured depth quantizer'
    errors=[max(abs(actual[i+k]-reference[i+k]) for k in range(3)) for i in range(0,len(actual),3)]
    return {'case':case,'ppm':str(Path(ppm).resolve()),'ppm_sha256':hashlib.sha256(data).hexdigest(),'oracle_rgb_sha256':hashlib.sha256(reference).hexdigest(),'mismatched_pixels':sum(x!=0 for x in errors),'max_channel_error':max(errors),'compared_pixels':320*460,'excluded_pixels':0,'quantized_and_ideal_colour_identical':True,**stats,'scope':'Intended diagnostic geometry, GL_LESS and write-mask semantics; independent analytical barycentric inversion. Not a physical MBX precision measurement. Case 6 trigonometry may require measured float32 matrix precision assessment.'}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('case',type=int,choices=range(7));p.add_argument('ppm',type=Path);p.add_argument('out',type=Path);a=p.parse_args()
    result=compare(a.case,a.ppm)
    with a.out.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
    print(json.dumps(result,indent=2));raise SystemExit(bool(result['mismatched_pixels']))
