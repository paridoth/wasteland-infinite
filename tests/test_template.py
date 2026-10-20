"""Synthetic archive fixtures only. Actual map bytes stay in ignored runtime/."""
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from tools.wasteland import stage_runtime, stage_template
from director.world.contracts import validate_schema

DAT_TOOL = os.environ.get('WI_DAT_TOOL')
ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(DAT_TOOL, 'run through CTest or set WI_DAT_TOOL')
class CopiedTemplateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'owned'
        self.source.mkdir()
        archive_input = self.root / 'archive-input'
        (archive_input / 'maps').mkdir(parents=True)
        maps = {'artemple.map': b'synthetic temple bytes', 'arcaves.map': b'synthetic cave bytes'}
        for name, content in maps.items():
            (archive_input / 'maps' / name).write_bytes(content)
        subprocess.run([DAT_TOOL, 'create', str(archive_input), str(self.source / 'master.dat')],
                       check=True, capture_output=True)
        for name in ('critter.dat', 'patch000.dat'):
            (self.source / name).write_bytes(b'synthetic unrelated archive')
        (self.source / 'data/SAVEGAME/SLOT01').mkdir(parents=True)
        (self.source / 'data/SAVEGAME/SLOT01/SAVE.DAT').write_bytes(b'original save')
        (self.source / 'fallout2.cfg').write_text('original configuration')
        recipe = {'schema_version': 1, 'id': 'artemple-copy.v1',
                  'archives': {name: stage_template.digest(self.source / name)
                               for name in ('master.dat', 'critter.dat', 'patch000.dat')},
                  'maps': {name: {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                           for name, data in maps.items()}}
        self.recipe = self.root / 'recipe.json'
        self.recipe.write_text(json.dumps(recipe))
        self.runtime = self.root / 'runtime'
        for mock in (patch.object(stage_runtime, 'RUNTIME', self.runtime),
                     patch.object(stage_template, 'RECIPE', self.recipe)):
            mock.start()
            self.addCleanup(mock.stop)

    def snapshot(self):
        return {str(p.relative_to(self.source)): stage_template.digest(p)
                for p in self.source.rglob('*') if p.is_file()}

    def test_native_extraction_preserves_maps_source_and_saves(self):
        before = self.snapshot()
        target = stage_template.stage(self.source, self.runtime / 'copy', Path(DAT_TOOL))
        self.assertEqual(before, self.snapshot())
        for name in ('artemple.map', 'arcaves.map'):
            self.assertEqual((target / 'data/maps' / name).read_bytes(),
                             (target / 'template-source' / name).read_bytes())
        self.assertFalse((target / 'data/SAVEGAME').exists())
        self.assertEqual((target / '.wasteland-template-copy').read_text(), stage_template.MARKER)
        self.assertTrue((target / 'ce.dat').is_file())
        with self.assertRaises(ValueError):
            stage_template.stage(self.source, target, Path(DAT_TOOL))

    def test_unknown_archives_and_wrong_map_digest_fail_before_copy(self):
        archive = self.source / 'critter.dat'
        original = archive.read_bytes()
        archive.write_bytes(b'unknown version')
        with self.assertRaises(ValueError):
            stage_template.stage(self.source, self.runtime / 'unknown', Path(DAT_TOOL))
        self.assertFalse((self.runtime / 'unknown').exists())
        archive.write_bytes(original)
        recipe = json.loads(self.recipe.read_text())
        recipe['maps']['artemple.map']['sha256'] = '0' * 64
        self.recipe.write_text(json.dumps(recipe))
        with self.assertRaises(ValueError):
            stage_template.stage(self.source, self.runtime / 'wrong-map', Path(DAT_TOOL))
        self.assertFalse((self.runtime / 'wrong-map').exists())

    def test_extra_patch_or_loose_override_cannot_mark_template_ready(self):
        extra = self.source / 'patch001.dat'
        extra.write_bytes(b'extra patch')
        with self.assertRaises(ValueError):
            stage_template.stage(self.source, self.runtime / 'extra', Path(DAT_TOOL))
        self.assertFalse((self.runtime / 'extra').exists())
        extra.unlink()
        (self.source / 'data/maps').mkdir()
        (self.source / 'data/maps/ARTEMPLE.MAP').write_bytes(b'loose override')
        with self.assertRaises(ValueError):
            stage_template.stage(self.source, self.runtime / 'override', Path(DAT_TOOL))
        self.assertFalse((self.runtime / 'override/.wasteland-template-copy').exists())


class TemplateAuditSchemaTests(unittest.TestCase):
    def test_closed_audit_contract_rejects_invalid_coordinates_and_types(self):
        schema = json.loads((ROOT / 'schemas/template-audit.v1.schema.json').read_text())
        audit = {'schema_version': 1, 'template_id': 'artemple-copy.v1', 'map': 'ARTEMPLE.MAP',
                 'entry_hex': 20100, 'entry_elevation': 0, 'entry_rotation': 0,
                 'reachable_exit_count': 1, 'exits': [{'hex': 20101, 'elevation': 0, 'hidden': False,
                 'destination_map_index': 3, 'destination_map': 'ARCAVES.MAP', 'destination_hex': 20102,
                 'destination_elevation': 0, 'destination_rotation': 0, 'reference_valid': True, 'path_length': 1}]}
        validate_schema(audit, schema)
        for mutate in (lambda d: d.update(schema_version=2), lambda d: d.update(entry_hex=40000),
                       lambda d: d.update(extra=True), lambda d: d['exits'][0].update(hidden=1),
                       lambda d: d['exits'][0].update(destination_map='../BAD.MAP'),
                       lambda d: d['exits'][0].update(path_length=True)):
            with self.subTest(mutation=mutate):
                changed = deepcopy(audit)
                mutate(changed)
                with self.assertRaises(ValueError):
                    validate_schema(changed, schema)
