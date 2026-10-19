"""Installer failure and recovery tests using disposable synthetic files."""
import argparse
import contextlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import ftl_ultrawide as uw

def number(value):
    out=bytearray()
    while True:
        byte=value&127; value>>=7
        if not value:
            out.append(byte|128); return bytes(out)
        out.append(byte); value-=1

def bps(source,target,operations):
    raw=b'BPS1'+number(len(source))+number(len(target))+number(0)+operations
    raw+=struct.pack('<II',zlib.crc32(source),zlib.crc32(target))
    return raw+struct.pack('<I',zlib.crc32(raw))

class BpsTests(unittest.TestCase):
    def test_all_commands_and_overlapping_target_copy(self):
        source=b'ABCDEF'; target=b'ABxyEFxyEFxy'
        operations=number((2-1)<<2)+number(((2-1)<<2)|1)+b'xy'
        operations+=number(((2-1)<<2)|2)+number(4<<1)
        operations+=number(((6-1)<<2)|3)+number(2<<1)
        self.assertEqual(uw.apply_bps(source,bps(source,target,operations)),target)

    def test_wrong_source_and_corrupt_patch_rejected(self):
        raw=bps(b'abc',b'XYZ',number(((3-1)<<2)|1)+b'XYZ')
        with self.assertRaises(ValueError):uw.apply_bps(b'bad',raw)
        changed=bytearray(raw); changed[8]^=1
        with self.assertRaises(ValueError):uw.apply_bps(b'abc',bytes(changed))

    def test_copy_before_output_rejected(self):
        raw=bps(b'a',b'a',number(3)+number(0))
        with self.assertRaises(ValueError):uw.apply_bps(b'a',raw)

