#!/usr/bin/env python3
"""Build our fixed-address ARMv6 diagnostics without an Apple SDK.

LLVM emits/links freestanding ARM ELF. This narrow writer wraps its resolved
segments in a classic 32-bit Mach-O executable for the guest's own dyld.
It imports only dlopen/dlsym via classic non-lazy pointers. No Apple bytes.
This is a diagnostic builder, not a general purpose Mach-O linker.
MIT; S5LBox engine by j0shua-SYSON remains separately attributed.
"""
import argparse, hashlib, io, json, os, plistlib, shutil, struct, subprocess, tarfile, zlib
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
# Set this to a complete LLVM/NDK bin directory; no machine-specific default.
LLVM=Path(os.environ.get('IOSRETROMOB_LLVM_BIN') or Path(shutil.which('clang') or 'clang').parent)
def ints(*v):return struct.pack('<'+'I'*len(v),*v)
def align(n):return (n+4095)&~4095
def name(s):return s.encode().ljust(16,b'\0')
def section(n,seg,addr,size,off,flags=0,reserved1=0):
 return name(n)+name(seg)+ints(addr,size,off,2,0,0,flags,reserved1,0)
def segment(n,addr,size,off,filesize,prot,sections=()):
 return ints(1,56+68*len(sections))+name(n)+ints(addr,size,off,filesize,prot,prot,len(sections),0)+b''.join(sections)
def pathcmd(cmd,path,extra=()):
 b=path.encode()+b'\0';header=12+4*len(extra);size=(header+len(b)+3)&~3
 return (ints(cmd,size,header,*extra)+b).ljust(size,b'\0')
def macho(elf):
 assert elf[:7]==b'\x7fELF\x01\x01\x01'
 e=struct.unpack_from('<16sHHIIIIIHHHHHH',elf);assert e[2]==40
 entry,phoff,phsize,phnum=e[4],e[5],e[9],e[10]
 loads=[]
 for i in range(phnum):
  p=struct.unpack_from('<8I',elf,phoff+i*phsize)
  if p[0]==1:loads.append(p)
 def at(addr):
  p=next(p for p in loads if p[2]==addr);return elf[p[1]:p[1]+p[4]].ljust(p[5],b'\0')
 text,data=at(0x2000),at(0x20000)
 assert entry==0x2000 and len(text)<0x1d000 and len(data)<=4096
 textsize=align(4096+len(text));dataoff=textsize;linkoff=dataoff+4096
 strings=b'\0_start\0_dlopen\0_dlsym\0'
 symbols=struct.pack('<IBBHI',1,0x0f,1,0,entry)+struct.pack('<IBBHI',8,1,0,0x100,0)+struct.pack('<IBBHI',16,1,0,0x100,0)
 link=symbols+ints(1,2)+strings
 cmds=[segment('__PAGEZERO',0,4096,0,0,0),
       segment('__TEXT',0x1000,textsize,0,textsize,5,[section('__text','__TEXT',0x2000,len(text),4096,0x80000400)]),
       segment('__DATA',0x20000,4096,dataoff,4096,3,[section('__nl_symbol_ptr','__DATA',0x20000,8,dataoff,6),section('__data','__DATA',0x20008,len(data)-8,dataoff+8)]),
       segment('__LINKEDIT',0x30000,align(len(link)),linkoff,len(link),1),
       pathcmd(14,'/usr/lib/dyld'),pathcmd(12,'/usr/lib/libSystem.B.dylib',(0,0x10000,0x10000)),
       ints(2,24,linkoff,3,linkoff+44,len(strings)),
       ints(11,80,0,0,0,1,1,2,0,0,0,0,0,0,linkoff+36,2,0,0,0,0),
       ints(5,84,1,17,*([0]*15),entry,0x10)]
 commands=b''.join(cmds);header=ints(0xfeedface,12,6,2,len(cmds),len(commands),0x85)
 assert len(header)+len(commands)<=4096
 return (header+commands).ljust(4096,b'\0')+text.ljust(textsize-4096,b'\0')+data.ljust(4096,b'\0')+link
def png(color):
 def chunk(k,b):return struct.pack('>I',len(b))+k+b+struct.pack('>I',zlib.crc32(k+b)&0xffffffff)
 rows=b''.join(b'\0'+bytes(color)*57 for _ in range(57))
 return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',57,57,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
