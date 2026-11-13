# Minimal device test protocol

**English** | [简体中文](DEVICE-TEST.zh.md) | [Home](../README.md)

Use this protocol only with a candidate that has been separately reviewed.
Nothing in this document operates a device automatically. Read the RAM-carrier
integration gaps in [REPRODUCE.md](REPRODUCE.md) first: this repository does not
directly supply a complete boot image.

For every attempt, the operator must physically return to the Xiaomi stock
FASTBOOT graphic screen. A temporary U-Boot Fastboot endpoint is not a substitute
for stock ABL Fastboot.

## Before booting

1. Obtain exclusive use of the phone; no other agent or project may access it
   concurrently.
2. Confirm `marble`, stock ABL Fastboot and non-userspace mode. Record the
   current slot; do not assume its value or change it.
3. On the computer, retain the full candidate SHA-256, source revisions,
   compiler, config, arguments and package-check results. Use a fresh log
   directory for the attempt.

Query only this basic information, then temporarily boot a reviewed candidate
once:

```sh
fastboot getvar product
fastboot getvar current-slot
fastboot getvar is-userspace
# Only after the operator confirms the stock FASTBOOT screen and matching identity:
fastboot boot /path/to/reviewed-candidate.img
```

Do not use `su` or `adb root`, flash or erase partitions, switch slots, change
security state, or read/write phone partitions. Do not follow upstream fixed-LBA,
UFS-root-partition or persistent-boot installation procedures. Preserve physical
access to stock Fastboot recovery; do not rely on EDL.

## Separate acceptance stages

| Stage | Required execution evidence |
|---|---|
| Menu | The menu appears, volume/power buttons work, and Tools can be entered. |
| XNU | Kernel output or a locatable panic belonging to this image; a loader handoff message alone is insufficient. |
| RAM root | APFS mounts and the target files are read successfully. |
| User program | Actual execution, syscall/exit/output evidence; file presence is insufficient. |
| `launchd` | Validate bootstrap, child services/Mach/XPC and sustained operation separately. |
| GUI/touch | SpringBoard remains responsive; Settings can actually be opened and scrolled. |
| GPU | Commands execute and pixel readback is correct; sustained desktop interaction is a separate test. |
| Multicore | Actual parallel work; a `cpus` argument or reported CPU count is insufficient. |

`examples/unixnu.toml` is a reference to the original experiment's parameters.
Its kernel, RAM root and EFI tools are not distributed. Begin with menu/Tools,
then the 64-return probe. Only after that passes should a separate attempt run
the 256-return probe. `stop7` is the intentional return-budget stop, not an error
root cause. Record at least `lost`, `errors/kept/over`, FIRST and copy status.

Capture a stable screen before resetting. Fastboot `OKAY` alone is not execution
success. Keep raw logs on the operator's computer. Public issues should contain
only a redacted minimal summary, not serials, GPT data, system resources or
original recordings.
