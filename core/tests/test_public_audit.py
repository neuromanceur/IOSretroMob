#!/usr/bin/env python3
"""Negative tests for positive-path/hash/secret/size guards, original MIT."""
import hashlib,importlib.util,json,struct,tempfile,unittest,zlib
from pathlib import Path
def fixture_png(status):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    header=chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,6,0,0,0))
    return b'\x89PNG\r\n\x1a\n'+header+chunk(b'tEXt',b'CLCD_Status\0'+status)+chunk(b'IDAT',zlib.compress(bytes((0,1,2,3,255))))+chunk(b'IEND',b'')
ROOT=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('audit',ROOT/'tools/audit_public.py');a=importlib.util.module_from_spec(s);s.loader.exec_module(a)
class AdmissionGuards(unittest.TestCase):
    def classify(self,path,data):
        issues=[];a.classify(path,data,{path},issues,'synthetic fixture');return issues
    def test_positive_selection_still_required(self):
        issues=[];a.classify('own.c',b'original C',set(),issues,'fixture');self.assertTrue(issues)
    def test_forbidden_payload_paths_even_when_allowlisted(self):
        for p in ('work/own.img','.local/own.txt','examples/build/app.o','examples/RelicMaze/Test.app/Info.plist','node_modules/a.js','.env.test','own.mdstate','own.zip'):
            self.assertTrue(self.classify(p,b'original synthetic bytes'),p)
    def test_secrets_magic_and_bounds_rejected(self):
        for b in (b'-----BEGIN PRIVATE KEY-----',b'\x7fELF'+b'own synthetic bytes',b'x'*(4*1024**2+1)):
            self.assertTrue(self.classify('own.c',b))
    def test_unreviewed_image_rejected(self):
        self.assertTrue(self.classify('examples/unreviewed.png',b'own synthetic bytes'))
    def test_hypothesis_requires_both_markers(self):
        original=a.ROOT
        with tempfile.TemporaryDirectory() as td:
            a.ROOT=Path(td);p=a.ROOT/'reports/relic-maze';p.mkdir(parents=True)
            path='reports/relic-maze/HYPOTHESIS-fixture.png'
            for payload,bad in [(fixture_png(b'not labelled'),True),(fixture_png(b'HYPOTHESIS ONLY - NOT HARDWARE VALIDATED'),False)]:
                (p/'PUBLIC_EVIDENCE_MANIFEST.json').write_text(json.dumps({'files':[{'path':path,'sha256':hashlib.sha256(payload).hexdigest(),'kind':'hypothesis','hardware_validated':False}]}))
                self.assertEqual(bool(self.classify(path,payload)),bad)
            a.ROOT=original
    def test_png_structure_even_with_known_hash(self):
        with self.assertRaises(ValueError):a.png_metadata(fixture_png(b'own')[:-1]+b'x')
    def test_unknown_classification_and_wrong_hash_rejected(self):
        path='reports/relic-maze/clcd-multilayer/victory/window-2-native.png'
        self.assertTrue(self.classify(path,b'own different pixels'))
if __name__=='__main__':unittest.main(verbosity=2)
