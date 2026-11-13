# Publication scope and redaction

**English** | [简体中文](PUBLICATION.zh.md) | [Home](../README.md)

This public repository has its own Git history. It does not inherit commits,
identities or old backups from the development computer's original repository.

Included: local patches, generic host tests, pinned upstream references,
parameter examples, progress and acceptance criteria. Dependencies remain in
their own projects. Retaining their copyright notices is not vendoring their
implementations.

Excluded: Apple images/caches/trust data/frameworks; FD/FAT/boot candidates;
compilers and SDKs; partition tables/backups; local GPT/resource-manifest/marker
byte arrays; local device serials or unique partition identifiers; photos,
recordings, QR payloads and raw logs; personal workspace paths and credentials;
and historical TWRP/partition-maintenance tools.

Mach-O UUIDs matching public Apple program versions, generic UEFI protocol
GUIDs and register constants are not device serials. These implementation
constants are retained rather than blindly replaced. Test addresses, PIDs and
QR sessions are synthetic fixtures.

Unified patches retain deleted public upstream context, including a pinned
board's volume-match UUID, manifest hash and storage geometry. Those bytes come
from that public revision, not from newly recorded local GPT/device data.
Retaining deletion lines allows exact replay against the pinned source. Do not
apply that geometry or identity to arbitrary phones.

The initial public export used a neutral Git author. Publication uses only this
separate, redacted repository at
[CZ-F114514/unixnu-marble-progress](https://github.com/CZ-F114514/unixnu-marble-progress).
Original local development files remain intact.

The SHA manifest checks published file bytes. It does not replace security
review or hardware validation. New files need review too: `.gitignore` alone
does not establish that content has been redacted.
