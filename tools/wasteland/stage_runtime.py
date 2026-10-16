#!/usr/bin/env python3
"""Copy owned vanilla assets into a NEW, ignored project-local runtime.

Reads the installation; never copies its saves or configuration. Uses real copies,
not links. No downloads. Existing destination directories are always refused.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / 'runtime'
CONFIG = '''[system]
master_dat=master.dat
critter_dat=critter.dat
master_patches=data
critter_patches=data
language=english
[screen]
windowed=1
resolution_x=1024
resolution_y=768
[ui]
skip_opening_movies=1
'''


def exclude_runtime_files(directory, names):
    return [name for name in names if name.lower() == 'savegame'
            or name.lower().endswith(('.sav', '.bak'))]


def stage(source: Path, destination: Path):
    source = source.resolve(strict=True)
    destination = destination.resolve()
    runtime_root = RUNTIME.resolve()
    if not destination.is_relative_to(runtime_root) or destination == runtime_root:
        raise ValueError('destination must be a new child of this project runtime directory')
    if destination.exists() or source == destination or source.is_relative_to(destination) or destination.is_relative_to(source):
        raise ValueError('source/destination overlap or destination already exists')
    entries = {item.name.lower(): item for item in source.iterdir()}
    if not all(entries.get(name, Path('/nonexistent')).is_file() for name in ('master.dat', 'critter.dat')):
        raise ValueError('owned master.dat and critter.dat required')
    # Refuse ambiguous case variants rather than silently selecting assets.
    if len(entries) != len(list(source.iterdir())):
        raise ValueError('installation has ambiguous case-duplicate names')
    data = entries.get('data')
    if data:
        if not data.is_dir() or data.is_symlink():
            raise ValueError('data must be a normal directory')
        for path in data.rglob('*'):
            if path.is_symlink():
                raise ValueError('source data contains symlinks; prepare a separate plain copy first')
    destination.mkdir(parents=True, exist_ok=False)
    for name in ('master.dat', 'critter.dat'):
        shutil.copyfile(entries[name], destination / name)
    for name, path in entries.items():
        if len(name) == 12 and name.startswith('patch') and name[5:8].isdigit() and name.endswith('.dat'):
            if not path.is_file():
                raise ValueError('patch archive is not a regular file')
            shutil.copyfile(path, destination / name)
    (destination / 'data').mkdir()
    if data:
        # Skip any saves/caches from the original, including nested savegame dirs.
        shutil.copytree(data, destination / 'data', dirs_exist_ok=True, ignore=exclude_runtime_files)
    (destination / 'fallout2.cfg').write_text(CONFIG)
    # This marker is last: interrupted copies cannot be launched as WI runtimes.
    (destination / '.wasteland-runtime').write_text('WastelandInfinite sandbox v1\n')
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('owned_installation', type=Path)
    parser.add_argument('--name', default='proof', help='new child directory below runtime/')
    args = parser.parse_args()
    if not args.name or Path(args.name).name != args.name or args.name in ('.', '..'):
        parser.error('--name must be a simple directory name')
    try:
        destination = stage(args.owned_installation, RUNTIME / args.name)
        dat_tool = ROOT / 'out/build/wi-dev/ce-dat-tool'
        if dat_tool.is_file():
            subprocess.run([str(dat_tool), 'create', str(ROOT / 'files/ce.dat'), str(destination / 'ce.dat')], check=True)
        else:
            print('Optional CE UI archive not built; build ce-dat-tool to create it.', file=sys.stderr)
        print(f'Runtime copied to {destination}. Original installation was only read.')
        print('Launch from that directory using the absolute wi-dev/fallout2-ce path and --wi-dev-seed=42; select NEW GAME.')
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(2, f'Cannot stage runtime: {error}\n')


if __name__ == '__main__':
    main()
