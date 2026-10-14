import importlib.util,plistlib,tarfile,tempfile,unittest,hashlib
from unittest import mock
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
 def test_large_streaming_bundle_and_digest(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);b=self.bundle(root);block=bytes(range(256))*4096
   for name in ['Test','a','b','c']:
    with (b/name).open('wb') as f:
     for _ in range(17):f.write(block)
   p=root/'out.tar'
   # Only Info.plist may use read_bytes: bodies and archive must stream.
   original=Path.read_bytes
   def guarded(path):
    if path.name!='Info.plist':raise AssertionError('unbounded read_bytes')
    return original(path)
   with mock.patch.object(Path,'read_bytes',guarded):result=pkg.package(b,p)
   self.assertGreater(result['content_bytes'],64*1024*1024)
   self.assertEqual(result['sha256'],pkg.sha256_file(p))
   with tarfile.open(p) as tf:
    for name in ['Test','a','b','c']:
     h=hashlib.sha256()
     with tf.extractfile('Applications/Test.app/'+name) as f:
      for piece in iter(lambda:f.read(1024*1024),b''):h.update(piece)
     self.assertEqual(h.hexdigest(),hashlib.sha256(block*17).hexdigest())
 def test_size_caps_and_backslash_refuse_before_output(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);b=self.bundle(root);p=root/'out.tar'
   with (b/'large').open('wb') as f:f.truncate(pkg.MAX_FILE_BYTES+1)
   with self.assertRaisesRegex(ValueError,'32 MiB'):pkg.package(b,p)
   self.assertFalse(p.exists());(b/'large').unlink()
   for i in range(4):
    with (b/str(i)).open('wb') as f:f.truncate(pkg.MAX_FILE_BYTES)
   with self.assertRaisesRegex(ValueError,'budget'):pkg.package(b,p)
   self.assertFalse(p.exists())
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);b=self.bundle(root);(b/'bad\\path').write_bytes(b'x');p=root/'out.tar'
   with self.assertRaisesRegex(ValueError,'unsafe'):pkg.package(b,p)
   self.assertFalse(p.exists())
if __name__=='__main__':unittest.main()
