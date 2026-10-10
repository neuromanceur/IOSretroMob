# Origin and source lineage

Foundation: [S5LBox](https://github.com/j0shua-SYSON/S5LBox), j0shua-SYSON, MIT. Historical reference: `6f203ba550b49afadee008c7eb55373a838eed33` (Render measured TA streams atomically). The historical checkout was on `codex/unity-compatibility`, HEAD at that reference, with no later local commits. Nine tracked files and original untracked sources contained the local developments.

Public upstream was inspected independently at `35bd00220d5bb0d1239bd04e0e4a7ac1eb1f784f`, main, 2026-10-09 (Record guest package finish actions and reboot recovery acceptance). A separate temporary bare clone was used. No merge, fetch or write touched the historical repository. [SOURCE_LINEAGE.json](reports/SOURCE_LINEAGE.json) records file hashes and both comparisons. Its hashes describe the imported working sources; later product portability changes are visible in the new Git history.

## Feature lineage matrix

| Feature | Historical base | Local contribution | Current public upstream comparison | Evidence |
|---|---|---|---|---|
| ARMv6 interpreter, MMU, devices, storage, boot | Inherited | Retained | Evolved independently | `core/src/arm`, `core/src/soc`, inherited unit tests |
| UIKit/touch, SpringBoard composition | Inherited platform mechanisms | Own diagnostic applications and controlled captures | Also supported upstream | `tests/guest-apps/uikit.c`, gallery |
| MBX2D, sprite/quad transforms and filtering | Inherited measured renderer | No invention claimed | Further BGRA8, affine/perspective quad handling and TA continuations in current `mbx.c` | Compare `mbx_execute_textured_sprite`, `mbx_ta_parse_draw`, `mbx_ta_apply_draw` |
| Fixed-function diagnostic VGP envelopes | No standalone local pipeline in base | Six digest-gated normalized envelopes, matrix/vertex inputs, staged triangle rasterizer | No `mbx_vgp.c/.h` in inspected upstream; upstream TA parser uses `0x48020000` quad boundary, ours identifies `0x78020000` VGP-input diagnostic boundary | `core/src/soc/mbx_vgp.c`, `core/src/soc/mbx.c`, `core/tests/test_mbx_vgp.c` |
| Own RGBA8 8×8 upload, normalized UV nearest / bilinear | No local diagnostic family | Measured texture resource staging and exact state checks | Upstream already samples BGRA8 composition textures; algorithms such as bilinear filtering are not claimed as new | `mbx_vgp.c`, `tests/guest-apps/texture.c`, independent `reference_texture.py` |
| Snapshot | Base v32 | v33 stores TA FIFO; explicit v32 reconstruction | Upstream v33 instead adds USB DMA state; same number does **not** imply compatible format | `snapshot.h`, `snapshot.c`, `test_snapshot.c` |
| Rootfs DMG importer | Inherited | `w+b` staging fix permits reads from the temporary stream | No novelty claim without independent confirmation | `tools/fwimport.c` |
| HFS extraction | Inherited | Import-safe `main` guard | Generic utility adaptation | `tools/hfsx_extract.py` |
| CLCD/Z instrumentation | Existing host diagnostic infrastructure inherited | Bounded real scanout capture and read-only Z journaling | Source-specific local instrumentation, no upstream-fidelity claim | `tools/guest_clcd_capture.h`, `tools/guest_z_journal_capture.h`, bootkernel integration |
| Diagnostics, analytic oracles | Original local sources | SDK-free builder, six apps, narrow TA-input inspector, pixel oracles | These exact files absent at inspected upstream | `tests/guest-apps`, `tools/guest_apps` |
| Public compatibility interface | Created for IOSretroMob | Doctor/build/test/replay/inspect/gallery, original fixtures and negative checks | Product addition; not an engine rewrite | `iosretromob`, `lab` |
| Depth / Unity / Android runtime | No verified local implementation | Unresolved / unexecuted | No whole-project comparison claim about these broader capabilities | Compatibility and roadmap gates |

## Curated history

The new repository starts with a positive selection of MIT source files, then imports local modifications and diagnostic sources, then adds the public product interface, docs and curated captures. It deliberately does not import historical Git objects, work machines, firmware or private reports. The first baseline commit is a curated subset, not a full upstream mirror; remaining required build and application sources are completed in the integration commit. Original copyright headers and MIT license are preserved.

The retained Objective-C shell is an upstream reference implementation. The public macOS CMake build is validated; an IOSretroMob-branded iOS shell is not validated. No large-scale mechanical source rename was performed. The S5LBOX_* build flags and internal types remain stable for attribution and compatibility.

## Snapshot boundary

Only use a snapshot with the exact compatible IOSretroMob/S5LBox-local binary and its matching media/state components. Our v33 and upstream v33 are divergent formats, even though their version integers coincide. Neither this project nor its tests establish interoperability with current upstream v33. Cross-lineage snapshot detection/migration is a future explicit format-design task; until then, treat provenance as mandatory and reject unknown-lineage inputs operationally.

## Relic Maze and the CLCD laboratory (2026-10-10)

The original MIT game in `examples/RelicMaze` is an IOSretroMob contribution,
not S5LBox or Unity content. Its ARMv6 frontend reuses the existing SDK-free
runtime bridge and classic Mach-O writer; the inherited emulator executed the
actual UIKit/EAGL/ES1 application through a 25-move victory. The C core, browser
variant and unvalidated Unity 3.3 source prototype are kept separately.

New generic host additions are bounded 32-touch CLI sequences, read-only native
CLCD multi-window capture, format/hash inspection, independent synthetic tests
and an explicitly experimental compositor. They do not add a game-name special
case to emulated hardware or prove layer priority, depth, Unity support or FPS.
The historical S5LBox checkout and its MIT copyright remain unchanged by this
integration. OpeniBoot/iDroid research is cited in reports; no GPL implementation
or Apple code is imported. Historical reports retain their original dates and
claims; later guest evidence supersedes their earlier incomplete milestones.
