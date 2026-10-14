#!/usr/bin/env python3
"""Package an explicitly selected .app as a create-only HFSX overlay.

No IPA unpacking, Apple payload acquisition, device installation, or disk edit.
Bundle files are bounded regular files, directories only; symlinks are refused.
Use with the create-only external-md provisioner, then prove guest discovery.
"""
import argparse, hashlib, json, plistlib, re, tarfile
from pathlib import Path
MAX_FILE_BYTES=32*1024*1024
MAX_CONTENT_BYTES=120*1024*1024

def sha256_file(path):
 h=hashlib.sha256()
 with Path(path).open("rb") as stream:
  for block in iter(lambda:stream.read(1024*1024),b""):h.update(block)
 return h.hexdigest()

def package(bundle,destination):
 bundle=Path(bundle);destination=Path(destination)
 if not bundle.is_dir() or bundle.is_symlink() or not re.fullmatch(r'[A-Za-z0-9_.-]+\.app',bundle.name):raise ValueError('Expected a plain ASCII .app directory')
 plist=plistlib.loads((bundle/'Info.plist').read_bytes());exe=plist.get('CFBundleExecutable','');identifier=plist.get('CFBundleIdentifier','')
 if not re.fullmatch(r'[A-Za-z0-9_.-]+',exe) or not identifier or plist.get('CFBundlePackageType')!='APPL':raise ValueError('Missing/invalid executable, identifier or APPL type')
 if not (bundle/exe).is_file():raise ValueError('Bundle executable missing')
 files=[bundle,*sorted(bundle.rglob('*'))];rows=[];total=0
 if len(files)>1000:raise ValueError('Provisioner entry budget exceeded')
 for p in files:
  rel=p.relative_to(bundle.parent).as_posix();name='Applications/'+rel
  if '\\' in rel or '..' in Path(rel).parts or any(ord(c)<32 or ord(c)>126 for c in rel):raise ValueError('Non-printable/unsafe HFSX path')
  if p.is_symlink() or not(p.is_file() or p.is_dir()):raise ValueError('Only regular files and directories accepted: '+rel)
  size=p.stat().st_size if p.is_file() else 0
  if size>MAX_FILE_BYTES:raise ValueError('File exceeds HFSX provisioner 32 MiB cap: '+rel)
  total+=size
  if total>MAX_CONTENT_BYTES:raise ValueError('Tar size budget exceeded')
  rows.append((p,name,size))
 with tarfile.open(destination,'x',format=tarfile.USTAR_FORMAT) as tf:
  for p,name,size in rows:
   t=tarfile.TarInfo(name);t.uid=t.gid=0;t.mtime=1261446392
   if p.is_dir():t.type=tarfile.DIRTYPE;t.mode=0o755;tf.addfile(t)
   else:
    t.mode=0o755 if p.name==exe or p.stat().st_mode&0o111 else 0o644;t.size=size
    with p.open('rb') as stream:tf.addfile(t,stream)
 return {'bundle':str(bundle.resolve()),'identifier':identifier,'executable':exe,'payload':str(destination.resolve()),'entries':len(rows),'content_bytes':total,'sha256':sha256_file(destination),'ownership':'root:root','scope':'/Applications system-bundle test; not MobileInstallation IPA/container support'}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('bundle',type=Path);p.add_argument('destination',type=Path);a=p.parse_args();print(json.dumps(package(a.bundle,a.destination),indent=2))
