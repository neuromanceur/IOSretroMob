#!/usr/bin/env python3
"""Read-only, full-path HFSX inspection of stopped diagnostic work images.

Uses S5LBox's read-only extent reader; emits metadata/hash evidence, never
mounts or edits a volume. Does not silently resolve symlinks or overflow forks.
"""
import argparse, hashlib, importlib.util, json
from pathlib import Path
spec=importlib.util.spec_from_file_location('hfs',Path(__file__).parents[1]/'hfsx_extract.py')
hfs=importlib.util.module_from_spec(spec);spec.loader.exec_module(hfs)
def catalog(path,wanted=None):
 v=hfs.Volume(path);c=v.read_fork(v.catalog);size=hfs.u16(c,32);node=hfs.u32(c,24);total=hfs.u32(c,36);folders={2:'/'};records=[];seen=set()
 while node:
  if node in seen or len(seen)>=total:raise ValueError('Catalog cycle')
  seen.add(node);b=c[node*size:(node+1)*size]
  if len(b)!=size:raise ValueError('Short catalog node')
  for raw in hfs.leaf_records(b,size):
   p=hfs.parse_record(raw)
   if p and p[2] in (1,2):records.append(p)
  node=hfs.u32(b,0)
 unresolved=[r for r in records if r[2]==1]
 while unresolved:
  progress=False
  for r in unresolved[:]:
   parent,name,typ,o,raw=r;own=hfs.u32(raw,o+8)
   if own==2:unresolved.remove(r);progress=True
   elif parent in folders:folders[own]=folders[parent].rstrip('/')+'/'+name;unresolved.remove(r);progress=True
  if not progress:raise ValueError('Unresolved directory parents')
 result={}
 for parent,name,typ,o,raw in records:
  if parent not in folders:continue
  name=folders[parent].rstrip('/')+'/'+name
  meta={'path':name,'kind':'directory' if typ==1 else 'file','uid':hfs.u32(raw,o+32),'gid':hfs.u32(raw,o+36),'mode_octal':oct(hfs.u16(raw,o+42))}
  if typ==2 and (wanted is None or name==wanted or name.startswith(wanted.rstrip('/')+'/')):
   fork=hfs.Fork(raw,o+88)
   if fork.covered()!=fork.total_blocks:meta['unsupported']='Extents overflow'
   else:
    data=v.read_fork(fork);meta.update(bytes=len(data),sha256=hashlib.sha256(data).hexdigest());meta['_data']=data
  result[name]=meta
 v.f.close();return result
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('image',type=Path);p.add_argument('path');p.add_argument('--extract',type=Path);p.add_argument('--json',type=Path);a=p.parse_args()
 rows=catalog(a.image,a.path);matches=[v for k,v in rows.items() if k==a.path or k.startswith(a.path.rstrip('/')+'/')]
 if not matches:raise SystemExit('Path not found: '+a.path)
 if a.extract:
  row=rows[a.path];data=row.get('_data')
  if data is None:raise SystemExit('Not a readable regular file')
  with a.extract.open('xb') as f:f.write(data)
 for row in matches:row.pop('_data',None)
 out=json.dumps({'image':str(a.image.resolve()),'records':matches},indent=2)+'\n'
 if a.json:a.json.write_text(out)
 print(out,end='')
