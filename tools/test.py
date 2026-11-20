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
for source,name in [(build.MPC/'tools/host_test.c','host_test'),(build.ROOT/'tests/controls_test.c','controls_test'),(build.ROOT/'tests/realtime_test.c','realtime_test'),(build.ROOT/'tests/ui_load_test.c','ui_load_test'),(build.ROOT/'tests/touch_test.c','touch_test'),(build.ROOT/'tests/engine_test.c','engine_test'),(build.ROOT/'tests/point_editor_test.c','point_editor_test'),(build.ROOT/'tests/free_envelope_test.c','free_envelope_test'),(build.ROOT/'tests/filter_test.c','filter_test'),(build.ROOT/'tests/kit_test.c','kit_test'),(build.ROOT/'tests/sample_data_test.c','sample_data_test'),(build.ROOT/'tests/sample_engine_test.c','sample_engine_test'),(build.ROOT/'tests/sample_vst_test.c','sample_vst_test'),(build.ROOT/'tests/pad_mapping_test.c','pad_mapping_test'),(build.ROOT/'tests/kit_controls_test.c','kit_controls_test'),(build.ROOT/'tests/edge_test.c','edge_test'),(build.ROOT/'tests/focus_echo_test.c','focus_echo_test'),(build.ROOT/'tests/free_chunk_test.c','free_chunk_test'),(build.ROOT/'tests/idle_action_test.c','idle_action_test'),(build.ROOT/'tests/filter_controls_test.c','filter_controls_test')]:
    if name in ('host_test','controls_test','realtime_test','ui_load_test','touch_test','focus_echo_test','free_chunk_test','idle_action_test','filter_controls_test','kit_controls_test','sample_vst_test'):
        if name=='host_test' and build.CFG['defines'].get('ASYNC_HOST_NOTIFY'):
            # The original framework suite assumes synchronous notification.
            # Allow the same checks to await our actual asynchronous UI path.
            source_text=source.read_text().replace('for (int k = 0; k < blocks; k++) a->pr(a, ip, o, 128);',
                'for (int k = 0; k < blocks; k++) a->pr(a, ip, o, 128); usleep(30000);')
            source=dest/'host_test_async.c';source.write_text(source_text)
        obj=dest/(name+'.o')
        build.run([cc,*f,'-Dmain=plugin_test_main','-c',source,'-o',obj])
        binary=dest/name
        build.run([cc,*f,obj,'tests/load_plugin.c','-ldl','-lpthread','-lm','-o',binary])
        build.run([*prefix,binary,dest/build.CFG['so']])
    else:
        binary=dest/name
        extra=['tests/sample_rt_guard.c','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free,--wrap=fopen,--wrap=opendir'] if name=='sample_engine_test' else []
        build.run([cc,*f,source,*objects,*extra,'-lpthread','-lm','-ldl','-Wl,--gc-sections','-o',binary])
        build.run([*prefix,binary])
(dest/'tested.json').write_text(json.dumps({'sha256':hashlib.sha256((dest/build.CFG['so']).read_bytes()).hexdigest(),'environment':target+' offline'+(' ASan/UBSan/leaks' if san else ' QEMU; not a device benchmark'),'tests':['actual VST shared library host','nine fine frequency adapters, wheel/Q-Link events, echoes and old automation','independent DSP oracle','state, presets, voice/cache lifetime, multi-instance isolation']},indent=2)+'\n')
