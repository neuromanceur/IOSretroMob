# Architecture

IOSretroMob contains one S5LBox-derived portable C11 core, a desktop host toolchain and a small compatibility laboratory. The ARMv6 CPU, MMU, interrupt and device models remain upstream work. Inherited architectural background is in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

```
Own diagnostic inputs → guest UIKit/OpenGLES (private firmware)
                     → TA / MBX registers / GART validation
                     → recognized VGP envelope → staged BGRA8 raster
                     → atomic guest surface commit → real CLCD capture

Original public fixture → decoded scene → same semantic rasterizer
                                      → synthetic PPM → independent analytic check
```

`core/src/soc/mbx_vgp.c` separates recognized program/state envelopes from geometry and pixel operations. Decoder gating validates the complete normalized envelope digest; rejected data does not become a fabricated image. The rasterizer validates dimensions, finite inputs, transforms, texture bounds and state before modifying staged pixels. `core/src/soc/mbx.c` owns GART/resource validation and atomic commit. Texture resources are staged in canonical opaque RGBA8; the supported samplers use normalized clamp-to-edge nearest or bilinear sampling.

The public lab bypasses private driver programs and submits original decoded geometry. It proves the semantic rasterizer's result, not an end-to-end guest boot or acceptance of a proprietary program. `reference_triangle.py` and `reference_texture.py` invert the geometry independently and compare every scanout pixel except explicitly counted exact boundary ties. A one-channel-unit tolerance is reported, never concealed.

Read-only Z instrumentation records observed addresses, surface bytes and hashes without enabling Z rasterization. Depth layout and test/write semantics are unresolved. Host straight-alpha blending checks are narrower than guest GL_BLEND support. The local pipeline rejects unrecognized dither/depth states.

The local v33 snapshot persists a transient TA FIFO and can reconstruct it empty from v32, requiring fresh TA submission. Current upstream v33 is different. See [UPSTREAM.md](UPSTREAM.md).

`app/Sources` retains the attributed upstream native shell because several portable policies and tests share its C sources. Its Objective-C UI is not the IOSretroMob public front end and has not been built in this validation. The supported project interface here is `iosretromob` and the desktop CMake targets.
