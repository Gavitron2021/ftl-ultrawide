"""Build/install FTL Ultrawide from player-owned files. No network operations.

Python 3.11+ and Pillow are required for build; install/restore use the stdlib.
All paths are supplied by the player. Build never writes to the game or saves.
"""
from __future__ import annotations
import argparse
import ctypes
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from datetime import datetime, timezone
import xml.etree.ElementTree as ET
import zipfile
import zlib

ROOT = Path(__file__).resolve().parent
STEAM_169 = '76df0aeddada8be9cabefd917964c9603c1bb5d4c08943362a463050996a746a'
STEAM_1614 = '216ab7ec04af5c9c5d09c9c449d409a4cf0501e6c09c8cf252f650d5674d634c'
VANILLA_DAT = 'd7c4744f91f96303c0342b30c2dcb63843050e0f489426200c683d02fb8abc7d'
PAYLOAD = {'FTLGame.exe', 'FTL-Ultrawide.exe', 'FTLGame-1.6.9.exe', 'ftl.dat',
           'Hyperspace.classic.dll', 'Hyperspace.dll', 'xinput1_4.dll',
           'settings.ini', 'ultrawide-runtime.cfg', 'steam_appid.txt'}
MUTABLE = {'settings.ini', 'Hyperspace.dll', 'ultrawide-runtime.cfg'}

def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def file_hash(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def require(condition, message):
    if not condition:
        raise ValueError(message)

def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))

def write_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8')

def local_path(root: Path, name: str) -> Path:
    path = (root/name).resolve()
    require(path.is_relative_to(root.resolve()), f'Path escapes selected directory: {name}')
    return path

class Archive:
    """Read the supported SIL/PKG archive without writing to it."""
    def __init__(self, filename):
        self.filename = Path(filename)
        self.entries = {}
        with self.filename.open('rb') as stream:
            magic, header, entry, count, names_size = struct.unpack('>4sHHII', stream.read(16))
            require((magic,header,entry)==(b'PKG\n',16,20), 'Unsupported FTL archive')
            require(count<=100000 and names_size<=0x1000000, 'Invalid archive index')
            index, names = stream.read(count*20), stream.read(names_size)
            for at in range(0,len(index),20):
                _, flags, offset, size, unpacked = struct.unpack_from('>5I',index,at)
                if not offset: continue
                name_at = flags & 0xffffff
                end = names.find(b'\0',name_at)
                require(end>=0,'Invalid archive path')
                name = names[name_at:end].decode('ascii')
                require(name not in self.entries,'Duplicate archive resource')
                self.entries[name] = offset,size,unpacked,flags>>24

    def read(self,name):
        offset,size,unpacked,flags = self.entries[name]
        require(flags in (0,1),'Unknown compression flag')
        with self.filename.open('rb') as stream:
            stream.seek(offset)
            data = stream.read(size)
        require(len(data)==size,'Truncated archive resource')
        if flags: data = zlib.decompress(data)
        require(len(data)==unpacked,'Unexpected resource size')
        return data