class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.game=self.root/'Game'; self.game.mkdir()
        self.build=self.root/'Build'; self.payload=self.build/'payload'; self.payload.mkdir(parents=True)
        self.saves=self.root/'Saves'; self.saves.mkdir()
        (self.saves/'ae_prof.sav').write_bytes(b'original-unlocks-statistics')
        (self.game/'FTLGame.exe').write_bytes(b'owned-steam-executable')
        (self.game/'ftl.dat').write_bytes(b'owned-pristine-data')
        (self.game/'settings.ini').write_text('fullscreen=1\nmusic=37\n')
        (self.game/'unrelated.txt').write_text('leave me')
        self.original={p.name:p.read_bytes() for p in self.game.iterdir()}
        for name in uw.PAYLOAD:(self.payload/name).write_bytes(('new '+name).encode())
        self.old_exe=uw.file_hash(self.game/'FTLGame.exe'); self.dat=uw.file_hash(self.game/'ftl.dat')
        self.new_native=uw.file_hash(self.payload/'FTLGame-1.6.9.exe')
        uw.write_json(self.build/'build.json',{'version':'test','sourceExeSha256':self.old_exe,
            'sourceArchiveSha256':self.dat,'files':{p.name:uw.file_hash(p) for p in self.payload.iterdir()}})
        self.stack=contextlib.ExitStack(); self.addCleanup(self.stack.close)
        self.stack.enter_context(patch.object(uw,'STEAM_1614',self.old_exe))
        self.stack.enter_context(patch.object(uw,'STEAM_169',self.new_native))
        self.stack.enter_context(patch.object(uw,'VANILLA_DAT',self.dat))
        self.stack.enter_context(contextlib.redirect_stdout(io.StringIO()))

    def install(self):
        uw.install(argparse.Namespace(game_dir=self.game,build=self.build,save_dir=self.saves))
        return next(self.game.glob('ftl-ultrawide-backup-*'))

    def assert_original(self):
        for name,data in self.original.items():self.assertEqual((self.game/name).read_bytes(),data)
        self.assertEqual((self.saves/'ae_prof.sav').read_bytes(),b'original-unlocks-statistics')

    def test_install_then_restore_preserves_original_and_new_progress(self):
        backup=self.install()
        self.assertEqual(uw.read_json(backup/'manifest.json')['status'],'installed')
        self.assertEqual((backup/'saves/ae_prof.sav').read_bytes(),b'original-unlocks-statistics')
        (self.saves/'hs_ultrawide_prof.sav').write_bytes(b'new earned progress')
        (self.game/'Hyperspace.dll').write_bytes(b'normal launch-generated dll')
        uw.restore_backup(backup)
        self.assert_original()
        self.assertEqual((self.saves/'hs_ultrawide_prof.sav').read_bytes(),b'new earned progress')
        self.assertFalse((self.game/'FTLGame-1.6.9.exe').exists())
        self.assertTrue(backup.exists())

    def test_corrupt_payload_refused_before_backup_or_game_changes(self):
        (self.payload/'FTLGame.exe').write_bytes(b'tampered')
        with self.assertRaises(ValueError):self.install()
        self.assert_original(); self.assertFalse(list(self.game.glob('ftl-ultrawide-backup-*')))

    def test_copy_failure_automatically_restores(self):
        real=uw.shutil.copy2
        def fail_once(source,destination,*a,**kw):
            if Path(source).resolve()==(self.payload/'xinput1_4.dll').resolve():raise OSError('synthetic interrupted install')
            return real(source,destination,*a,**kw)
        with patch.object(uw.shutil,'copy2',side_effect=fail_once):
            with self.assertRaises(OSError):self.install()
        self.assert_original()
        backup=next(self.game.glob('ftl-ultrawide-backup-*'))
        self.assertEqual(uw.read_json(backup/'manifest.json')['status'],'automatically-rolled-back')

    def test_modified_added_executable_is_not_deleted(self):
        backup=self.install(); (self.game/'FTL-Ultrawide.exe').write_bytes(b'unknown change')
        with self.assertRaises(ValueError):uw.restore_backup(backup)
        self.assertEqual((self.game/'FTL-Ultrawide.exe').read_bytes(),b'unknown change')

    def test_corrupt_backup_refused_before_restore(self):
        backup=self.install(); (backup/'files/FTLGame.exe').write_bytes(b'damaged original backup')
        before=(self.game/'FTLGame.exe').read_bytes()
        with self.assertRaises(ValueError):uw.restore_backup(backup)
        self.assertEqual((self.game/'FTLGame.exe').read_bytes(),before)

    def test_manifest_path_escape_refused(self):
        backup=self.install(); manifest=uw.read_json(backup/'manifest.json')
        manifest['files'][0]['name']='../outside.txt'; uw.write_json(backup/'manifest.json',manifest)
        with self.assertRaises(ValueError):uw.restore_backup(backup)
        with self.assertRaises(ValueError):uw.local_path(self.game,'../outside.txt')

    def test_display_update_preserves_other_settings_and_resets_manual_overrides(self):
        configured=uw.configure_settings('music=37\nmanual=1\nscreen_x=2\nscreen_x=3\nfullscreen=0\n')
        self.assertIn('music=37',configured); self.assertNotIn('manual=1',configured)
        self.assertEqual(configured.count('screen_x=0'),2); self.assertIn('fullscreen=4',configured)

    def prepare_update(self):
        self.install()
        (self.saves/'hs_ultrawide_continue.sav').write_bytes(b'current campaign')
        (self.saves/'hs_ultrawide_prof.sav').write_bytes(b'current earned progress')
        (self.game/'settings.ini').write_text('fullscreen=0\nmusic=37\ncustom=preserved\n')
        (self.game/'Hyperspace.dll').write_bytes(b'current runtime dll')
        def current(game):
            return {'gameDirectory':str(game.resolve()),'files':{n:uw.file_hash(game/n) for n in sorted(uw.PAYLOAD)}}
        expected=self.root/'current.json';uw.write_json(expected,current(self.game))
        for name in ['FTLGame.exe','FTL-Ultrawide.exe','ftl.dat']:
            (self.payload/name).write_bytes(('beta3 '+name).encode())
        report=uw.read_json(self.build/'build.json')
        report['files']={p.name:uw.file_hash(p) for p in self.payload.iterdir()}
        uw.write_json(self.build/'build.json',report)
        args=argparse.Namespace(command='update',game_dir=self.game,build=self.build,
                                save_dir=self.saves,expected_current=expected)
        self.stack.enter_context(patch.object(uw,'inventory',side_effect=current))
        return args

    def test_update_and_rollback_keep_current_campaign_progress_and_settings(self):
        args=self.prepare_update()
        before={n:(self.game/n).read_bytes() for n in uw.PAYLOAD}
        saves=uw.snapshots(self.saves)
        uw.install(args)
        backup=max(self.game.glob('ftl-ultrawide-backup-*'))
        for name in uw.MUTABLE:self.assertEqual((self.game/name).read_bytes(),before[name])
        self.assertEqual((self.game/'ftl.dat').read_bytes(),b'beta3 ftl.dat')
        self.assertEqual(uw.snapshots(self.saves),saves)
        self.assertEqual(uw.snapshots(backup/'saves'),saves)
        (self.saves/'hs_ultrawide_prof.sav').write_bytes(b'even newer progress')
        uw.restore_backup(backup)
        for n,data in before.items():self.assertEqual((self.game/n).read_bytes(),data)
        self.assertEqual((self.saves/'hs_ultrawide_prof.sav').read_bytes(),b'even newer progress')

    def test_update_refuses_stale_inventory_before_backup(self):
        args=self.prepare_update()
        (self.game/'settings.ini').write_text('changed since review')
        count=len(list(self.game.glob('ftl-ultrawide-backup-*')))
        with self.assertRaises(ValueError):uw.install(args)
        self.assertEqual(len(list(self.game.glob('ftl-ultrawide-backup-*'))),count)

    def test_borderless_preference_survives_update(self):
        args=self.prepare_update()
        settings=b'fullscreen=5\nlast_fullscreen=5\nmusic=37\ncustom=preserved\n'
        (self.game/'settings.ini').write_bytes(settings)
        uw.write_json(args.expected_current,uw.inventory(self.game))
        uw.install(args)
        self.assertEqual((self.game/'settings.ini').read_bytes(),settings)

    def test_update_copy_failure_restores_current_modded_files(self):
        args=self.prepare_update()
        before={n:(self.game/n).read_bytes() for n in uw.PAYLOAD}
        real=uw.shutil.copy2
        def fail_once(source,destination,*a,**kw):
            if Path(source).resolve()==(self.payload/'ftl.dat').resolve():raise OSError('interrupted update')
            return real(source,destination,*a,**kw)
        with patch.object(uw.shutil,'copy2',side_effect=fail_once):
            with self.assertRaises(OSError):uw.install(args)
        for n,data in before.items():self.assertEqual((self.game/n).read_bytes(),data)
        self.assertEqual((self.saves/'hs_ultrawide_continue.sav').read_bytes(),b'current campaign')

    def test_nvidia_profile_and_files_roll_back_together(self):
        import nvidia_presentation as n
        from test_nvidia_presentation import MemoryDrs, driver_store
        store=driver_store()
        with patch.object(n,'Drs',side_effect=lambda:MemoryDrs(store)):
            uw.install(argparse.Namespace(game_dir=self.game,build=self.build,save_dir=self.saves,nvidia_dxgi=True))
            backup=next(self.game.glob('ftl-ultrawide-backup-*'))
            self.assertEqual(len(store['profiles']),2)
            (self.saves/'hs_ultrawide_prof.sav').write_bytes(b'earned progress')
            uw.restore_backup(backup)
            self.assert_original()
            self.assertEqual(store,driver_store())
            self.assertEqual((self.saves/'hs_ultrawide_prof.sav').read_bytes(),b'earned progress')

    def test_nvidia_failure_after_commit_restores_profile_and_files(self):
        import nvidia_presentation as n
        from test_nvidia_presentation import MemoryDrs, driver_store
        store=driver_store();store['fail_after_commit']=True
        with patch.object(n,'Drs',side_effect=lambda:MemoryDrs(store)):
            with self.assertRaisesRegex(ValueError,'after driver commit'):
                uw.install(argparse.Namespace(game_dir=self.game,build=self.build,save_dir=self.saves,nvidia_dxgi=True))
        self.assert_original()
        self.assertEqual(store,driver_store())
        backup=next(self.game.glob('ftl-ultrawide-backup-*'))
        self.assertEqual(uw.read_json(backup/'manifest.json')['status'],'automatically-rolled-back')

    def test_changed_driver_profile_refused_before_file_restore(self):
        import nvidia_presentation as n
        from test_nvidia_presentation import MemoryDrs, driver_store
        store=driver_store()
        with patch.object(n,'Drs',side_effect=lambda:MemoryDrs(store)):
            uw.install(argparse.Namespace(game_dir=self.game,build=self.build,save_dir=self.saves,nvidia_dxgi=True))
            backup=next(self.game.glob('ftl-ultrawide-backup-*'))
            plan=uw.read_json(backup/'manifest.json')['nvidiaPresentation']
            store['profiles'][plan['profileName']]['settings']=2
            before={n:(self.game/n).read_bytes() for n in uw.PAYLOAD}
            with self.assertRaisesRegex(ValueError,'customized'):
                uw.restore_backup(backup)
            for name,data in before.items():self.assertEqual((self.game/name).read_bytes(),data)

if __name__=='__main__':unittest.main()
