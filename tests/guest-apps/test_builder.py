"""Check the executable ABI envelope independently of the writer's helpers."""
import contextlib,importlib.util,io,re,struct,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class MachOEnvelope(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  spec=importlib.util.spec_from_file_location('builder',ROOT/'tools/guest_apps/build.py')
  cls.builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(cls.builder)
  if not all((cls.builder.LLVM/name).is_file() for name in ('clang','ld.lld','llvm-objdump')):raise unittest.SkipTest('Diagnostic LLVM runtime unavailable; no Apple SDK required')
  cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup);cls.out=Path(cls.temp.name)
  cls.outputs=[]
  for kind,variant in [('uikit',False),('gles1',False),('gles1',True),('texture',False),('texture-linear',False),('depth',False),('blend',False),('zprobe',False)]:
   linear=kind=='texture-linear'
   directory=cls.out/(kind+('-nodither' if variant else ''))
   if linear:kind='texture'
   with contextlib.redirect_stdout(io.StringIO()):cls.builder.build(kind,directory,variant,linear)
   cls.outputs.append((directory,{'uikit':'RMUIKit','gles1':'RMGLES1','texture':'RMTexture','depth':'RMDepth','blend':'RMBlend','zprobe':'RMZProbe'}[kind]))
 def test_r9_abi_regression(self):
  llvm=self.builder.LLVM
  for fixed in (False,True):
   obj=self.out/('abi-fixed.o' if fixed else 'abi-default.o')
   flags=['-ffixed-r9'] if fixed else []
   subprocess.check_call([str(llvm/'clang'),'-target','armv6-none-eabi','-mcpu=arm1176jzf-s','-marm','-mfloat-abi=softfp','-mfpu=vfp','-ffreestanding','-fno-builtin','-fno-pic','-fno-pie','-fno-stack-protector','-O1','-nostdinc',*flags,'-I',str(ROOT/'tests/guest-apps'),'-c',str(ROOT/'tests/guest-apps/uikit.c'),'-o',str(obj)])
   text=subprocess.check_output([str(llvm/'llvm-objdump'),'-d',str(obj)],text=True)
   instructions=[line for line in text.splitlines() if re.match(r'^\s*[0-9a-f]+:\s*[0-9a-f]{8}\s+\w',line) and '.word' not in line]
   self.assertEqual(any(re.search(r'\br9\b',line) for line in instructions),not fixed)
 def test_guest_binaries(self):
  for directory,binary in self.outputs:
   p=directory/(binary+'.app')/binary;b=p.read_bytes();h=struct.unpack_from('<7I',b)
   self.assertEqual(h[:4],(0xfeedface,12,6,2));self.assertEqual(h[6]&0x200000,0) # no PIE
   pos=28;segments=[];sections=[];symtab=None;dysym=None;entry=None;libs=[]
   for _ in range(h[4]):
    cmd,size=struct.unpack_from('<2I',b,pos);self.assertGreaterEqual(size,8);self.assertLessEqual(pos+size,len(b))
    if cmd==1:
     seg=struct.unpack_from('<16s8I',b,pos+8);segments.append(seg)
     for i in range(seg[7]):sections.append(struct.unpack_from('<16s16s9I',b,pos+56+68*i))
    elif cmd==2:symtab=struct.unpack_from('<4I',b,pos+8)
    elif cmd==11:dysym=struct.unpack_from('<18I',b,pos+8)
    elif cmd==5:entry=struct.unpack_from('<I',b,pos+16+15*4)[0]
    elif cmd==12:
     off=struct.unpack_from('<I',b,pos+8)[0];libs.append(b[pos+off:pos+size].split(b'\0')[0])
    self.assertNotEqual(cmd,0x80000028) # LC_MAIN unsupported by old dyld
    self.assertNotEqual(cmd,0x80000034) # no chained fixups
    pos+=size
   self.assertEqual(pos,28+h[5]);self.assertEqual(entry,0x2000);self.assertEqual(libs,[b'/usr/lib/libSystem.B.dylib'])
   for s in segments:
    self.assertLessEqual(s[3]+s[4],len(b));self.assertLessEqual(s[4],s[2])
   so,nsyms,stro,strlen=symtab;syms=[struct.unpack_from('<IBBHI',b,so+12*i) for i in range(nsyms)];strings=b[stro:stro+strlen]
   imports=[strings[s[0]:].split(b'\0')[0] for s in syms if s[1]==1];self.assertEqual(imports,[b'_dlopen',b'_dlsym'])
   ptr=next(s for s in sections if s[0].rstrip(b'\0')==b'__nl_symbol_ptr')
   self.assertEqual(ptr[2:4],(0x20000,8));self.assertEqual(ptr[8]&0xff,6)
   io,ic=dysym[12:14];self.assertEqual(ic,2);self.assertEqual(struct.unpack_from('<2I',b,io),(1,2))
if __name__=='__main__':unittest.main()
