# UniXNU on marble: bring-up progress and reproducibility

**English** | [简体中文](README.zh-CN.md)

Experimental UniXNU bring-up for the **Redmi Note 12 Turbo / POCO F5**:
marble, Qualcomm SM7475, Adreno 725.

As of **2026-11-14**, the stock Fastboot temporary-boot route has produced a
working UniXNU menu, XNU on-screen output, a mounted RAM APFS root, and execution
of original Apple user-space binaries. Original `launchd` bootstrap is still
under investigation. **Normal iOS services, SpringBoard, touch, GPU acceleration
and multicore execution have not passed validation on this UniXNU route.**

This repository shares progress, reviewable changes and reproduction steps.
Upstream dependencies are [referenced at fixed revisions](dependencies.json),
not vendored. It contains local patches, a host-side observer test harness,
an offline QR decoder and documentation. Apple resources and prebuilt boot
images are not distributed here.

## Start here

| Guide | What it covers |
|---|---|
| [Progress and evidence](docs/PROGRESS.md) | What ran, what failed, and what remains unverified |
| [Reproduction](docs/REPRODUCE.md) | Host checks, pinned dependencies, patch application and build prerequisites |
| [Device test protocol](docs/DEVICE-TEST.md) | Stock Fastboot temporary boot and separate acceptance stages |
| [Next work](docs/NEXT.md) | Trace completeness, carrier integration, services, display and input |
| [Publication scope](docs/PUBLICATION.md) | Redaction boundaries and upstream patch context |
| [Contributing](CONTRIBUTING.md) | Small patches, useful issue reports and documentation updates |

The corresponding Chinese guides are linked from each English page and the
[Chinese README](README.zh-CN.md).

## Run the self-contained host checks

On Linux or WSL, with Git, Bash, Python 3 and an ASan/UBSan-capable Clang:

```sh
git clone https://github.com/CZ-F114514/unixnu-marble-progress.git
cd unixnu-marble-progress
bash tools/run-host-checks.sh
python3 tools/verify_manifest.py
```

Once cloned and with those tools installed, these checks need neither Apple
resources nor a phone or network connection. They exercise synthetic inputs;
passing them is not hardware validation.

## Current reproduction boundary

The host observer regression and the exported patch replays have been checked.
**A complete cold rebuild of the custom v4 RAM boot carrier from this public
snapshot has not been validated.** The carrier also needs firmware RAM drivers,
APRIORI ordering, a visible kernel and matched CRC bindings. The reproduction
guide separates self-contained checks, recorded build commands and missing
integration work. A generic upstream `bootimg` output is not the frozen carrier
used in the reported device experiments.

The active system target is **iPadOS 17.7.11 / 21H461**. iOS/iPadOS 27 PAC and
4 KiB page-table research is separate. HakiOS / 20F66 results do not count as
UniXNU validation.

## Repository layout

```text
docs/           Progress, reproduction steps and acceptance boundaries in English and Chinese
patches/        Two overlays against pinned upstream sources, with per-file hashes
examples/       Boot arguments, storage-disabled configuration and patch ordering
host-tests/     Current bootstrap observer and synthetic QR tests
tools/          Offline checks, QR decoding and release-file integrity tools
LICENSES/       Upstream notices relevant to the patches
```

Original notices and licenses are retained; see [NOTICE.md](NOTICE.md).
Dependencies remain maintained by their upstream projects.
