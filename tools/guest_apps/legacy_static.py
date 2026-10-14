#!/usr/bin/env python3
"""Read-only legacy IPA/Mach-O metadata inspection. Never executes guest code.

Original IOSretroMob analysis utility. MIT; no SDK or player implementation.
Outputs are private unless their individual provenance has been reviewed.
"""
import argparse, hashlib, json, pathlib, plistlib, re, stat, struct, zipfile

def digest(b): return hashlib.sha256(b).hexdigest()
def checked(b,o,n):
    if o < 0 or n < 0 or o+n > len(b): raise ValueError('out-of-bounds binary range')
    return b[o:o+n]
def unpack(fmt,b,o): return struct.unpack(fmt,checked(b,o,struct.calcsize(fmt)))
def zstr(b,o,limit=4096):
    q=checked(b,o,min(limit,len(b)-o));e=q.find(b'\0')
    if e<0: raise ValueError('unterminated bounded string')
    return q[:e].decode('utf-8','replace')

def slices(b):
    if b[:4]==b'\xca\xfe\xba\xbe':
        n=unpack('>I',b,4)[0]
        if n>32: raise ValueError('excessive slices')
        rows=[]
        for i in range(n):
            cpu,sub,off,size,align=unpack('>5I',b,8+i*20)
            checked(b,off,size)
            rows.append(dict(cputype=cpu,cpusubtype=sub,offset=off,size=size,alignment=align))
        for i,a in enumerate(rows):
            if any(max(a['offset'],c['offset'])<min(a['offset']+a['size'],c['offset']+c['size']) for c in rows[i+1:]): raise ValueError('overlapping slices')
        return rows
    if b[:4]==b'\xce\xfa\xed\xfe':
        return [dict(cputype=unpack('<I',b,4)[0],cpusubtype=unpack('<I',b,8)[0],offset=0,size=len(b),alignment=None)]
    raise ValueError('unsupported Mach-O container')