def build(kind,out,disable_dither=False,linear_after_nearest=False):
 out.mkdir(parents=True,exist_ok=False);src=ROOT/'tests/guest-apps';commands=[]
 def run(cmd):
  p=subprocess.run([str(x) for x in cmd],capture_output=True,text=True);commands.append(dict(command=[str(x) for x in cmd],exit=p.returncode,stdout=p.stdout,stderr=p.stderr))
  (out/'build-log.json').write_text(json.dumps(commands,indent=2)+'\n')
  if p.returncode:raise RuntimeError(p.stderr)
 # iOS 3 makes r9 volatile; ELF AAPCS would retain values across Darwin calls.
 opts=['-target','armv6-none-eabi','-mcpu=arm1176jzf-s','-marm','-ffixed-r9','-mfloat-abi=softfp','-mfpu=vfp','-ffreestanding','-fno-builtin','-fno-pic','-fno-pie','-fno-stack-protector','-O1','-nostdinc','-I',src]
 if disable_dither:
  assert kind=='gles1'
  opts+=['-DRM_DISABLE_DITHER=1']
 if linear_after_nearest:
  assert kind=='texture'
  opts+=['-DRM_LINEAR_AFTER_NEAREST=1']
 run([LLVM/'clang',*opts,'-c',src/(kind+'.c'),'-o',out/'app.o'])
 run([LLVM/'clang','-target','armv6-none-eabi','-c',src/'start.S','-o',out/'start.o'])
 run([LLVM/'ld.lld','-T',src/'layout.ld',out/'start.o',out/'app.o','-o',out/'app.elf'])
 binary=macho((out/'app.elf').read_bytes());basename={'uikit':'RMUIKit','gles1':'RMGLES1','texture':'RMTexture','depth':'RMDepth','blend':'RMBlend','zprobe':'RMZProbe'}[kind]
 bundle=out/(basename+'.app');bundle.mkdir();(bundle/basename).write_bytes(binary);(bundle/basename).chmod(0o755)
 info={'CFBundleIdentifier':'org.retromobile.test.'+kind,'CFBundleExecutable':basename,'CFBundleName':basename,'CFBundleDisplayName':basename,'CFBundleVersion':'1','CFBundlePackageType':'APPL','CFBundleInfoDictionaryVersion':'6.0','MinimumOSVersion':'3.1.3','LSRequiresIPhoneOS':True,'UIDeviceFamily':[1],'CFBundleIconFile':'Icon.png','UIStatusBarHidden':False}
 (bundle/'Info.plist').write_bytes(plistlib.dumps(info,fmt=plistlib.FMT_XML,sort_keys=True));(bundle/'Icon.png').write_bytes(png((40,80,220) if kind=='uikit' else (220,80,40)))
 tar=out/'payload.tar'
 with tarfile.open(tar,'x',format=tarfile.USTAR_FORMAT) as tf:
  for p in [bundle,*sorted(bundle.iterdir())]:
   n='Applications/'+p.relative_to(out).as_posix();t=tarfile.TarInfo(n);t.uid=t.gid=0;t.mtime=1261446392
   if p.is_dir():t.type=tarfile.DIRTYPE;t.mode=0o755;tf.addfile(t)
   else:b=p.read_bytes();t.mode=0o755 if p.name==basename else 0o644;t.size=len(b);tf.addfile(t,io.BytesIO(b))
 result={'kind':kind,'linear_after_nearest':linear_after_nearest,'disable_dither':disable_dither or kind in ('texture','depth','blend','zprobe'),'binary':str((bundle/basename).resolve()),'binary_sha256':hashlib.sha256(binary).hexdigest(),'bytes':len(binary),'cpu':'ARMv6 Mach-O 32-bit, unsigned, fixed address, classic dyld imports','proprietary_sdk_used':False,'payload':str(tar.resolve()),'payload_sha256':hashlib.sha256(tar.read_bytes()).hexdigest(),'note':'Compatibility must be proved by execution in iPhone OS 3.1.3; ELF is only an intermediate.'}
 (out/'manifest.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('kind',choices=['uikit','gles1','texture','depth','blend','zprobe']);p.add_argument('out',type=Path);p.add_argument('--disable-dither',action='store_true');p.add_argument('--linear-after-nearest',action='store_true');a=p.parse_args();build(a.kind,a.out,a.disable_dither,a.linear_after_nearest)
