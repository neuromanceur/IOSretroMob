#!/usr/bin/env python3
"""Verify curated original-game evidence without private firmware/media. MIT.
PNG is lossless storage, not an alpha/composition fidelity claim. Four selected
native spans are reconstructed temporarily; originals and hardware are untouched.
"""
import hashlib,json,re,struct,tempfile,zlib
from pathlib import Path
import clcd_composition_experimental as exp
ROOT=Path(__file__).resolve().parents[1];PROOF=ROOT/'reports/relic-maze'
def sha(data):return hashlib.sha256(data).hexdigest()
def png_pixels(data):
    if len(data)>4*1024**2 or data[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError('bounded PNG required')
    p=8;compressed=b'';header=None
    while p<len(data):
        n=struct.unpack_from('>I',data,p)[0];kind=data[p+4:p+8];chunk=data[p+8:p+8+n]
        if p+12+n>len(data) or zlib.crc32(kind+chunk)&0xffffffff!=struct.unpack_from('>I',data,p+8+n)[0]:raise ValueError('PNG CRC/span')
        if kind==b'IHDR':header=struct.unpack('>IIBBBBB',chunk)
        if kind==b'IDAT':compressed+=chunk
        p+=n+12
    w,h,depth,mode,comp,filt,interlace=header
    if not (0<w<=320 and 0<h<=504 and depth==8 and mode in (2,6) and (comp,filt,interlace)==(0,0,0)):raise ValueError('unsupported proof PNG')
    channels=3 if mode==2 else 4;rowbytes=w*channels;raw=zlib.decompress(compressed)
    if len(raw)!=h*(rowbytes+1):raise ValueError('PNG rows')
    packed=bytearray()
    for y in range(h):
        row=raw[y*(rowbytes+1):(y+1)*(rowbytes+1)]
        if row[0]!=0:raise ValueError('unsupported PNG filter')
        packed.extend(row[1:])
    rgba=bytes(packed) if channels==4 else b''.join(bytes((*packed[p:p+3],255)) for p in range(0,len(packed),3))
    return w,h,rgba

def verify():
    registry=json.loads((PROOF/'PUBLIC_EVIDENCE_MANIFEST.json').read_text());decoded={}
    for row in registry['files']:
        p=ROOT/row['path'];data=p.read_bytes()
        if sha(data)!=row['sha256']:raise ValueError('immutable image hash: '+row['path'])
        decoded[row['path']]=png_pixels(data)
        if row['kind']=='hypothesis' and (not p.name.startswith('HYPOTHESIS-') or b'CLCD_Status\0HYPOTHESIS ONLY - NOT HARDWARE VALIDATED' not in data or row.get('hardware_validated') is not False):raise ValueError('hypothesis marker')
    first=json.loads((PROOF/'FIRST_LAUNCH_EVIDENCE.json').read_text());full=json.loads((PROOF/'full-playthrough/EVIDENCE.json').read_text())
    pairs=[(PROOF/(x['capture']+'.png'),x) for x in first['captures']]+[(PROOF/'full-playthrough'/x['png'],x) for x in full['milestone_captures']]
    for p,row in pairs:
        w,h,rgba=decoded[str(p.relative_to(ROOT))];rgb=b''.join(rgba[q:q+3] for q in range(0,len(rgba),4))
        ppm=f'P6\n{w} {h}\n255\n'.encode()+rgb
        if sha(rgb)!=row['rgb_sha256'] or sha(ppm)!=row['ppm_sha256']:raise ValueError('historical lossless RGB/PPM identity')
    log=(PROOF/'full-playthrough/GUEST_EXECUTION.log').read_bytes()
    if sha(log)!=full['guest_log_sha256']:raise ValueError('guest journal hash')
    text=log.decode();fields=['player_x','player_y','crate_x','crate_y','moves','mask','plate','won'];observed=[]
    for block in text.split('RELIC_FRAME=')[1:]:
        row={'frame':int(block[:8],16)}
        for key in fields:
            m=re.search('RELIC_'+key.upper()+r'=([0-9a-f]{8})',block);row['relic_mask' if key=='mask' else key]=int(m[1],16)
        observed.append(row)
    if observed!=full['states'] or len(observed)!=26 or observed[-1]['won']!=1 or observed[-1]['moves']!=25:raise ValueError('guest states differ')
    directions=[int(x,16) for x in re.findall(r'RELIC_TOUCH_DIRECTION=([0-9a-f]{8})',text)]
    if directions!=[{'U':1,'D':2,'L':3,'R':4}[x] for x in 'DDDDDDUURRRUURUUDDRRDDDDL']:raise ValueError('guest callbacks differ')
    if [int(x,16) for x in re.findall(r'RELIC_STEP_RESULT=([0-9a-f]{8})',text)]!=[1]*24+[2]:raise ValueError('guest step results')
    for key in ('TEXTURE_ERROR','DRAW_ERROR','PRESENT_ERROR'):
        values=re.findall('RELIC_'+key+r'=([0-9a-f]{8})',text)
        if len(values)!=26 or any(int(x,16) for x in values):raise ValueError('guest GLES error')
    if re.findall(r'RELIC_PRESENT=([0-9a-f]{8})',text)!=['00000001']*26:raise ValueError('guest presentations')
    native=0;hypotheses=0
    for state in ('initial','victory'):
        folder=PROOF/'clcd-multilayer'/state;manifest=folder/'CAPTURE.json';raw=manifest.read_bytes();doc=json.loads(raw);prev=json.loads((folder/'PREVIEWS.json').read_text())
        if sha(raw)!=prev['manifest_sha256']:raise ValueError('native manifest hash')
        with tempfile.TemporaryDirectory() as td:
            dest=Path(td);(dest/'CAPTURE.json').write_bytes(raw)
            for row in doc['windows']:
                if row['status']!='ok':continue
                w,h,rgba=decoded[str((folder/f"window-{row['index']}-native.png").relative_to(ROOT))]
                if row['stride']!=w*4 or (w,h)!=(row['width'],row['height']):raise ValueError('selected reconstruction only supports tightly packed BGRA8')
                bgra=b''.join(bytes((rgba[q+2],rgba[q+1],rgba[q],rgba[q+3])) for q in range(0,len(rgba),4))
                if sha(bgra)!=row['sha256']:raise ValueError('reconstructed native SHA')
                (dest/row['file']).write_bytes(bgra);native+=1
            _,loaded=exp.load_capture(dest/'CAPTURE.json',prev['manifest_sha256'])
            hf=PROOF/'clcd-composition-hypotheses'/state;hy=json.loads((hf/'HYPOTHESES.json').read_text())
            if hy['hardware_validated'] is not False or hy['instruction']!=doc['instruction']:raise ValueError('hypothesis scope/instant')
            for row in hy['hypotheses']:
                expected=exp.compose([loaded[k] for k in row['bottom_to_top']],320,480,row['equation'],(0,0,0));w,h,pixels=decoded[str((hf/row['panel']['file']).relative_to(ROOT))]
                if (w,h)!=(320,480) or pixels!=expected or sha(pixels)!=row['rgba_sha256']:raise ValueError('hypothesis pixels')
                _,lh,labelled=decoded[str((hf/row['labelled_preview']['file']).relative_to(ROOT))]
                if lh!=504 or labelled[320*24*4:]!=pixels:raise ValueError('labelled panel modified')
                hypotheses+=1
    return {'status':'PASS','curated_PNGs':len(decoded),'lossless_PPM_identities':len(pairs),'native_spans_reconstructed_and_hashed':native,'guest_states':len(observed),'UIKit_callbacks':len(directions),'guest_won':1,'mathematical_hypotheses':hypotheses,'hardware_composition_validated':False,'new_guest_execution':False}
if __name__=='__main__':print(json.dumps(verify(),indent=2))
