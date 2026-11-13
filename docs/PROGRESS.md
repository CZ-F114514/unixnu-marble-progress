# Progress: real user code ran; normal services remain unvalidated

**English** | [简体中文](PROGRESS.zh.md) | [Home](../README.md)

Date: **2026-11-14**. Observations below belong to this UniXNU boot chain.
Original photos, recordings and device logs are kept private.

| Stage | Observed result and scope |
|---|---|
| Stock ABL → UEFI → UniXNU | Menu and Tools appeared; volume buttons and a short power-button press worked. This is more than Fastboot accepting the image. |
| XNU | On-screen panic and stage output were obtained; the earlier root-device failure was followed by the RAM-root experiments. |
| RAM APFS | Mount succeeded; original `expr`, `dyld` and `launchd` files could be read. |
| Original `expr` child | A custom PID 1 supervised original `expr` as a child; it exited with status 0 and 31 bytes were read back through the pipe. |
| Original `launchd` | Original code executed; its three standard-I/O initialization `dup2` calls returned normally. Full service startup was not demonstrated. |
| Latest 256-return probe | Intentional threshold stop: pre265/ret256, lost1. A complete, loss-free 256-return run is not accepted. |
| Normal SystemOS/Data, services and GUI | Unvalidated. The restore RAM root lacks the selected SpringBoard/backboardd resources. |
| Touch, GPU pixel readback, sustained interaction and multicore work | Unvalidated on this route. The current candidate uses `cpus=1`. |

## Latest 256-return observation

```text
n1333 / pre265 / ret256 / limit256 / stop7 / dup3 / lost1
errors64 / kept16 / over48
```

`stop7` is the observer's intentional stop after reaching its return budget.
Only the first 16 nonzero results were visible: two ENOMEM, thirteen ENOENT and
one ESRCH. Details of the remaining 48 errors were not displayed.

The newly visible `persona` syscall 494 returned ESRCH (3). Its operation and
complete arguments were not captured, so a fatal startup blocker has not been
established. The last `close_nocancel` syscall 399, on fd 4, returned zero from
its native C handler; that alone does not prove the higher-level flow completed.

FIRST belongs to the original libc `sysctlbyname` query path. The sampled MIB
was `[1,149]`, with length samples of 128 → 0. OID 149 is in the dynamic range;
its live name and the specific ENOMEM branch are unresolved. Path fields hold
only 20-character prefixes. The pre-ret difference of nine is not a thread count.

`lost1` records an unmatched ticket or generation-state check. Overwriting a
`pending[8]` slot is one possible mechanism. Without the failed sequence, TID
and reason, the cause cannot be identified or claimed fixed.

## Standard I/O

Using the original program directly as diagnostic PID 1 previously produced
EBADF after exec. Lifecycle probes showed that descriptors were still present
at the exec-preservation stage, followed by closes on a later user-code path.
This did not establish CLOEXEC or unexplained descriptor loss as the cause.

A custom PID 1 supervising original `expr` as a child produced a normal exit
and actual output. That limited success does not validate original `launchd`
service startup.

## Carrier and logging boundaries

The latest frozen image uses an Android v4 header with a zero ramdisk field,
and a RAM carrier containing BootShim, a 3 MiB FD, UXRAM02 and a 192 MiB FAT
payload. Its SHA-256 is:

```text
9fde85ff4d2221cda0e7a0d36bff29e252aec10fcb01c5acea897c61f8aa1c1a
```

The image is not distributed. This hash identifies the historical experiment;
it does not validate a newly generated image from the public patches.

Physical UFS, DMA, partition writes and the physical log backend are disabled
in the current candidate. Its actual `PANIC_TEXT=0`: automatic disk persistence
or RAM-text backfill of panic output has not been demonstrated. HakiOS GPU,
touch and multicore results remain separate.
