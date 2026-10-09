"""Verify exact authentic artifacts and decode lossless RGB8 PNG independently. MIT."""
import hashlib,json,struct,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def rgb_png(data):
    if data[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError('PNG signature')
    pos=8;compressed=b'';w=h=None;ended=False
    while pos<len(data):
        size=struct.unpack_from('>I',data,pos)[0];kind=data[pos+4:pos+8];payload=data[pos+8:pos+8+size];crc=struct.unpack_from('>I',data,pos+8+size)[0];pos+=size+12
        if zlib.crc32(kind+payload)&0xffffffff!=crc:raise ValueError('PNG CRC')
        if kind==b'IHDR':
            w,h,bits,colour,comp,filt,interlace=struct.unpack('>IIBBBBB',payload)
            if not 0<w<=320 or not 0<h<=480 or(bits,colour,comp,filt,interlace)!=(8,2,0,0,0):raise ValueError('Expected bounded RGB8 PNG')
        elif kind==b'IDAT':compressed+=payload
        elif kind==b'IEND':ended=True;break
    if not ended or w is None or pos!=len(data):raise ValueError('PNG framing')
    raw=zlib.decompress(compressed);stride=w*3
    if len(raw)!=(stride+1)*h:raise ValueError('PNG extent')
    result=bytearray();prev=bytearray(stride)
    def paeth(a,b,c):
        p=a+b-c;aa,bb,cc=abs(p-a),abs(p-b),abs(p-c)
        return a if aa<=bb and aa<=cc else b if bb<=cc else c
    for y in range(h):
        mode=raw[y*(stride+1)];row=bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
        if mode>4:raise ValueError('PNG filter')
        for x in range(stride):
            a=row[x-3]if x>=3 else 0;b=prev[x];c=prev[x-3]if x>=3 else 0
            predictor=(0,a,b,(a+b)//2,paeth(a,b,c))[mode];row[x]=(row[x]+predictor)&255
        result+=row;prev=row
    return w,h,bytes(result)
def verify():
    manifest=json.loads((ROOT/'demos/gallery/manifest.json').read_text());results=[]
    for e in manifest['captures']:
        png_rgb=None
        for f in e['files']:
            data=(ROOT/f['path']).read_bytes()
            if len(data)!=f['bytes']or hashlib.sha256(data).hexdigest()!=f['sha256']:raise ValueError('Artifact hash mismatch: '+f['path'])
            if f['path'].endswith('.png'):
                w,h,rgb=rgb_png(data);png_rgb=rgb
                if(w,h)!=(e['width'],e['height'])or hashlib.sha256(rgb).hexdigest()!=e['rgb_sha256']:raise ValueError('PNG pixel mismatch')
            else:
                header,rgb=data.split(b'\n255\n',1)
                if header.split()!=[b'P6',str(e['width']).encode(),str(e['height']).encode()]or len(rgb)!=e['width']*e['height']*3 or hashlib.sha256(rgb).hexdigest()!=e['rgb_sha256']:raise ValueError('PPM pixel mismatch')
                if png_rgb!=rgb:raise ValueError('PNG is not lossless')
        r={'capture':e['id'],'sha256_and_pixels':'PASS','classification':e['classification']}
        if 'independent_reference_inputs'in e:
            from lab.replay import module
            inputs=json.loads((ROOT/e['independent_reference_inputs']).read_text());ppm=ROOT/next(f['path']for f in e['files']if f['path'].endswith('.ppm'));check=module('reference_texture').compare(inputs,ppm);check['ppm']=str(ppm.relative_to(ROOT));r['independent_reference']=check
            if check['mismatched_pixels_tolerance_1']or not check['covered_pixels']:raise ValueError('Capture reference mismatch')
        results.append(r)
    return {'captures_verified':len(results),'results':results,'scope':'Exact historical artifacts; three texture scanouts independently rechecked. No guest boot performed.'}