def apply_bps(source: bytes, patch: bytes) -> bytes:
    """BPS1 decoder, based on byuu's public-domain file-format specification.

    https://github.com/Alcaro/Flips/blob/master/bps_spec.md
    Includes all three CRC32 checks, bounds checks, and caller SHA-256 checks.
    """
    require(patch[:4]==b'BPS1' and len(patch)>=16,'Invalid rollback patch')
    source_crc,target_crc,patch_crc = struct.unpack('<III',patch[-12:])
    require(zlib.crc32(source)==source_crc,'Rollback source CRC mismatch')
    require(zlib.crc32(patch[:-4])==patch_crc,'Rollback patch CRC mismatch')
    pos = 4
    def number():
        nonlocal pos
        data,shift = 0,1
        while True:
            require(pos<len(patch)-12 and shift<1<<63,'Invalid BPS number')
            byte=patch[pos]; pos+=1
            data+=(byte&127)*shift
            if byte&128: return data
            shift<<=7; data+=shift
    source_size,target_size,metadata_size = number(),number(),number()
    require(source_size==len(source) and 0<target_size<=128*1024*1024,'Unexpected rollback sizes')
    pos+=metadata_size
    require(pos<=len(patch)-12,'Invalid rollback metadata')
    out=bytearray(); source_at=target_at=0
    while pos<len(patch)-12:
        command=number(); action,length=command&3,(command>>2)+1
        require(len(out)+length<=target_size,'Rollback output overflow')
        if action==0:
            require(len(out)+length<=len(source),'SourceRead overflow')
            out.extend(source[len(out):len(out)+length])
        elif action==1:
            require(pos+length<=len(patch)-12,'TargetRead overflow')
            out.extend(patch[pos:pos+length]); pos+=length
        else:
            relative=number()
            offset=-(relative>>1) if relative&1 else relative>>1
            if action==2:
                source_at+=offset
                require(0<=source_at and source_at+length<=len(source),'SourceCopy overflow')
                out.extend(source[source_at:source_at+length]); source_at+=length
            else:
                target_at+=offset
                require(0<=target_at<len(out),'TargetCopy underflow')
                for _ in range(length):
                    out.append(out[target_at]); target_at+=1
    require(len(out)==target_size and zlib.crc32(out)==target_crc,'Rollback target verification failed')
    return bytes(out)

def discover_dependencies(directory, ftlman=None):
    directory=Path(directory).resolve()
    require(directory.is_dir(),'Dependency directory does not exist')
    result={}
    for key,spec in read_json(ROOT/'dependencies.json').items():
        candidates=[Path(ftlman).resolve()] if key=='ftlman' and ftlman else list(directory.rglob(spec['filename']))
        matches=[p for p in candidates if p.is_file() and file_hash(p)==spec['sha256']]
        require(matches, f'Missing or incorrect dependency: {spec["filename"]}. See docs/INSTALL.md.')
        result[key]=sorted(matches)[0]
    return result

def prepare_hyperspace(original):
    recipe=read_json(ROOT/'hyperspace-patches.json')
    require(digest(original)==recipe['originalSha256'],'Unexpected Hyperspace DLL')
    require(original.count(b'\0-opengl\0')==1,'Legacy OpenGL flag check changed')
    patched=original.replace(b'\0-opengl\0',b'\0-oldgl_\0')
    logger=recipe['logger']; offset=logger['fileOffset']
    old,new=bytes.fromhex(logger['originalBytes']),bytes.fromhex(logger['patchedBytes'])
    require(patched[offset:offset+len(old)]==old,'Hyperspace logger prologue changed')
    patched=patched[:offset]+new+patched[offset+len(old):]
    for item in recipe['uiSignatureWorkarounds']:
        old,new=item['old'].encode()+b'\0',item['new'].encode()+b'\0'
        require(patched.count(old)==1 and len(old)==len(new),'Hyperspace mouse signature changed')
        patched=patched.replace(old,new)
    require(digest(patched)==recipe['patchedSha256'],'Patched Hyperspace hash mismatch')
    return patched

def production_config(original):
    tree=ET.fromstring(original)
    require(tree.tag=='FTL','Unexpected Hyperspace XML')
    tree.find('saveFile/prefix').text='ultrawide'
    tree.find('saveFile/inheritMode').text='0'
    ET.SubElement(tree.find('scripts'),'script').text='data/ultrawide.lua'
    for tag in ['preIgniteChargers','rightClickDoorOpening','hullNumbers','hackingDroneFix',
                'repairDroneRecoveryFix','crystalShardFix','altCreditSystem',
                'advancedCrewTooltips','titleScreen','console']:
        node=tree.find(tag)
        require(node is not None,f'Missing Hyperspace setting: {tag}')
        node.set('enabled','false')
    return ET.tostring(tree,encoding='utf-8',xml_declaration=True)

