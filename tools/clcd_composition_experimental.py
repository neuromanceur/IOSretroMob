#!/usr/bin/env python3
"""HYPOTHETICAL CLCD source-over comparisons, never hardware capture.

Original IOSretroMob tooling, MIT; S5LBox structures/capture provenance retained.
No inferred register priority, constant alpha or Samsung-controller substitution.
Exactly one input instant; explicit order and alpha convention, opaque RGB
backdrop and integer nearest rounding are mathematical assumptions only.
"""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path
import clcd_layers

STATUS = 'HYPOTHESIS ONLY - NOT HARDWARE VALIDATED'

def compose(layers, width, height, mode, backdrop):
    """Bottom-to-top sequence of (RGBA,w,h,x,y), opaque backdrop RGB.

    premultiplied: Cs + Cd*(1-As); straight: Cs*As + Cd*(1-As).
    Values use 8-bit channels, divide by 255, round nearest after EACH layer.
    Output alpha stays 255. Input pixels are never changed.
    """
    if mode not in ('premultiplied', 'straight'):
        raise ValueError('unknown experimental equation')
    if type(width) is not int or type(height) is not int or not (0 < width <= 2047 and 0 < height <= 1023):
        raise ValueError('invalid bounded panel')
    if len(backdrop) != 3 or any(type(c) is not int or not 0 <= c <= 255 for c in backdrop):
        raise ValueError('opaque RGB backdrop required explicitly')
    if len(layers) > 4:
        raise ValueError('at most four layers')
    out = bytearray(bytes((*backdrop,255)) * (width*height))
    for rgba,w,h,x,y in layers:
        if not all(type(v) is int for v in (w,h,x,y)) or not (0 < w <= 2047 and 0 < h <= 1023) or len(rgba) != w*h*4:
            raise ValueError('invalid layer geometry')
        if mode == 'premultiplied' and any(max(rgba[p:p+3]) > rgba[p+3] for p in range(0,len(rgba),4)):
            raise ValueError('pixels incompatible with bounded premultiplied representation')
        for dy in range(max(0,y),min(height,y+h)):
            for dx in range(max(0,x),min(width,x+w)):
                s=((dy-y)*w+dx-x)*4;d=(dy*width+dx)*4;a=rgba[s+3]
                for k in range(3):
                    source=rgba[s+k]*255 if mode=='premultiplied' else rgba[s+k]*a
                    out[d+k]=(source+out[d+k]*(255-a)+127)//255
    return bytes(out)

def load_capture(manifest, expected_sha256):
    if manifest.is_symlink() or manifest.stat().st_size > 32768:
        raise ValueError('invalid bounded manifest')
    raw=manifest.read_bytes();digest=hashlib.sha256(raw).hexdigest()
    if digest != expected_sha256:
        raise ValueError('manifest SHA-256 differs from trusted record')
    doc=json.loads(raw)
    if not isinstance(doc,dict) or doc.get('schema') != 1 or doc.get('scope') != 'native-window-observation;not-a-composite' or doc.get('running') is not True:
        raise ValueError('unknown or inactive capture schema')
    if type(doc.get('instruction')) is not int or not 0 <= doc['instruction'] <= 2**64-1:
        raise ValueError('invalid instruction instant')
    windows=doc.get('windows');regs=doc.get('registers')
    if not isinstance(windows,list) or len(windows)!=4 or not isinstance(regs,dict) or type(regs.get('ctrl')) is not int or not 0 <= regs['ctrl'] <= 2**32-1:
        raise ValueError('invalid controller descriptors')
    loaded={}
    for k,row in enumerate(windows):
        if not isinstance(row,dict) or type(row.get('index')) is not int or row['index']!=k:
            raise ValueError('duplicate or reordered descriptor')
        enabled=bool(regs['ctrl'] & (0x40 >> k))
        if row.get('status')=='disabled':
            if enabled:raise ValueError('disabled status conflicts with enable bit')
            continue
        if not enabled or row.get('status')!='ok':
            raise ValueError('unsupported active window')
        r=row.get('registers',{})
        if not isinstance(r,dict) or any(type(r.get(key)) is not int or not 0 <= r[key] <= 2**32-1 for key in ('stride','control','fbaddr','geometry','linewords','position')):
            raise ValueError('invalid raw registers')
        if r['control'] not in (0x300,0x700):
            raise ValueError('unsupported control bits; no swizzle, fetch or blend guessing')
        if (row['width'],row['height'],row['x'],row['y'],row['stride'],row['format_code'],row['order_code']) != (
            (r['geometry']>>16)&0x7ff,r['geometry']&0x3ff,r['position']>>16,r['position']&0xffff,r['stride'],(r['control']>>8)&7,(r['control']>>16)&3):
            raise ValueError('decoded descriptor conflicts with raw registers')
        if row.get('row_bytes') != row['width']*(4 if row['format_code']==7 else 2):
            raise ValueError('row length differs')
        rgba=clcd_layers.load_window(manifest.parent,row)
        loaded[k]=(rgba,row['width'],row['height'],row['x'],row['y'])
    return doc,loaded

