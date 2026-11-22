#!/usr/bin/env python3
"""Prepare pinned public wrapper/release dependencies without changing existing checkouts."""
import hashlib,json,shlex,subprocess,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='ef05a26c6e59f1bfd3ef2023e6a9d8b371b61310'
PATCHES=['mpc-float32','mpc-fine-int-sweep','mpc-async-host-notify','mpc-display-parameter-budget','mpc-coalesced-graph-and-fine-input','mpc-graph-rebind-and-input-observation','mpc-accumulated-fine-input','mpc-canonical-continuous-echo','mpc-envelope-state-capacity','mpc-idle-action-release','mpc-filter-control-feedback','mpc-control-context','mpc-sample-dynamic-units','mpc-native-display-context','mpc-ui-only-inputs','mpc-engine-program-bank','mpc-async-revision-cache','mpc-bounded-ui-feedback','mpc-numeric-parameter-read','mpc-indexed-popup-feedback','mpc-readonly-input-fastpath','mpc-lazy-input-context']
base=Path(os.environ.get('GEONKICK_DEPS',ROOT/'build/deps'));base.mkdir(parents=True,exist_ok=True)
def run(*args,cwd=None):subprocess.run(args,cwd=cwd,check=True)
def checkout(name,sha,patches):
 dest=base/name;expected={'base':sha,'patches':{p:hashlib.sha256((ROOT/'patches'/(p+'.patch')).read_bytes()).hexdigest() for p in patches}}
 receipt=dest/'.geonkick-dependency.json'
 if dest.exists():
  assert receipt.exists() and json.loads(receipt.read_text())==expected,'Dependency cache differs; choose a fresh build/deps directory'
  return dest
 run('git','init',str(dest));run('git','remote','add','origin','https://github.com/open-mpc/mpc-vst-plugins.git',cwd=dest);run('git','fetch','--depth','1','origin',sha,cwd=dest);run('git','checkout','--detach','FETCH_HEAD',cwd=dest)
 for p in patches:run('git','apply',str(ROOT/'patches'/(p+'.patch')),cwd=dest)
 receipt.write_text(json.dumps(expected,indent=2)+'\n');return dest
wrapper=checkout('wrapper',BASE,PATCHES)
release=json.loads((ROOT/'release.json').read_text());tooling=checkout('release-tools',release['tools_ref'],[])
print('export MPC_VST='+shlex.quote(str(wrapper)))
print('export MPC_RELEASE_TOOLS='+shlex.quote(str(tooling)))