def stage_art(mod, vanilla, dependencies):
    from PIL import Image, ImageOps
    zips={k:zipfile.ZipFile(dependencies[k]) for k in ['highres','asteroids']}
    def save(name,image):
        path=local_path(mod,name); path.parent.mkdir(parents=True,exist_ok=True)
        image.save(path)
    try:
        for spec in read_json(ROOT/'assets/scenes.json'):
            path=local_path(ROOT,spec['image'])
            require(file_hash(path)==spec['sha256'],'Scene file changed')
            image=Image.open(path).convert('RGB')
            require(image.size==(1720,720) and digest(image.tobytes())==spec['pixelSha256'],'Scene dimensions/pixels changed')
            save('img/ultrawide/'+path.name,image)
            save(spec['source'],image.crop((220,0,1500,720)))
            if spec['source']=='img/stars/bg_darknebula.png':
                save('img/ultrawide/credits_1.png',image)
        for spec in read_json(ROOT/'assets/texture-patches.json'):
            raw=vanilla.read(spec['source']) if spec['sourceKind']=='vanilla' else zips[spec['sourceKind']].read(spec['source'])
            require(digest(raw)==spec['sourceSha256'],'Original texture version mismatch')
            original=Image.open(io.BytesIO(raw)).convert('RGBA')
            patch=local_path(ROOT,spec['delta'])
            require(file_hash(patch)==spec['deltaSha256'],'Texture patch changed')
            if spec['method']=='menu':
                center=original.copy()
                if spec['source'].endswith('main_base2.png'):
                    center.paste(center.crop((1,0,2,720)),(0,0))
                    center.paste(center.crop((1278,0,1279,720)),(1279,0))
                    save(spec['source'],center)
                base=ImageOps.expand(center,(220,0,220,0),fill=(0,0,0,0))
                # The baseline uses the unmodified input edge; center cleanup
                # above applies only to the two one-pixel title fringes.
                base.paste(original.crop((0,0,1,720)).resize((220,720)),(0,0))
                base.paste(original.crop((1279,0,1280,720)).resize((220,720)),(1500,0))
                before=base.crop((0,0,220,720)).tobytes()+base.crop((1500,0,1720,720)).tobytes()
            else:
                base=original.resize(tuple(x*4 for x in original.size),Image.Resampling.NEAREST)
                before=base.tobytes()
            delta=zlib.decompress(patch.read_bytes())
            require(len(delta)==len(before),'Texture patch size mismatch')
            after=bytes((a+b)&255 for a,b in zip(before,delta))
            if spec['method']=='menu':
                size=220*720*4
                base.paste(Image.frombytes('RGBA',(220,720),after[:size]),(0,0))
                base.paste(Image.frombytes('RGBA',(220,720),after[size:]),(1500,0))
            else:
                base=Image.frombytes('RGBA',tuple(spec['dimensions']),after)
            require(list(base.size)==spec['dimensions'] and digest(base.tobytes())==spec['pixelSha256'],'Rebuilt texture differs from approved pixels')
            save(spec['target'],base)
    finally:
        for archive in zips.values(): archive.close()

def configure_settings(text):
    values={'fullscreen':'4','last_fullscreen':'4','manual':'0','screen_x':'0','screen_y':'0',
            'windowed':'0','stretched':'0'}
    seen=set(); lines=[]
    for line in text.splitlines():
        key=line.split('=',1)[0].strip()
        if key in values:
            line=key+'='+values[key]; seen.add(key)
        lines.append(line)
    lines.extend(k+'='+v for k,v in values.items() if k not in seen)
    return '\n'.join(lines)+'\n'

