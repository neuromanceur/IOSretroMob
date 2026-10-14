#!/usr/bin/env python3
"""Original MIT, bounded read-only Unity 3.1/CLI/Objective-C metadata inspector.

No guest code, IL, shader or texture data is executed or exported. Supports
serialized format 8 without type trees and the observed 3.1 Texture2D layout.
Other layouts fail explicitly. CLI table layouts follow ECMA-335 II.22.
"""
import argparse, collections, hashlib, json, pathlib, struct, zipfile

def take(b, p, n):
    if p < 0 or n < 0 or p+n > len(b): raise ValueError('binary bounds')
    return b[p:p+n]
def u(b,p,f='<I'): return struct.unpack(f,take(b,p,struct.calcsize(f)))[0]
def cz(b,p):
    q=b.find(b'\0',p,min(len(b),p+4096))
    if p < 0 or q < p: raise ValueError('bounded string')
    return b[p:q].decode('utf8','replace'),q+1
def sha(b):return hashlib.sha256(b).hexdigest()
def leb(b,p,signed=False):
    val=shift=0
    for _ in range(10):
        x=u(b,p,'B');p+=1;val|=(x&127)<<shift;shift+=7
        if not x&128:
            if signed and x&64:val-=1<<shift
            return val,p
    raise ValueError('LEB overflow')
def bindings(b,lazy=False):
    p=0;ordinal=0;name='';flags=0;segment=None;address=0;typ=1;addend=0;out=[]
    def emit():
        if not name or segment is None:raise ValueError('incomplete bind state')
        if len(out)>=100000:raise ValueError('bind budget')
        out.append(dict(name=name,ordinal=ordinal,weak=bool(flags&1),segment=segment,offset=address,type=typ,addend=addend))
    while p<len(b):
        op=u(b,p,'B');p+=1;imm=op&15;op&=0xf0
        if op==0:
            if not lazy:break
            ordinal=0;name='';flags=0;segment=None;address=0;typ=1;addend=0
        elif op==0x10:ordinal=imm
        elif op==0x20:ordinal,p=leb(b,p)
        elif op==0x30:ordinal=imm-16 if imm else 0
        elif op==0x40:flags=imm;name,p=cz(b,p)
        elif op==0x50:typ=imm
        elif op==0x60:addend,p=leb(b,p,True)
        elif op==0x70:segment=imm;address,p=leb(b,p)
        elif op==0x80:x,p=leb(b,p);address+=x
        elif op==0x90:emit();address+=4
        elif op==0xa0:emit();x,p=leb(b,p);address+=4+x
        elif op==0xb0:emit();address+=4+4*imm
        elif op==0xc0:
            count,p=leb(b,p);skip,p=leb(b,p)
            if count>100000:raise ValueError('bind count')
            for _ in range(count):emit();address+=4+skip
        else:raise ValueError('unsupported bind opcode')
    return out
def asset(b):
    ms,fs,v,do=struct.unpack('>4I',take(b,0,16))
    if v!=8 or fs!=len(b) or ms>fs-16:raise ValueError('only format 8 accepted')
    p=fs-ms;start=p
    endian=u(b,p,'B');p+=1
    if endian!=0:raise ValueError('only little endian metadata accepted')
    ver,p=cz(b,p);plat=u(b,p);p+=4;types=u(b,p);p+=4
    if types!=0:raise ValueError('type trees unsupported')
    big=u(b,p);p+=4
    if big:raise ValueError('64 bit IDs unsupported')
    n=u(b,p);p+=4
    if n>100000 or p+n*20>fs:raise ValueError('object count')
    objects=[]
    for i in range(n):
        pid,off,size,tid,cid,destroyed=struct.unpack('<iIIiHH',take(b,p,20));p+=20
        if off+do<16 or off+do+size>start:raise ValueError('object overlaps metadata')
        objects.append(dict(path_id=pid,offset=off+do,bytes=size,type_id=tid,class_id=cid,destroyed=destroyed))
    ext=u(b,p);p+=4;external=[]
    if ext>10000:raise ValueError('external count')
    for _ in range(ext):
        temp,p=cz(b,p);guid=take(b,p,16).hex();p+=16;t=u(b,p);p+=4;path,p=cz(b,p)
        external.append(dict(path=path,guid=guid,type=t))
    return dict(format=v,unity_version=ver,target_platform=plat,data_offset=do,metadata_offset=start,
                metadata_bytes=ms,objects=objects,external=external,metadata_tail_bytes=fs-p)

