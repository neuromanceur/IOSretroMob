"""Independent macOS CFBundle metadata regression; never executes the ARM binary.
The optional historical guest builder needs an explicit complete LLVM bin path.
"""
import ctypes,importlib.util,os,plistlib,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
class FlatBundleLayout(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  if sys.platform!='darwin':raise unittest.SkipTest('Native CFBundle metadata check requires macOS')
  llvm=os.environ.get('IOSRETROMOB_LLVM_BIN')
  if not llvm:raise unittest.SkipTest('Explicit complete LLVM directory required')
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);cls.directory=Path(cls.tmp.name)
  p=ROOT/'examples/RelicMaze/ios/build_armv6.py';s=importlib.util.spec_from_file_location('rm_build',p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
  import contextlib,io
  with contextlib.redirect_stdout(io.StringIO()):m.build(cls.directory/'flat',llvm)
  cls.cf=ctypes.CDLL('/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation');P=ctypes.c_void_p
  cls.cf.CFURLCreateFromFileSystemRepresentation.argtypes=[P,ctypes.c_char_p,ctypes.c_long,ctypes.c_ubyte];cls.cf.CFURLCreateFromFileSystemRepresentation.restype=P
  cls.cf.CFBundleCreate.argtypes=[P,P];cls.cf.CFBundleCreate.restype=P
  cls.cf.CFBundleGetIdentifier.argtypes=[P];cls.cf.CFBundleGetIdentifier.restype=P
  cls.cf.CFStringGetCString.argtypes=[P,ctypes.c_char_p,ctypes.c_long,ctypes.c_uint];cls.cf.CFStringGetCString.restype=ctypes.c_ubyte
  cls.cf.CFRelease.argtypes=[P]
 def identifier(self,path):
  p=str(path.resolve()).encode();u=self.cf.CFURLCreateFromFileSystemRepresentation(None,p,len(p),1);self.assertTrue(u)
  b=self.cf.CFBundleCreate(None,u);self.cf.CFRelease(u);self.assertTrue(b)
  s=self.cf.CFBundleGetIdentifier(b);buf=ctypes.create_string_buffer(256)
  result=buf.value.decode() if s and self.cf.CFStringGetCString(s,buf,len(buf),0x08000100) else None
  self.cf.CFRelease(b);return result
 def test_built_flat_bundle_is_discoverable(self):
  self.assertEqual(self.identifier(self.directory/'flat/RelicMaze.app'),'org.iosretromob.relicmaze')
 def test_resources_directory_selects_incompatible_layout(self):
  b=self.directory/'legacy-layout.app';(b/'Resources').mkdir(parents=True)
  (b/'Info.plist').write_bytes(plistlib.dumps({'CFBundleIdentifier':'org.example.layout-regression','CFBundleExecutable':'original','CFBundlePackageType':'APPL'}))
  (b/'original').write_bytes(b'Own test fixture, not an executable.')
  self.assertIsNone(self.identifier(b))
if __name__=='__main__':unittest.main()
