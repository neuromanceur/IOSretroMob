"""Original MIT synthetic fixtures. No Apple or commercial binary fixtures."""
import struct, unittest
import sys
from pathlib import Path
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"tools/guest_apps"))
from unittest.mock import patch
import legacy_static as m
import unity31_static as u
import dyld_cache_static as d

def unity_asset(offset=16,version=8):
    meta=b'\0'+b'3.1.0f4\0'+struct.pack('<4I',9,0,0,1)
    meta+=struct.pack('<iIIiHH',1,offset,4,28,28,0)+struct.pack('<I',0)+b'\0'
    return struct.pack('>4I',len(meta),20+len(meta),version,0)+b'test'+meta

def pe_fixture():
    # One Assembly row and no executable method body. Distinct RVA/file offsets.
    strings=b'\0Example\0';tables=struct.pack('<IBBBBQQ',0,2,0,0,1,1<<32,0)+struct.pack('<I',1)
    tables+=struct.pack('<I4HI3H',0x8004,1,2,3,4,0,0,1,0)
    streams=[('#~',tables),('#Strings',strings)]
    md=bytearray(b'BSJB'+struct.pack('<HHII',1,1,0,12)+b'v2.0.50727\0\0'+struct.pack('<HH',0,2))
    headers=[];off=len(md)+sum(8+((len(n)+1+3)&~3) for n,x in streams)
    for n,x in streams:
        name=n.encode()+b'\0';name+=b'\0'*((-len(name))%4)
        headers.append(struct.pack('<II',off,len(x))+name);off+=len(x)
    md+=b''.join(headers)+b''.join(x for n,x in streams)
    b=bytearray(2048);b[:2]=b'MZ';struct.pack_into('<I',b,0x3c,0x80);b[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HH',b,0x84,0x14c,1);struct.pack_into('<H',b,0x94,224)
    o=0x98;struct.pack_into('<H',b,o,0x10b);struct.pack_into('<II',b,o+96+14*8,0x2000,72)
    struct.pack_into('<4I',b,o+224+8,1024,0x2000,1024,0x200)
    struct.pack_into('<IHH4I',b,0x200,72,2,5,0x2100,len(md),1,0);b[0x300:0x300+len(md)]=md
    return bytes(b)

class Fixtures(unittest.TestCase):
    def test_bounds(self):
        for p,n in [(-1,1),(0,-1),(2,2)]:
            with self.subTest(p=p,n=n),self.assertRaises(ValueError):u.take(b'abc',p,n)
        self.assertEqual(u.take(b'abc',1,2),b'bc')
    def test_legacy_unity_metadata_at_eof(self):
        a=u.asset(unity_asset());self.assertEqual(a['unity_version'],'3.1.0f4');self.assertEqual(a['objects'][0]['offset'],16)
        self.assertEqual(a['target_platform'],9)
        for b in [unity_asset(offset=1000),unity_asset(version=9),unity_asset()[:-1]]:
            with self.subTest(length=len(b)),self.assertRaises(ValueError):u.asset(b)
    def test_texture_analytical(self):
        # Independent expected byte sizes, not obtained from the parser.
        for fmt,w,h,n in [(3,2,2,15),(7,2,2,10),(32,8,8,128),(33,8,8,128),(4,2,2,20)]:
            b=struct.pack('<I4i',0,w,h,n,fmt)+b'\1\0\1\0'+struct.pack('<4if2i',1,2,1,1,0.,0,0)+struct.pack('<I',n)+bytes(n)
            b+=bytes((-len(b))%4);a=u.texture(b);self.assertEqual(a['analytical_expected_bytes'],n);self.assertTrue(a['analytical_size_matches'])
            with self.assertRaises(ValueError):u.texture(b[:20])
    def test_macho_headers(self):
        b=struct.pack('<7I',0xfeedface,12,6,2,0,0,0)
        a=m.macho(b);self.assertEqual(a['header']['cpusubtype'],6)
        for invalid in [b[:20],struct.pack('<7I',0xfeedface,12,6,2,1,8,0)+struct.pack('<2I',1,0)]:
            with self.assertRaises(ValueError):m.macho(invalid)
        fat=struct.pack('>7I',0xcafebabe,1,12,6,28,len(b),2)+b
        self.assertEqual(m.slices(fat)[0]['size'],28)
    def test_binding_streams(self):
        b=b'\x11\x41_x\0\x70\0\x90\0'
        a=u.bindings(b);self.assertEqual((a[0]['name'],a[0]['ordinal'],a[0]['weak']),('_x',1,True))
        self.assertEqual(len(u.bindings(b+b,True)),2)
        for b in [b'\x90',b'\xd0',b'\x20\x80',b'\x70\0\xc0'+b'\xff'*10]:
            with self.assertRaises(ValueError):u.bindings(b)
    def test_export_trie(self):
        a=d.exports_trie(b'\0\1_x\0\6\2\0\x10\0');self.assertEqual(a,{'_x':{'flags':0,'address':16}})
        for b in [b'\0\1x\0\0',b'\x80'*10]:
            with self.assertRaises(ValueError):d.exports_trie(b)
    def test_ordinal_ownership_and_weak(self):
        ident={'current_version':1};images={'A':dict(exports={},dependencies=[dict(path='B',reexport=True)],identity=ident),
            'B':dict(exports={'_ok':{'value':123}},dependencies=[],identity=ident),'C':dict(exports={'_wrong':{'value':456}},dependencies=[],identity=ident)}
        obj=dict(libraries=[dict(path='A',compatibility_version=1)],imports=[dict(name=n,library='A',weak=w) for n,w in [('_ok',False),('_wrong',False),('_absent',True)]])
        with patch.object(d,'parse_cache',return_value=(images,None)):a=d.compare(b'synthetic',obj)
        self.assertEqual(a['counts'],{'available':1,'absent-strong':1,'absent-weak':1});self.assertEqual(a['imports'][0]['resolved']['provider'],'B')
    def test_cli_tables(self):
        b=pe_fixture();a=u.managed(b);self.assertEqual(a['assemblies'],[dict(name='Example',version='1.2.3.4',flags=0)])
        self.assertEqual(a['metadata_version'],'v2.0.50727')
        for b in [b[:100],b'not a PE']:
            with self.assertRaises(ValueError):u.managed(b)

if __name__=='__main__':unittest.main(verbosity=2)