def verify_archive(path, original):
    from PIL import Image
    archive=Archive(path)
    config=ET.fromstring(archive.read('data/hyperspace.xml'))
    require(config.findtext('saveFile/prefix')=='ultrawide' and config.findtext('saveFile/inheritMode')=='0','Production profile import is not enabled')
    require(config.find('console').get('enabled')=='false','Developer console must be disabled')
    lua=archive.read('data/ultrawide.lua')
    require(all(x not in lua for x in [b'F9',b'review.active',b'UW_QA',b'io.open(\'ultrawide-art.log']), 'Testing artifacts found')
    for spec in read_json(ROOT/'assets/scenes.json'):
        full=Image.open(io.BytesIO(archive.read('img/ultrawide/'+Path(spec['image']).name))).convert('RGB')
        crop=Image.open(io.BytesIO(archive.read(spec['source']))).convert('RGB')
        require(digest(full.tobytes())==spec['pixelSha256'] and crop.tobytes()==full.crop((220,0,1500,720)).tobytes(),'Background panorama/crop mismatch')
    for spec in read_json(ROOT/'assets/texture-patches.json'):
        image=Image.open(io.BytesIO(archive.read(spec['target']))).convert('RGBA')
        require(digest(image.tobytes())==spec['pixelSha256'],'Rebuilt asset pixel mismatch')
    prefixes=('img/ship/','img/people/','img/weapons/')
    checked=0
    for name in original.entries:
        if name.startswith(prefixes) and name.endswith('.png'):
            require(archive.read(name)==original.read(name),f'Unexpected base sprite change: {name}')
            checked+=1
    return {'scenes':31,'texturePatches':6,'baseSpritePathsUnchanged':checked,
            'profilePrefix':'ultrawide','vanillaProfileInheritance':True,
            'developerConsole':False,'artworkReviewRemoved':True}

def build(args):
    game=Path(args.game_dir).resolve(); out=Path(args.out).resolve()
    require(not out.exists(),'Output directory already exists; choose a new empty build location')
    require(not out.is_relative_to(game),'Build output must be outside the game installation')
    require(file_hash(game/'ftl.dat')==VANILLA_DAT,'Requires the verified pristine Steam ftl.dat. Restore vanilla before building.')
    exe=(game/'FTLGame.exe').read_bytes()
    require(digest(exe) in (STEAM_169,STEAM_1614),'Only verified Steam 1.6.14 or rolled-back Steam 1.6.9 is supported')
    deps=discover_dependencies(args.dependencies,args.ftlman)
    launcher=Path(args.launcher).resolve()
    require(launcher.is_file(),'Build/download our launcher first; see docs/BUILD.md')
    binary=launcher.read_bytes()
    require(binary[:2]==b'MZ','Launcher is not a Windows executable')
    if digest(exe)==STEAM_1614:
        exe=apply_bps(exe,deps['rollback'].read_bytes())
    require(digest(exe)==STEAM_169,'Rollback executable SHA-256 mismatch')
    out.mkdir(parents=True)
    payload=out/'payload'; payload.mkdir()
    mods=out/'local-mods'; mods.mkdir()
    mod=mods/'ultrawide'; shutil.copytree(ROOT/'mod',mod)
    vanilla=Archive(game/'ftl.dat')
    stage_art(mod,vanilla,deps)
    shiny=mods/'shiny-ui'; shiny.mkdir()
    with zipfile.ZipFile(deps['shiny']) as archive:
        for name in read_json(ROOT/'shiny-files.json'):
            target=local_path(shiny,name); target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(archive.read(name))
    config=mods/'config/data'; config.mkdir(parents=True)
    with zipfile.ZipFile(deps['hyperspace_mod']) as archive:
        (config/'hyperspace.xml').write_bytes(production_config(archive.read('data/hyperspace.xml')))
    shutil.copy2(game/'ftl.dat',payload/'ftl.dat')
    # The manager receives only the disposable payload directory, never the
    # user's live game directory. Archive and DLL changes are completely local.
    command=[str(deps['ftlman']),'patch','--data-dir',str(payload),
             str(deps['hyperspace_mod']),str(deps['highres']),str(deps['asteroids']),
             str(shiny),str(deps['combat']),str(config.parent),str(mod)]
    subprocess.run(command,check=True,cwd=out)
    verification=verify_archive(payload/'ftl.dat',vanilla)
    dll=prepare_hyperspace(deps['hyperspace_dll'].read_bytes())
    for name in ['Hyperspace.dll','Hyperspace.classic.dll']:(payload/name).write_bytes(dll)
    shutil.copy2(deps['proxy'],payload/'xinput1_4.dll')
    (payload/'FTLGame-1.6.9.exe').write_bytes(exe)
    for name in ['FTLGame.exe','FTL-Ultrawide.exe']: (payload/name).write_bytes(binary)
    settings=(game/'settings.ini').read_text(encoding='utf-8-sig') if (game/'settings.ini').exists() else 'dlcEnabled=1\nlowend=0\nlanguage=en\n'
    (payload/'settings.ini').write_text(configure_settings(settings),encoding='utf-8')
    (payload/'ultrawide-runtime.cfg').write_text('enabled=1\nwidth=1720\n',encoding='ascii')
    (payload/'steam_appid.txt').write_text('212680\n',encoding='ascii')
    # ftlman intentionally creates a local pristine backup. It is for this
    # player's build only, and never belongs in the installation payload.
    if (payload/'ftl.dat.vanilla').exists():(payload/'ftl.dat.vanilla').unlink()
    require({p.name for p in payload.iterdir()}==PAYLOAD,'Unexpected build payload file')
    write_json(out/'build.json',{'version':'0.1.0-beta.2','sourceExeSha256':file_hash(game/'FTLGame.exe'),
               'sourceArchiveSha256':VANILLA_DAT,'verification':verification,
               'files':{p.name:file_hash(p) for p in sorted(payload.iterdir())}})
    print(json.dumps({'built':str(out),'verification':verification},indent=2))
    print('Local build only. To install, close FTL and use the install command. Do not upload this payload.')

