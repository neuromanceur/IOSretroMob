# Local build

Supported and validated here: macOS arm64, AppleClang 21, CMake, Python 3.9+, 8 GiB RAM. The portable core is C11. CMake 3.16+ and a compatible C compiler are required; Python uses only the standard library. Install Xcode Command Line Tools and CMake using your preferred local package manager.

```sh
./iosretromob doctor
./iosretromob build
./iosretromob test
```

Equivalent commands:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

The CLI uses two build jobs and requires a 5 GiB reserve plus a bounded 512 MiB build allowance. Measure actual storage before campaigns. Public test inputs need no Apple firmware, network download or SDK. The inherited `host_egress` test opens loopback TCP/UDP sockets; an OS sandbox that disallows local sockets can reject it. Run that local test with ordinary loopback permission and record the outcome, rather than marking the sandbox rejection a pass.

Optional diagnostic builds require a complete LLVM toolset with `clang`, `ld.lld` and `llvm-objdump`. An Android NDK LLVM toolchain can provide these host tools; no Android SDK or Apple SDK is used by the builder itself. Supply the bin directory explicitly:

```sh
export IOSRETROMOB_LLVM_BIN=/path/to/llvm/bin
python3 tools/guest_apps/build.py texture .local/rmtexture --linear-after-nearest
python3 -m unittest discover -s tests/guest-apps -p 'test_*.py'
```

The ARMv6 diagnostics are unsigned fixed-address classic Mach-O files produced from freestanding ELF, intended for the specific historical guest. `-ffixed-r9`, softfp/VFP, classic dyld non-lazy imports and no modern chained fixups are intentional. They are not general iOS packages or modern App Store apps. Outputs stay in ignored local directories.

Build-time flags `S5LBOX_JIT` and `S5LBOX_STATIC_A64_ENGINE` retain their upstream names and default OFF. They have separate upstream validation requirements; this publication build uses defaults. Android and the retained Objective-C host shell are not newly validated here.
