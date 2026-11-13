# Reproduction guide

**English** | [简体中文](REPRODUCE.zh.md) | [Home](../README.md)

## 1. Self-contained checks without a phone

Run from this repository's root on Linux/WSL. Requirements are Bash, Python 3
and Clang with ASan/UBSan support. The recorded host compiler was Clang 21.
These checks need no Apple files, root privileges, phone or network connection.

```sh
bash tools/run-host-checks.sh
python3 tools/verify_manifest.py
```

Results go into a fresh `build/host.*` directory. Existing records are not
overwritten. Observer tests use synthetic processes, addresses and strings to
exercise error visibility, sysctl argument bounds, copy faults, one-use tickets,
generation changes, overflow, 64/256 thresholds and copies outside the spinlock.
The four QR tests also use synthetic frames only.

Host success does not validate ARM64 hardware execution, the complete kernel
stack or services. The target compiler, Clang 24, and the host-test compiler,
Clang 21, are recorded separately.

## 2. Fetch upstream sources and apply the local changes

URLs and full revisions are pinned in [dependencies.json](../dependencies.json).
Do not substitute the repositories' default branches. The following commands
use a new directory on the computer and do not access a phone:

```sh
PUBLIC_REPO="$PWD"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/unixnu-repro.XXXXXX")"
git clone https://github.com/UniXNU/UniXNU "$WORK/UniXNU"
git -C "$WORK/UniXNU" checkout --detach 04b4e558bcded5213c2d802984862dc062f28257
git clone https://github.com/MiLacy/UniXNU-marble "$WORK/UniXNU-marble"
git -C "$WORK/UniXNU-marble" checkout --detach 78f4e7bf00e88129f8b6ef5680d81770d386832d

git -C "$WORK/UniXNU" apply --check "$PUBLIC_REPO/patches/unixnu-host-menu-overlay.patch"
git -C "$WORK/UniXNU" apply "$PUBLIC_REPO/patches/unixnu-host-menu-overlay.patch"
git -C "$WORK/UniXNU-marble" apply --check "$PUBLIC_REPO/patches/board-runtime-overlay.patch"
git -C "$WORK/UniXNU-marble" apply "$PUBLIC_REPO/patches/board-runtime-overlay.patch"
python3 "$PUBLIC_REPO/tools/check_overlays.py" --unixnu "$WORK/UniXNU" --board "$WORK/UniXNU-marble"
```

`check_overlays.py` compares every changed output file with its expected SHA.
A successful patch command alone is insufficient. The 27 file changes were
independently replayed against preserved pinned sources; remote download
availability was not rechecked during that export.

The board overlay includes the ordered RAM-root, expr/stdio, original-launchd
and error-observer changes. Do not apply only the final error patch to pristine
XNU. `examples/xnu-series` records device-level ordering; upstream still resolves
the arch/soc/os series in their own order. Do not manually reapply the nested
patches to a UniXNU build tree after the resolver has applied them.

## 3. Toolchain and recorded kernel build commands

Follow the Linux toolchain instructions at the pinned UniXNU revision.
Upstream scripts, dependency source and installation instructions remain in
their own repositories. Tool versions are in `dependencies.json`. Linux system
headers are not replacements for Darwin target headers. Keep host BSD
headers/libraries separate from target search paths.

Select your own toolchain directory and clear inherited host search paths from
the target environment:

```sh
export UNIXNU_TOOLCHAIN="/path/to/your/unixnu-toolchain"
export PATH="$UNIXNU_TOOLCHAIN/shims:$UNIXNU_TOOLCHAIN/bin:$PATH"
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH
```

Supply your own matched 21H461 kernel cache, trust data and restore resources.
Do not extract them from phone partitions or upload them publicly. Set the real
local decoded-cache path and run in the same shell where `WORK` was created:

```sh
KERNELCACHE="/path/to/private/21H461/kernelcache.decompressed"
test -f "$KERNELCACHE"
cd "$WORK/UniXNU"
./unixnu defconfig marble-ram-apfs_defconfig -D "$WORK/UniXNU-marble"
./unixnu status --json -D "$WORK/UniXNU-marble" > "$WORK/resolved-status.json"
./unixnu tree -D "$WORK/UniXNU-marble"
./unixnu build kernel -D "$WORK/UniXNU-marble" --kernelcache "$KERNELCACHE" -j 4
```

These are the driver-command forms used in the existing build. **They have not
been rerun from a completely clean environment downloading only public
dependencies.** Prerequisites include the dependency trees, archive/PMGR prelink
stages, host toolchain libraries, decoded restore resources and APFS matching.

If a stage requires a previous matching prelink, retain the first-pass kernel
and complete the second pass according to the upstream PMGR bridge requirements.
Replacing it with a no-op bridge does not establish success. Save each pass's
config, resolved status, command, log, compiler version and SHA.

Physical UFS, owner-storage and virtio-storage options must remain disabled.
Check the actual final `.config`, not just the sample or defconfig. Verify GIC
redistributor stride/region against the board DTS and hardware evidence rather
than copying another SoC's values.

## 4. Custom RAM-carrier integration remains unvalidated as a public cold build

The two overlays are neither a complete firmware patch set nor a boot image.
Matched integration is still required for the firmware RAM filesystem driver,
pre-menu APRIORI order, BootShim/DT, kernel-entry/panic visualization, ADT,
trust cache and restore RAM volume. Complete Mu sources, Apple resources and
existing FD/FAT images are not copied into this repository.

Use the pinned Mu-Silicium source and prepare/review its RAM-carrier changes
before using the upstream firmware build entry point. Do not test an unreviewed
generic `bootimg` or old v0 output on a phone. In particular, the old v0 fields
retained in the board are historical configuration, **not a validated v4
packaging recipe**.

The latest actual carrier uses an Android v4 header with a 4096-byte header
page and `ramdisk=0`. Its kernel field holds:

```text
gzip(BootShim[144 B] + FD[0x300000] + UXRAM02 header[64 B] + FAT[192 MiB]) + DTB
```

UXRAM02 includes payload length, payload CRC and header CRC. The firmware's
expected CRC must be updated consistently. Check reserved memory, load
addresses, exception level, MMIO mappings, DT `/chosen` and framebuffer stride
separately. File extensions and Fastboot responses are insufficient.

Before a device attempt, at minimum verify unpacked bytes, the DTB trailing the
compressed stream, actual FFS/APRIORI/RAM drivers, CRC consistency, exclusion
of physical storage providers and source revisions. Only then follow the
[device test protocol](DEVICE-TEST.md). This repository does not claim to offer
a one-command reconstruction of the complete boot image.
