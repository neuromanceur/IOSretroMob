# Public repository audit — IOSretroMob

Audit date: 2026-10-09. **Local preparation and validation passed; publication remains owner-gated.** No remote has been added to the product repository, no push or repository creation occurred, and no CI/GitHub Actions or cloud build was run.

## Independence and provenance

IOSretroMob has its own directory and Git history. The historical S5LBox checkout was on `codex/unity-compatibility`, HEAD `6f203ba550b49afadee008c7eb55373a838eed33`, with no subsequent local commits. Nine tracked modifications and 25 selected original untracked sources were integrated instead of discarding the working tree.

The curated baseline commit is `28368367d024ad7029069601c9675718a69830e1`; the local engine/diagnostic integration is `c1fdac6b1f689dcd99be84b925bd12100cc8e0d8`; the public-lab/gallery source revision validated from a clean clone is `b5892491d8e5b32e28256e58bf379bf31f5f95e4`. A final local commit adds this report, measured logs and the audit-tool path/image safeguards. Tested engine, fixture, CLI, gallery and diagnostic-builder sources are unchanged by that evidence commit. Exact final HEAD is available from `git log -1`.

Upstream was inspected separately at `35bd00220d5bb0d1239bd04e0e4a7ac1eb1f784f`. No merge or historical-repository fetch/write was performed. [SOURCE_LINEAGE.json](SOURCE_LINEAGE.json) contains 330 source/document records, baseline/current-upstream hashes and both comparisons, with 118 selected upstream-derived paths differing from the historical base at current upstream. 304 records retain baseline ancestry (including locally edited inherited files), one records the local CMake build adaptation, and 25 are original local sources. Product additions appear separately in the new Git history. This is positive source selection, not a copy of the old repository or its Git objects.

[UPSTREAM.md](../UPSTREAM.md) is the feature matrix. ARMv6, XNU boot, SpringBoard/touch and MBX composition remain S5LBox work. Current upstream already has transforms, filtered BGRA8 quads, additional TA handling and app/USB progress. IOSretroMob's distinct additions are the narrow VGP-input diagnostic family, own RGBA8/UV path, read-only capture/Z instrumentation, original apps/oracles and public compatibility lab. A common algorithm such as bilinear filtering is not claimed as an invention.

**Snapshot caveat:** our v33 stores the TA FIFO; current upstream v33 stores a different USB state. Their version numbers do not establish format compatibility. No snapshots are distributed and unknown-lineage states must not be mixed.

## Build and tests

Clean local clone of `b5892491d8e5b32e28256e58bf379bf31f5f95e4`; macOS arm64, AppleClang 21.0.0.21000334, Release defaults, `-j2`. No Apple firmware, SDK, IPSW, kernel, DeviceTree, rootfs or VM checkpoint was present in that clone.

| Check | Measured result | Evidence |
|---|---|---|
| Configure and desktop build | PASS | [clean-build-test.log](validation/clean-build-test.log) |
| CTest | **71/71 entries passed**, 21.12 s test wall time | [detailed test outputs](validation/clean-ctest-detail.log) |
| Original public replay lab | Four test methods, five fixtures; invalid/truncated/nonfinite input refusal, no overwrite, corrupt-pixel detection | CTest `iosretromob_public_replays`, `lab/test_lab.py` |
| CLI smoke checks | Eight commands passed: doctor, inspector, five replays, gallery | [cli-checks.json](validation/cli-checks.json) |
| Independent synthetic images | Zero pixels outside tolerance 1 on all five fixtures; linear maximum channel error 1, others 0 | Same CLI results |
| Gallery | Six exact PNG hashes and decoded RGB hashes; five PPM/PNG pixel identities; three texture scanouts independently rechecked, zero mismatches and max error 0 | CTest `iosretromob_gallery` and CLI results |
| SDK-free diagnostics with explicit local LLVM | **8/8 Python tests passed**, including both builder checks across eight app/variant builds | [llvm-diagnostics.log](validation/llvm-diagnostics.log) |
| Firmware-dependent boot | Not executed in this mission | Historical own captures, separately classified |
| Android / Unity runtime | Not executed or newly validated | [COMPATIBILITY.md](../COMPATIBILITY.md) |

CTest entry passes are not proof that every optional subcase ran. Detailed output explicitly retains skips for absent firmware, private app checkpoints/packages and optional signed-static A64 handlers. The standard diagnostic-tools entry passes its available tests and reports one skipped builder class when a complete LLVM is not selected. The separate explicit-LLVM run executes that class: all eight Python tests pass. `host_egress` passed with local loopback sockets permitted; no external service was needed.

