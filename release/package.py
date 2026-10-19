"""Create the public ZIP from the reviewed Git index, plus our own launcher."""
from pathlib import Path
import hashlib
import json
import subprocess
import zipfile

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'release/dist'
VERSION='0.1.0-beta.6'
ALLOWED_ROOT={'README.md','LICENSE','CREDITS.md','CHANGELOG.md','.gitignore'}
ALLOWED_DIR={'native','docs','release','.github'}
FORBIDDEN_DIR={'_test','_work','dependencies','dist','bin','__pycache__','backups','runtime','downloads','research'}
def allowed(name):
    p=Path(name)
    if p.is_absolute() or '..' in p.parts:return False
    if len(p.parts)==1:return name in ALLOWED_ROOT
    return p.parts[0] in ALLOWED_DIR and not set(p.parts)&FORBIDDEN_DIR and p.suffix.lower() not in {'.sav','.dll','.exe','.log','.bak','.ftl'} and not p.name.startswith('ftl.dat') and p.name not in {'settings.ini','ultrawide-runtime.cfg'}

def main():
    files=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')
    files=[f for f in files if f]
    rejected=[f for f in files if not allowed(f)]
    if rejected:raise SystemExit('Refusing unexpected public paths: '+repr(rejected))
    if len(files)<50:raise SystemExit('Stage the reviewed public source files before packaging')
    launcher=ROOT/'release/bin/FTL-Ultrawide.exe'
    if not launcher.is_file():raise SystemExit('Build the launcher first')
    OUT.mkdir(parents=True,exist_ok=True)
    target=OUT/f'FTL-Ultrawide-{VERSION}.zip'
    prefix=f'FTL-Ultrawide-{VERSION}/'
    with zipfile.ZipFile(target,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name in sorted(files):archive.write(ROOT/name,prefix+name)
        archive.write(launcher,prefix+'release/bin/FTL-Ultrawide.exe')
    checksum=hashlib.sha256(target.read_bytes()).hexdigest()
    (OUT/'SHA256SUMS.txt').write_text(checksum+'  '+target.name+'\n',encoding='ascii')
    print(json.dumps({'file':str(target),'sha256':checksum,'bytes':target.stat().st_size,'publicSourceFiles':len(files),'bundledGameFiles':False},indent=2))

if __name__=='__main__':main()
