#!/usr/bin/env python3
"""Independent analytic pixel check for the own opaque GLES diagnostic.

Reads captured TA input and a test-target or real scanout PPM. No rendering,
pixel modification, guest execution, or shader bytes embedded in source.
The independent reference uses inverse screen mapping and analytic weights.
MIT; S5LBox by j0shua-SYSON.
"""
import argparse, hashlib, json, math, struct
from pathlib import Path

def floats(words):
    return struct.unpack('<'+'f'*len(words),struct.pack('<'+'I'*len(words),*words))

def compare(words, ppm):
    data=Path(ppm).read_bytes()
    header,payload=data.split(b'\n255\n',1)
    tokens=header.split();assert tokens[0]==b'P6' and len(tokens)==3
    width,height=map(int,tokens[1:]);assert len(payload)==width*height*3
    assert 0<width<=320 and 0<height<=480
    # Locate measured commands, independent of the C decoder offsets/hash.
    u=words.index(0xa0000003);control=next(i for i in range(1,len(words)) if words[i]==0xf0020004 and words[i-1]==0x78020000)
    matrix=floats(words[u+1:u+17]);viewport=floats(words[control-8:control-4])
    cx,sx,cy,sy=viewport
    points=[];colours=[]
    for v in range(3):
        start=control+1+8*v;xyz=floats(words[start+1:start+4]);colours.append(floats(words[start+4:start+8]))
        vector=(*xyz,1)
        clip=[sum(matrix[4*r+c]*vector[c] for c in range(4)) for r in range(4)]
        assert clip[3]==1, 'Reference limited to measured affine w=1 scenes'
        points.append((clip[0],clip[1]))
    (ax,ay),(bx,by),(cxv,cyv)=points
    determinant=(bx-ax)*(cyv-ay)-(by-ay)*(cxv-ax)
    assert abs(determinant)>1e-8
    clear=next(i for i,w in enumerate(words) if w==0x10000063)
    rgba=words[clear+11];background=(rgba&255,(rgba>>8)&255,(rgba>>16)&255)
    bad=inside=boundary=0;max_error=0
    for y in range(height):
        for x in range(width):
            nx=(x+.5-cx)/sx;ny=(y+.5-cy)/sy
            beta=((nx-ax)*(cyv-ay)-(ny-ay)*(cxv-ax))/determinant
            gamma=((bx-ax)*(ny-ay)-(by-ay)*(nx-ax))/determinant
            alpha=1-beta-gamma
            if min(abs(alpha),abs(beta),abs(gamma))<1e-7:
                boundary+=1;continue
            if min(alpha,beta,gamma)>0:
                expected=tuple(math.floor(255*(alpha*colours[0][k]+beta*colours[1][k]+gamma*colours[2][k])+.5) for k in range(3));inside+=1
            else:expected=background
            actual=payload[3*(width*y+x):3*(width*y+x)+3]
            error=max(abs(actual[k]-expected[k]) for k in range(3))
            max_error=max(max_error,error);bad+=error>1
    return {'ppm':str(Path(ppm).resolve()),'sha256':hashlib.sha256(data).hexdigest(),'width':width,'height':height,'triangle_pixels_checked':inside,'excluded_edge_ties':boundary,'mismatched_pixels_tolerance_1':bad,'max_channel_error':max_error,'reference_scope':'Measured opaque affine triangle, pixel centres; analytic weights independent of C edge rasterizer, one-unit channel tolerance, boundary ties excluded.'}

def main():
    p=argparse.ArgumentParser();p.add_argument('scene_json',type=Path);p.add_argument('scene_index',type=int);p.add_argument('ppm',type=Path);p.add_argument('output_json',type=Path);args=p.parse_args()
    scenes=json.loads(args.scene_json.read_text());r=compare(scenes[args.scene_index]['words'],args.ppm)
    with args.output_json.open('x') as f:json.dump(r,f,indent=2);f.write('\n')
    print(json.dumps(r,indent=2))
    return int(r['mismatched_pixels_tolerance_1']!=0 or r['triangle_pixels_checked']==0)

if __name__=='__main__':raise SystemExit(main())
