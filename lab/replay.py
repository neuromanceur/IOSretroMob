"""Original decoded geometry fixtures; public replays bypass proprietary programs. MIT."""
import importlib.util,json,math,subprocess,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def module(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'tools/guest_apps'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
def load(path):
    p=Path(path)
    if p.stat().st_size>65536:raise ValueError('Fixture exceeds 64 KiB')
    s=json.loads(p.read_text())
    if s.get('schema')!='iosretromob-scene-1' or s.get('synthetic') is not True:raise ValueError('Expected an original synthetic decoded scene')
    w,h=s['size'];v=s['vertices']
    if type(w) is not int or type(h) is not int or not 0<w<=320 or not 0<h<=480 or len(v)not in(3,6):raise ValueError('Invalid dimensions or vertex count')
    if len(s['matrix'])!=16 or len(s['viewport'])!=4 or len(s['background_rgb'])!=3:raise ValueError('Invalid matrix, viewport or background')
    if s['filter']not in('none','nearest','linear'):raise ValueError('Unsupported filter')
    if any(type(c)is not int or not 0<=c<=255 for c in s['background_rgb']):raise ValueError('Invalid background')
    numbers=s['matrix']+s['viewport']
    for x in v:
        if [len(x[k])for k in('xyz','rgba','uv')]!=[3,4,2]:raise ValueError('Invalid vertex')
        numbers+=x['xyz']+x['rgba']+x['uv']
    if any(type(x)not in(int,float) or not math.isfinite(x) or abs(x)>1e6 for x in numbers):raise ValueError('Nonfinite or unbounded scene')
    return s
def encode(s):
    values=[*s['size'],len(s['vertices']),int(s['filter']!='none'),int(s['filter']=='linear'),*s['background_rgb'],*s['viewport'],*s['matrix']]
    for v in s['vertices']:values+=v['xyz']+v['rgba']+v['uv']
    return ' '.join(map(str,values))+'\n'
def compare(s,ppm):
    if s['filter']!='none':return module('reference_texture').compare(s,ppm)
    # Original metadata in the inspector's control layout, never driver code.
    def f(x):return struct.unpack('<I',struct.pack('<f',x))[0]
    words=[0x10000063]+[0]*10+[s['background_rgb'][0]|s['background_rgb'][1]<<8|s['background_rgb'][2]<<16]
    words += [0xa0000003]+list(map(f,s['matrix']))+list(map(f,s['viewport']))+[0]*3+[0x78020000,0xf0020004]
    for v in s['vertices']:words += [0]+list(map(f,v['xyz']+v['rgba']))
    words += [3,0xf0000000]
    return module('reference_triangle').compare(words,ppm)
def run(fixture,output,build):
    s=load(fixture);out=Path(output)
    if out.exists():raise FileExistsError('Replay output must be a fresh directory')
    binary=Path(build)/'lab/iosretromob-replay'
    if not binary.is_file():raise ValueError('Run ./iosretromob build first')
    out.mkdir(parents=True);ppm=out/'synthetic.ppm'
    p=subprocess.run([str(binary),str(ppm)],input=encode(s),text=True,capture_output=True)
    if p.returncode:raise ValueError(p.stderr.strip())
    result=compare(s,ppm);result['ppm']='synthetic.ppm';result['classification']='synthetic semantic replay; no guest boot or accepted TA program'
    result['raster']=json.loads(p.stdout)
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    if result['mismatched_pixels_tolerance_1'] or not result.get('covered_pixels',result.get('triangle_pixels_checked',0)):raise ValueError('Independent reference check failed')
    return result
