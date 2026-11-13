"""Check release files without reading private inputs, Git history or devices."""
from pathlib import Path
import hashlib, json

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'repo-files.json').read_text())
total = 0
for item in manifest['files']:
    candidate = root / item['path']
    path = candidate.resolve(strict=True)
    if not path.is_relative_to(root) or candidate.is_symlink():
        raise SystemExit('Invalid release path: ' + item['path'])
    data = path.read_bytes()
    if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest() != item['sha256']:
        raise SystemExit('Release SHA mismatch: ' + item['path'])
    total += len(data)
print(f'RELEASE_FILES_SHA256=PASS ({len(manifest["files"])} files, {total} bytes; manifest excluded)')
