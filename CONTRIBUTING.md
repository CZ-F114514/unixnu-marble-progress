# Contributing

[English home](README.md) | [中文首页](README.zh-CN.md)

Small, evidence-backed contributions are welcome. Start with
[current progress](docs/PROGRESS.md) and [next work](docs/NEXT.md).

## Useful issue reports

Write in English when possible so more contributors can follow the discussion.
Include:

- The exact source/dependency revisions and candidate SHA-256.
- Relevant build commands, configuration and boot arguments.
- Which acceptance stage was attempted and what actually happened.
- A short redacted error summary and the expected behavior.
- Which observations belong to this UniXNU route, and which come from another
  project or upstream report.

Keep Apple resources, device serials, GPT/backups, credentials and raw recordings
private. Summarize only the evidence needed to understand the issue.

## Patch scope and checks

Keep dependencies upstream and submit focused changes. Separate documentation,
observer behavior and hardware/ABI changes when they have different validation
requirements. Read the [reproduction boundary](docs/REPRODUCE.md) before making
bootability claims. A successful host test is not a successful phone test.

For code changes, run the relevant self-contained host checks:

```sh
bash tools/run-host-checks.sh
```

For documentation changes, keep English and Chinese versions aligned. Preserve
commands, version pins, error counters and all unverified-result qualifications.
Check relative links in both language versions.

After reviewing and staging only the intended files, refresh the release
manifest and stage its update:

```sh
python3 tools/update_manifest.py
python3 tools/verify_manifest.py
git add repo-files.json
```

The updater uses tracked/staged paths, not arbitrary untracked files. Review
the staged contents yourself; the manifest is an integrity check, not a privacy
or safety approval.

Any phone experiment requires a separate reviewed candidate and the
[device protocol](docs/DEVICE-TEST.md). Do not run phone commands as part of a
documentation or host-test contribution.
