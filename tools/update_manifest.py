"""Refresh release hashes from reviewed Git paths. No network or device access."""
from pathlib import Path
import hashlib
import json
import subprocess

root = Path(__file__).resolve().parents[1]
paths = subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z']).decode('utf-8').split('\0')
records = []
for name in sorted(set(paths) - {'', 'repo-files.json'}):
    candidate = root / name
    path = candidate.resolve(strict=True)
    if candidate.is_symlink() or not path.is_relative_to(root):
        raise SystemExit('Invalid release path: ' + name)
    data = path.read_bytes()
    records.append(dict(path=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
destination = root / 'repo-files.json'
destination.write_text(json.dumps(dict(schema=1, files=records), indent=2) + '\n', encoding='utf-8')
print(f'Updated manifest for {len(records)} files; inspect and stage repo-files.json.')
