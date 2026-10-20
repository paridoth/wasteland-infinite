import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from director.world.contracts import neighbors
from tools.wasteland.alcove_contract import load_alcove
from tools.wasteland.encounter_contract import load_encounter

ROOT = Path(__file__).resolve().parents[1]
ENGINE = os.environ.get('WI_ENGINE')


def sample_alcove():
    # Independent radius-four patch with a five-blocker alcove on its east side.
    cells, frontier = {20100}, {20100}
    for _ in range(4):
        frontier = {other for cell in frontier for other in neighbors(cell)} - cells
        cells |= frontier
    return {
        'schema_version': 1, 'generator_version': 1, 'module': 'grove-alcove.v1',
        'seed': '42', 'entry': 20100, 'walkable': sorted(cells),
        'protected': [19898, 20099, 20100], 'interior': 20101, 'opening': 20100,
        'blockers': [19900, 19901, 19902, 20102, 20301],
        'loot': [{'hex': 20101, 'prototype': 'vanilla.stimpak'},
                 {'hex': 19500, 'prototype': 'vanilla.money'},
                 {'hex': 20901, 'prototype': 'vanilla.money'}],
    }


def sample():
    alcove = sample_alcove()
    return {
        'schema_version': 1, 'generator_version': 1,
        'encounter': 'alcove-guard.v1', 'layout': 'grove-alcove.v1',
        'seed': '42', 'entry': 20100,
        'walkable': sorted(set(alcove['walkable']) - set(alcove['blockers'])),
        'protected': [19500, 19898, 20099, 20100, 20101, 20901],
        'entry_clearance': 4,
        'enemy': {'hex': 20900, 'prototype': 'template.hostile-ant'},
    }


