#!/usr/bin/env python3
"""Local native/ARM64 and skin builds. Reuses the neighbouring MPC wrapper.
MPC_VST, MPC_TOOLCHAIN and QEMU_AARCH64 can select external tools.
"""
import json,os,shutil,subprocess,sys,fcntl
from contextlib import contextmanager
from pathlib import Path
import spec

ROOT=Path(__file__).resolve().parents[1]
MPC=Path(os.environ.get('MPC_VST',ROOT.parent/'mpc-vst-plugins')).resolve()
LOCAL=Path(os.environ.get('MPC_TOOLCHAIN',ROOT.parent/'mpc-vst-jc303/.tools/toolchain/usr')).resolve()
CFG=json.loads((ROOT/'vst.json').read_text())
@contextmanager
def platform_lock():
    (ROOT/'build').mkdir(exist_ok=True)
    with (ROOT/'build/.platform-build.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        yield

def run(args,**kw):
    env=os.environ.copy()
    env['LD_LIBRARY_PATH']=str(LOCAL/'lib/x86_64-linux-gnu')+':'+env.get('LD_LIBRARY_PATH','')
    env['PYTHONPATH']=str(LOCAL/'lib/python3/dist-packages')+':'+env.get('PYTHONPATH','')
    env['SHADOW_TITLE_FONT']=str(ROOT/CFG['title_font'])
    subprocess.run([str(a) for a in args],cwd=ROOT,env=env,check=True,**kw)
def compiler(target):
    if target=='native':return os.environ.get('CC','gcc')
    return os.environ.get('AARCH64_CC') or shutil.which('aarch64-linux-gnu-gcc') or shutil.which('aarch64-linux-gnu-gcc-13') or str(LOCAL/'bin/aarch64-linux-gnu-gcc-13')
def flags(target,sanitize=False):
    f=['-std=gnu11','-fPIC','-fvisibility=hidden','-Ibuild','-I'+str(MPC/'wrapper'),'-Isrc/dsp','-Isrc/dsp/quamplex_dsp_tools',
       '-DGEONKICK_VERSION_HEX=0x030900','-DGEONKICK_OS_GNU','-Wall','-Wextra','-Wno-unused-parameter','-ffunction-sections','-fdata-sections']
    f+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitize else ['-O2']
    if target=='aarch64' and str(LOCAL) in compiler(target):f+=['--sysroot='+str(LOCAL.parent),'-B'+str(LOCAL/'bin')+'/']
    return f
def prepare():
    spec.generate()
    if not (MPC/'wrapper/audio_format.h').exists():raise SystemExit('Apply the JC303 float ABI patch to the MPC toolchain first.')
    if "Fine sub-step gestures" not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-fine-int-sweep.patch after mpc-float32.patch first.')
    if 'ASYNC_HOST_NOTIFY' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-async-host-notify.patch first.')
    if 'LIVE_PARAMS_READONLY' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-display-parameter-budget.patch first.')
    if 'GRAPH_INVALIDATION_ONLY' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-coalesced-graph-and-fine-input.patch first.')
    if 'GRAPH_REBIND_ON_REFRESH' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-graph-rebind-and-input-observation.patch first.')
    if 'FINE_TRACK_RAW_INPUT' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-accumulated-fine-input.patch first.')
    if 'CANONICAL_CONTINUOUS_ECHO' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-canonical-continuous-echo.patch after mpc-accumulated-fine-input.patch.')
    if 'ENGINE_CHUNK_CAPACITY' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-envelope-state-capacity.patch.')
    if 'IDLE_ACTION_RELEASE' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-idle-action-release.patch.')
    if 'FILTER_CONTROL_FEEDBACK' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-filter-control-feedback.patch.')
    if 'CONTROL_CONTEXT_REV' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-control-context.patch.')
    if 'DYNAMIC_PARAM_UNITS' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-sample-dynamic-units.patch last.')
    if 'UI_DISPLAY_CONTEXT' not in (MPC/'wrapper/vst2_wrap.c').read_text():raise SystemExit('Apply patches/mpc-native-display-context.patch last.')
    run([sys.executable,MPC/'tools/gen_vst.py','vst.json','--params-h'])
def build(target,sanitize=False):
    prepare()
    dest=ROOT/'build'/(target+('-san' if sanitize else ''));dest.mkdir(parents=True,exist_ok=True)
    objects=[]
    for src in CFG['build']['sources']+[str(MPC/'wrapper/vst2_wrap.c')]:
        obj=dest/(Path(src).stem+'.o');run([compiler(target),*flags(target,sanitize),'-c',src,'-o',obj]);objects.append(obj)
    run([compiler(target),*flags(target,sanitize),'-shared',*objects,'-lpthread','-lm','-ldl','-Wl,--no-undefined','-Wl,--gc-sections','-Wl,--version-script=src/exports.map','-o',dest/CFG['so']])
    print('Built',dest/CFG['so'],flush=True);return objects
def skin():
    prepare()
    run([sys.executable,'tools/tile.py'])
    run([sys.executable,'tools/plot_assets.py'])
    run(['gcc','-O2','-I'+str(MPC/'tools/vendor/force-shadow/tools'),MPC/'tools/shadow_art.c','-lm','-o','build/shadow_art'])
    generated=ROOT/'build/skin'/f"{CFG['vendor']} - VST - {CFG['name']}"
    if generated.exists():shutil.rmtree(generated)
    run([sys.executable,MPC/'tools/gen_vst.py','vst.json'])
    run([sys.executable,'tools/finish_skin.py'])
    run([sys.executable,'tools/navigation.py'])
    run([sys.executable,'tools/skin_design.py'])
    run([sys.executable,'tools/check_point_skin.py'])
    run([sys.executable,'tools/check_navigation.py'])
    folder=ROOT/'build/skin'/f"{CFG['vendor']} - VST - {CFG['name']}"/'Plugin Skins'
    run([sys.executable,MPC/'tools/studio.py','preview',folder,'-o',ROOT/'build/preview_%d.png'])
if __name__=='__main__':
    target=sys.argv[1] if len(sys.argv)>1 else 'all'
    with platform_lock():
        if target in ('native','all'):build('native')
        if target in ('aarch64','all'):build('aarch64')
        if target in ('skin','all'):skin()
