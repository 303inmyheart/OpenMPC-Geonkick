#!/usr/bin/env python3
"""Exact-package Live III benchmark; only own /tmp files are written on device.
Requires an explicitly confirmed empty MPC project. Official host plus separately
labelled paced worker measurement. No installed files/settings/services changed.
"""
import argparse,hashlib,io,json,shlex,subprocess,tarfile,time,uuid,zipfile
from pathlib import Path
import build
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--host',required=True);ap.add_argument('--empty-project-confirmed',action='store_true',required=True);args=ap.parse_args()
root=build.ROOT;cfg=json.loads((root/'release.json').read_text());version=cfg['version'];tools=root.parent/cfg['tools_dir'];source=tools/'tools/bench.c'
assert subprocess.check_output(['git','-C',str(tools),'rev-parse','HEAD'],text=True).strip()==cfg['tools_ref']
work=root/'build/store-benchmark';work.mkdir(exist_ok=True)
wrapper=work/'main.c';wrapper.write_text('''#define _GNU_SOURCE
#include <sched.h>
#include <sys/resource.h>
#include <stdio.h>
extern int official_bench_main(int,char**);
int main(int argc,char**argv){cpu_set_t mask;CPU_ZERO(&mask);CPU_SET(4,&mask);
 if(sched_setaffinity(0,sizeof mask,&mask)||setpriority(PRIO_PROCESS,0,19)){perror("benchmark scheduling");return 2;}
 return official_bench_main(argc,argv);}
''')
original=source.read_text();paced=original.replace('    for (int b = 0; b < nblocks; b++) {','    struct timespec deadline;clock_gettime(CLOCK_MONOTONIC,&deadline);\n    for (int b = 0; b < nblocks; b++) {',1).replace('        ti.samplePos += BLOCK;','''        deadline.tv_nsec += (long)(1000000000.0*BLOCK/SR);
        if(deadline.tv_nsec>=1000000000){deadline.tv_nsec-=1000000000;deadline.tv_sec++;}
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&deadline,NULL);
        ti.samplePos += BLOCK;''',1)
assert original!=paced
(work/'paced.c').write_text(paced)
cc=build.compiler('aarch64');flags=build.flags('aarch64')
for label,path in [('official',source),('paced',work/'paced.c')]:
 obj=work/(label+'.o');build.run([cc,*flags,'-Dmain=official_bench_main','-c',path,'-o',obj]);build.run([cc,*flags,wrapper,obj,'-ldl','-lm','-o',work/label])
archive=root/f'build/releases/Geonkick-MPC-{version}-mpc-aarch64.zip'
with zipfile.ZipFile(archive) as z:
 so=[n for n in z.namelist() if n.endswith('/geonkick_mpc.so')];assert len(so)==1;plugin=z.read(so[0])
digest=hashlib.sha256(plugin).hexdigest();base=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=5',args.host];q=shlex.quote
remote='/tmp/geonkick-release-'+uuid.uuid4().hex[:12]
def ssh(cmd,**kw):return subprocess.run(base+[cmd],check=True,**kw)
def read(cmd):return subprocess.check_output(base+[cmd],text=True)
pid=read('pidof MPC').strip();assert pid.isdigit();maps=read('cat /proc/'+pid+'/maps');assert '/VST2 Plugins/' not in maps,'Close loaded external plugins before benchmark'
installed_folder=f"{build.CFG['vendor']} - VST - {build.CFG['name']}"
installed=read("sha256sum "+q("/nvme/VST2 Plugins/"+installed_folder+"/"+build.CFG['so'])).split()[0];assert installed==digest
facts={k:read(p).strip() for k,p in [('cpu_governor','cat /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor'),('cpu_frequency_khz','cat /sys/devices/system/cpu/cpu4/cpufreq/scaling_cur_freq')]}
files={'plugin.so':plugin,'official':(work/'official').read_bytes(),'paced':(work/'paced').read_bytes()};payload=io.BytesIO()
with tarfile.open(fileobj=payload,mode='w') as t:
 for name,data in files.items():
  info=tarfile.TarInfo(name);info.size=len(data);info.mode=0o700;t.addfile(info,io.BytesIO(data))
ssh('test ! -e '+q(remote)+' && mkdir '+q(remote)+' && tar -xf - -C '+q(remote),input=payload.getvalue())
dest=root/'benchmarks'/version;dest.mkdir(parents=True,exist_ok=True)
try:
 assert read('sha256sum '+q(remote+'/plugin.so')).split()[0]==digest
 for label,extra in [('standard',[]),('stress',['-p']),('paced-worker',[])]:
  binary='paced' if label=='paced-worker' else 'official'
  command=q(remote+'/'+binary)+' '+q(remote+'/plugin.so')+' -s 10 -v 1,4,8,16 '+' '.join(extra+['-j'])
  print('START',label,'exact binary',digest,flush=True);start=time.time()
  r=subprocess.run(base+[command],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=600)
  (work/(label+'-private.txt')).write_text(r.stdout)
  safe=r.stdout.replace(remote+'/plugin.so','geonkick_mpc.so').replace(remote,'temporary-benchmark-directory');lines=safe.strip().splitlines();report=json.loads(lines[-1]);assert r.returncode in (0,1)
  report.update(binary_sha256=digest,version=version,device='MPC Live III',os_version='3.10.1 (maintainer reported)',tool_ref=cfg['tools_ref'],bench_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),arguments='-s 10 -v 1,4,8,16 '+(' '.join(extra)+' ' if extra else '')+'-j',affinity='Cortex-A76 core 4',priority='SCHED_OTHER nice19',mpc_state='Maintainer confirmed empty project; zero external-VST mappings verified; app running',date_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),wall_seconds=round(time.time()-start,3),measurement='Official host, unchanged stages/timers/thresholds' if binary=='official' else 'Separate paced worker measurement; absolute wall-clock sleep after timed blocks, official audio workload unchanged',**facts)
  (dest/(label+'.json')).write_text(json.dumps(report,indent=2)+'\n');(dest/(label+'.txt')).write_text('\n'.join(lines[:-1])+'\n'+json.dumps(report,separators=(',',':'))+'\n')
  print(label,json.dumps(report),flush=True)
finally:
 cleanup='rm -f '+' '.join(q(remote+'/'+n) for n in ['plugin.so','official','paced','.sample-folder'])+'; rmdir '+q(remote)
 ssh(cleanup);print('Exact own temporary files removed; installed plugin/settings/project/services unchanged.',flush=True)
