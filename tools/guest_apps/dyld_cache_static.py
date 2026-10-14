#!/usr/bin/env python3
"""Compare legacy Mach-O imports to a read-only ARM dyld shared cache.

Original IOSretroMob utility (MIT). No Apple implementation or binary included.
Checks ordinal ownership, external exports, export tries and reexports.
Does not certify Objective-C selector ABI or runtime execution.
"""
import argparse, collections, hashlib, importlib.util, json, pathlib, struct, sys
sys.dont_write_bytecode=True
from legacy_static import checked, unpack, zstr

def uleb(b,p):
    value=shift=0
    for _ in range(10):
        v=checked(b,p,1)[0];p+=1;value|=(v&127)<<shift
        if not v&128:return value,p
        shift+=7
    raise ValueError('ULEB overflow')

def exports_trie(b):
    result={};visits=0
    def walk(p,prefix,ancestors):
        nonlocal visits
        visits+=1
        if visits>200000 or p in ancestors or len(prefix)>4096:raise ValueError('export trie budget/cycle')
        ts,q=uleb(b,p);end=q+ts;checked(b,q,ts)
        if ts:
            fl,r=uleb(b,q);item={'flags':fl}
            if fl&8:
                ordinal,r=uleb(b,r);item.update(reexport_ordinal=ordinal,import_name=zstr(b,r,end-r))
            else:
                addr,r=uleb(b,r);item['address']=addr
            result[prefix]=item
        n=checked(b,end,1)[0];q=end+1
        for _ in range(n):
            name=zstr(b,q);q+=len(name.encode())+1;child,q=uleb(b,q)
            walk(child,prefix+name,ancestors|{p})
    if b:walk(0,'',set())
    return result

def parse_cache(cache):
    if not cache.startswith(b'dyld_v1'):raise ValueError('legacy dyld cache expected')
    mo,mn,io,ni=unpack('<4I',cache,16)
    if mn>64 or ni>10000:raise ValueError('cache budget')
    mappings=[unpack('<3Q2I',cache,mo+i*32) for i in range(mn)]
    def fo(va):
        for addr,size,offset,mp,ip in mappings:
            if addr<=va<addr+size:return offset+va-addr
        raise ValueError('unmapped cache address')
    images={}
    for i in range(ni):
        va,mtime,inode,po,pad=unpack('<3Q2I',cache,io+i*32);path=zstr(cache,po);h=fo(va)
        magic,cpu,sub,typ,ncmd,cmdsize,flags=unpack('<7I',cache,h)
        if magic!=0xfeedface or ncmd>4096:raise ValueError('unexpected cached Mach-O')
        p=h+28;exports={};locals_count=0;symtab=None;dysym=None;trie=None;deps=[];ident=None;objc_sections=[]
        for _ in range(ncmd):
            cmd,size=unpack('<2I',cache,p)
            if size<8 or p+size>h+28+cmdsize:raise ValueError('cache command bounds')
            if cmd==2:symtab=unpack('<4I',cache,p+8)
            elif cmd==11:dysym=unpack('<18I',cache,p+8)
            elif cmd in (0x22,0x80000022):vals=unpack('<10I',cache,p+8);trie=vals[8:10]
            elif cmd in (12,13,0x80000018,0x8000001f):
                no,ts,cv,comp=unpack('<4I',cache,p+8);name=zstr(cache,p+no,size-no)
                d=dict(path=name,current_version=cv,compatibility_version=comp,weak=cmd==0x80000018,reexport=cmd==0x8000001f)
                if cmd==13:ident=d
                else:deps.append(d)
            elif cmd==1:
                ns=unpack('<I',cache,p+48)[0]
                if 56+ns*68>size:raise ValueError('cache sections bounds')
                for j in range(ns):
                    o=p+56+j*68;name=checked(cache,o,16).rstrip(b'\0').decode();addr,sz=unpack('<2I',cache,o+32)
                    if name in ('__objc_classlist','__objc_methname'):objc_sections.append(dict(name=name,address=addr,size=sz))
            p+=size
        if symtab:
            so,ns,st,ss=symtab;strings=checked(cache,st,ss);checked(cache,so,ns*12)
            start,end=(dysym[2],dysym[2]+dysym[3]) if dysym else (0,ns)
            for j in range(start,end):
                ix,t,sec,desc,val=unpack('<IBBHI',cache,so+j*12)
                if t&0xe0==0 and t&1 and not t&0x10 and t&0xe in (2,10,14):
                    n=zstr(strings,ix)
                    exports[n]={'value':val,'source':'external-nlist','alias':zstr(strings,val) if t&0xe==10 else None}
        if trie and trie[1]:exports.update({n:{**v,'source':'export-trie'} for n,v in exports_trie(checked(cache,*trie)).items()})
        images[path]=dict(address=va,header_offset=h,exports=exports,dependencies=deps,identity=ident,objc_sections=objc_sections,cputype=cpu,cpusubtype=sub)
    return images,fo

