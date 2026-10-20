from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from tools.wasteland import stage_runtime


class StagingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'owned'
        self.source.mkdir()
        # Synthetic byte fixtures only; these are NOT real game archives.
        (self.source / 'MASTER.DAT').write_bytes(b'fixture-master')
        (self.source / 'CRITTER.DAT').write_bytes(b'fixture-critter')
        (self.source / 'PATCH000.DAT').write_bytes(b'fixture-patch')
        (self.source / 'fallout2.cfg').write_text('original configuration')
        (self.source / 'data/SaveGame/SLOT01').mkdir(parents=True)
        (self.source / 'data/SaveGame/SLOT01/SAVE.DAT').write_bytes(b'original save')
        (self.source / 'data/maps').mkdir()
        (self.source / 'data/maps/test.MAP').write_bytes(b'fixture-map')
        (self.source / 'data/maps/test.SAV').write_bytes(b'fixture-cache')
        (self.source / 'data/maps/AUTOMAP.DB').write_bytes(b'original automap cache')
        (self.source / 'data/worldmap.dat').write_bytes(b'original worldmap cache')
        (self.source / 'sound/music').mkdir(parents=True)
        (self.source / 'sound/music/temple.acm').write_bytes(b'fixture-music')
        self.runtime = self.root / 'runtime'
        mock = patch.object(stage_runtime, 'RUNTIME', self.runtime)
        mock.start()
        self.addCleanup(mock.stop)

    def snapshot(self):
        return {str(p.relative_to(self.source)): p.read_bytes()
                for p in self.source.rglob('*') if p.is_file()}

    def test_copies_assets_without_touching_installation_or_saves(self):
        before = self.snapshot()
        target = stage_runtime.stage(self.source, self.runtime / 'proof')
        self.assertEqual(self.snapshot(), before)
        self.assertEqual((target / 'master.dat').read_bytes(), b'fixture-master')
        self.assertEqual((target / 'patch000.dat').read_bytes(), b'fixture-patch')
        self.assertEqual((target / 'data/maps/test.MAP').read_bytes(), b'fixture-map')
        self.assertFalse((target / 'data/maps/test.SAV').exists())
        self.assertFalse((target / 'data/maps/AUTOMAP.DB').exists())
        self.assertFalse((target / 'data/worldmap.dat').exists())
        self.assertFalse((target / 'data/SaveGame').exists())
        self.assertEqual((target / 'sound/music/temple.acm').read_bytes(), b'fixture-music')
        self.assertEqual((target / 'fallout2.cfg').read_text(), stage_runtime.CONFIG)
        self.assertFalse(any(p.is_symlink() for p in target.rglob('*')))
        with self.assertRaises(ValueError):
            stage_runtime.stage(self.source, target)
        self.assertEqual(self.snapshot(), before)

    def test_rejects_overlap_unsafe_paths_and_missing_assets(self):
        for destination in [self.source / 'runtime', self.source, self.root / 'outside']:
            with self.assertRaises(ValueError):
                stage_runtime.stage(self.source, destination)
        (self.source / 'MASTER.DAT').unlink()
        with self.assertRaises(ValueError):
            stage_runtime.stage(self.source, self.runtime / 'missing')

    def test_rejects_symlinks_in_loose_data(self):
        for directory in ('data', 'sound'):
            with self.subTest(directory=directory):
                link = self.source / directory / 'link'
                link.symlink_to(self.source, target_is_directory=True)
                with self.assertRaises(ValueError):
                    stage_runtime.stage(self.source, self.runtime / 'links')
                self.assertFalse((self.runtime / 'links').exists())
                link.unlink()
