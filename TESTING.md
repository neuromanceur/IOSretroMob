# Tests and reproducible public graphics

Run `./iosretromob test` from a clean clone. The inherited CMake tests, local MBX/snapshot tests and new public lab are separate CTest entries. The final [publication audit](reports/PUBLIC_REPOSITORY_AUDIT.md) and [validation logs](reports/validation) contain actual outcomes, including skipped firmware-dependent subcases. No IPSW, kernel, DeviceTree, rootfs, snapshot or Apple SDK is present in the clean validation checkout.

## Synthetic semantic replays

```sh
./iosretromob replay lab/fixtures/triangle.json --output .local/triangle
./iosretromob replay lab/fixtures/triangle-90.json --output .local/triangle-90
./iosretromob replay lab/fixtures/textured-triangle.json --output .local/texture-triangle
./iosretromob replay lab/fixtures/square-nearest.json --output .local/nearest
./iosretromob replay lab/fixtures/square-linear.json --output .local/linear
```

Each fresh output directory contains `synthetic.ppm` and `result.json`. The fixture describes our own decoded matrix, viewport, vertices, colour/UV and background. It does not contain an Apple-generated program. The C replay links the same emucore semantic rasterizer. Independent Python inversion compares the complete scanout, reports coverage, exact-boundary exclusions, mismatches and maximum channel error. Pixel-write counts include clear/overdraw and are not frame counts.

`iosretromob_public_replays` additionally checks refusal of malformed/nonfinite/truncated inputs, no overwrite of existing outputs, and detection of a corrupted pixel. `core/tests/test_mbx_vgp.c` checks geometry, clipping, samplers, alpha semantics and atomic rejection. Accepted proprietary-envelope replay remains a separate private validation and is not replaced by a synthetic pass.

## TA-input inspection

```sh
./iosretromob inspect lab/fixtures/ta-input.json
```

This original synthetic control-layout fixture contains no driver shader. The bounded inspector locates a single three-vertex input boundary, finite uniforms/inputs and valid colours; ambiguity and truncation are rejected. It establishes neither program semantics nor pixels.

## Historical capture verification

```sh
./iosretromob verify-gallery
```

Checks SHA-256 and dimensions of all six original PNGs, decodes RGB8 PNG with CRC checks, confirms pixel identity to all five retained PPMs, and rechecks three texture scanouts against safe own-input metadata. UIKit's original PPM was not selected; its retained PNG's decoded RGB hash is verified against the original capture record. Prior triangle analytic checks are recorded in the gallery manifest; they are historical results, not new guest executions.

`iosretromob_diagnostic_tools` runs TA-layout and safe-package tests. Its optional Mach-O builder class reports a skip if a complete LLVM toolchain is absent. Set `IOSRETROMOB_LLVM_BIN` to run its two ABI/envelope tests across eight original builds. Do not count a skip as a runtime success.
