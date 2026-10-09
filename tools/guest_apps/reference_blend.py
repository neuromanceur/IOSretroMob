#!/usr/bin/env python3
"""Independent straight-alpha equations for our ARMv6 diagnostic, not a capture.
MIT; no emulator decoder/rasterizer import. Background bytes are from the
previous independently checked clear conversion. Model rounds destination
RGBA8 after each fragment; this is not a physical MBX rounding measurement.
"""
import argparse,hashlib,json,math,struct
from pathlib import Path

def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def expected(case):
 assert case in range(7)
 result=bytearray(bytes((12,12,30,255))*(320*460));writes=ties=0
 shapes=[(-.75,-.65,.45,-.65,-.15,.65),(-.45,-.45,.75,-.45,.15,.85)]
 for face in ([1,0] if case==2 else [0,1]):
  xy=[(160+160*f32(shapes[face][2*i]),230-230*f32(shapes[face][2*i+1]))for i in range(3)]
  (ax,ay),(bx,by),(cx,cy)=xy;det=(bx-ax)*(cy-ay)-(by-ay)*(cx-ax)
  alpha=1 if case in (0,4) else 0 if case==3 else (.25 if face==0 else .75) if case==5 else .5
  source=(255,0,0,alpha*255) if face==0 else (0,255,0,alpha*255)
  for y in range(max(0,math.floor(min(ay,by,cy))),min(460,math.ceil(max(ay,by,cy)))):
   for x in range(max(0,math.floor(min(ax,bx,cx))),min(320,math.ceil(max(ax,bx,cx)))):
    px,py=x+.5,y+.5
    beta=((px-ax)*(cy-ay)-(py-ay)*(cx-ax))/det
    gamma=((bx-ax)*(py-ay)-(by-ay)*(px-ax))/det
    weights=(1-beta-gamma,beta,gamma)
    if min(weights)<-1e-10:continue
    if min(abs(v)for v in weights)<1e-10:ties+=1
    pos=4*(y*320+x)
    for k in range(4):
     value=source[k] if case in(0,6) else alpha*source[k]+(1-alpha)*result[pos+k]
     result[pos+k]=min(255,max(0,math.floor(value+.5)))
    writes+=1
 return bytes(result),{'case':case,'fragment_writes':writes,'boundary_ties':ties,'alpha_test':False,'depth_test':False,'blend_factors':'SRC_ALPHA / ONE_MINUS_SRC_ALPHA' if case not in(0,6) else 'BLEND disabled','scope':'Analytic own geometry plus straight-alpha equations, including As squared in output alpha. No physical rounding claim.'}
def compare(case,ppm):
 b=Path(ppm).read_bytes();head,rgb=b.split(b'\n255\n',1);assert head.split()==[b'P6',b'320',b'460'] and len(rgb)==320*460*3
 reference,meta=expected(case);ref=bytes(v for i,v in enumerate(reference)if i%4!=3)
 error=[max(abs(rgb[i+k]-ref[i+k])for k in range(3))for i in range(0,len(rgb),3)]
 return {**meta,'ppm_sha256':hashlib.sha256(b).hexdigest(),'reference_rgb_sha256':hashlib.sha256(ref).hexdigest(),'mismatched_pixels':sum(v!=0 for v in error),'pixels_over_tolerance_1':sum(v>1 for v in error),'max_channel_error':max(error),'compared_pixels':320*460,'excluded_pixels':0}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('case',type=int,choices=range(7));p.add_argument('ppm',type=Path);p.add_argument('out',type=Path);a=p.parse_args();r=compare(a.case,a.ppm)
 with a.out.open('x')as f:json.dump(r,f,indent=2);f.write('\n')
 print(json.dumps(r,indent=2));raise SystemExit(bool(r['pixels_over_tolerance_1']))
