# Next work

**English** | [简体中文](NEXT.zh.md) | [Home](../README.md)

1. **Complete the observer evidence.** Record failed tickets, threads and
   reasons; distinguish generation mismatch, slot overwrite and duplicate
   returns. Retain later nonzero results and complete bounded paths instead of
   only increasing the budget. Resolve dynamic OID names/handlers and persona
   operations while preserving native failure semantics; do not fake success.
2. **Make the carrier reproducible.** Turn the remaining local firmware RAM
   drivers, APRIORI, DT/BootShim and CRC bindings into separate changes without
   private resources, then rebuild in a clean environment. The current public
   overlays cover only menu/host/board kernel-diagnostic changes.
3. **Establish a normal service environment.** The current 176 MiB restore root
   has few service plists and cannot supply a normal SpringBoard/backboardd
   environment. Integrate matched SystemOS resources and RAM-writable
   Data/var/devfs, then validate original `launchd` child services, Mach/XPC
   bootstrap, signals and thread behavior individually.
4. **Display, input and GPU.** Once services are stable, check display handoff,
   IOSurface/mmap, caches, fences and completion callbacks. A different kernel
   cannot be assumed to solve HakiOS interaction failures. Do not copy A740 GMEM
   settings to A725 without evidence.
5. **Validate the 27 route separately.** Per-core PAC, cross-core signature
   recognition, context switching and execution with actual 4 KiB page tables
   each require runtime evidence. `pac-codegen.S` is only an offline codegen
   example; it proves none of these capabilities or a successful 27 boot.

Every public progress update should state source/dependency versions, commands,
artifact SHA, arguments, actual observations, counterexamples and unverified
items. A build, static desktop or single triangle is not full userland acceptance.
