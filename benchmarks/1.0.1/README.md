# Geonkick MPC 1.0.1 — exact final binary

MPC Live III, OS 3.10.1 (maintainer reported), 44.1 kHz, 128 samples, A76 core4,
performance governor, 2.4 GHz, SCHED_OTHER nice19. The maintainer confirmed an
empty project; zero external VST mappings were verified before testing. MPC
stayed running. Own temporary files were removed, with no project/settings or
service changes for these measurements.

Stripped SO SHA256:
`43049e439a0226c902563590938b98ccdf242a56b58021b33069c562eaf87eea`.
Reviewed OpenMPC host at `186718df51cddce58b607988171bd2dce3bb2ae5`, bench.c SHA256
`6a839caaf0482f401c32838123adf1405b248f096d4d210179d7b47df693bca5`.
Scheduling wrapper selects core4/nice19; official stages/timers/limits unchanged.

| Measurement | Audio p99 | Audio maximum | Verdict | Background maximum |
|---|---:|---:|---|---:|
| Standard OpenMPC | 1.1% | 2.0% | PASS | 0.7% |
| Additional full-parameter storm | 77.0% | 97.2% | FAIL | 60.5% |
| Separately paced worker workload | 1.9% | 4.0% | PASS | 77.5% |

The standard result is the installer/catalog CPU value. It does NOT replace or
hide the full-stress FAIL. Background percentages describe one CPU core; audio
percentages describe an audio-block budget, neither is the MPC project meter.
The paced run sleeps to each block's absolute wall-clock deadline after timing
the unchanged audio work. It exposes asynchronous synthesis cost; it is separate
from the stock host/verdict. All original outputs and commands are retained.

Each run separately fires 25 one-shot controls. This does not test every sample,
kit, project or multi-instance arrangement. No downsampling/quality reduction
was added. The 1.0.0 raw results, including its full-stress FAIL, remain unchanged.