The first product run failed one new replay-reference adaptation (viewport offset); its actual [initial log](validation/initial-test.log) is preserved. The corrected lab passed its [targeted rerun](validation/lab-retest.log) and then the complete clean-clone suite. Initial linker warnings concern duplicate inherited static libraries; they did not fail compilation. Logs redact only local machine paths; results, failures and skips remain intact.

## Authentic demonstrations

Six retained originals: UIKit blue no-touch control; RMGLES1 190° and 330° triangle poses; RGBA8 textured triangle; nearest square; linear-filtered 270° square. [Gallery manifest](../demos/gallery/manifest.json) includes SHA-256, dimensions, pixel hash, original relative artifact/log names and log hashes. Two triangle records also retain the historical engine binary identity and prior independent reference result. Own numeric texture inputs are provided without driver program bytes.

Every image was visually inspected. These are authentic historical CLCD scanouts, not new guest executions, reference-rendered stand-ins or generated images. The UIKit status bar is incidental and disclosed. No Apple logo, commercial artwork or game asset is present. No video, sustained FPS or general GLES conformance is claimed.

## Licenses and sensitive-material review

Original S5LBox MIT LICENSE remains byte-exact, SHA-256 `f66cf6e71571d392f9776ad0b1fff36857d16385be6af97b570b5566b82e9a54`. Original copyright headers are retained. Added diagnostics/resources/lab are original MIT work. [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) preserves attribution and exclusions.

No SHMUP/GPL source, Unity runtime, Temple Trouble resources, commercial IPA, Apple SDK, framework, firmware, decryption key, guest disk or RAM state was selected. Source parsers under `core/src/firmware` are MIT program text, not firmware payloads. Public AES unit-test vectors, measured register constants, code-patch definitions and digest/firmware identity hashes are documented technical/test data, not private keys or copied libraries.

[PUBLIC_FILE_ALLOWLIST.json](PUBLIC_FILE_ALLOWLIST.json) is the exact positive publication selection. `.gitignore` is an additional guard only. `tools/audit_public.py` examines every tracked file, each historical tree, commit metadata and **every object returned by Git's all-object enumeration**, including unreachable blobs. It checks admitted paths, file modes, UTF-8/text versus approved image bytes, forbidden extensions/magic, size, secret/personal-path patterns, approved image SHA-256 and README links. An initial false positive on the MIT parser-directory name was corrected; binary payload exclusions remain intact. The final full scan is retained in [public-object-audit.json](validation/public-object-audit.json); rerun locally with:

```sh
python3 tools/audit_public.py
```

This mechanical screen is combined with provenance and license review. It is not a rights guarantee for arbitrary future additions. No old private Git object was transferred. Superseded new-repository objects containing the former machine-specific LLVM path were pruned before public-source preparation; the final all-object check verifies the resulting history.

## Storage and historical integrity

Measured product including local build: 43,130,302 logical bytes; allocated block sum 48,402,432 bytes. Product Git directory: 3,370,640 logical bytes, 3,612,672 allocated-block bytes. Product, clean validation checkout, separate upstream audit clone and private audit evidence together: approximately **88.6 MiB**, well below the 1 GiB mission budget. These measurements precede the small final evidence commit, whose measured change is also audited. No historical work machine was copied. See [storage.json](validation/storage.json) for df/du and the allocation caveat.

Allocated block sums are not exclusive APFS physical usage. No copy-on-write saving is claimed and host df fluctuations are not attributed to this mission. More than 5 GiB free reserve remained available. No historical file was removed or modified.

**Historical integrity PASS:** all **15,263** files were inventoried and compared by SHA-256, size/mode or symlink target, including `.git`, sources, firmware, checkpoints, reports, patches, captures and builds. **Zero changed, added or removed paths.** [historical-integrity.json](validation/historical-integrity.json) records the result. Raw private inventories remain outside the public repository.

## Readiness and remaining scope

Local publication preparation is complete: separate repository, preserved engine adaptations, attributed origin, English/French docs, original diagnostics, working CLI, reproducible clean build/tests, authentic gallery and explicit compatibility limitations. No engine/diagnostic test failure remains. The retained upstream Objective-C shell is reference source and has not been built or branded as a shipping IOSretroMob host app.

Depth, arbitrary VGP programs, guest blending/dithering, historical Unity execution, cross-lineage snapshot migration and physical Android acceptance remain research milestones rather than publication-blocking advertised features. No APK or validated game is announced. Storage policy and publication guardrails are operational in the new AGENTS.md.

**Remaining action before first publication:** owner review and explicit authorization of the GitHub destination and upload. No repository URL is invented. No push, CI, GitHub Actions, release or external message is authorized by this report.
