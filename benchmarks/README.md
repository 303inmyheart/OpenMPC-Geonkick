# Live III benchmark reports

Each version folder contains reports for one exact stripped ARM64 binary, with
its SHA-256, tool revision, stages and conditions. Older failed runs are retained.
The installed binary must match the installer before measurement.

- `standard`: unchanged OpenMPC benchmark, default parameter state,1/4/8/16 voices.
- `stress`: the same official host with `-p`, sweeping every writable VST input
  each block. This is an additional deliberately severe parameter-path test.
- `paced-worker`: separately labelled wall-clock-paced run to expose background
  synthesis cost. Audio work/timers remain the same; this is not the official
  catalog result and does not replace it.

Percentages use one44.1kHz/128-frame audio block or one CPU core, as labelled.
They are not the MPC project CPU meter. A standard PASS does not imply that the
additional sweep passes, that every project is glitch-free, or catalog acceptance.

Hardware: MPC Live III only, MPC OS3.10.1 reported by the maintainer; empty project
confirmed, no external-VST mappings, MPC application running. Benchmark helpers
use their own temporary directory and are removed afterward; no settings/project
or installed plugin files are changed by the benchmark.
