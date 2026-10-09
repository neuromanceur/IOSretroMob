# Original diagnostic applications

Source only, MIT; no historical IPA, SDK headers, Apple frameworks or commercial resources are included. Own solid-colour icons and procedural texture data are generated from source. `runtime.h` implements a small dynamic Objective-C/GL call interface; the guest supplies its own frameworks.

| Example / source | Goal and API use | Actual local outcome / limit |
|---|---|---|
| RMUIKit / `uikit.c` | UIKit window, label, UIButton event and colour change via Objective-C runtime | Blue no-touch and red touch controls executed; public gallery retains blue control. |
| RMGLES1 / `gles1.c` | UIKit/EAGLContext API1, ES1 matrix/colour arrays, glDrawArrays, presentRenderbuffer | Dither-disabled coloured triangle and sampled rotations validated; default dither variant rejected locally. |
| RMTexture / `texture.c` | Own procedural RGBA8 8×8 upload, UVs, nearest then optional linear | Triangle and square guest scanouts independently checked; opaque normalized clamp-to-edge subset. |
| RMDepth / `depth.c` | Two original overlapping triangles, FBO depth allocation, depth test modes, readback/journal diagnostics | Probe app executes, true Z-buffer rendering unresolved. Analytic reference is synthetic only. |
| RMBlend / `blend.c` | Original colour/alpha geometry and GL_BLEND source-alpha state | Guest draw family unsupported; host semantic blending checks do not prove guest rendering. |
| RMZProbe / `zprobe.c` | Controlled framebuffer/depth allocation and measured observer/journal experiments | Read-only instrumentation and addresses observed; no verified depth layout or Unity runtime. |

For each variant choose a fresh ignored output directory:

```sh
export IOSRETROMOB_LLVM_BIN=/path/to/llvm/bin
python3 tools/guest_apps/build.py uikit .local/rmuikit
python3 tools/guest_apps/build.py gles1 .local/rmgles --disable-dither
python3 tools/guest_apps/build.py texture .local/rmtexture --linear-after-nearest
python3 tools/guest_apps/build.py depth .local/rmdepth
python3 tools/guest_apps/build.py blend .local/rmblend
python3 tools/guest_apps/build.py zprobe .local/rmzprobe
```

The SDK-free builder compiles ARMv6 ELF and wraps resolved segments in an unsigned classic Mach-O envelope. It imports dlopen/dlsym; API definitions and entry stubs are original narrow diagnostic code, not copied SDKs. The guest resolves its locally provided UIKit/OpenGLES/libSystem. This writer is not a general Mach-O linker, and output compilation alone does not establish guest success. Independent ABI checks inspect CPU type, load commands, entry point, r9 reservation and import pointers.

Local bundle packaging uses `tools/guest_apps/package.py`; install only into authorized private guest media following [FIRMWARE.md](../FIRMWARE.md). Neither guest media nor generated bundles are committed. The [gallery manifest](gallery/manifest.json) provides immutable capture hashes and safe own texture-input metadata. Historical raw TA programs and full journals remain private.
