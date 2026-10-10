#!/usr/bin/env python3
"""Build Relic Maze for ARMv6 using the existing SDK-free classic Mach-O writer.
Original C game and assets only. No proprietary headers or framework payloads.
"""
import argparse,hashlib,importlib.util,json,os,plistlib,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
GAME=ROOT/'examples/RelicMaze'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def build(out,llvm):
 llvm=Path(llvm)
 for name in ('clang','ld.lld','llvm-objdump'):
  if not(llvm/name).is_file():raise ValueError('Complete LLVM bin directory required: '+name)
 out.mkdir(parents=True,exist_ok=False);commands=[];sdkfree=ROOT/'tests/guest-apps'
 def run(cmd):
  p=subprocess.run(list(map(str,cmd)),capture_output=True,text=True)
  commands.append({'command':list(map(str,cmd)),'exit':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
  (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if p.returncode:raise RuntimeError(p.stderr)
  return p.stdout
 opts=['-target','armv6-none-eabi','-mcpu=arm1176jzf-s','-marm','-ffixed-r9','-mfloat-abi=softfp','-mfpu=vfp','-ffreestanding','-fno-builtin','-fno-pic','-fno-pie','-fno-stack-protector','-O1','-nostdinc','-Wall','-Wextra','-Werror','-I',sdkfree,'-I',GAME/'core','-I',GAME/'ios']
 objects=[]
 for name,source in [('app',GAME/'ios/armv6_main.c'),('core',GAME/'core/relic_maze.c'),('board',GAME/'ios/board_texture.c')]:
  obj=out/(name+'.o');run([llvm/'clang',*opts,'-c',source,'-o',obj]);objects.append(obj)
 run([llvm/'clang','-target','armv6-none-eabi','-c',sdkfree/'start.S','-o',out/'start.o'])
 run([llvm/'ld.lld','-T',sdkfree/'layout.ld',out/'start.o',*objects,'-o',out/'app.elf'])
 writer=module('guest_builder',ROOT/'tools/guest_apps/build.py')
 binary=writer.macho((out/'app.elf').read_bytes());bundle=out/'RelicMaze.app';bundle.mkdir()
 (bundle/'RelicMaze').write_bytes(binary);(bundle/'RelicMaze').chmod(0o755)
 info=plistlib.loads((GAME/'ios/Info.plist').read_bytes());info.update(MinimumOSVersion='3.1.3',CFBundleIconFile='Icon.png',UIDeviceFamily=[1])
 (bundle/'Info.plist').write_bytes(plistlib.dumps(info));(bundle/'Icon.png').write_bytes(writer.png((51,104,74)))
 # iPhone OS flat bundle: a Resources/ directory selects a different CFBundle layout.
 resources=bundle;shutil.copyfile(GAME/'LICENSE',resources/'LICENSE.txt')
 level = re.findall(r'"([#.RTEG]{9})"', (GAME/'core/relic_maze.c').read_text().split('static const int relic_x')[0])
 assert len(level)==9
 (resources/'LEVEL.txt').write_text('\n'.join(level)+'\n')
 (resources/'RENDERER.txt').write_text('Original Relic Maze C99 game. SDK-free UIKit/EAGL ES1 frontend.\nOpaque 8x8 procedural atlas regenerated from the live game state;\n7x7 playable interior, one nearest-filtered quad per input redraw.\nNo depth, blend, dither, SDK or Unity runtime.\n')
 packager=module('guest_packager',ROOT/'tools/guest_apps/package.py');payload=packager.package(bundle,out/'payload.tar')
 dis=run([llvm/'llvm-objdump','--macho','--private-headers','--syms',bundle/'RelicMaze']);(out/'macho-inspection.log').write_text(dis)
 assembly=run([llvm/'llvm-objdump','-d',out/'app.elf']);(out/'disassembly.log').write_text(assembly)
 rows=[{'path':str(p.relative_to(bundle)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(bundle.rglob('*')) if p.is_file()]
 result={'binary_sha256':hashlib.sha256(binary).hexdigest(),'binary_bytes':len(binary),'files':rows,'payload':payload,'proprietary_sdk_used':False,'target':'armv6-none-eabi, ARM1176JZF-S, ARM mode, softfp/VFP, r9 reserved; classic Mach-O ARMv6 wrapper','game_core_sha256':hashlib.sha256((GAME/'core/relic_maze.c').read_bytes()).hexdigest(),'execution_proven':False}
 (out/'manifest.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('out',type=Path);p.add_argument('--llvm-bin',default=os.environ.get('IOSRETROMOB_LLVM_BIN'));a=p.parse_args()
 if not a.llvm_bin:p.error('--llvm-bin or IOSRETROMOB_LLVM_BIN required')
 build(a.out,a.llvm_bin)
