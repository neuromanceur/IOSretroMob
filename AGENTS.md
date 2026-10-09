# IOSretroMob maintenance and storage policy

Read README.md, UPSTREAM.md, COMPATIBILITY.md and reports/PUBLIC_REPOSITORY_AUDIT.md before changing compatibility claims. Preserve S5LBox author attribution and original MIT notices. Keep this repository independent of the historical S5LBox checkout; do not modify that checkout as part of product work.

Public sources are positively selected. Never commit firmware, SDKs, keys, Apple libraries, games, IPAs, disks, RAM snapshots, private shader streams or personal paths. Audit tracked files **and all Git objects** with tools/audit_public.py before proposing publication. Ignore rules are only an extra guard. Label synthetic replays explicitly and preserve authentic capture hashes. Do not substitute unit tests for guest or device evidence. Current upstream v33 and local v33 snapshots are not interchangeable.

No push, remote creation, release, remote tag or CI/GitHub Actions without explicit owner authorization. No external messages are authorized. Public validation runs locally with -j2.

Budget every campaign: log df -h, df -k, logical bytes and allocated blocks before producing guest media. Keep work below 10,000,000,000 logical bytes and at least 5 GiB free after worst-case allocation. Default new publication work budget is 1 GiB. Stop disk-producing work if the reserve or budget would be exceeded. One active fresh temporary disk by default; no machine copy or snapshot per experiment. Reuse compatible preserved references, never their mutable media.

Preserve small reports, hashes, own traces, captures and commands. New durable checkpoints require a reason, replacement/dependency plan and peak allocation. Cleanup requires explicit exact-path/hash approval, ancestor/symlink and open-handle checks, immediate stop on divergence, individual removals and per-file results. Never recursive VM deletion or wildcards. Never replace a file used by an active process or interrupt it to permit cleanup. Do not claim APFS clone savings without byte-exact validation of the actual provisioning path. Do not convert references in place.