def ensure_unlocked(paths):
    if os.name!='nt': return
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    create=kernel.CreateFileW; create.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p]
    create.restype=ctypes.c_void_p
    close=kernel.CloseHandle; close.argtypes=[ctypes.c_void_p]
    for path in paths:
        if not path.exists(): continue
        handle=create(str(path),0x80000000,0,None,3,0x80,None)
        require(handle not in (None,ctypes.c_void_p(-1).value),f'Close FTL and any process using {path.name} before continuing')
        close(handle)

def default_save_dir():
    require(os.name=='nt','An explicit --save-dir is required outside Windows')
    buffer=ctypes.create_unicode_buffer(32768)
    require(ctypes.windll.shell32.SHGetFolderPathW(None,5,None,0,buffer)==0,'Could not locate Documents; provide --save-dir')
    return Path(buffer.value)/'My Games/FasterThanLight'

def snapshots(directory):
    return {p.name:file_hash(p) for p in directory.glob('*.sav') if p.is_file()}

def validate_install(build_dir,game):
    report=read_json(build_dir/'build.json'); payload=build_dir/'payload'
    require(set(report['files'])==PAYLOAD,'Unexpected payload manifest')
    require(report['sourceArchiveSha256']==VANILLA_DAT and file_hash(game/'ftl.dat')==VANILLA_DAT,'Target must be the same pristine Steam archive used for the build')
    require(file_hash(game/'FTLGame.exe')==report['sourceExeSha256'],'Game executable differs from the build source')
    require(report['sourceExeSha256'] in (STEAM_169,STEAM_1614),'Unsupported source game')
    for name,expected in report['files'].items():
        require(file_hash(local_path(payload,name))==expected,f'Payload changed: {name}')
    require(file_hash(payload/'FTLGame-1.6.9.exe')==STEAM_169,'Native executable changed')
    ensure_unlocked([game/name for name in PAYLOAD])
    return report,payload

