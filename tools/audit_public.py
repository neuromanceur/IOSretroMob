#!/usr/bin/env python3
"""Positive-path and all-object public repository audit. IOSretroMob, MIT.

A reproducible mechanical screen, supplemented by the documented origin/legal
review. It does not establish rights to arbitrary future input data.
"""
import argparse,hashlib,json,re,stat,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def git(*args):return subprocess.check_output(['git','-C',str(ROOT),*args])
SECRET=[re.compile(rb'(?m)^-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),re.compile(rb'\bgh[pousr]_[A-Za-z0-9]{30,}\b'),re.compile(rb'\bAKIA[0-9A-Z]{16}\b'),re.compile(rb'\bsk-proj-[A-Za-z0-9_-]{30,}\b'),re.compile(rb'(?m)(?:/Users|/home)/[A-Za-z0-9_.-]+/')]
MAGIC=(b'\x7fELF',b'\xcf\xfa\xed\xfe',b'\xce\xfa\xed\xfe',b'\xfe\xed\xfa\xcf',b'\xfe\xed\xfa\xce',b'PK\x03\x04',b'S5LBox SNAPSHOT')
FORBIDDEN={'.ipsw','.ipa','.snap','.mdimage','.mdstate','.img','.dmg','.macho','.bin','.elf','.o','.a','.dylib','.so','.p12','.pem','.key'}
def classify(path,data,allowed,issues,label):
    if path not in allowed:issues.append({'location':label,'reason':'path absent from positive allowlist','path':path})
    p=Path(path)
    if p.suffix.lower()in FORBIDDEN or p.name=='.env' or (p.parts[0]in('firmware','work')or any(x in('.github','__pycache__')for x in p.parts)):issues.append({'location':label,'reason':'forbidden path','path':path})
    if len(data)>4*1024**2:issues.append({'location':label,'reason':'individual artifact exceeds 4 MiB','path':path})
    if data.startswith(MAGIC):issues.append({'location':label,'reason':'forbidden executable/archive/VM magic','path':path})
    if p.suffix in('.png','.ppm'):
        if not path.startswith('demos/gallery/'):issues.append({'location':label,'reason':'image outside curated gallery','path':path})
        image_manifest=json.loads((ROOT/'demos/gallery/manifest.json').read_text())
        accepted={f['path']:f['sha256']for e in image_manifest['captures']for f in e['files']}
        if hashlib.sha256(data).hexdigest()!=accepted.get(path):issues.append({'location':label,'reason':'image differs from approved immutable capture','path':path})
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
    links=[]
    for name in('README.md','README.fr.md'):
        txt=(ROOT/name).read_text()
        for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',txt):
            if target.startswith(('https://','http://','#')):continue
            if not(ROOT/target.split('#')[0]).exists():links.append({'document':name,'missing':target})
    if links:issues+=links
    return {'pass':not issues,'tracked_files':len(tracked),'tracked_logical_bytes':sum(x['bytes']for x in inventory),'commits':len(history),'historical_tree_entries':trees,'all_git_objects':len(objects),'all_blobs_inspected':blob_count,'all_object_logical_bytes':object_bytes,'positive_allowlist_paths':len(allowed),'issues':issues,'inventory':inventory,'scope':'All tracked files and every current Git object, including unreachable objects. Public NIST AES test vectors, register constants and SHA identity/digest values are not secret keys. Image rights/provenance reviewed separately.'}
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path);a=p.parse_args();r=audit()
    if a.output:
        with a.output.open('x')as f:json.dump(r,f,indent=2);f.write('\n')
    print(json.dumps({k:v for k,v in r.items()if k!='inventory'},indent=2));return int(not r['pass'])
if __name__=='__main__':raise SystemExit(main())
