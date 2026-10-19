"""Driver-profile ownership, race and rollback tests; no installed driver needed."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import nvidia_presentation as n


class MemoryDrs:
    def __init__(self, store):
        self.store = store
        self.profiles = copy.deepcopy(store['profiles'])

    def __enter__(self):
        return self

    def __exit__(self, *_):
        pass

    def lookup_application(self, application):
        for profile in self.profiles.values():
            if profile.get('application', '').replace('\\', '/').casefold() == application.replace('\\', '/').casefold():
                return copy.deepcopy(profile)
        return None

    def inspect(self, name):
        return copy.deepcopy(self.profiles.get(name))

    def backup(self, path):
        if path.exists():
            raise ValueError('Backup already exists')
        path.write_text(json.dumps(self.store))

    def create(self, plan):
        self.profiles[plan['profileName']] = {
            'name': plan['profileName'], 'predefined': False, 'apps': 1, 'settings': 1,
            'application': plan['application'].replace('\\', '/').lower(),
            'marker': n.MARKER, 'setting': n.DXGI, 'setting_location': 0, 'setting_type': 0}

    def delete(self, name):
        del self.profiles[name]

    def save(self):
        self.store['profiles'] = copy.deepcopy(self.profiles)
        if self.store.pop('fail_after_commit', False):
            raise ValueError('Synthetic failure after driver commit')


def driver_store():
    return {'global': {'unrelatedSetting': 17}, 'profiles': {
        'Another game': {'name': 'Another game', 'application': 'another.exe',
                         'predefined': False, 'apps': 1, 'settings': 3}}}


class PresentationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.game = Path(self.temp.name) / 'Game'
        self.game.mkdir()
        (self.game / 'FTLGame-1.6.9.exe').write_bytes(b'fixture')
        (self.game / n.MARKER).write_text('fixture')
        self.backup = Path(self.temp.name) / 'Backup'
        self.backup.mkdir()
        self.store = driver_store()
        self.original = copy.deepcopy(self.store)
        self.driver = patch.object(n, 'Drs', side_effect=lambda: MemoryDrs(self.store))
        self.driver.start()
        self.addCleanup(self.driver.stop)

    def test_create_and_restore_touch_only_owned_profile(self):
        plan = n.plan(self.game)
        n.apply(plan, self.backup)
        self.assertEqual(self.store['global'], self.original['global'])
        self.assertEqual(self.store['profiles']['Another game'], self.original['profiles']['Another game'])
        self.assertEqual(json.loads((self.backup / 'nvidia-before.dat').read_text()), self.original)
        n.restore(plan)
        self.assertEqual(self.store, self.original)
        n.restore(plan)  # Recovery after an already-completed removal is safe.
        self.assertEqual(self.store, self.original)

    def test_preexisting_application_profile_is_not_modified(self):
        self.store['profiles']['User profile'] = {
            'name': 'User profile', 'application': str(self.game / 'FTLGame-1.6.9.exe')}
        before = copy.deepcopy(self.store)
        with self.assertRaisesRegex(ValueError, 'existing profile'):
            n.plan(self.game)
        self.assertEqual(self.store, before)

    def test_customized_owned_profile_blocks_rollback(self):
        plan = n.plan(self.game)
        n.apply(plan, self.backup)
        self.store['profiles'][plan['profileName']]['setting'] = 0
        before = copy.deepcopy(self.store)
        with self.assertRaisesRegex(ValueError, 'contents changed'):
            n.restore(plan)
        self.assertEqual(self.store, before)

    def test_profile_added_after_review_is_not_overwritten(self):
        plan = n.plan(self.game)
        self.store['profiles']['New user profile'] = {
            'name': 'New user profile', 'application': plan['application']}
        before = copy.deepcopy(self.store)
        with self.assertRaisesRegex(ValueError, 'changed after review'):
            n.apply(plan, self.backup)
        self.assertEqual(self.store, before)

    def test_repeated_enable_and_update_rollback_keep_original_profile(self):
        first = n.plan(self.game)
        n.apply(first, self.backup)
        second = n.plan(self.game)
        self.assertFalse(second['changed'])
        n.apply(second, self.backup)
        before = copy.deepcopy(self.store)
        n.restore(second)
        self.assertEqual(self.store, before)
        n.restore(first)
        self.assertEqual(self.store, self.original)


if __name__ == '__main__':
    unittest.main()