FORMATS={1:'Alpha8',2:'ARGB4444',3:'RGB24',4:'RGBA32',5:'ARGB32',7:'RGB565',10:'DXT1',12:'DXT5',13:'RGBA4444',14:'BGRA32',30:'PVRTC_RGB2',31:'PVRTC_RGBA2',32:'PVRTC_RGB4',33:'PVRTC_RGBA4'}
def texture(b):
    ln=u(b,0);name=take(b,4,ln).decode('utf8','replace');p=(4+ln+3)&~3
    w,h,complete,fmt=struct.unpack('<4i',take(b,p,16));p+=16
    mip,readable,allowed=struct.unpack('3B',take(b,p,3));p=(p+3+3)&~3
    count,dim,fil,aniso,bias,wrap,lightmap=struct.unpack('<4ifi i'.replace(' ',''),take(b,p,28));p+=28
    sz=u(b,p);p+=4;data=take(b,p,sz)
    padding=len(b)-(p+sz)
    if not (0<=padding<=3) or any(b[p+sz:]) or not (0<w<=8192 and 0<h<=8192) or any(x not in (0,1) for x in (mip,readable,allowed)):
        raise ValueError('Texture2D layout not corroborated')
    def level(a,c):
        if fmt in (30,31):return max(a,16)*max(c,8)//4
        if fmt in (32,33):return max(a,8)*max(c,8)//2
        if fmt==10:return max(1,(a+3)//4)*max(1,(c+3)//4)*8
        if fmt==12:return max(1,(a+3)//4)*max(1,(c+3)//4)*16
        if fmt in (1,2,3,4,5,7,13,14):return a*c*{1:1,2:2,3:3,4:4,5:4,7:2,13:2,14:4}[fmt]
        return None
    sizes=[];a=w;c=h
    while True:
        sizes.append(level(a,c))
        if not mip or (a==1 and c==1):break
        a=max(1,a//2);c=max(1,c//2)
    expected=sum(sizes)*count if all(x is not None for x in sizes) else None
    return dict(name=name,width=w,height=h,format_id=fmt,format=FORMATS.get(fmt,'UNKNOWN'),mipmaps=bool(mip),
                readable=bool(readable),read_allowed=bool(allowed),image_count=count,dimension=dim,filter=fil,
                aniso=aniso,mip_bias=bias,wrap=wrap,complete_image_bytes=complete,data_bytes=sz,
                analytical_expected_bytes=expected,analytical_size_matches=expected==sz,data_sha256=sha(data))

# Coded-index sets, with tag-bit counts. Table numbers are normative ECMA IDs.
CODED={'ResolutionScope':(2,[0,26,35,1]),'TypeDefOrRef':(2,[2,1,27]),'MemberRefParent':(3,[2,1,26,6,27]),
 'HasConstant':(2,[4,8,23]),'HasCustomAttribute':(5,[6,4,1,2,8,9,10,0,14,23,20,17,26,27,32,35,38,39,40,42,44,43]),
 'CustomAttributeType':(3,[6,10]),'HasFieldMarshal':(1,[4,8]),'HasDeclSecurity':(2,[2,6,32]),
 'HasSemantics':(1,[20,23]),'MethodDefOrRef':(1,[6,10]),'MemberForwarded':(1,[4,6]),
 'Implementation':(2,[38,35,39]),'TypeOrMethodDef':(1,[2,6])}
SCHEMA={0:'h s g g g',1:'ResolutionScope s s',2:'i s s TypeDefOrRef t4 t6',3:'t4',4:'h s b',5:'t6',6:'i h h s b t8',7:'t8',8:'h h s',9:'t2 TypeDefOrRef',10:'MemberRefParent s b',11:'h HasConstant b',12:'HasCustomAttribute CustomAttributeType b',13:'HasFieldMarshal b',14:'h HasDeclSecurity b',15:'h i t2',16:'i t4',17:'b',18:'t2 t20',19:'t20',20:'h s TypeDefOrRef',21:'t2 t23',22:'t23',23:'h s b',24:'h t6 HasSemantics',25:'t2 MethodDefOrRef MethodDefOrRef',26:'s',27:'b',28:'h MemberForwarded s t26',29:'i t4',30:'i i',31:'i',32:'i h h h h i b s s',33:'i',34:'i i i',35:'h h h h i b s s b',36:'i t35',37:'i i i t35',38:'i s b',39:'i i s s Implementation',40:'i i s Implementation',41:'t2 t2',42:'h h TypeOrMethodDef s',43:'MethodDefOrRef b',44:'t42 TypeDefOrRef'}
def managed(b):
    if take(b,0,2)!=b'MZ':raise ValueError('not PE')
    p=u(b,0x3c)
    if take(b,p,4)!=b'PE\0\0':raise ValueError('PE signature')
    machine=u(b,p+4,'<H');ns=u(b,p+6,'<H');opt=p+24;osz=u(b,p+20,'<H')
    if u(b,opt,'<H')!=0x10b:raise ValueError('only PE32 supported')
    secs=[struct.unpack('<IIII',take(b,opt+osz+i*40+8,16)) for i in range(ns)]
    def rva(a,n=1):
        for vs,va,rs,ro in secs:
            if va<=a and a+n<=va+rs:return ro+a-va
        raise ValueError('RVA outside file-backed section')
    cli=rva(u(b,opt+96+14*8),72);flags=u(b,cli+16);entry=u(b,cli+20)
    md=rva(u(b,cli+8),u(b,cli+12));raw=take(b,md,u(b,cli+12))
    if take(raw,0,4)!=b'BSJB':raise ValueError('CLI metadata signature')
    vl=u(raw,12);version=take(raw,16,vl).rstrip(b'\0').decode();p=(16+vl+3)&~3
    n=u(raw,p+2,'<H');p+=4;streams={}
    for _ in range(n):
        off,sz=struct.unpack('<II',take(raw,p,8));name,e=cz(raw,p+8);p=(e+3)&~3;streams[name]=take(raw,off,sz)
    t=streams.get('#~',streams.get('#-'));strings=streams['#Strings']
    if t is None:raise ValueError('no CLI tables')
    heaps=u(t,6,'B');valid=u(t,8,'<Q');p=24;counts={}
    for i in range(64):
        if valid>>i&1:counts[i]=u(t,p);p+=4
    def width(s):
        if s=='h':return 2
        if s=='i':return 4
        if s in 'sgb':return 4 if heaps & {'s':1,'g':2,'b':4}[s] else 2
        if s.startswith('t'):return 4 if counts.get(int(s[1:]),0)>=65536 else 2
        bits,tables=CODED[s];return 4 if max(counts.get(i,0) for i in tables)>=(1<<(16-bits)) else 2
    rows={}
    for i,nr in counts.items():
        if i not in SCHEMA:raise ValueError('unsupported CLI table')
        cols=SCHEMA[i].split();ws=[width(s) for s in cols];size=sum(ws);take(t,p,size*nr)
        rows[i]=[]
        for j in range(nr):
            row=[];q=p+j*size
            for col,w in zip(cols,ws):
                val=u(t,q,'<H' if w==2 else '<I');q+=w
                row.append(cz(strings,val)[0] if col=='s' else val)
            rows[i].append(row)
        p+=size*nr
    asms=[dict(name=r[7],version='.'.join(map(str,r[1:5])),flags=r[5]) for r in rows.get(32,[])]
    refs=[dict(name=r[6],version='.'.join(map(str,r[:4])),flags=r[4]) for r in rows.get(35,[])]
    methods=[dict(token=hex(0x06000000+i+1),name=r[3],rva=r[0],impl_flags=r[1],flags=r[2]) for i,r in enumerate(rows.get(6,[]))]
    types=[]
    for i,r in enumerate(rows.get(2,[])):
        stop=rows[2][i+1][5] if i+1<len(rows[2]) else len(methods)+1
        types.append(dict(name=(r[2]+'.' if r[2] else '')+r[1],method_start=r[5],method_end_exclusive=stop))
    pinvokes=[dict(flags=r[0],member=r[1],name=r[2],module=rows[26][r[3]-1][0]) for r in rows.get(28,[])]
    return dict(pe_machine=hex(machine),cli_flags=hex(flags),entry_token=hex(entry),metadata_version=version,
                table_counts=counts,assemblies=asms,assembly_refs=refs,types=types,methods=methods,pinvokes=pinvokes)

def objc(b,m):
    def off(a,n=1):
        for s in m['segments']:
            if s['vmaddr']<=a and a+n<=s['vmaddr']+s['filesize']:return s['fileoff']+a-s['vmaddr']
        raise ValueError('Objective-C address out of file')
    def word(a):return u(b,off(a,4))
    def string(a):return cz(b,off(a))[0]
    def methods(a):
        if not a:return []
        flags=word(a);count=word(a+4);size=flags&0xffff
        if size!=12 or count>10000:raise ValueError('method list unsupported')
        return [dict(selector=string(word(a+8+i*size)),types=string(word(a+12+i*size)),imp=word(a+16+i*size)) for i in range(count)]
    def info(a):
        ro=word(a+16)&~3
        return dict(name=string(word(ro+16)),address=a,methods=methods(word(ro+20)))
    sec=next(s for s in m['sections'] if s['name']=='__objc_classlist');result=[]
    for p in range(sec['offset'],sec['offset']+sec['size'],4):
        a=u(b,p);row=info(a);row['class_methods']=info(word(a))['methods'];result.append(row)
    return result

def inspect(ipa,macho_json):
    assets=[];dlls=[];classes=collections.Counter();formats=collections.Counter();textures=[]
    with zipfile.ZipFile(ipa) as z:
        for n in z.namelist():
            if '/Data/' not in n or n.endswith('/'):continue
            b=z.read(n)
            if len(b)>=16 and u(b,8,'>I')==8 and u(b,4,'>I')==len(b):
                a=asset(b);a.update(path=n,sha256=sha(b));objs=a.pop('objects');a['object_count']=len(objs)
                a['classes']=dict(collections.Counter(x['class_id'] for x in objs));classes.update(a['classes'])
                for x in objs:
                    if x['class_id']==28:
                        tr=texture(take(b,x['offset'],x['bytes']));tr.update(asset=n,path_id=x['path_id']);textures.append(tr);formats[tr['format']]+=1
                assets.append(a)
            if '/Managed/' in n and b[:2]==b'MZ':
                d=managed(b);d.update(path=n,sha256=sha(b));dlls.append(d)
    m=json.loads(pathlib.Path(macho_json).read_text());b=(pathlib.Path(macho_json).parent/'player-armv6.macho').read_bytes()
    cmd=next(c for c in m['commands'] if c['command']=='LC_DYLD_INFO_ONLY');streams=cmd['streams'];binds={}
    for label,i in [('normal',2),('weak',4),('lazy',6)]:binds[label]=bindings(take(b,streams[i],streams[i+1]),label=='lazy')
    return dict(serialized_assets=assets,object_classes=dict(classes),texture_formats=dict(formats),textures=textures,
                managed=dlls,objc_classes=objc(b,m),dyld_bindings=binds)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('ipa');p.add_argument('macho_json');p.add_argument('output');a=p.parse_args()
    result=inspect(a.ipa,a.macho_json);pathlib.Path(a.output).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(assets=len(result['serialized_assets']),textures=len(result['textures']),formats=result['texture_formats'],assemblies=[d['assemblies'] for d in result['managed']],classes=len(result['objc_classes']))))
