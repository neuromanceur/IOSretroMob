#!/usr/bin/env python3
"""Read-only inspection of the measured GLES VGP-input triangle payload.

This is not a VGP interpreter or a renderer. It extracts bounded uniforms and
vertex inputs from one identified diagnostic layout, and fails closed on
ambiguity or malformed data. It never writes guest memory or screen pixels.
MIT; S5LBox engine by j0shua-SYSON.
"""
import argparse, hashlib, json, math, struct
from pathlib import Path

def inspect(words):
    if not 30 <= len(words) <= 32768 or any(type(v) is not int or not 0 <= v <= 0xffffffff for v in words):
        raise ValueError('Invalid bounded word array')
    if words[-1] != 0xf0000000:
        raise ValueError('Missing TA submit terminator')
    controls = [i for i in range(1,len(words)) if words[i] == 0xf0020004 and words[i-1] == 0x78020000]
    if len(controls) != 1:
        raise ValueError('Ambiguous or unsupported vertex input boundary')
    c = controls[0]
    if c + 27 != len(words) or words[c+25] != 3:
        raise ValueError('Not the measured three-vertex, eight-word input layout')
    def floats(values):
        out=[struct.unpack('<f',struct.pack('<I',v))[0] for v in values]
        if not all(math.isfinite(v) for v in out):raise ValueError('Nonfinite input')
        return out
    vertices=[]
    for n in range(3):
        row=words[c+1+n*8:c+9+n*8]
        if row[0] != 0:raise ValueError('Unsupported vertex prefix')
        xyz=floats(row[1:4]);rgba=floats(row[4:8])
        if not all(0 <= v <= 1 for v in rgba):raise ValueError('Unbounded colour')
        vertices.append({'xyz':xyz,'rgba':rgba})
    uniforms=[i for i in range(c-16) if words[i] == 0xa0000003]
    if len(uniforms) != 1:raise ValueError('Ambiguous uniform-0 matrix')
    u=uniforms[0];matrix=floats(words[u+1:u+17])
    packed=struct.pack('<'+'I'*len(words),*words)
    return {'word_count':len(words),'stream_sha256':hashlib.sha256(packed).hexdigest(),
            'vertex_control_index':c,'uniform0_index':u,'uniform0_16_floats':matrix,
            'vertex_inputs':vertices,
            'scope':'Extracted measured input fields only. Shader program semantics, framebuffer flags, execution and pixel output are NOT validated.'}

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('scenes',type=Path);p.add_argument('--json',type=Path,required=True);a=p.parse_args()
    scenes=json.loads(a.scenes.read_text());assert isinstance(scenes,list) and len(scenes)<=4096
    rows=[inspect(s['words']) for s in scenes]
    r={'input':str(a.scenes.resolve()),'scenes':len(rows),
       'distinct_streams':len({v['stream_sha256'] for v in rows}),
       'distinct_uniform0_matrices':len({tuple(v['uniform0_16_floats']) for v in rows}),
       'rendered_frames':None,'rendered_frames_note':'This tool observes submitted bytes, not rendered frames.',
       'records':rows}
    with a.json.open('x') as f:json.dump(r,f,indent=2);f.write('\n')
    print(json.dumps({k:v for k,v in r.items() if k!='records'},indent=2))
