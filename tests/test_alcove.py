import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from tools.wasteland.alcove_contract import load_alcove

ROOT = Path(__file__).resolve().parents[1]
ENGINE = os.environ.get('WI_ENGINE')


def sample():
    # Explicit radius-three hex patch, with an east-side alcove opening at entry.
    return {
        'schema_version': 1, 'generator_version': 1, 'module': 'grove-alcove.v1',
        'seed': '42', 'entry': 20100,
        'walkable': [19500, 19698, 19699, 19700, 19701, 19702, 19897, 19898,
                     19899, 19900, 19901, 19902, 19903, 20097, 20098, 20099,
                     20100, 20101, 20102, 20103, 20297, 20298, 20299, 20300,
                     20301, 20302, 20303, 20497, 20498, 20499, 20500, 20501,
                     20502, 20503, 20699, 20700, 20701],
        'protected': [19898, 20099, 20100], 'interior': 20101, 'opening': 20100,
        'blockers': [19900, 19901, 19902, 20102, 20301],
        'loot': [{'hex': 20101, 'prototype': 'vanilla.stimpak'},
                 {'hex': 19500, 'prototype': 'vanilla.money'},
                 {'hex': 20701, 'prototype': 'vanilla.money'}],
    }


class AlcoveContractTests(unittest.TestCase):
    def reject(self, mutate):
        plan = sample()
        mutate(plan)
        with self.assertRaises(ValueError):
            load_alcove(json.dumps(plan))

    def test_closed_alcove_schema(self):
        original = sample()
        self.assertEqual(load_alcove(json.dumps(original)), original)
        for mutate in [lambda p: p.update(schema_version=2),
                       lambda p: p.update(generator_version=True),
                       lambda p: p.update(module='grove-alcove.v2'),
                       lambda p: p.update(seed=42),
                       lambda p: p.update(seed='01'),
                       lambda p: p.update(seed='18446744073709551616'),
                       lambda p: p.update(seed='-1'),
                       lambda p: p.update(entry=True),
                       lambda p: p.update(interior=40000),
                       lambda p: p.update(execute='code'),
                       lambda p: p['loot'][0].update(id='invented.entity'),
                       lambda p: p['loot'][0].update(prototype='invented.critter'),
                       lambda p: p['loot'][0].update(hex=True),
                       lambda p: p['walkable'].append(p['walkable'][0]),
                       lambda p: p['protected'].append(p['protected'][0]),
                       lambda p: p['blockers'].__setitem__(0, p['blockers'][1]),
                       lambda p: p['blockers'].pop(),
                       lambda p: p['loot'].pop(),
                       lambda p: p.update(walkable=list(range(128)))]:
            with self.subTest(mutate=mutate):
                self.reject(mutate)
        with self.assertRaises(ValueError):
            load_alcove(json.dumps(original).replace('"seed": "42"', '"seed": "42", "seed": "1"'))

    def test_geometry_protection_and_loot_rejections(self):
        for mutate in [lambda p: p['protected'].remove(p['entry']),
                       lambda p: p['protected'].append(1000),
                       lambda p: p['protected'].append(p['blockers'][0]),
                       lambda p: p['protected'].append(p['interior']),
                       lambda p: p.update(interior=p['entry']),
                       lambda p: p.update(interior=1000),
                       lambda p: p.update(opening=p['blockers'][0]),
                       lambda p: p.update(opening=19500),
                       lambda p: p['blockers'].__setitem__(0, 19500),
                       lambda p: p['walkable'].append(1000),
                       lambda p: p['loot'][0].update(hex=19500),
                       lambda p: p['loot'][1].update(hex=p['interior']),
                       lambda p: p['loot'][1].update(hex=p['opening']),
                       lambda p: p['loot'][1].update(hex=p['protected'][0]),
                       lambda p: p['loot'][1].update(hex=p['blockers'][0]),
                       lambda p: p['loot'][1].update(hex=1000),
                       lambda p: p['loot'][1].update(hex=p['loot'][2]['hex'])]:
            with self.subTest(mutate=mutate):
                self.reject(mutate)
        # All cells are initially connected, but the ring cuts off the north spur.
        disconnected = sample()
        disconnected.update(walkable=[19700, 19701, 19702, 19900, 19901, 19902,
                                      20100, 20101, 20102, 20301], protected=[20100])
        with self.assertRaisesRegex(ValueError, 'disconnects'):
            load_alcove(json.dumps(disconnected))
        # Protecting the exterior leaves insufficient room for the two outside items.
        no_capacity = sample()
        no_capacity['protected'] = sorted(set(no_capacity['walkable'])
                                          - set(no_capacity['blockers']) - {no_capacity['interior']})
        with self.assertRaisesRegex(ValueError, 'loot capacity'):
            load_alcove(json.dumps(no_capacity))
        too_small = sample()
        too_small['walkable'] = too_small['walkable'][:9]
        with self.assertRaises(ValueError):
            load_alcove(json.dumps(too_small))


@unittest.skipUnless(ENGINE, 'run through CTest or set WI_ENGINE')
class AlcoveReplayTests(unittest.TestCase):
    def inspect(self, seed):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([ENGINE, f'--wi-inspect-alcove={seed}'], cwd=directory,
                                    capture_output=True, check=True, timeout=10)
            self.assertEqual(list(Path(directory).iterdir()), [])
            return result.stdout

    def test_pinned_alcove_replays(self):
        fixture = json.loads((ROOT / 'tests/fixtures/alcove-seeds.v1.json').read_text())
        self.assertEqual([item['seed'] for item in fixture['cases']],
                         ['0', '1', '42', '9007199254740993', '18446744073709551615'])
        for item in fixture['cases']:
            with self.subTest(seed=item['seed']):
                raw = self.inspect(item['seed'])
                self.assertEqual(raw, self.inspect(item['seed']))
                self.assertEqual(hashlib.sha256(raw).hexdigest(), item['sha256'])
                plan = load_alcove(raw)
                for field in ('seed', 'interior', 'opening', 'blockers'):
                    self.assertEqual(plan[field], item[field])
                self.assertEqual(plan, load_alcove(json.dumps(plan)))

    def test_invalid_alcove_options_without_assets(self):
        with tempfile.TemporaryDirectory() as directory:
            for value in ('', '-1', '01', '18446744073709551616'):
                result = subprocess.run([ENGINE, f'--wi-inspect-alcove={value}'], cwd=directory,
                                        capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 2)
                self.assertIn(b'Wasteland Infinite:', result.stderr)
                self.assertEqual(list(Path(directory).iterdir()), [])
