# Local build

Pinned Geonkick source is committed under src/dsp. No upstream checkout is
needed at build time. The upstream desktop CMake requires CMake 3.31, C++23,
Redkite and desktop-format dependencies; this C11 DSP-only port does not use it.

Build dependency: neighbouring open-mpc/mpc-vst-plugins at
`ef05a26c6e59f1bfd3ef2023e6a9d8b371b61310`, with
patches/mpc-float32.patch applied. The wrapper is not copied into this repository.
Release tooling: open-mpc/mpc-vst-plugins at
`a2dd2b78a19c4cd46257c09eb9a485a155ab2968` in the neighbouring
mpc-vst-plugins-release-tools checkout.

Requires Python 3 with Pillow, gcc, aarch64-linux-gnu-gcc 13 and its matching
ARM64 sysroot, binutils and qemu-aarch64. `MPC_VST`, `MPC_TOOLCHAIN` (the toolchain's
usr directory), `AARCH64_CC` and `QEMU_AARCH64` can override defaults. The current
workstation reuses the previously unpacked local JC303 toolchain; it is excluded
from source control and from installers. The compiler's glibc must be <=2.39.

```
python3 tools/build.py native
python3 tools/test.py native
python3 tools/test.py aarch64
python3 tools/build.py skin
python3 tools/package.py
```

The native tests load the actual VST shared library under ASan/UBSan/leak
detection, then check the engine against an independent signal oracle and
exercise asynchronous rendering. ARM64 runs the shared-library host and engine
tests under QEMU. QEMU is not a hardware test or CPU benchmark.

tools/spec.py generates params.json, src/param_defs.h and layout.conf. Indices in
params.lock are frozen from 0.0.1 onward; append new parameters. An edit to
generated files must also update the generator. Skin finishing computes Q-Link
bounds from native controls, including labels, and renames the browser default
to Init. All build products, previews, logs and receipts stay under build/.

Packaging uses official release.py, verifies the test receipts and validates
the result with catalog_check.py. The ZIP contains only installation material,
including install/uninstall scripts, manifests, checksums and license notices.
The source-repository field is reserved for the intended future
303inmyheart/OpenMPC-Geonkick repository; publication is still pending.
Do not submit this prototype to the catalog or claim public source availability.
