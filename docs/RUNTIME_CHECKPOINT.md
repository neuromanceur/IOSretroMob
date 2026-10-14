# Runtime source checkpoint — 2026-10-14

This source checkpoint preserves the seven preceding IOSretroMob commits and
positively selected original development code. S5LBox by j0shua-SYSON remains
the MIT foundation; original notices and approved Relic Maze captures are intact.
No commercial application, player runtime, Apple firmware, cache, SDK, VM media,
private capture or private campaign report is distributed.

## Scope and limits

- Local snapshot v34 retains a 64K-word TA array, with a maximum consumed count
  of 65535. It reads the exact local v32/v33 layouts; this does not establish
  compatibility with upstream's divergent v33 or with unknown-lineage states.
- The measured TA path supports RGB565 targets and a bounded square Morton
  BGRA8 texture class. Unknown texture classes and state envelopes are refused.
  Transform specialization is not arbitrary VGP instruction emulation, general
  GLES conformance, validated depth testing or a claim of playable Unity games.
- The explicit IIS clock/FIFO development hypothesis is OFF by default. Its
  44100 Hz, 32-byte queue and stereo frame assumptions are not physical hardware
  validation. Active experimental snapshots are refused. Its audio export is
  an 8 MiB prefix with complete counters; a prefix is not the full recording.
- Runtime observers correlate returns with thread, address space, stack,
  instruction state and PC. Numeric address or PID alone is not application
  execution evidence. Diagnostic output may contain proprietary guest bytes
  and must stay private unless separately reviewed.
- Native FIFO/GART collectors have independent packet, word and byte ceilings.
  The GART callback occurs before draw parsing, not after acceptance. Early
  refusals and direct VGP shadow-FIFO execution can bypass it. Collectors still
  retain at most 32768 words per stream even though device retention is larger.
  A truncated capture is not a complete stream or a rendered frame.
- CLCD formats 3/7 with unknown channel order preserve raw bytes and registers;
  colour conversion is refused rather than inferred. Hardware composition is
  still unvalidated, and existing composites remain labelled hypotheses.
- Legacy Mach-O, dyld-cache and Unity metadata parsers are original source,
  not proprietary implementations. Their output and any extracted executable
  are private. Use only authorized local inputs. No decryption is performed.
- Bundle packaging streams large bodies with bounded file, archive and path
  budgets. It is not a general MobileInstallation implementation.

## Review corrections

Two synthetic observer-test calls now pass `&machine.mbx`, matching the MBX
function's declared type. The test compares render decision, status and all
307200 framebuffer bytes with and without the callback for its synthetic
unsupported stream. This proves that fixture's invariance, not every observer
or every guest input. API comments now describe the actual callback timing.
No game-name condition or new renderer behavior was added for this checkpoint.

## Validation and publication size exception

Validation is local and firmware-free: CMake Release, build with `-j2`, complete
CTest, synthetic public replays, immutable own-game evidence and metadata parser
fixtures. Optional firmware, signed static-A64 and LLVM cases may be skipped;
exact results are recorded in the publication report. No guest boot is authorized
or performed by this checkpoint. Relic Maze's documented historical victory is
preserved; it does not establish Unity gameplay or Android runtime acceptance.

The owner explicitly authorized a **one-time 32 MiB Git size exception** on
2026-10-14 for this isolated checkpoint. The ordinary AGENTS.md policy and audit
default remain 20 MiB. The exception changes only the size gate, not any content,
positive-path, secret, image or all-object history checks. Reproduce that audit:

```sh
python3 tools/audit_public.py --size-limit-mib 32
```

Do not infer permission for future publication or guest campaigns from that
option. The isolated object store contains only the seven historical public
commits and reviewed checkpoint commits, with no alternates, private checkpoint
refs or shared objects. The original private working checkout is preserved.
