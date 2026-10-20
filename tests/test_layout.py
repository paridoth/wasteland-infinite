from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from tools.wasteland.layout_contract import load_layout

ROOT = Path(__file__).resolve().parents[1]
ENGINE = os.environ.get('WI_ENGINE')


@unittest.skipUnless(ENGINE, 'run through CTest or set WI_ENGINE')
class LayoutTests(unittest.TestCase):
    def inspect(self, seed):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([ENGINE, f'--wi-inspect-layout={seed}'], cwd=directory,
                                    capture_output=True, check=True, timeout=10)
            self.assertEqual(list(Path(directory).iterdir()), [])
            return result.stdout

    def test_pinned_layout_replay_and_contract(self):
        fixture = json.loads((ROOT / 'tests/fixtures/layout-seeds.v1.json').read_text())
        for item in fixture['cases']:
            with self.subTest(seed=item['seed']):
                raw = self.inspect(item['seed'])
                self.assertEqual(raw, self.inspect(item['seed']))
                self.assertEqual(hashlib.sha256(raw).hexdigest(), item['sha256'])
                plan = load_layout(raw)
                self.assertEqual(plan['blockers'], item['blockers'])
                self.assertEqual(plan['seed'], item['seed'])
                self.assertEqual(plan, load_layout(json.dumps(plan)))

    def test_invalid_layout_contract(self):
        original = load_layout(self.inspect('42'))
        changes = [lambda p: p.update(schema_version=2), lambda p: p.update(generator_version=True),
                   lambda p: p.update(seed='18446744073709551616'), lambda p: p.update(seed='01'),
                   lambda p: p.update(execute='code'), lambda p: p.update(entry=True),
                   lambda p: p['blockers'].__setitem__(0, p['entry']),
                   lambda p: p['blockers'].__setitem__(0, p['blockers'][1]),
                   lambda p: p['protected'].append(40000),
                   lambda p: p['protected'].remove(p['entry']),
                   lambda p: p['walkable'].append(1000)]
        for change in changes:
            with self.subTest(change=change):
                plan = deepcopy(original)
                change(plan)
                with self.assertRaises(ValueError):
                    load_layout(json.dumps(plan))
        corridor = {'schema_version': 1, 'generator_version': 1, 'layout': 'entry-partition.v1',
                    'seed': '42', 'entry': 20100, 'walkable': list(range(20100, 20107)),
                    'protected': [20100, 20106], 'blockers': [20102, 20103]}
        with self.assertRaises(ValueError):
            load_layout(json.dumps(corridor))
