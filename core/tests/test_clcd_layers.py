#!/usr/bin/env python3
"""Independent byte/pixel oracles for SYNTHETIC CLCD fixtures, MIT."""
import hashlib
import importlib.util
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('layers',ROOT/'tools/clcd_layers.py')
layers=importlib.util.module_from_spec(spec);spec.loader.exec_module(layers)
BINARY=Path(sys.argv.pop(1))

class NativeReference(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.path=Path(self.tmp.name)
        self.run=subprocess.run([str(BINARY),str(self.path)],capture_output=True,text=True,check=True)
        self.a=json.loads((self.path/'layers-0000-at-123.json').read_text())
        self.b=json.loads((self.path/'layers-0001-at-124.json').read_text())
    def tearDown(self):
        self.tmp.cleanup()
    def test_exact_native_span_and_hashes(self):
        # These byte references are literal original fixture inputs, independent
        # of the C export loop and Python channel decoding implementation.
        first=bytes([3,2,1,0,6,5,4,128,9,8,7,255]+[238]*4+[12,11,10,64,15,14,13,255,18,17,16,0])
        second=bytes([22,21,20,255,25,24,23,0]+[238]*4+[28,27,26,128,31,30,29,255])
        for index,want in [(0,first),(2,second)]:
            row=self.a['windows'][index]
            self.assertEqual((self.path/row['file']).read_bytes(),want)
            self.assertEqual(row['sha256'],hashlib.sha256(want).hexdigest())
        for ordinal,n in [(0,123),(1,124)]:
            digest=hashlib.sha256((self.path/f'layers-{ordinal:04}-at-{n}.json').read_bytes()).hexdigest()
            self.assertIn('manifest'+str(ordinal)+'_sha256='+digest,self.run.stdout)
        self.assertEqual(self.a['instruction'],123)
        self.assertEqual(self.a['windows'][1]['status'],'unsupported-format-or-order')
        self.assertEqual(self.a['windows'][3]['status'],'disabled')
    def test_native_alpha_and_legacy_rgb(self):
        expected=bytes([1,2,3,0,4,5,6,128,7,8,9,255,10,11,12,64,13,14,15,255,16,17,18,0])
        self.assertEqual(layers.load_window(self.path,self.a['windows'][0]),expected)
        # Exact historical sampler framing and RGB, including padding exclusion.
        self.assertEqual((self.path/'clcd-0000-at-123.ppm').read_bytes(),b'P6\n3 2\n255\n'+bytes(range(1,19)))
    def test_565_independent_primary_colours(self):
        self.assertEqual(layers.load_window(self.path,self.b['windows'][2]),bytes([255,255,255,255,255,0,0,255,0,255,0,255,0,0,255,255]))
    def test_unknown_order_preserves_bytes_without_png(self):
        doc=json.loads((self.path/'layers-0002-at-125.json').read_text());row=doc['windows'][2]
        expected=bytes([255,255,0,248,238,238,238,238,224,7,31,0])
        self.assertEqual(row['status'],'raw-uninterpreted-order')
        self.assertEqual(row['order_code'],1)
        self.assertEqual(layers.load_native_bytes(self.path,row),expected)
        self.assertEqual(row['sha256'],hashlib.sha256(expected).hexdigest())
        with self.assertRaises(ValueError):layers.load_window(self.path,row)
        out=self.path/'raw-only-preview';result=layers.inspect(self.path/'layers-0002-at-125.json',out)
        self.assertFalse(list(out.glob('window-2*.png')))
        self.assertEqual(next(v for v in result['previews'] if v['index']==2)['native_sha256'],row['sha256'])
        with self.assertRaises(ValueError):layers.load_native_bytes(self.path,{**row,'sha256':'0'*64})
    def test_clipping_against_coordinate_oracle(self):
        pixels=bytes(range(24))
        for x,y in [(1,2),(-1,-1),(3,2),(4,3),(65535,65535)]:
            want=bytearray(4*3*4)
            for sy in range(2):
                for sx in range(3):
                    dx,dy=x+sx,y+sy
                    if 0<=dx<4 and 0<=dy<3:
                        want[(dy*4+dx)*4:(dy*4+dx+1)*4]=pixels[(sy*3+sx)*4:(sy*3+sx+1)*4]
            self.assertEqual(layers.place_window(pixels,3,2,x,y,4,3),want)
    def test_refusals_and_no_overwrite(self):
        row=self.a['windows'][0]
        for change in [{'format_code':6},{'order_code':3},{'stride':11},{'file':'../escape.raw'},{'sha256':'0'*64},{'width':0},{'span_bytes':29}]:
            with self.assertRaises(ValueError):layers.load_window(self.path,{**row,**change})
        manifest=self.path/'layers-0000-at-123.json';output=self.path/'previews';layers.inspect(manifest,output)
        with self.assertRaises(FileExistsError):layers.inspect(manifest,output)
    def sampler(self, name, **changes):
        out=self.path/name;out.mkdir()
        env=dict(os.environ,IOSRETROMOB_CLCD_LAYERS_DIR=str(out),
            IOSRETROMOB_CLCD_LAYERS_START='123',IOSRETROMOB_CLCD_LAYERS_END='150000123',
            IOSRETROMOB_CLCD_LAYERS_INTERVAL='1000000',IOSRETROMOB_CLCD_LAYERS_MAX_BYTES='1000000')
        env.update(changes)
        result=subprocess.run([str(BINARY),'--sampler',str(out)],env=env,capture_output=True,text=True,check=True)
        return out,result
    def test_sampler_bounded_config_and_output(self):
        out,p=self.sampler('valid')
        self.assertEqual(len(list(out.glob('*.json'))),128)
        self.assertIn('SAMPLER count=128 enabled=0',p.stdout)
        for j,changes in enumerate([{'IOSRETROMOB_CLCD_LAYERS_START':'-1'},
            {'IOSRETROMOB_CLCD_LAYERS_END':'0'}, {'IOSRETROMOB_CLCD_LAYERS_INTERVAL':'999999'},
            {'IOSRETROMOB_CLCD_LAYERS_INTERVAL':'1x'}, {'IOSRETROMOB_CLCD_LAYERS_MAX_BYTES':'600000001'},
            {'IOSRETROMOB_CLCD_LAYERS_END':'18446744073709551615'}]):
            out,p=self.sampler('invalid'+str(j),**changes)
            self.assertEqual(list(out.iterdir()),[])
            self.assertIn('invalid bounded configuration',p.stderr)
    def test_sampler_budget_prevents_partial_export(self):
        out,p=self.sampler('budget',IOSRETROMOB_CLCD_LAYERS_MAX_BYTES='32768')
        self.assertEqual(list(out.iterdir()),[])
        self.assertIn('byte budget exhausted',p.stderr)
        self.assertIn('SAMPLER count=0 enabled=0 bytes=0',p.stdout)
    def test_png_every_channel_round_trip(self):
        rgba=layers.load_window(self.path,self.a['windows'][0]);png=layers.png_rgba(3,2,rgba)
        offset=8;compressed=b'';self.assertEqual(png[:8],b'\x89PNG\r\n\x1a\n')
        while offset<len(png):
            size=int.from_bytes(png[offset:offset+4],'big');kind=png[offset+4:offset+8];data=png[offset+8:offset+8+size]
            self.assertEqual(int.from_bytes(png[offset+8+size:offset+12+size],'big'),zlib.crc32(kind+data)&0xffffffff)
            if kind==b'IHDR':self.assertEqual(struct.unpack('>IIBBBBB',data),(3,2,8,6,0,0,0))
            if kind==b'IDAT':compressed+=data
            offset+=size+12
        raw=zlib.decompress(compressed);self.assertEqual(raw,b'\0'+rgba[:12]+b'\0'+rgba[12:])

if __name__=='__main__':unittest.main(verbosity=2)
