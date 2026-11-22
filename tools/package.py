#!/usr/bin/env python3
"""Local development installer only. Does not install, publish or touch a device."""
import argparse,hashlib,json,os,re,shutil,subprocess,sys,zipfile
from pathlib import Path
import build
root=build.ROOT
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--with-benchmark',action='store_true',help='Attach the qualifying report only for its exact measured binary');ap.add_argument('--desktop',action='store_true',help='Explicitly copy installer to Desktop');ap.add_argument('--no-desktop',action='store_true',help=argparse.SUPPRESS);args=ap.parse_args()
cfg=json.loads((root/'release.json').read_text())
build.run([sys.executable,'tools/check_sound_overview.py'])
build.run([sys.executable,'tools/check_redesign.py'])
build.run([sys.executable,'tools/check_point_skin.py'])
build.run([sys.executable,'tools/check_native_presets.py'])
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
    f'Geonkick MPC {cfg["version"]} — GPL-3.0-or-later\nBased on Geonkick 3.9.0, Iurie Nistor and contributors.\nUpstream: https://github.com/quamplex/geonkick\nCorresponding modified source and build scripts: https://github.com/{cfg["repo"]}/tree/geonkick-mpc-v{cfg["version"]}\nFactory sound licences and modifications: Factory-Licenses.txt.\nSee repository for exact hardware-test scope and known limitations.',
    (root/'LICENSE').read_text(),
    'OpenMPC VST wrapper — MIT\n'+(build.MPC/'LICENSE').read_text(),
    'Artwork rendered using DejaVu Sans:\n'+(root/'art/FONT-LICENSE.txt').read_text(),
]))
dest=root/'build/releases'
bench=root/'benchmarks'/cfg['version']/'standard.txt'
include_bench=args.with_benchmark
bench_args=['--bench',bench] if include_bench else []
if include_bench:
    assert bench.exists(),'Measure this exact installer before attaching its benchmark'
    measurement=json.loads(bench.read_text().strip().splitlines()[-1])
    assert measurement['binary_sha256']==hashlib.sha256(binary.read_bytes()).hexdigest()
    assert measurement['verdict'] in ('PASS','WARN'),'Resolve failed release benchmark before packaging'
build.run([sys.executable,release_tools/'tools/release.py','--so',binary,'--skin',skin,
           '--entry',root/'build/pluginlist-entry.xml','--version',cfg['version'],
           '--id',cfg['id'],'--repo',cfg['repo'],'--license',cfg['license'],
           '--requires',cfg['requires'],'--about',cfg['about'],'--user-data','.sample-folder','--extra',str(notice)+':LICENSES.txt',*bench_args,'-o',dest])
archive=dest/f'Geonkick-MPC-{cfg["version"]}-mpc-aarch64.zip'
# The upstream --bench prose still says Gen1 even for an aarch64 package.
# Correct documentation only; retain current installer/uninstaller verbatim.
if include_bench:
    with zipfile.ZipFile(archive) as z:
        entries=[(info,z.read(info.filename)) for info in z.infolist()]
    top=entries[0][0].filename.split('/')[0]
    payload={info.filename:raw for info,raw in entries}
    name=top+'/INSTALL.md';text=payload[name].decode()
    old='Measured on a Gen1 device (MPC Live/One/X/Force class, Cortex-A17)'
    assert old in text
    text=text.replace(old,'Measured on MPC Live III (Gen2, Cortex-A76 core4, MPC OS3.10.1)')
    # Keep current script templates verbatim, but document OS3.10.1's native
    # folder/manager route first; root is needed only for the optional SSH route.
    start=text.index('## Requirements\n');end=text.index('## Install (scripted)\n',start)
    text=text[:start]+"""## Requirements

- Gen2 MPC (aarch64), MPC OS 3.10.1 or newer. Hardware tested on MPC Live III only.
- Enable Preferences > Enhancements > External Plugin Support.
- No Gen1/32-bit build is provided.

## Install (recommended)

Give the release ZIP directly to the OpenMPC Installer or Plugin Manager.
For the native folder route, unpack portable/OpenMPC - VST - Geonkick MPC/ into
VST2 Plugins/ at a drive's root. The folder route needs no SSH or settings edits.

The optional scripted/manual routes below require root SSH access. Save your
project before scripted installation, which restarts the MPC application.
For those routes, <your Synths folder> means a supported plugin location chosen
for the device, such as its VST2 Plugins folder. Use the recommended manager
route if unsure.

"""+text[end:]
    text=text.replace('As a rule of thumb, several instances run comfortably when p99 is under 15%.',
        'This standard audio-thread result alone does not predict multi-instance capacity; see the additional worker measurement below.')
    data_start=text.index('Your own files go in ');data_end=text.index('\n\nThen add ',data_start)
    text=text[:data_start]+"The remembered sample-folder setting is the `.sample-folder` file inside the plugin folder; it is preserved on upgrade. Imported sample audio is embedded in saved kits. Keep your own presets in the normal MPC user-data locations."+text[data_end:]

    stress=json.loads((bench.parent/'stress.json').read_text())
    paced=json.loads((bench.parent/'paced-worker.json').read_text())
    for report in (stress,paced):
        assert report['binary_sha256']==measurement['binary_sha256'] and report['version']==cfg['version']
    reports=f"https://github.com/{cfg['repo']}/tree/geonkick-mpc-v{cfg['version']}/benchmarks/{cfg['version']}"
    text+=f"\n## Additional workload checks\n\nThe standard CPU rating above does not cover the additional full-parameter sweep. That test is {stress['verdict']}: p99 {stress['p99_pct']}%, maximum {stress['max_pct']}% of one audio block.\n\nA separately paced measurement finds background synthesis up to {paced['threads_pct']}% of one CPU core. Audio-thread and one-core percentages are not the MPC project CPU meter. See the exact reports: {reports}.\n"

    payload[name]=text.encode()
    sums=top+'/SHA256SUMS';lines=[]
    for row in payload[sums].decode().splitlines():
        _,relative=row.split('  ',1);lines.append(hashlib.sha256(payload[top+'/'+relative]).hexdigest()+'  '+relative)
    payload[sums]=('\n'.join(lines)+'\n').encode()
    temporary=archive.with_suffix('.zip.stage')
    with zipfile.ZipFile(temporary,'w',compression=zipfile.ZIP_DEFLATED) as z:
        for info,_ in entries:z.writestr(info,payload[info.filename])
    temporary.replace(archive)
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
print('Validated MPC package:',archive,'SHA256:',digest)
if args.desktop and not args.no_desktop:
    desktop=Path.home()/'Schreibtisch';desktop.mkdir(exist_ok=True)
    out=desktop/f'Geonkick-MPC-Gen2-{cfg["version"]}-dev1.zip'
    if out.exists():raise SystemExit('Refusing to overwrite '+str(out))
    shutil.copy2(archive,out);print('Installer:',out)
