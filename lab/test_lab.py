"""Cross-check production rasterization against independent analytic references. MIT."""
import json,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from lab import replay
BUILD=Path(sys.argv[1]).resolve() if len(sys.argv)>1 and not sys.argv[1].startswith('-') else ROOT/'build'
if len(sys.argv)>1 and not sys.argv[1].startswith('-'):del sys.argv[1]
class PublicReplays(unittest.TestCase):
 def test_all_fixtures_and_no_overwrite(self):
  with tempfile.TemporaryDirectory() as d:
   for f in sorted((ROOT/'lab/fixtures').glob('*.json')):
    if f.name=='ta-input.json':continue
    out=Path(d)/f.stem;r=replay.run(f,out,BUILD)
    self.assertEqual(r['mismatched_pixels_tolerance_1'],0)
    old=(out/'synthetic.ppm').read_bytes()
    with self.assertRaises(FileExistsError):replay.run(f,out,BUILD)
    self.assertEqual(old,(out/'synthetic.ppm').read_bytes())
 def test_corrupted_pixel_detected(self):
  with tempfile.TemporaryDirectory() as d:
   f=ROOT/'lab/fixtures/square-linear.json';out=Path(d)/'r';replay.run(f,out,BUILD)
   p=out/'synthetic.ppm';b=bytearray(p.read_bytes());b[-1]^=255;p.write_bytes(b)
   self.assertGreater(replay.compare(replay.load(f),p)['mismatched_pixels_tolerance_1'],0)
 def test_malformed_fixture_refused(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'bad.json';s=json.loads((ROOT/'lab/fixtures/triangle.json').read_text());s['matrix'][0]=float('nan');p.write_text(json.dumps(s))
   with self.assertRaises(ValueError):replay.load(p)
 def test_c_rejects_truncation_and_nonfinite_without_output(self):
  s=replay.load(ROOT/'lab/fixtures/triangle.json')
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'bad.ppm'
   for data in ('320 460 3',replay.encode(s).replace('1.0','nan',1)):
    r=subprocess.run([str(BUILD/'lab/iosretromob-replay'),str(p)],input=data,text=True,capture_output=True)
    self.assertNotEqual(r.returncode,0);self.assertFalse(p.exists())
if __name__=='__main__':unittest.main()
