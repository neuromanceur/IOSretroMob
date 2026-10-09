#!/usr/bin/env python3
"""Package an explicitly selected .app as a create-only HFSX overlay.

No IPA unpacking, Apple payload acquisition, device installation, or disk edit.
Bundle files are bounded regular files, directories only; symlinks are refused.
Use with guest_apps/run.py on an unpublished new machine, then prove discovery.
"""
import argparse, hashlib, io, json, plistlib, re, tarfile
from pathlib import Path
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
  if '..' in Path(rel).parts or any(ord(c)<32 or ord(c)>126 for c in rel):raise ValueError('Non-printable/unsafe HFSX path')
  if p.is_symlink() or not(p.is_file() or p.is_dir()):raise ValueError('Only regular files and directories accepted: '+rel)
  size=p.stat().st_size if p.is_file() else 0
  if size>16*1024*1024:raise ValueError('File exceeds HFSX provisioner 16 MiB cap: '+rel)
  total+=size
  if total>60*1024*1024:raise ValueError('Tar size budget exceeded')
  rows.append((p,name,size))
 with tarfile.open(destination,'x',format=tarfile.USTAR_FORMAT) as tf:
  for p,name,size in rows:
   t=tarfile.TarInfo(name);t.uid=t.gid=0;t.mtime=1261446392
   if p.is_dir():t.type=tarfile.DIRTYPE;t.mode=0o755;tf.addfile(t)
   else:t.mode=0o755 if p.name==exe or p.stat().st_mode&0o111 else 0o644;t.size=size;tf.addfile(t,io.BytesIO(p.read_bytes()))
 return {'bundle':str(bundle.resolve()),'identifier':identifier,'executable':exe,'payload':str(destination.resolve()),'entries':len(rows),'content_bytes':total,'sha256':hashlib.sha256(destination.read_bytes()).hexdigest(),'ownership':'root:root','scope':'/Applications system-bundle test; not MobileInstallation IPA/container support'}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('bundle',type=Path);p.add_argument('destination',type=Path);a=p.parse_args();print(json.dumps(package(a.bundle,a.destination),indent=2))
