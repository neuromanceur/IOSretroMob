# Private firmware-dependent validation

Apple components are not supplied. Obtain an authorized local iPhone 3G/iPhone OS 3.1.3 (7E18) copy yourself. The inherited [BOOT_CHAIN.md](docs/BOOT_CHAIN.md) documents inspection, local decryption/extraction, identity hashes and direct-boot requirements. Hashes are public identity checks, not decryption keys.

Use local ignored paths for `kernel.macho`, `devicetree.bin` and `rootfs.img`. Never record keys in Git, public reports or shell transcripts. Public tests/replays need none of these files. The host `bootkernel --print-config` can inspect options without executing a kernel; it is not boot evidence.

Before any guest campaign, budget logical and allocated storage, keep 5 GiB free after worst-case allocation, and use one fresh temporary work disk. Never mutate a canonical image or a retained reference. A snapshot and its `.mdimage`/`.mdstate` must belong to the same lineage and compatible binary. Current upstream v33 and our local v33 have different layouts: never interchange them.

Keep small own journals, numeric diagnostics and authentic captures. A durable checkpoint requires an explicit need and bounded allocation. Any cleanup requires approval of exact paths and hashes; verify ancestors without following symlinks, open handles and preserved references, and remove individual approved files only. This public project ships no VM state.

Recreating a guest diagnostic needs the builder output, private firmware preparation and local installation policy. The public gallery is historical evidence, not a turnkey firmware-free guest boot. Do not source commercial games, historical IPAs, Unity binaries or third-party assets from this repository.
