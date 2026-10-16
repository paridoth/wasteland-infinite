from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from director.world.contracts import load_location, validate_location, strict_json

ROOT = Path(__file__).resolve().parents[1]
ENGINE = os.environ.get('WI_ENGINE')


@unittest.skipUnless(ENGINE, 'run through CTest or set WI_ENGINE to the built engine')
class ContractTests(unittest.TestCase):
    def inspect(self, seed='42'):
        # An empty cwd proves inspection does not resolve proprietary assets.
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([ENGINE, f'--wi-inspect={seed}'], cwd=directory,
                                    capture_output=True, timeout=10, check=True)
            self.assertEqual(list(Path(directory).iterdir()), [])
        return result.stdout

    def test_seed_replay_and_golden_fixtures(self):
        seeds = json.loads((ROOT / 'tests/fixtures/seeds.v1.json').read_text())
        for item in seeds['cases']:
            with self.subTest(seed=item['seed']):
                output = self.inspect(item['seed'])
                self.assertEqual(output, self.inspect(item['seed']))
                self.assertEqual(hashlib.sha256(output).hexdigest(), item['sha256'])
                location = load_location(output)
                self.assertEqual(location['seed'], item['seed'])
                self.assertEqual(location, load_location(json.dumps(location)))

    def test_closed_schema(self):
        base = load_location(self.inspect())
        for mutate in [lambda d: d.update(schema_version=2),
                       lambda d: d.update(generator_version=True),
                       lambda d: d.update(seed=42),
                       lambda d: d.update(seed='18446744073709551616'),
                       lambda d: d.update(elevation=True),
                       lambda d: d.update(arbitrary_action='execute'),
                       lambda d: d['template'].update(floor=999999),
                       lambda d: d['template'].update(map='MISSING.MAP'),
                       lambda d: d['spawns'][0].update(prototype='invented.critter'),
                       lambda d: d['spawns'][0].update(hex=40000),
                       lambda d: d['spawns'][0].update(id='nonexistent.entity'),
                       lambda d: d['spawns'][0].update(hex=d['spawns'][1]['hex']),
                       lambda d: d['spawns'][0].update(hex=d['exits'][0]),
                       lambda d: d['walkable'].append(1000),
                       lambda d: d['obstacles'].append(d['entrances'][0]),
                       lambda d: d.update(exits=[1000]),
                       lambda d: d['walkable'].append(d['walkable'][0])]:
            changed = deepcopy(base)
            mutate(changed)
            with self.assertRaises(ValueError):
                validate_location(changed)

    def test_invalid_options_fail_without_assets(self):
        for option in ['--wi-inspect=-1', '--wi-inspect=18446744073709551616',
                       '--wi-inspect=01', '--wi-inspect=', '--wi-typo=42']:
            result = subprocess.run([ENGINE, option], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 2)
            self.assertIn(b'Wasteland Infinite:', result.stderr)
        result = subprocess.run([ENGINE, '--wi-dev-seed=42', '--dev-load-game=1'],
                                capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 2)

    def test_developer_mode_requires_isolated_runtime(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([ENGINE, '--wi-dev-seed=42'], cwd=directory,
                                    capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(list(Path(directory).iterdir()), [])


class JsonTests(unittest.TestCase):
    def test_duplicate_keys_and_nonfinite_rejected(self):
        for text in ['{"seed":"1","seed":"2"}', '{"value":NaN}', '{"value":Infinity}', '{']:
            with self.assertRaises(ValueError):
                strict_json(text)

    def test_body_bound(self):
        with self.assertRaises(ValueError):
            strict_json(' ' * 1048577)
