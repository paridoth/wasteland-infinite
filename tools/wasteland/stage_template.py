#!/usr/bin/env python3
"""Stage the pinned owned ARTEMPLE/ARCAVES copies in a NEW isolated runtime.

No map writer or renamed registry entries. Native ce-dat-tool extracts the two
maps; SHA-256 pins identify the tested asset version. No game bytes enter Git.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

if __package__:
    from . import stage_runtime
else:
    import stage_runtime

ROOT = stage_runtime.ROOT
RECIPE = ROOT / 'templates/artemple-copy.v1.json'
MARKER = 'WastelandInfinite art temple copy v1\n'


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(chunk)
    return result.hexdigest()


def stage(source: Path, destination: Path, dat_tool: Path):
    recipe = json.loads(RECIPE.read_text())
    source = source.resolve(strict=True)
    entries = {p.name.lower(): p for p in source.iterdir()}
    if not dat_tool.is_file():
        raise ValueError('build ce-dat-tool before staging a template')
    # Fail before copying if this is not the pinned, previously tested asset set.
    for name, expected in recipe['archives'].items():
        path = entries.get(name)
        if path is None or path.is_symlink() or not path.is_file() or digest(path) != expected:
            raise ValueError(f'unsupported asset version: {name}')
    if {name for name in entries if name.startswith('patch') and name.endswith('.dat')} != {'patch000.dat'}:
        raise ValueError('copied template requires only the pinned patch000.dat')
    # Extract from the original archive without writing anything to its directory.
    maps = {}
    for name, expected in recipe['maps'].items():
        result = subprocess.run([str(dat_tool), str(entries['master.dat']), 'cat', f'maps\\{name}'],
                                capture_output=True, check=True, timeout=30)
        content = result.stdout
        if len(content) != expected['bytes'] or hashlib.sha256(content).hexdigest() != expected['sha256']:
            raise ValueError(f'copied map does not match recipe: {name}')
        maps[name] = content
    destination = stage_runtime.stage(source, destination)
    # A partial template copy cannot pass the separate --wi-template-seed guard.
    references = destination / 'template-source'
    references.mkdir()
    for path in (destination / 'data').iterdir():
        if path.name.lower() == 'maps' and path.name != 'maps':
            raise ValueError('ambiguous map directory casing')
    map_dir = destination / 'data/maps'
    map_dir.mkdir(exist_ok=True)
    if any(p.name.lower() in maps for p in map_dir.iterdir()):
        raise ValueError('installation already overrides a pinned map')
    for name, content in maps.items():
        (references / name).write_bytes(content)
        (map_dir / name).write_bytes(content)
    subprocess.run([str(dat_tool), 'create', str(ROOT / 'files/ce.dat'), str(destination / 'ce.dat')],
                   check=True, timeout=30)
    (destination / 'template-provenance.json').write_text(json.dumps(recipe, indent=2) + '\n')
    config = destination / 'fallout2.cfg'
    config.write_text(config.read_text() + '\n[debug]\nmode=log\n')
    # Last: startup also requires exact equality with both staged reference maps.
    (destination / '.wasteland-template-copy').write_text(MARKER)
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('owned_installation', type=Path)
    parser.add_argument('--name', default='proof-template')
    args = parser.parse_args()
    if not args.name or Path(args.name).name != args.name or args.name in ('.', '..'):
        parser.error('--name must be a simple directory name')
    try:
        target = stage(args.owned_installation, stage_runtime.RUNTIME / args.name,
                       ROOT / 'out/build/wi-dev/ce-dat-tool')
        print(f'Copied template staged at {target}. Original installation was only read.')
        print('Launch wi-dev/fallout2-ce there with --wi-template-seed=42; select NEW GAME.')
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(2, f'Cannot stage copied template: {error}\n')


if __name__ == '__main__':
    main()