def macho(b):
    magic,cpu,sub,typ,ncmds,cmdbytes,flags=unpack('<7I',b,0)
    if magic!=0xfeedface or ncmds>4096: raise ValueError('expected bounded 32-bit Mach-O')
    checked(b,28,cmdbytes);p=28;commands=[];segments=[];sections=[];libs=[];symbols=[];imports=[];entry=None;symtab=None;indirect=None;crypt=[];signature=[]
    names={1:'LC_SEGMENT',2:'LC_SYMTAB',11:'LC_DYSYMTAB',5:'LC_UNIXTHREAD',12:'LC_LOAD_DYLIB',13:'LC_ID_DYLIB',14:'LC_LOAD_DYLINKER',0x1b:'LC_UUID',0x1d:'LC_CODE_SIGNATURE',0x21:'LC_ENCRYPTION_INFO',0x80000018:'LC_LOAD_WEAK_DYLIB',0x8000001f:'LC_REEXPORT_DYLIB',0x22:'LC_DYLD_INFO',0x80000022:'LC_DYLD_INFO_ONLY',0x25:'LC_VERSION_MIN_IPHONEOS',0x26:'LC_FUNCTION_STARTS',0x80000028:'LC_MAIN'}
    for _ in range(ncmds):
        cmd,size=unpack('<2I',b,p)
        if size<8 or size%4 or p+size>28+cmdbytes: raise ValueError('bad load command')
        c=dict(command=names.get(cmd,hex(cmd)),cmd=cmd,offset=p,size=size)
        if cmd==1:
            name=checked(b,p+8,16).rstrip(b'\0').decode();va,vms,fo,fs,maxp,initp,ns,sflags=unpack('<8I',b,p+24)
            checked(b,fo,fs)
            if 56+68*ns>size: raise ValueError('bad section count')
            seg=dict(name=name,vmaddr=va,vmsize=vms,fileoff=fo,filesize=fs,maxprot=maxp,initprot=initp,flags=sflags);segments.append(seg)
            for j in range(ns):
                so=p+56+j*68;sn=checked(b,so,16).rstrip(b'\0').decode();sg=checked(b,so+16,16).rstrip(b'\0').decode()
                addr,ss,off,al,rel,nrel,sf,r1,r2=unpack('<9I',b,so+32)
                if sf&255 not in (1,12):checked(b,off,ss)
                sections.append(dict(name=sn,segment=sg,address=addr,size=ss,offset=off,align=al,flags=sf,reserved1=r1,reserved2=r2))
        elif cmd==2: symtab=unpack('<4I',b,p+8);c['table']=symtab
        elif cmd==11:
            ds=unpack('<18I',b,p+8);indirect=(ds[12],ds[13]);c['fields']=ds
        elif cmd in (12,13,0x80000018,0x8000001f,0x80000023):
            no,ts,cv,compat=unpack('<4I',b,p+8);name=zstr(b,p+no,size-no);c.update(path=name,current_version=cv,compatibility_version=compat)
            if cmd!=13:libs.append(dict(ordinal=len(libs)+1,path=name,weak=cmd==0x80000018,reexport=cmd==0x8000001f,current_version=cv,compatibility_version=compat))
        elif cmd==14:c['path']=zstr(b,p+unpack('<I',b,p+8)[0],size-12)
        elif cmd==5:
            flavor,count=unpack('<2I',b,p+8);c.update(flavor=flavor,count=count)
            if flavor==1 and count>=17:
                regs=unpack('<17I',b,p+16);entry=dict(pc=regs[15],cpsr=regs[16],sp=regs[13],registers=list(regs));c['entry']=entry
        elif cmd==0x21:
            off,sz,cid=unpack('<3I',b,p+8);checked(b,off,sz);x=dict(cryptoff=off,cryptsize=sz,cryptid=cid);crypt.append(x);c.update(x)
        elif cmd==0x1d:
            off,sz=unpack('<2I',b,p+8);blob=checked(b,off,sz);sig=dict(offset=off,size=sz,sha256=digest(blob));signature.append(sig);c.update(sig)
            if len(blob)>=12:
                sm,sl,sc=unpack('>3I',blob,0);sig.update(magic=hex(sm),length=sl,count=sc,slots=[])
                if sm==0xfade0cc0 and sc<100:
                    for j in range(sc):
                        slot,bo=unpack('>2I',blob,12+j*8);bm,bl=unpack('>2I',blob,bo);x=dict(type=slot,magic=hex(bm),size=bl);payload=checked(blob,bo,bl)
                        if bm==0xfade0c02 and bl>=44:
                            ver,fl,ho,io,nsp,ncode,clim=unpack('>7I',payload,8);hs,ht,plat,ps=unpack('4B',payload,36);x.update(version=hex(ver),flags=hex(fl),identifier=zstr(payload,io),code_limit=clim,hash_type=ht,hash_size=hs,page_size=1<<ps)
                            if ht==1 and hs==20 and clim<=len(b):
                                x['code_hash_mismatches']=sum(hashlib.sha1(b[k*(1<<ps):min((k+1)*(1<<ps),clim)]).digest()!=checked(payload,ho+k*hs,hs) for k in range(ncode))
                        sig['slots'].append(x)
        elif cmd in (0x22,0x80000022):c['streams']=list(unpack('<10I',b,p+8))
        elif cmd==0x25:c['version_sdk']=list(unpack('<2I',b,p+8))
        commands.append(c);p+=size
    if p!=28+cmdbytes:raise ValueError('command size mismatch')
    if symtab:
        so,ns,st,sz=symtab;checked(b,so,ns*12);strings=checked(b,st,sz)
        if ns>1_000_000:raise ValueError('symbol budget')
        for i in range(ns):
            ni,t,sec,desc,value=unpack('<IBBHI',b,so+12*i);name=zstr(strings,ni) if ni else ''
            s=dict(index=i,name=name,type=t,section=sec,desc=desc,value=value,thumb=bool(desc&8));symbols.append(s)
            if t&0xe0==0 and t&0x0e==0 and t&1 and value==0:
                ordinal=desc>>8;lib=next((l for l in libs if l['ordinal']==ordinal),None)
                imports.append(dict(name=name,weak=bool(desc&0x40),ordinal=ordinal,library=lib['path'] if lib else None,index=i))
    stubs=[];initializers=[]
    for sec in sections:
        if sec['flags']&255 in (6,7,8) and indirect:
            width=sec['reserved2'] if sec['flags']&255==8 else 4
            if width:
                for i in range(sec['size']//width):
                    ix=sec['reserved1']+i
                    if ix>=indirect[1]:raise ValueError('indirect table range')
                    si=unpack('<I',b,indirect[0]+ix*4)[0]
                    if si< len(symbols):stubs.append(dict(address=sec['address']+i*width,name=symbols[si]['name'],section=sec['name']))
        if sec['flags']&255==9:initializers += list(unpack('<'+str(sec['size']//4)+'I',b,sec['offset']))
    return dict(header=dict(magic=hex(magic),cputype=cpu,cpusubtype=sub,filetype=typ,ncmds=ncmds,sizeofcmds=cmdbytes,flags=hex(flags)),sha256=digest(b),bytes=len(b),commands=commands,segments=segments,sections=sections,libraries=libs,encryption=crypt,signature=signature,entry=entry,symbols=symbols,imports=imports,stubs=stubs,initializers=initializers)

def audit(ipa,out):
    ipa=pathlib.Path(ipa);out=pathlib.Path(out)
    if ipa.is_symlink() or not ipa.is_file():raise ValueError('regular original IPA required')
    original=ipa.read_bytes();out.mkdir(parents=True,exist_ok=False);inventory=[];assets=[];managed=[]
    with zipfile.ZipFile(ipa) as z:
        names=set();infos=z.infolist()
        if len(infos)>10000 or sum(i.file_size for i in infos)>150_000_000:raise ValueError('inspection budget exceeded')
        for i in infos:
            p=pathlib.PurePosixPath(i.filename);mode=i.external_attr>>16
            if i.filename in names or p.is_absolute() or '..' in p.parts or stat.S_ISLNK(mode):raise ValueError('unsafe ZIP entry')
            names.add(i.filename);data=z.read(i)
            inventory.append(dict(path=i.filename,bytes=len(data),compressed_bytes=i.compress_size,sha256=digest(data),mode=oct(mode),timestamp=list(i.date_time)))
            if '/Data/' in i.filename and len(data)>=20:
                a=dict(path=i.filename,bytes=len(data));a['serialized_header']=None
                metadata,size,ver,offset=unpack('>4I',data,0)
                if 5<=ver<=17 and size==len(data) and offset<=size:
                    # Legacy formats place their endian byte and metadata at EOF.
                    start=20 if ver>=9 else len(data)-metadata+1
                    endian_offset=16 if ver>=9 else len(data)-metadata
                    try:
                        checked(data,endian_offset,1)
                        a['serialized_header']=dict(format=ver,metadata_size=metadata,data_offset=offset,unity_version=zstr(data,start,100),little_endian=data[endian_offset]==0)
                    except ValueError:pass
                assets.append(a)
            if '/Data/Managed/' in i.filename and data[:2]==b'MZ':
                managed.append(dict(path=i.filename,bytes=len(data),sha256=digest(data),strings=[s.decode(errors='replace') for s in re.findall(rb'[\x20-\x7e]{5,}',data) if re.search(rb'Assembly|UnityEngine|mscorlib|Crystal|Mono|Texture|Shader|Application|Network|Movie|Audio|OpenGL',s)][:150]))
        if z.testzip() is not None:raise ValueError('ZIP CRC failure')
        plistpaths=[n for n in names if re.fullmatch(r'Payload/[^/]+\.app/Info.plist',n)]
        if len(plistpaths)!=1:raise ValueError('single app expected')
        pp=plistpaths[0];info=plistlib.loads(z.read(pp));bundle=pp.rsplit('/',1)[0];exe=bundle+'/'+info['CFBundleExecutable'];fat=z.read(exe);arch=slices(fat);results=[]
        for a in arch:
            b=checked(fat,a['offset'],a['size']);m=macho(b);m['container_slice']=a
            results.append(m)
            if a['cputype']==12 and a['cpusubtype']&0xffffff==6:
                (out/'player-armv6.macho').write_bytes(b)
                (out/'armv6-macho.json').write_text(json.dumps(m,indent=2)+'\n')
                relevant=[s for s in m['symbols'] if re.search('unity|mono|aot|EAGL|present|init|render|main|crystal',s['name'],re.I)]
                (out/'runtime-symbols.json').write_text(json.dumps(relevant,indent=2)+'\n')
                strings=[dict(offset=x.start(),text=x.group().decode()) for x in re.finditer(rb'[\x20-\x7e]{5,}',b)]
                relevantstrings=[s for s in strings if re.search(r'Unity|Mono|AOT|EAGL|OpenGL|3\.1\.0|renderbuffer|framebuffer|pvrtc|Init|Crystal|RGB565|colorFormat|depthFormat|A8|BGRA|GL_|could not|failed|initialize',s['text'],re.I)]
                (out/'runtime-strings.json').write_text(json.dumps(relevantstrings,indent=2)+'\n')
        summary=dict(ipa_bytes=len(original),ipa_sha256=digest(original),zip_crc_pass=True,entries=len(infos),expanded_bytes=sum(i.file_size for i in infos),inventory=inventory,info_plist=info,executable=exe,slices=[dict(slice=r['container_slice'],header=r['header'],encryption=r['encryption'],signature=r['signature'],sha256=r['sha256']) for r in results],assets=assets,managed=managed,itunes_metadata_keys=list(plistlib.loads(z.read('iTunesMetadata.plist'))) if 'iTunesMetadata.plist' in names else [],signature_related_paths=[n for n in sorted(names) if re.search(r'SC_Info|CodeResources|embedded.mobileprovision|ResourceRules|Signature',n)])
    (out/'ipa-inventory.json').write_text(json.dumps(summary,indent=2)+'\n')
    if digest(ipa.read_bytes())!=digest(original):raise ValueError('original changed')
    return summary

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('ipa');p.add_argument('out');a=p.parse_args();s=audit(a.ipa,a.out)
    print(json.dumps({k:s[k] for k in ['ipa_bytes','ipa_sha256','zip_crc_pass','entries','expanded_bytes','slices','signature_related_paths','itunes_metadata_keys']},indent=2))
