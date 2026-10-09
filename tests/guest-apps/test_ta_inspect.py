"""Independent synthetic layout checks, with no Apple shader bytes."""
import importlib.util, struct, unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('inspect_ta',Path(__file__).resolve().parents[2]/'tools/guest_apps/ta_inspect.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def f(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def scene():
    words=[0xa0000003]+[f(v) for v in (1,0,0,0,0,1,0,0,0,0,-1,0,0,0,0,1)]+[0x78020000,0xf0020004]
    for xyz,rgba in [((-0.7,-0.6,0),(1,0,0,1)),((0.7,-0.6,0),(0,1,0,1)),((0,0.7,0),(0,0,1,1))]:
        words += [0]+[f(v) for v in xyz+rgba]
    return words+[3,0xf0000000]
class Checks(unittest.TestCase):
    def test_inputs_and_uniforms(self):
        r=m.inspect(scene());self.assertEqual(r['vertex_inputs'][1]['rgba'],[0,1,0,1]);self.assertAlmostEqual(r['vertex_inputs'][2]['xyz'][1],0.7,places=6);self.assertEqual(r['uniform0_16_floats'][10],-1)
    def test_truncation_and_ambiguity_refused(self):
        for w in [scene()[:-1],scene()+[0],scene()+scene(),[0x78020000,0xf0020004]+scene()]:
            with self.assertRaises(ValueError):m.inspect(w)
    def test_invalid_inputs_refused(self):
        for pos,val in [(19,1),(20,0x7fc00000),(23,f(1.5)),(0,0)]:
            w=scene();w[pos]=val
            with self.assertRaises(ValueError):m.inspect(w)
if __name__=='__main__':unittest.main()
