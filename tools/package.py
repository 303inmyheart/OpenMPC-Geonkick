#!/usr/bin/env python3
"""Local development installer only. Does not install, publish or touch a device."""
import argparse,hashlib,json,os,re,shutil,subprocess,sys,zipfile
from pathlib import Path
import build
root=build.ROOT
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--desktop',action='store_true',help='Explicitly copy installer to Desktop');ap.add_argument('--no-desktop',action='store_true',help=argparse.SUPPRESS);args=ap.parse_args()
cfg=json.loads((root/'release.json').read_text())
release_tools=Path(os.environ.get('MPC_RELEASE_TOOLS',root.parent/cfg.get('tools_dir','mpc-vst-plugins-release-tools')))
ref=subprocess.check_output(['git','-C',str(release_tools),'rev-parse','HEAD'],text=True).strip()
assert ref==cfg['tools_ref'],'Unexpected release tooling revision'
for target in ('native-san','aarch64'):
    directory=root/'build'/target;binary=directory/build.CFG['so']
    receipt=json.loads((directory/'tested.json').read_text())
    assert hashlib.sha256(binary.read_bytes()).hexdigest()==receipt['sha256'],'Run tools/test.py '+target
arm=root/'build/aarch64'/build.CFG['so']
assert 'AArch64' in subprocess.check_output(['readelf','-h',str(arm)],text=True)
versions=re.findall(r'GLIBC_(\d+\.\d+)',subprocess.check_output(['readelf','-V',str(arm)],text=True))
assert max(tuple(map(int,v.split('.'))) for v in versions)<=(2,39)
symbols=subprocess.check_output(['readelf','--dyn-syms','-W',str(arm)],text=True)
exports=[x.split()[-1] for x in symbols.splitlines() if ' GLOBAL ' in x and ' UND ' not in x]
assert exports==['VSTPluginMain'],exports
folder=f"{build.CFG['vendor']} - VST - {build.CFG['name']}"
skin=root/'build/skin'/folder
assert (skin/'Presets/0000-Init.xpl').exists()
stage=root/'build/package';stage.mkdir(parents=True,exist_ok=True)
binary=stage/arm.name;shutil.copy2(arm,binary)
strip=shutil.which('aarch64-linux-gnu-strip') or str(build.LOCAL/'bin/aarch64-linux-gnu-strip')
build.run([strip,'--strip-unneeded',binary])
notice=stage/'LICENSES.txt'
notice.write_text('\n\n'.join([
    f'Geonkick MPC {cfg["version"]} DEVELOPMENT PROTOTYPE — GPL-3.0-or-later\nBased on Geonkick 3.9.0, Iurie Nistor and contributors.\nUpstream: https://github.com/quamplex/geonkick\nMPC-app acceptance testing pending. Public adapted-source publication is pending;\nthe source_repo manifest URL is reserved for a future public fork.\nLocal corresponding source and build scripts are supplied in the project.',
    (root/'LICENSE').read_text(),
    'OpenMPC VST wrapper — MIT\n'+(build.MPC/'LICENSE').read_text(),
    'Artwork rendered using DejaVu Sans:\n'+(root/'art/FONT-LICENSE.txt').read_text(),
]))
dest=root/'build/releases'
build.run([sys.executable,release_tools/'tools/release.py','--so',binary,'--skin',skin,
           '--entry',root/'build/pluginlist-entry.xml','--version',cfg['version'],
           '--id',cfg['id'],'--repo',cfg['repo'],'--license',cfg['license'],
           '--requires',cfg['requires'],'--about',cfg['about'],'--user-data','.sample-folder','--extra',str(notice)+':LICENSES.txt','-o',dest])
archive=dest/f'Geonkick-MPC-{cfg["version"]}-mpc-aarch64.zip'
build.run([sys.executable,release_tools/'tools/catalog_check.py',archive,'--catalog','--require-portable','--expect-id',cfg['id'],'--expect-repo',cfg['repo']])
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    top=z.namelist()[0].split('/')[0]
    for name in ['install.sh','uninstall.sh','plugin_list.awk','mpc-plugin.json','SHA256SUMS']:
        assert top+'/'+name in z.namelist()
    for line in z.read(top+'/SHA256SUMS').decode().splitlines():
        sha,name=line.split('  ',1);assert hashlib.sha256(z.read(top+'/'+name)).hexdigest()==sha
    assert sum(n.endswith('.so') for n in z.namelist())==1
    akai=json.loads(z.read(top+'/portable/'+folder+'/plugin.json'));assert akai['format']=='vst2' and akai['file64']==build.CFG['so']
    assert not any(n.endswith(('.c','.h','.o','.py','.log','.ttf')) for n in z.namelist())
digest=hashlib.sha256(archive.read_bytes()).hexdigest()
(dest/'SHA256SUMS').write_text(digest+'  '+archive.name+'\n')
print('Validated local development package:',archive,'SHA256:',digest)
if args.desktop and not args.no_desktop:
    desktop=Path.home()/'Schreibtisch';desktop.mkdir(exist_ok=True)
    out=desktop/f'Geonkick-MPC-Gen2-{cfg["version"]}-dev1.zip'
    if out.exists():raise SystemExit('Refusing to overwrite '+str(out))
    shutil.copy2(archive,out);print('Installer:',out)
