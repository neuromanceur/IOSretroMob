# Native iPhone OS 3.1.3 (experimental source)

The Objective-C file `main.m` plus `../core/relic_maze.c` and `Info.plist` provide a **native GLES1/UIkit frontend** for the same level and game logic as the tested C99 host engine.

- Original game; `org.iosretromob.relicmaze`; intended for ARMv6 and iPhone OS 3.x.
- Uses `CAEAGLLayer`, `EAGLContext` ES1, renderbuffer/FBO OES, fixed-function arrays, opaque triangles, and touch input.
- Explicitly disables GL_DITHER, GL_DEPTH_TEST, and GL_BLEND to avoid currently unsupported MBX paths.
- Draws only on startup/layout and after touch, avoiding unbounded animation workloads in S5LBox.
- No Apple firmware, headers, SDK, signing identities, Unity runtime or IPA are included.

**The Objective-C `main.m` variant was not compiled or run.** A genuine ARMv6 Mach-O `.app` still requires the legal iPhone SDK and LLVM/Darwin linking setup already documented or validated in the user's local IOSretroMob diagnostics. The Linux environment where these files were created does not have Apple iPhoneOS UIKit/OpenGLES frameworks. Do not assert guest success from the host C test.

## Steps for the local integration

1. Copy the folder to `IOSretroMob/examples/relic-maze/` in a private work branch.
2. Inspect the local guest diagnostic build scripts (RMGLES1/RMTexture) for **actual** SDK paths, cross-link flags and bundle installation logic; do not invent an iPhoneOS toolchain.
3. Compile `main.m` plus `core/relic_maze.c` against the verified historical SDK for target ARMv6; preserve the bundle Info.plist.
4. Install in an independent, MBX-configured *cold-boot* guest disk when required. Do not inject a new app into a restored RAM snapshot that assumes a different disk/registry.
5. Verify EAGL `FRAMEBUFFER_COMPLETE_OES`, draw submissions and actual CLCD pixels before claiming that the native version works. Inspect mismatches in TA/VGP commands, not simply `glGetError`.
6. Preserve the three historical checkpoint trios. Follow AGENTS.md storage budgets; do not push firmware/guest disk/source SDK.

Known limitations: many triangles per input render, unverified compositing/texture state, no depth, no alpha blending, no audio, no runtime Unity.


## SDK-free ARMv6 frontend (local integration)

`armv6_main.c` reuses the project's dynamic Objective-C runtime bridge and the
unchanged C game. `build_armv6.py` compiles it with a complete existing LLVM
installation and uses the already validated classic Mach-O writer. It needs no
Apple SDK headers or copied framework binaries. `main.m` remains the original,
unbuilt many-triangle frontend; its ornaments and board-tap input are not newly
validated by the SDK-free build.

```sh
export IOSRETROMOB_LLVM_BIN=/path/to/llvm/bin
python3 examples/RelicMaze/ios/build_armv6.py .local/relic-maze-armv6
```

The resulting unsigned ARMv6 `.app` is for the specifically authorized iPhone
OS 3.1.3 research guest, not a general iPhone installer or App Store package.
Outputs are create-only and must stay outside Git. The build includes the
original level, license, a procedural icon and rendering-scope text.

The current MBX decoder accepts narrow measured draw envelopes. The compatibility
frontend therefore redraws one opaque 8x8 texture containing the 7x7 playable
interior as coloured cells; the solid outer wall is implicit in the margin and
in the unchanged collision logic. Wall/floor, relic, plate, gate, exit, crate and
player colours derive from the original frontend. It omits the original cell
ornaments. This texture is generated from the live C game on startup and after
each input, not a fixed screenshot. ES1 submits a six-vertex quad with nearest
sampling; UIKit provides the title, move/relic counters, reset and four real
buttons. Direct adjacent-board tapping from `main.m` is not implemented in this
compatibility frontend. No timers, depth, blend, dither or audio are enabled.

The atlas test exercises opacity, blocked moves, changing player/relic pixels,
crate/plate/gate state and the complete winning route. These are host tests;
actual guest launch, draw and touch evidence is recorded separately in
`reports/relic-maze/ARMV6_FIRST_LAUNCH.md`.

The generated bundle uses a flat iPhone layout: keep `Info.plist`, executable
and resources at its root. A `Resources/` directory with this root plist makes
CFBundle select an incompatible layout; the native metadata regression checks
this independently. The input-driven app disables its UIKit idle timer, rather
than running an animation to stay awake. The first real guest run proved six Down callbacks and one collected relic.
The subsequent full playthrough proved 25 UIKit movements, all three relics,
crate/plate progression and **`won=1` in iPhone OS 3.1.3**. The multi-window
capture then observed both the raw 320x460 GL window at (0,20) and the 320x480
UIKit layer with alpha. See the root Relic Maze reports. Hardware-faithful full
composition and real-time fluidity remain unvalidated; the game does not use
Unity.
