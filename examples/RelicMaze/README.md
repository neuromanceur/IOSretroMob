# Relic Maze — original mini-game for IOSretroMob

A **small, self-contained 2011-style temple puzzle**, authored in this ChatGPT session. No Temple Trouble or Unity proprietary content is copied or included.

## Play now

Open [`RelicMaze_Play.html`](RelicMaze_Play.html) in Safari or Chrome, or open `web/index.html` from the extracted archive. The preview is **real, playable JavaScript** with no internet connection or dependencies. Use arrow keys/WASD or the touch D-pad.

The game has a 9×9 map, three relics, a pushable crate, pressure plate, gated route, exit and win condition. Solution and complete tested C engine are included.

## What's in the archive

| Component | File | Validated here? |
|---|---|---|
| Playable web game | `RelicMaze_Play.html`, `web/index.html`, `web/game.js` | **Yes** — Node logic tests and independent puzzle solver |
| Portable gameplay engine | `core/relic_maze.c` / `.h` | **Yes** — compiled and CTest passed |
| Unity 3.x-oriented script | `unity33/Assets/Scripts/RelicMaze.cs` | **Source only** — editor/build/iPhone player not available |
| Native iPhone OS ES1/UIKit front end | `ios/main.m`, `ios/Info.plist` | **Source only** — uncompiled Objective-C variant; executed SDK-free frontend is separate |

**Initial archive scope**: The original delivery contained no Unity `.unity` scene, IPA, ARMv6 Mach-O, simulator run or guest rendering proof. The later SDK-free guest integration is documented below. The Unity 3.3 script uses old Unity patterns but requires confirmation in the actual historical editor. The native front end requires an appropriate verified 3.x-era iPhoneOS SDK/toolchain and testing against S5LBox's known MBX subset.

## Host tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
python3 tests/verify_level.py
node web/test_game.cjs
```

No C library dependencies beyond standard C99. Browser preview is offline, Node is only needed for the JavaScript test.

## Integrated local project

This original example is integrated under `examples/RelicMaze`. Build outputs, firmware and installed bundles remain outside Git. See `ios/README.md` and `unity33/README.md` for exact scope and next steps.

Credits: Original mini-game source released under MIT. IOSretroMob's emulation foundation remains S5LBox by j0shua-SYSON (MIT). This package does **not** contain S5LBox code or binaries, nor any Apple/Unity SDKs or commercial game assets.


## ARMv6 integration in IOSretroMob

A separate SDK-free UIKit/EAGL frontend and create-only builder now exist under
`ios/`; the portable game and Unity 3.3 source are unchanged. The original
Objective-C renderer remains unbuilt. See `ios/README.md` for the limited visual
adaptation and the local launch report for actual execution evidence. The
original delivery statements above describe the initial archive, before this
local integration; they are not claims about the later guest campaign.

First guest result (10 October 2026): the ARMv6 process launched under iPhone OS
3.1.3, rendered the original game board through EAGL/ES1 and responded to six
UIKit Down inputs, collecting one relic. Authentic raw CLCD window captures and
the guest journal are in [the launch report](../../reports/relic-maze/ARMV6_FIRST_LAUNCH.md).
The later [full playthrough](../../reports/relic-maze/FULL_GUEST_PLAYTHROUGH.md)
proved **25 UIKit movements and `won=1` in the guest**, with 26 successful GLES1
presentations. The [multi-window campaign](../../reports/relic-maze/CLCD_MULTILAYER_CAPTURE.md)
then captured the initial/final GLES board and real UIKit controls/HUD separately.
[Composition previews](../../reports/relic-maze/CLCD_COMPOSITION.md) remain
explicitly hypothetical. No Unity runtime, original `main.m` build or sustained
real-time frame rate is claimed.
