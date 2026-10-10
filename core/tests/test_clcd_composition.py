#!/usr/bin/env python3
"""Synthetic analytic oracles for EXPERIMENTAL composition, original MIT.

Fraction oracle visits panel pixels and inverse-translates each layer. It does
not reuse the compositor's clipping traversal or integer equation implementation.
These tests validate mathematics only, not S5L8900 hardware rules.
"""
import hashlib
import importlib.util
import json
import struct
import sys
import tempfile
import unittest
import zlib
from fractions import Fraction
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import clcd_composition_experimental as exp

def nearest(value):
    return (2*value.numerator+value.denominator)//(2*value.denominator)

def reference(layers,width,height,mode,background):
    result=[]
    for y in range(height):
        for x in range(width):
            colour=list(background)
            for pixels,w,h,px,py in layers:
                sx,sy=x-px,y-py
                if not (0<=sx<w and 0<=sy<h):continue
                r,g,b,a=pixels[(sy*w+sx)*4:(sy*w+sx)*4+4];opacity=Fraction(a,255)
                colour=[nearest(Fraction(c)*(1 if mode=='premultiplied' else opacity)+Fraction(d)*(1-opacity)) for c,d in zip((r,g,b),colour)]
            result.extend((*colour,255))
    return bytes(result)

def decode_png(png):
    pos=8;chunks={};compressed=b''
    assert png[:8]==b'\x89PNG\r\n\x1a\n'
    while pos<len(png):
        n=int.from_bytes(png[pos:pos+4],'big');key=png[pos+4:pos+8];data=png[pos+8:pos+8+n]
        assert int.from_bytes(png[pos+8+n:pos+12+n],'big')==zlib.crc32(key+data)&0xffffffff
        if key==b'IDAT':compressed+=data
        else:chunks[key]=data
        pos+=n+12
    w,h,*fields=struct.unpack('>IIBBBBB',chunks[b'IHDR']);assert fields==[8,6,0,0,0]
    data=zlib.decompress(compressed);assert len(data)==h*(1+w*4)
    pixels=b''
    for y in range(h):
        row=data[y*(w*4+1):(y+1)*(w*4+1)];assert row[0]==0;pixels+=row[1:]
    return w,h,pixels,chunks

