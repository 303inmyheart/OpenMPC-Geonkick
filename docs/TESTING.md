# Geonkick MPC 1.0.1 validation

Gen2/aarch64, MPC OS 3.10.1+. Hardware checks are limited to MPC Live III.

- Native ASan/UBSan and actual ARM64/QEMU checks passed for this binary.
  QEMU checks are not hardware CPU benchmarks.
- 118 native preset states/export hashes, kit/sample/envelope recall, control
  context, fine input, feedback and short/Cut playback regressions are covered.
- Final skin checks cover XY points, waveform bindings, native touch targets,
  rotary-only Q-Links and frame bounds. PRESET INFO adds no sound parameter.
- The maintainer confirmed loading an older project, matching sounds/settings,
  and opening/closing the new English information window on Live III.
- A historical GKMP9 state was accepted by the actual native VST. An explicitly
  approved user-preset copy was created with its original/payload unchanged;
  exhaustive MPC loading of all converted presets is not claimed.
- The common Python converter passes 11 file-safety regressions and the
  131-preset factory batch. GUI runtime coverage is limited where Tk is absent.
- Isolated installer checks cover vendor retirement, repeat installs, native
  preset paths, retained sample-folder setting, unrelated registry entries,
  foreign-id protection and uninstall. Current catalog validation is required.

Final exact-binary benchmark: standard PASS 1.1% p99 / 2.0% max; extra full
storm FAIL 77.0% / 97.2%; paced audio PASS 1.9% / 4.0%, background up to 77.5%
of one CPU core. Raw reports and methodology: [1.0.1](../benchmarks/1.0.1/).
Earlier [1.0.0 results](../benchmarks/1.0.0/) remain immutable, including the
additional full-stress FAIL. Standard audio-thread, background-worker and project
CPU percentages are different quantities; do not substitute one for another.

Detailed superseded candidate narratives/test plans remain in Git history.
Raw benchmark reports and licence/attribution notices are retained in this tree.
