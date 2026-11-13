#!/usr/bin/env python3
"""Native ASan/UBSan + ARM64 shared-library host tests, offline only."""
import hashlib,json,os,shutil,sys
import build
os.environ['ASAN_OPTIONS']='detect_leaks=1:halt_on_error=1'
os.environ['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
target=sys.argv[1] if len(sys.argv)>1 else 'native'
san=target=='native'
objects=build.build(target,san)
dest=build.ROOT/'build'/(target+('-san' if san else ''))
f=build.flags(target,san);cc=build.compiler(target)
prefix=[]
if target=='aarch64':
    qemu=os.environ.get('QEMU_AARCH64') or shutil.which('qemu-aarch64') or str(build.ROOT.parent/'mpc-vst-jc303/.tools/qemu/usr/bin/qemu-aarch64')
    sysroot=build.LOCAL/'aarch64-linux-gnu'
    if not sysroot.exists():sysroot=build.Path('/usr/aarch64-linux-gnu')
    prefix=[qemu,'-L',sysroot]
for source,name in [(build.MPC/'tools/host_test.c','host_test'),(build.ROOT/'tests/engine_test.c','engine_test')]:
    if name=='host_test':
        obj=dest/'host_test.o'
        build.run([cc,*f,'-Dmain=plugin_test_main','-c',source,'-o',obj])
        binary=dest/name
        build.run([cc,*f,obj,'tests/load_plugin.c','-ldl','-lpthread','-lm','-o',binary])
        build.run([*prefix,binary,dest/build.CFG['so']])
    else:
        binary=dest/name
        build.run([cc,*f,source,*objects,'-lpthread','-lm','-ldl','-Wl,--gc-sections','-o',binary])
        build.run([*prefix,binary])
(dest/'tested.json').write_text(json.dumps({'sha256':hashlib.sha256((dest/build.CFG['so']).read_bytes()).hexdigest(),'environment':target+' offline'+(' ASan/UBSan/leaks' if san else ' QEMU; not a device benchmark'),'tests':['actual VST shared library host','independent DSP oracle','state, presets, voice/cache lifetime, multi-instance isolation']},indent=2)+'\n')