class CompositionReference(unittest.TestCase):
    def test_analytic_distinction(self):
        a=[(bytes((64,32,16,128)),1,1,0,0)]
        self.assertEqual(exp.compose(a,1,1,'premultiplied',(80,100,120)),bytes((104,82,76,255)))
        self.assertEqual(exp.compose(a,1,1,'straight',(80,100,120)),bytes((72,66,68,255)))
    def test_transparent_and_opaque_identities(self):
        for mode in ('premultiplied','straight'):
            self.assertEqual(exp.compose([(bytes((0,0,0,0)),1,1,0,0)],1,1,mode,(81,121,191)),bytes((81,121,191,255)))
            self.assertEqual(exp.compose([(bytes((11,27,93,255)),1,1,0,0)],1,1,mode,(81,121,191)),bytes((11,27,93,255)))
    def test_all_256_alpha_values_exact_fraction_reference(self):
        pixels=b''.join(bytes((a,a//2,a//3,a)) for a in range(256));layers=[(pixels,256,1,0,0)]
        for mode in ('premultiplied','straight'):
            self.assertEqual(exp.compose(layers,256,1,mode,(19,73,131)),reference(layers,256,1,mode,(19,73,131)))
    def test_clipping_positions_and_four_layers(self):
        layers=[(bytes((31,22,10,64))*12,4,3,-2,-1),(bytes((7,70,24,128))*6,3,2,5,4),(bytes((5,6,9,255))*4,2,2,65535,65535),(bytes((101,63,7,191))*6,2,3,2,1)]
        original=[x[0] for x in layers]
        for mode in ('premultiplied','straight'):
            self.assertEqual(exp.compose(layers,7,6,mode,(20,40,60)),reference(layers,7,6,mode,(20,40,60)))
        self.assertEqual(original,[x[0] for x in layers])
    def test_explicit_order_changes_overlap(self):
        red=(bytes((255,0,0,255)),1,1,0,0);blue=(bytes((0,0,255,255)),1,1,0,0)
        self.assertEqual(exp.compose([red,blue],1,1,'premultiplied',(0,0,0)),bytes((0,0,255,255)))
        self.assertEqual(exp.compose([blue,red],1,1,'premultiplied',(0,0,0)),bytes((255,0,0,255)))
    def test_missing_window_pixels_leave_explicit_backdrop(self):
        for mode in ('premultiplied','straight'):
            self.assertEqual(exp.compose([],2,3,mode,(4,5,6)),bytes((4,5,6,255))*6)
    def test_invalid_assumptions_refused(self):
        bad=[([],0,1,'straight',(0,0,0)),([],2048,1,'straight',(0,0,0)),([],1,1,'automatic',(0,0,0)),([],1,1,'straight',(0,0)),([],1,1,'straight',(256,0,0)),([(b'\0',1,1,0,0)],1,1,'straight',(0,0,0)),([(bytes((255,0,0,128)),1,1,0,0)],1,1,'premultiplied',(0,0,0))]
        for args in bad:
            with self.assertRaises(ValueError):exp.compose(*args)
    def fixture(self,root):
        rows=[];m={'schema':1,'scope':'native-window-observation;not-a-composite','running':True,'instruction':123,'registers':{'ctrl':0x51},'windows':rows}
        for k in range(4):
            if k not in (0,2):rows.append({'index':k,'status':'disabled'});continue
            data=bytes((20,30,40,128)) if k==2 else bytes((60,70,80,255));name=f'fixture-window-{k}.raw';(root/name).write_bytes(data)
            rows.append({'index':k,'status':'ok','width':1,'height':1,'x':0,'y':20 if k==0 else 0,'stride':4,'format_code':7,'order_code':0,'file':name,'span_bytes':4,'row_bytes':4,'sha256':hashlib.sha256(data).hexdigest(),'registers':{'stride':4,'control':0x700,'fbaddr':0x08000000+k*16,'geometry':0x10001,'linewords':1,'position':20 if k==0 else 0}})
        p=root/'fixture.json';p.write_text(json.dumps(m));return p,m,hashlib.sha256(p.read_bytes()).hexdigest()
    def test_trusted_manifest_and_raw_hashes_and_register_consistency(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);p,m,h=self.fixture(root);doc,loaded=exp.load_capture(p,h);self.assertEqual(set(loaded),{0,2})
            with self.assertRaises(ValueError):exp.load_capture(p,'0'*64)
            (root/'fixture-window-0.raw').write_bytes(b'0000')
            with self.assertRaises(ValueError):exp.load_capture(p,h)
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);p,m,h=self.fixture(root);m['windows'][0]['y']=21;p.write_text(json.dumps(m));h=hashlib.sha256(p.read_bytes()).hexdigest()
            with self.assertRaises(ValueError):exp.load_capture(p,h)
            m['windows'][0]['y']=20;m['windows'][0]['registers']['control']=0x10700;p.write_text(json.dumps(m));h=hashlib.sha256(p.read_bytes()).hexdigest()
            with self.assertRaises(ValueError):exp.load_capture(p,h)
            for malformed in ([], {**m,'windows':[None]*4}, {**m,'registers':{'ctrl':-1}}):
                p.write_text(json.dumps(malformed));h=hashlib.sha256(p.read_bytes()).hexdigest()
                with self.assertRaises(ValueError):exp.load_capture(p,h)
    def test_all_outputs_marked_and_original_panel_preserved(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);p,m,h=self.fixture(root);out=root/'new';result=exp.compare(p,h,[0,2],(0,0,0),out)
            self.assertFalse(result['hardware_validated']);self.assertEqual(len(result['hypotheses']),4)
            for row in result['hypotheses']:
                w,ht,rgba,chunks=decode_png((out/row['panel']['file']).read_bytes());self.assertEqual((w,ht),(320,480));self.assertIn(exp.STATUS.encode(),chunks[b'tEXt']);self.assertEqual(hashlib.sha256(rgba).hexdigest(),row['rgba_sha256'])
                w,ht,labelled,chunks=decode_png((out/row['labelled_preview']['file']).read_bytes());self.assertEqual((w,ht),(320,504));self.assertEqual(labelled[320*24*4:],rgba)
            with self.assertRaises(FileExistsError):exp.compare(p,h,[0,2],(0,0,0),out)
            with self.assertRaises(ValueError):exp.compare(p,h,[0,1],(0,0,0),root/'wrong')
            self.assertFalse((root/'wrong').exists())
    def test_strided_565_native_input(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);p,m,h=self.fixture(root);r=m['windows'][0];data=bytes((0,248,238,238,224,7));(root/r['file']).write_bytes(data)
            r.update(width=1,height=2,stride=4,format_code=3,span_bytes=6,row_bytes=2,sha256=hashlib.sha256(data).hexdigest());r['registers'].update(control=0x300,geometry=0x10002)
            p.write_text(json.dumps(m));h=hashlib.sha256(p.read_bytes()).hexdigest();doc,l=exp.load_capture(p,h)
            self.assertEqual(l[0][0],bytes((255,0,0,255,0,255,0,255)))

if __name__=='__main__':unittest.main(verbosity=2)
