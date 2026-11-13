"""Read-only SHA check of the 27 exported source changes. No device access."""
from pathlib import Path
import argparse, hashlib, json

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--unixnu', type=Path, required=True)
parser.add_argument('--board', type=Path, required=True)
args = parser.parse_args()
manifest = json.loads((Path(__file__).resolve().parents[1] / 'patches/overlay-inputs.json').read_text())
count = 0
for key, root in [('unixnu', args.unixnu), ('board', args.board)]:
    root = root.resolve(strict=True)
    for record in manifest[key]:
        path = (root / record['path']).resolve(strict=True)
        if not path.is_relative_to(root):
            raise SystemExit('Outside source root: ' + record['path'])
        if hashlib.sha256(path.read_bytes()).hexdigest() != record['after']:
            raise SystemExit('Source SHA mismatch: ' + record['path'])
        count += 1
print(f'OVERLAY_FILES_SHA256=PASS ({count} files)')