def compare(cache,m):
    images,fo=parse_cache(cache)
    def resolve(path,name,seen):
        if (path,name) in seen:return None
        image=images.get(path)
        if not image:return None
        seen=seen|{(path,name)};e=image['exports'].get(name)
        if e:
            if e.get('alias'):return resolve(path,e['alias'],seen)
            if 'reexport_ordinal' in e:
                ord=e['reexport_ordinal'];deps=image['dependencies']
                if ord<1 or ord>len(deps):return None
                return resolve(deps[ord-1]['path'],e['import_name'] or name,seen)
            return dict(provider=path,export=e)
        for d in image['dependencies']:
            if d['reexport']:
                r=resolve(d['path'],name,seen)
                if r:return r
        return None
    rows=[]
    for imp in m['imports']:
        r=resolve(imp['library'],imp['name'],set()) if imp['library'] else None
        other=[p for p,im in images.items() if imp['name'] in im['exports']] if not r else []
        status='available' if r else ('uncertain-special-ordinal' if not imp['library'] else ('absent-weak' if imp['weak'] else 'absent-strong'))
        rows.append({**imp,'status':status,'resolved':r,'other_image_exports':other})
    libs=[]
    for l in m['libraries']:
        image=images.get(l['path']);ident=image.get('identity') if image else None
        libs.append({**l,'present_in_cache':bool(image),'guest_identity':ident,'version_compatible':ident['current_version']>=l['compatibility_version'] if ident else None,'external_exports':len(image['exports']) if image else None})
    return dict(cache_sha256=hashlib.sha256(cache).hexdigest(),cache_bytes=len(cache),cache_images=len(images),libraries=libs,imports=rows,counts=dict(collections.Counter(r['status'] for r in rows)),scope='Static external exports in the named ARMv6 guest cache; selectors, initializer behavior and runtime dlopen/dlsym are separate questions.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('rootfs');p.add_argument('macho_json');p.add_argument('output');a=p.parse_args()
    spec=importlib.util.spec_from_file_location('hfs_inspect',pathlib.Path(__file__).with_name('inspect_hfs.py'));hfs=importlib.util.module_from_spec(spec);spec.loader.exec_module(hfs)
    path='/System/Library/Caches/com.apple.dyld/dyld_shared_cache_armv6';rows=hfs.catalog(a.rootfs,path);cache=rows[path]['_data'];m=json.loads(pathlib.Path(a.macho_json).read_text());r=compare(cache,m)
    r['rootfs_system_version']=None
    system=hfs.catalog(a.rootfs,'/System/Library/CoreServices/SystemVersion.plist')
    import plistlib
    if '/System/Library/CoreServices/SystemVersion.plist' in system:r['rootfs_system_version']=plistlib.loads(system['/System/Library/CoreServices/SystemVersion.plist']['_data'])
    pathlib.Path(a.output).write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps(dict(counts=r['counts'],cache_images=r['cache_images'],cache_sha256=r['cache_sha256'],missing=[i for i in r['imports'] if i['status']!='available'],libraries=[l for l in r['libraries'] if not l['present_in_cache'] or l['version_compatible'] is not True],system=r['rootfs_system_version']),indent=2))
