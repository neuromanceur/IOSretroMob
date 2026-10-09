import importlib.util,plistlib,tarfile,tempfile,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('pkg',Path(__file__).resolve().parents[2]/'tools/guest_apps/package.py');pkg=importlib.util.module_from_spec(spec);spec.loader.exec_module(pkg)
class Packaging(unittest.TestCase):
 def bundle(self,root):
  p=root/'Test.app';p.mkdir();(p/'Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable':'Test','CFBundleIdentifier':'org.example.test','CFBundlePackageType':'APPL'}));(p/'Test').write_bytes(b'own test fixture');return p
 def test_metadata_and_no_overwrite(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);b=self.bundle(root);p=root/'out.tar';pkg.package(b,p)
   with tarfile.open(p) as tf:
    m=tf.getmember('Applications/Test.app/Test');self.assertEqual((m.uid,m.gid,m.mode),(0,0,0o755));self.assertEqual(tf.extractfile(m).read(),b'own test fixture')
   old=p.read_bytes()
   with self.assertRaises(FileExistsError):pkg.package(b,p)
   self.assertEqual(p.read_bytes(),old)
 def test_symlink_refused_before_publication(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);b=self.bundle(root);(b/'escape').symlink_to('/private/var/tmp');p=root/'out.tar'
   with self.assertRaises(ValueError):pkg.package(b,p)
   self.assertFalse(p.exists())
 def test_bad_executable_path_refused(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);b=self.bundle(root);(b/'Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable':'../outside','CFBundleIdentifier':'test','CFBundlePackageType':'APPL'}));p=root/'out.tar'
   with self.assertRaises(ValueError):pkg.package(b,p)
   self.assertFalse(p.exists())
if __name__=='__main__':unittest.main()
