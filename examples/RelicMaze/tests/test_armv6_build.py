"""Independent ARMv6 ABI/resource regression for our own game, original MIT.
Never executes a guest binary; missing explicit LLVM is a reported skip.
"""
import contextlib,importlib.util,io,os,re,struct,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
class OriginalGameBundle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        llvm=os.environ.get('IOSRETROMOB_LLVM_BIN')
        if not llvm or not all((Path(llvm)/n).is_file() for n in ('clang','ld.lld','llvm-objdump')):
            raise unittest.SkipTest('Explicit complete LLVM unavailable; ARMv6 game not built')
        cls.llvm=Path(llvm);cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup);cls.out=Path(cls.temp.name)/'game'
        spec=importlib.util.spec_from_file_location('game_builder',ROOT/'examples/RelicMaze/ios/build_armv6.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
        with contextlib.redirect_stdout(io.StringIO()):m.build(cls.out,cls.llvm)
    def test_classic_macho_envelope_and_reserved_r9(self):
        b=(self.out/'RelicMaze.app/RelicMaze').read_bytes();h=struct.unpack_from('<7I',b)
        self.assertEqual(h[:4],(0xfeedface,12,6,2));self.assertEqual(h[6]&0x200000,0)
        p=28;libs=[];entry=None;imports=[];sym=None
        for _ in range(h[4]):
            cmd,size=struct.unpack_from('<2I',b,p);self.assertGreaterEqual(size,8);self.assertLessEqual(p+size,len(b))
            self.assertNotIn(cmd,(0x80000028,0x80000034))
            if cmd==1:
                seg=struct.unpack_from('<16s8I',b,p+8);self.assertLessEqual(seg[3]+seg[4],len(b));self.assertLessEqual(seg[4],seg[2])
            if cmd==12:
                off=struct.unpack_from('<I',b,p+8)[0];libs.append(b[p+off:p+size].split(b'\0')[0])
            if cmd==5:entry=struct.unpack_from('<I',b,p+16+15*4)[0]
            if cmd==2:sym=struct.unpack_from('<4I',b,p+8)
            p+=size
        self.assertEqual(p,28+h[5]);self.assertEqual(entry,0x2000);self.assertEqual(libs,[b'/usr/lib/libSystem.B.dylib'])
        so,n,st,sl=sym;strings=b[st:st+sl]
        for i in range(n):
            off,kind,*_=struct.unpack_from('<IBBHI',b,so+i*12)
            if kind==1:imports.append(strings[off:].split(b'\0')[0])
        self.assertEqual(imports,[b'_dlopen',b'_dlsym'])
        for name in ('app','core','board','start'):
            text=subprocess.check_output([str(self.llvm/'llvm-objdump'),'-d',str(self.out/(name+'.o'))],text=True)
            instructions=[s for s in text.splitlines() if re.match(r'^\s*[0-9a-f]+:\s*[0-9a-f]{8}\s+\w',s) and '.word' not in s]
            self.assertFalse(any(re.search(r'\br9\b',s) for s in instructions),name)
    def test_original_level_and_flat_resources(self):
        import plistlib
        bundle=self.out/'RelicMaze.app';self.assertEqual({p.name for p in bundle.iterdir()},{'RelicMaze','Info.plist','Icon.png','LICENSE.txt','LEVEL.txt','RENDERER.txt'})
        info=plistlib.loads((bundle/'Info.plist').read_bytes());self.assertEqual(info['MinimumOSVersion'],'3.1.3');self.assertEqual(info['CFBundleIdentifier'],'org.iosretromob.relicmaze')
        self.assertEqual((bundle/'LICENSE.txt').read_bytes(),(ROOT/'examples/RelicMaze/LICENSE').read_bytes())
        level=re.findall(r'"([#.RTEG]{9})"',(ROOT/'examples/RelicMaze/core/relic_maze.c').read_text().split('static const int relic_x')[0])
        self.assertEqual((bundle/'LEVEL.txt').read_text(),'\n'.join(level)+'\n')
if __name__=='__main__':unittest.main(verbosity=2)