class EncounterContractTests(unittest.TestCase):
    def reject(self, mutate):
        plan = sample()
        mutate(plan)
        with self.assertRaises(ValueError):
            load_encounter(json.dumps(plan))

    def test_closed_schema_and_scalar_bounds(self):
        original = sample()
        self.assertEqual(load_encounter(json.dumps(original)), original)
        for mutate in [lambda p: p.update(schema_version=2),
                       lambda p: p.update(generator_version=True),
                       lambda p: p.update(encounter='alcove-guard.v2'),
                       lambda p: p.update(layout='grove-alcove.v2'),
                       lambda p: p.update(seed=42),
                       lambda p: p.update(seed='01'),
                       lambda p: p.update(seed='-1'),
                       lambda p: p.update(seed='18446744073709551616'),
                       lambda p: p.update(entry=True),
                       lambda p: p.update(entry=-1),
                       lambda p: p.update(entry_clearance=3),
                       lambda p: p.update(entry_clearance=True),
                       lambda p: p.update(execute='code'),
                       lambda p: p['enemy'].update(prototype='vanilla.ant'),
                       lambda p: p['enemy'].update(prototype=1),
                       lambda p: p['enemy'].update(hex=40000),
                       lambda p: p['enemy'].update(hex=True),
                       lambda p: p['enemy'].update(script='code'),
                       lambda p: p['enemy'].pop('prototype'),
                       lambda p: p['walkable'].append(p['walkable'][0]),
                       lambda p: p['protected'].append(p['protected'][0]),
                       lambda p: p.update(walkable=list(range(128))),
                       lambda p: p.update(walkable=p['walkable'][:4]),
                       lambda p: p.update(protected=[]),
                       lambda p: p.update(protected=list(range(128)))]:
            with self.subTest(mutate=mutate):
                self.reject(mutate)
        for raw in [json.dumps(original).replace('"seed": "42"', '"seed": "42", "seed": "1"'),
                    json.dumps(original).replace('"hex": 20900', '"hex": 20900, "hex": 20901'),
                    json.dumps(original).replace('"entry_clearance": 4', '"entry_clearance": NaN'),
                    ' ' * (1_048_576 + 1)]:
            with self.subTest(raw=raw[:100]):
                with self.assertRaises(ValueError):
                    load_encounter(raw)

    def test_geometry_and_entry_clearance(self):
        for mutate in [lambda p: p['protected'].remove(p['entry']),
                       lambda p: p['protected'].append(1000),
                       lambda p: p['protected'].append(p['enemy']['hex']),
                       lambda p: p['enemy'].update(hex=1000),
                       lambda p: p['enemy'].update(hex=20700),
                       lambda p: p['walkable'].append(1000)]:
            with self.subTest(mutate=mutate):
                self.reject(mutate)

        # A distant endpoint is allowed, but a guard in its narrow approach is not.
        corridor = sample()
        corridor.update(walkable=[20100, 20101, 20102, 20103, 20104, 20105],
                        protected=[20100], enemy={'hex': 20105, 'prototype': 'template.hostile-ant'})
        self.assertEqual(load_encounter(json.dumps(corridor)), corridor)
        corridor['enemy']['hex'] = 20104
        with self.assertRaisesRegex(ValueError, 'disconnects'):
            load_encounter(json.dumps(corridor))

        # Physical clearance must ignore a long walk forced by absent terrain.
        detour = sample()
        detour.update(walkable=[20100, 19900, 19700, 19500, 19501, 19502,
                               19702, 19902, 20102], protected=[20100],
                      enemy={'hex': 20102, 'prototype': 'template.hostile-ant'})
        with self.assertRaisesRegex(ValueError, 'clearance'):
            load_encounter(json.dumps(detour))

    def test_paired_alcove_geometry_and_loot_protection(self):
        alcove, original = sample_alcove(), sample()
        self.assertEqual(load_alcove(json.dumps(alcove)), alcove)
        self.assertEqual(load_encounter(json.dumps(original), json.dumps(alcove)), original)
        # Membership, not serialization order, defines the matching geometry.
        reordered = copy.deepcopy(original)
        reordered['walkable'].reverse()
        reordered['protected'].reverse()
        self.assertEqual(load_encounter(json.dumps(reordered), json.dumps(alcove)), reordered)
        for mutate in [lambda p: p.update(seed='43'),
                       lambda p: p.update(entry=20099),
                       lambda p: p['walkable'].remove(19300),
                       lambda p: p['protected'].remove(20101),
                       lambda p: p['protected'].remove(19500),
                       lambda p: p['protected'].remove(20901),
                       lambda p: p['protected'].append(19300)]:
            plan = copy.deepcopy(original)
            mutate(plan)
            with self.subTest(mutate=mutate):
                with self.assertRaisesRegex(ValueError, 'match alcove'):
                    load_encounter(json.dumps(plan), json.dumps(alcove))
        # A loot collision is caught when an externally supplied plan omits its reservation.
        collision = copy.deepcopy(original)
        collision['protected'].remove(20901)
        collision['enemy']['hex'] = 20901
        self.assertEqual(load_encounter(json.dumps(collision)), collision)
        with self.assertRaisesRegex(ValueError, 'match alcove'):
            load_encounter(json.dumps(collision), json.dumps(alcove))
        # The opening also needs a reservation when it differs from the entry.
        separate_opening = copy.deepcopy(alcove)
        separate_opening.update(interior=20298, opening=20297,
                                blockers=[20098, 20299, 20497, 20498, 20499])
        separate_opening['loot'][0]['hex'] = 20298
        matched = copy.deepcopy(original)
        matched['walkable'] = sorted(set(separate_opening['walkable'])
                                     - set(separate_opening['blockers']))
        matched['protected'] = sorted(set(separate_opening['protected']) | {20297, 20298, 19500, 20901})
        self.assertEqual(load_encounter(json.dumps(matched), json.dumps(separate_opening)), matched)
        matched['protected'].remove(20297)
        self.assertEqual(load_encounter(json.dumps(matched)), matched)
        with self.assertRaisesRegex(ValueError, 'match alcove'):
            load_encounter(json.dumps(matched), json.dumps(separate_opening))
        bad_alcove = copy.deepcopy(alcove)
        bad_alcove['loot'][0]['hex'] = 19500
        with self.assertRaises(ValueError):
            load_encounter(json.dumps(original), json.dumps(bad_alcove))


@unittest.skipUnless(ENGINE, 'run through CTest or set WI_ENGINE')
class EncounterReplayTests(unittest.TestCase):
    def inspect(self, seed):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([ENGINE, f'--wi-inspect-encounter={seed}'], cwd=directory,
                                    capture_output=True, check=True, timeout=10)
            self.assertEqual(list(Path(directory).iterdir()), [])
            return result.stdout

    def test_pinned_encounter_replays(self):
        fixture = json.loads((ROOT / 'tests/fixtures/encounter-seeds.v1.json').read_text())
        self.assertEqual([item['seed'] for item in fixture['cases']],
                         ['0', '1', '42', '9007199254740993', '18446744073709551615'])
        for item in fixture['cases']:
            with self.subTest(seed=item['seed']):
                raw = self.inspect(item['seed'])
                self.assertEqual(raw, self.inspect(item['seed']))
                self.assertEqual(hashlib.sha256(raw).hexdigest(), item['sha256'])
                plan = load_encounter(raw)
                self.assertEqual(plan['seed'], item['seed'])
                self.assertEqual(plan['enemy']['hex'], item['enemy_hex'])
                self.assertEqual(plan, load_encounter(json.dumps(plan)))