def restore_backup(backup,automatic=False):
    backup=backup.resolve(); manifest=read_json(backup/'manifest.json')
    game=Path(manifest['gameDirectory']).resolve()
    require(backup.parent==game and backup.name.startswith('ftl-ultrawide-backup-'),'Backup is not inside its recorded game directory')
    records=manifest['files']
    require({item['name'] for item in records}==PAYLOAD and len(records)==len(PAYLOAD),'Invalid backup file list')
    ensure_unlocked([local_path(game,item['name']) for item in records])
    for item in records:
        path=local_path(game,item['name'])
        if item['existed']:
            require(file_hash(local_path(backup/'files',item['name']))==item['beforeSha256'],'Original backup is damaged')
        elif path.exists() and item['name'] not in MUTABLE:
            require(file_hash(path)==item['installedSha256'],f'Added file changed: {item["name"]}; inspect it before restoring')
    for item in records:
        path=local_path(game,item['name'])
        if item['existed']:
            shutil.copy2(local_path(backup/'files',item['name']),path)
            require(file_hash(path)==item['beforeSha256'],'Restored file verification failed')
        elif path.exists(): path.unlink()
    manifest['status']='automatically-rolled-back' if automatic else 'restored'
    write_json(backup/'manifest.json',manifest)
    print('Original game files restored. Saves and backups were left untouched; normal diagnostic logs may remain.')

def install(args):
    game=Path(args.game_dir).resolve(); build_dir=Path(args.build).resolve()
    report,payload=validate_install(build_dir,game)
    saves=Path(args.save_dir).resolve() if args.save_dir else default_save_dir().resolve()
    before=snapshots(saves)
    stamp=datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S-%f')
    backup=game/('ftl-ultrawide-backup-'+stamp)
    (backup/'files').mkdir(parents=True); (backup/'saves').mkdir()
    for name in before:shutil.copy2(local_path(saves,name),backup/'saves'/name)
    require(snapshots(backup/'saves')==before,'Save backup verification failed')
    records=[]
    for name in sorted(PAYLOAD):
        path=local_path(game,name); existed=path.exists()
        old=file_hash(path) if existed else None
        if existed:
            shutil.copy2(path,backup/'files'/name)
            require(file_hash(backup/'files'/name)==old,'Original game backup verification failed')
        records.append({'name':name,'existed':existed,'beforeSha256':old,'installedSha256':report['files'][name]})
    manifest={'gameDirectory':str(game),'version':report['version'],'files':records,
              'saveFiles':before,'status':'backed-up','profile':'ultrawide','inheritMode':0}
    write_json(backup/'manifest.json',manifest)
    # Recheck after backup before the first installed byte is changed.
    validate_install(build_dir,game)
    try:
        for item in records:
            shutil.copy2(payload/item['name'],game/item['name'])
            require(file_hash(game/item['name'])==item['installedSha256'],'Installed file hash mismatch')
        require(snapshots(saves)==before,'Saves changed during installation; close the game and inspect the backup')
    except Exception:
        restore_backup(backup,automatic=True)
        raise
    manifest['status']='installed'; write_json(backup/'manifest.json',manifest)
    print('Installed. Launch through Steam. Check Stats and ship unlocks immediately after first launch.')
    print('Original ae_prof.sav remains untouched; Hyperspace imports it into hs_ultrawide_prof.sav on first launch.')
    print('This does not promise migration of an active vanilla Continue run. New mod progress does not sync back to vanilla.')
    print('Rollback command: python release/ftl_ultrawide.py restore --backup "'+str(backup)+'"')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='command',required=True)
    p=sub.add_parser('build',help='Build locally; do not change installed game/saves')
    p.add_argument('--game-dir',required=True); p.add_argument('--dependencies',required=True)
    p.add_argument('--ftlman'); p.add_argument('--launcher',default=str(ROOT/'bin/FTL-Ultrawide.exe'))
    p.add_argument('--out',required=True); p.set_defaults(run=build)
    p=sub.add_parser('install',help='Back up then install a verified local build')
    p.add_argument('--game-dir',required=True); p.add_argument('--build',required=True)
    p.add_argument('--save-dir'); p.set_defaults(run=install)
    p=sub.add_parser('restore',help='Restore game files; never restore/overwrite saves')
    p.add_argument('--backup',required=True); p.set_defaults(run=lambda a:restore_backup(Path(a.backup)))
    args=parser.parse_args()
    try: args.run(args)
    except (ValueError,OSError,KeyError,subprocess.CalledProcessError,zipfile.BadZipFile) as error:
        parser.exit(1,f'FTL Ultrawide: {error}\n')

if __name__=='__main__': main()
