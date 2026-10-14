#!/usr/bin/env python3
"""Inspect faithful native CLCD exports. Original IOSretroMob, MIT.

Per-window PNG and panel placement are diagnostics, NEVER a hardware composite.
Native alpha bytes are preserved; their blend/premultiplication semantics remain
unresolved. RGB565 has no native alpha (decoded alpha is explicitly 255).
"""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

LIMIT = 8 * 1024 * 1024

def load_native_bytes(folder, row):
    """Validate a known-format byte span, including explicitly undecoded order."""
    fmt, order = row['format_code'], row['order_code']
    if row['status'] not in ('ok', 'raw-uninterpreted-order'):
        raise ValueError('window was not exported: ' + row['status'])
    if type(order) is not int or not 0 <= order <= 3 or fmt not in (3, 7):
        raise ValueError('unsupported format or channel order')
    if (order == 0) != (row['status'] == 'ok'):
        raise ValueError('status/order inconsistency')
    name = row['file']
    if not isinstance(name, str) or Path(name).name != name or name in ('', '.', '..'):
        raise ValueError('raw path must be a local filename')
    path = folder / name
    if path.is_symlink():
        raise ValueError('symlink raw input')
    w, h, stride = row['width'], row['height'], row['stride']
    if not all(type(x) is int for x in (w, h, stride)) or not (0 < w <= 2047 and 0 < h <= 1023):
        raise ValueError('invalid geometry')
    bpp = 4 if fmt == 7 else 2
    if stride < w * bpp or stride % bpp:
        raise ValueError('invalid stride')
    length = (h - 1) * stride + w * bpp
    if length > LIMIT or row['span_bytes'] != length or path.stat().st_size != length:
        raise ValueError('invalid bounded native span')
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != row['sha256']:
        raise ValueError('raw SHA-256 differs')
    return data

def load_window(folder, row):
    if row['status'] != 'ok' or row['order_code'] != 0:
        raise ValueError('channel order is not decoded')
    data = load_native_bytes(folder, row)
    fmt, w, h, stride = row['format_code'], row['width'], row['height'], row['stride']
    bpp = 4 if fmt == 7 else 2
    rgba = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            source = y * stride + x * bpp
            if fmt == 7:
                b, g, r, a = data[source:source + 4]
            else:
                word = data[source] | data[source + 1] << 8
                r5, g6, b5 = word >> 11, (word >> 5) & 63, word & 31
                r, g, b, a = (r5 << 3) | r5 >> 2, (g6 << 2) | g6 >> 4, (b5 << 3) | b5 >> 2, 255
            dest = (y * w + x) * 4
            rgba[dest:dest + 4] = bytes((r, g, b, a))
    return bytes(rgba)

def place_window(rgba, w, h, x, y, panel_w=320, panel_h=480):
    """Clip ONE window onto a transparent panel. No blending or layer ordering.

    Signed test translations are geometry helpers, not an interpretation of
    unverified signed hardware position bits (exported hardware x/y are u16).
    """
    if not all(type(v) is int for v in (w, h, x, y, panel_w, panel_h)):
        raise ValueError('noninteger geometry')
    if w <= 0 or h <= 0 or not (0 < panel_w <= 2047 and 0 < panel_h <= 1023) or len(rgba) != w * h * 4:
        raise ValueError('invalid panel geometry')
    out = bytearray(panel_w * panel_h * 4)
    left, top, right, bottom = max(0, x), max(0, y), min(panel_w, x + w), min(panel_h, y + h)
    for row in range(top, bottom):
        if right <= left:
            break
        src = ((row - y) * w + left - x) * 4
        dst = (row * panel_w + left) * 4
        out[dst:dst + (right - left) * 4] = rgba[src:src + (right - left) * 4]
    return bytes(out)

def png_rgba(w, h, rgba):
    if len(rgba) != w * h * 4:
        raise ValueError('invalid RGBA length')
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    raw = b''.join(b'\0' + rgba[y*w*4:(y+1)*w*4] for y in range(h))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w,h,8,6,0,0,0)) + chunk(b'IDAT', zlib.compress(raw,9)) + chunk(b'IEND',b'')

def inspect(manifest, output):
    if manifest.is_symlink() or manifest.stat().st_size > 32768:
        raise ValueError('invalid manifest')
    doc = json.loads(manifest.read_text())
    if doc['schema'] != 1 or doc['scope'] != 'native-window-observation;not-a-composite' or len(doc['windows']) != 4:
        raise ValueError('unsupported capture schema')
    output.mkdir(exist_ok=False)
    results = []
    for row in doc['windows']:
        if row['status'] == 'raw-uninterpreted-order':
            data = load_native_bytes(manifest.parent, row)
            results.append({'index':row['index'],'status':row['status'],
                'native_sha256':hashlib.sha256(data).hexdigest(),
                'scope':'Verified byte span only; no PNG or colour interpretation'})
            continue
        if row['status'] != 'ok':
            results.append({'index':row['index'],'status':row['status']})
            continue
        rgba = load_window(manifest.parent, row)
        for kind, w, h, pixels in [('native',row['width'],row['height'],rgba),
            ('panel-placement',320,480,place_window(rgba,row['width'],row['height'],row['x'],row['y']))]:
            data = png_rgba(w,h,pixels)
            path = output / ('window-' + str(row['index']) + '-' + kind + '.png')
            with path.open('xb') as f:
                f.write(data)
            results.append({'index':row['index'],'kind':kind,'file':path.name,'sha256':hashlib.sha256(data).hexdigest(),
                'rgba_sha256':hashlib.sha256(pixels).hexdigest(),'width':w,'height':h,'scope':'one window only; no hardware composition'})
    evidence={'instruction':doc['instruction'],'manifest_sha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),
        'scope':'Uncomposited native-channel preview; alpha preserved, blend semantics unresolved.','previews':results}
    (output/'PREVIEWS.json').write_text(json.dumps(evidence,indent=2)+'\n')
    return evidence

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('manifest',type=Path)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    try:
        result=inspect(a.manifest,a.output)
    except (ValueError,KeyError,OSError,TypeError) as exc:
        p.exit(1,str(exc)+'\n')
    print(json.dumps(result,indent=2))

if __name__ == '__main__':
    main()
