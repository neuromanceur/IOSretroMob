#!/usr/bin/env python3
"""Positive-path and all-object public repository audit. IOSretroMob, MIT.

A reproducible mechanical screen, supplemented by the documented origin/legal
review. It does not establish rights to arbitrary future input data.
"""
import argparse,hashlib,json,re,stat,struct,subprocess,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def git(*args):return subprocess.check_output(['git','-C',str(ROOT),*args])
SECRET=[re.compile(rb'(?m)^-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),re.compile(rb'\bgh[pousr]_[A-Za-z0-9]{30,}\b'),re.compile(rb'\bAKIA[0-9A-Z]{16}\b'),re.compile(rb'\bsk-proj-[A-Za-z0-9_-]{30,}\b'),re.compile(rb'(?m)(?:/Users|/home)/[A-Za-z0-9_.-]+/')]
MAGIC=(b'\x7fELF',b'\xcf\xfa\xed\xfe',b'\xce\xfa\xed\xfe',b'\xfe\xed\xfa\xcf',b'\xfe\xed\xfa\xce',b'PK\x03\x04',b'S5LBox SNAPSHOT')
MAX_REPOSITORY_BYTES=20*1024**2
FORBIDDEN={'.ipsw','.ipa','.snap','.mdimage','.mdstate','.img','.dmg','.macho','.bin','.elf','.o','.a','.dylib','.so','.p12','.pem','.key','.exe','.dll','.obj','.class','.pyc','.pdb','.zip','.tar','.gz','.7z'}
def png_metadata(data):
    if data[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError('PNG signature')
    p=8;text={};ended=False
    while p<len(data):
        if p+12>len(data):raise ValueError('PNG span')
        n=struct.unpack_from('>I',data,p)[0];kind=data[p+4:p+8]
        if p+12+n>len(data):raise ValueError('PNG span')
        chunk=data[p+8:p+8+n]
        if zlib.crc32(kind+chunk)&0xffffffff!=struct.unpack_from('>I',data,p+8+n)[0]:raise ValueError('PNG CRC')
        if kind==b'tEXt':
            key,sep,value=chunk.partition(b'\0')
            if sep:text[key]=value
        p+=n+12
        if kind==b'IEND':
            ended=True
            if n or p!=len(data):raise ValueError('PNG trailing data')
    if not ended:raise ValueError('PNG end')
    return text
def classify(path,data,allowed,issues,label):
    if path not in allowed:issues.append({'location':label,'reason':'path absent from positive allowlist','path':path})
    p=Path(path)
    if p.is_absolute() or '..' in p.parts:issues.append({'location':label,'reason':'noncanonical path','path':path})
    if p.suffix.lower()in FORBIDDEN or p.name=='.env' or p.name.startswith('.env.') or (p.parts[0]in('firmware','work')or any(x in('.github','__pycache__','.local','node_modules','Library','Temp','Caches') or x=='build' or x.startswith('build-') or x.endswith('.app') for x in p.parts[:-1])):issues.append({'location':label,'reason':'forbidden path','path':path})
    if len(data)>4*1024**2:issues.append({'location':label,'reason':'individual artifact exceeds 4 MiB','path':path})
    if data.startswith(MAGIC):issues.append({'location':label,'reason':'forbidden executable/archive/VM magic','path':path})
    if p.suffix in('.png','.ppm'):
        metadata={}
        if p.suffix=='.png':
            try:metadata=png_metadata(data)
            except ValueError:issues.append({'location':label,'reason':'invalid PNG structure/CRC','path':path})
        if path.startswith('demos/gallery/'):
            image_manifest=json.loads((ROOT/'demos/gallery/manifest.json').read_text())
            accepted={f['path']:f['sha256']for e in image_manifest['captures']for f in e['files']}
            if hashlib.sha256(data).hexdigest()!=accepted.get(path):issues.append({'location':label,'reason':'image differs from approved immutable capture','path':path})
        elif path.startswith('reports/relic-maze/'):
            registry=json.loads((ROOT/'reports/relic-maze/PUBLIC_EVIDENCE_MANIFEST.json').read_text())
            matches=[row for row in registry['files'] if row['path']==path]
            if len(matches)!=1 or matches[0]['sha256']!=hashlib.sha256(data).hexdigest():
                issues.append({'location':label,'reason':'image not an approved immutable Relic Maze artifact','path':path})
            elif matches[0]['kind']=='hypothesis':
                if matches[0].get('hardware_validated') is not False or not p.name.startswith('HYPOTHESIS-') or not metadata.get(b'CLCD_Status',b'').startswith(b'HYPOTHESIS ONLY - NOT HARDWARE VALIDATED'):
                    issues.append({'location':label,'reason':'hypothesis not explicitly marked','path':path})
            elif matches[0]['kind'] not in ('guest-window-rgb','native-window','single-window-placement'):
                issues.append({'location':label,'reason':'unknown evidence classification','path':path})
        else:issues.append({'location':label,'reason':'image outside reviewed registries','path':path})
    else:
        try:data.decode('utf-8')
        except UnicodeDecodeError:issues.append({'location':label,'reason':'unreviewed binary content','path':path})
        if b'\0'in data:issues.append({'location':label,'reason':'binary NUL in text','path':path})
        for pattern in SECRET:
            if pattern.search(data):issues.append({'location':label,'reason':'secret/personal-path pattern','path':path})
def audit():
    allowed=set(json.loads((ROOT/'reports/PUBLIC_FILE_ALLOWLIST.json').read_text())['paths']);issues=[];inventory=[]
    tracked=git('ls-files','-z').decode().split('\0')[:-1]
    for path in tracked:
        p=ROOT/path
        if p.is_symlink()or not p.is_file():issues.append({'path':path,'reason':'tracked file is missing or symlink'});continue
        data=p.read_bytes();classify(path,data,allowed,issues,'working tree');inventory.append({'path':path,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
    history=git('rev-list','--all').decode().splitlines();mapped={};trees=0
    for commit in history:
        for entry in git('ls-tree','-r','-z',commit).split(b'\0'):
            if not entry:continue
            header,path=entry.split(b'\t',1);mode,kind,oid=header.decode().split();path=path.decode()
            if mode not in('100644','100755'):issues.append({'path':path,'reason':'unsupported historical mode','mode':mode})
            mapped.setdefault(oid,set()).add(path);trees+=1
    for entry in git('ls-files','--stage','-z').split(b'\0'):
        if not entry:continue
        header,path=entry.split(b'\t',1);mode,oid,stage=header.decode().split();mapped.setdefault(oid,set()).add(path.decode())
    objects=git('cat-file','--batch-all-objects','--batch-check=%(objectname) %(objecttype) %(objectsize)').decode().splitlines();blob_count=0;object_bytes=0
    for record in objects:
        oid,kind,size=record.split();object_bytes+=int(size)
        if kind!='blob':
            if kind=='commit':
                data=git('cat-file','commit',oid)
                for pattern in SECRET:
                    if pattern.search(data):issues.append({'location':oid,'reason':'secret/personal path in commit metadata'})
            continue
        blob_count+=1;data=git('cat-file','blob',oid);paths=mapped.get(oid)
        if not paths:issues.append({'location':oid,'reason':'unreachable/unmapped blob requires review'});continue
        for path in paths:classify(path,data,allowed,issues,oid)
    tracked_bytes=sum(x['bytes'] for x in inventory)
    metadata_allocated=sum(p.stat().st_blocks*512 for p in (ROOT/'.git').rglob('*') if p.is_file())
    for name,value in [('tracked logical bytes',tracked_bytes),('all Git-object logical bytes',object_bytes),('Git allocated metadata bytes',metadata_allocated)]:
        if value>MAX_REPOSITORY_BYTES:issues.append({'location':'size gate','reason':name+' exceeds 20 MiB','bytes':value})
    links=[]
    for name in('README.md','README.fr.md'):
        txt=(ROOT/name).read_text()
        for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',txt):
            if target.startswith(('https://','http://','#')):continue
            if not(ROOT/target.split('#')[0]).exists():links.append({'document':name,'missing':target})
    if links:issues+=links
    return {'pass':not issues,'tracked_files':len(tracked),'tracked_logical_bytes':sum(x['bytes']for x in inventory),'commits':len(history),'historical_tree_entries':trees,'all_git_objects':len(objects),'all_blobs_inspected':blob_count,'all_object_logical_bytes':object_bytes,'git_metadata_allocated_bytes':metadata_allocated,'size_limit_bytes':MAX_REPOSITORY_BYTES,'positive_allowlist_paths':len(allowed),'issues':issues,'inventory':inventory,'scope':'All tracked files and every current Git object, including unreachable objects. Public NIST AES test vectors, register constants and SHA identity/digest values are not secret keys. Image rights/provenance reviewed separately.'}
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path);a=p.parse_args();r=audit()
    if a.output:
        with a.output.open('x')as f:json.dump(r,f,indent=2);f.write('\n')
    print(json.dumps({k:v for k,v in r.items()if k!='inventory'},indent=2));return int(not r['pass'])
if __name__=='__main__':raise SystemExit(main())