def hypothetical_png(width,height,rgba,description):
    png=clcd_layers.png_rgba(width,height,rgba)
    kind=b'tEXt';data=b'CLCD_Status\0'+(STATUS+'; '+description).encode('ascii')
    chunk=struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    return png[:33]+chunk+png[33:]

# Original tiny diagnostic label only, outside the captured panel.
_FONT={
'H':['101','101','111','101','101'],'Y':['101','101','010','010','010'],
'P':['110','101','110','100','100'],'O':['111','101','101','101','111'],
'T':['111','010','010','010','010'],'E':['111','100','110','100','111'],
'S':['111','100','111','001','111'],'I':['111','010','010','010','111'],
'N':['101','111','111','111','101'],'L':['100','100','100','100','111']}
def labelled_panel(width,height,rgba):
    text='HYPOTHESIS ONLY';scale=2
    banner=bytearray(bytes((70,38,0,255))*width*24)
    for i,ch in enumerate(text):
        for y,row in enumerate(_FONT.get(ch,['000']*5)):
            for x,bit in enumerate(row):
                if bit!='1':continue
                for sy in range(scale):
                    for sx in range(scale):
                        dx=8+(i*4+x)*scale+sx;dy=7+y*scale+sy
                        if dx<width:
                            p=(dy*width+dx)*4;banner[p:p+4]=bytes((255,226,128,255))
    return bytes(banner)+rgba

def compare(manifest,manifest_sha,indices,backdrop,output):
    doc,loaded=load_capture(manifest,manifest_sha)
    if len(indices)!=2 or len(set(indices))!=2 or set(indices)!=set(loaded):
        raise ValueError('comparison requires exactly the two enabled windows; no silent omission')
    pixels={};results=[]
    # No default preferred order. Both directions and equations are hypotheses.
    for order in (indices,indices[::-1]):
        for mode in ('premultiplied','straight'):
            key='w'+str(order[0])+'-under-w'+str(order[1])+'-'+mode
            rgba=compose([loaded[k] for k in order],320,480,mode,backdrop)
            pixels[key]=rgba
            results.append({'id':key,'bottom_to_top':order,'equation':mode,'panel_width':320,'panel_height':480,'rgba_sha256':hashlib.sha256(rgba).hexdigest(),'status':STATUS})
    # Everything validated/calculated before creating any output directory.
    output.mkdir(exist_ok=False)
    for row in results:
        key=row['id'];rgba=pixels[key]
        for labelled in (False,True):
            h=504 if labelled else 480;data=hypothetical_png(320,h,labelled_panel(320,480,rgba) if labelled else rgba,key)
            name='HYPOTHESIS-'+key+('-labelled' if labelled else '')+'.png'
            with (output/name).open('xb') as f:f.write(data)
            row['labelled_preview' if labelled else 'panel']={'file':name,'sha256':hashlib.sha256(data).hexdigest(),'width':320,'height':h}
    differences=[]
    for i,a in enumerate(results):
        for b in results[i+1:]:
            pa,pb=pixels[a['id']],pixels[b['id']]
            differences.append({'a':a['id'],'b':b['id'],'different_pixels':sum(pa[p:p+4]!=pb[p:p+4] for p in range(0,len(pa),4)),'maximum_channel_difference':max(abs(x-y) for x,y in zip(pa,pb))})
    evidence={'schema':1,'status':STATUS,'hardware_validated':False,'instruction':doc['instruction'],'manifest_sha256':manifest_sha,'input_windows':[{'index':k,'raw_sha256':doc['windows'][k]['sha256'],'raw_file':doc['windows'][k]['file']} for k in indices],'assumptions':{'backdrop_rgb':list(backdrop),'backdrop_alpha':255,'rounding':'nearest /255 after each layer, not a hardware rounding claim','clipping':'panel intersection; unsigned register positions; no scaling, CSC, gamma, key or constant-alpha interpretation'},'hypotheses':results,'pairwise_comparisons':differences}
    (output/'HYPOTHESES.json').write_text(json.dumps(evidence,indent=2)+'\n')
    return evidence

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('manifest',type=Path);p.add_argument('--manifest-sha256',required=True)
    p.add_argument('--windows',required=True,help='Two explicit indices, e.g. 0,2; both orders will be evaluated')
    p.add_argument('--backdrop-rgb',required=True,help='Three explicit 8-bit values, e.g. 0,0,0; not inferred hardware semantics')
    p.add_argument('--output',required=True,type=Path);a=p.parse_args()
    try:
        r=compare(a.manifest,a.manifest_sha256,[int(v) for v in a.windows.split(',')],tuple(int(v) for v in a.backdrop_rgb.split(',')),a.output)
    except (ValueError,KeyError,OSError,TypeError) as exc:p.exit(1,str(exc)+'\n')
    print(json.dumps(r,indent=2))
if __name__=='__main__':main()
