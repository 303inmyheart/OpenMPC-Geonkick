# Geonkick MPC roadmap

Gen2 / aarch64, MPC OS 3.10.1+. Hardware tested on MPC Live III only.

## 1.0.1 release work

- Manufacturer 303inmyheart; standard installer retires the old matching-id
  OpenMPC vendor folder. Stable UID, parameters, sound/state formats.
- English PRESET INFO, prominent migration notice, tutorial and common Python
  converter for Geonkick/JC303. The converter creates copies on a computer;
  the plugin installer does not rewrite personal presets automatically.
- Maintainer confirmed old-project loading with correct sounds/settings and
  native help open/dismissal. Expand coverage across older user presets/kits.
- Final exact-binary measurements and public-source CI before publication.
  Historical benchmark failures remain visible; no old result is relabelled.

## Next features

1. Broader user-preset/project migration testing and converter usability.
2. Import arbitrary original `.gkick` / `.gkit` user files, preserving richer
   original curves/data without reducing them to the native point limit.
3. Kit management, choke/mute/solo and output-routing review; currently stereo.
4. Supported folders in the native MPC Edit preset selector. Current VST menu
   is flat; slash-prefixed program names do not create native categories.
5. Later performance work: incremental/background synthesis scheduling and
   more complete parameter-sweep scenarios without lowering sound quality.
6. Extended busy-project/multi-instance stability testing on additional Gen2
   models. Live III results are not evidence for untested models.

## Deliberately deferred/removed

- Extra filter/Q/drive/noise-density envelope editing and completely free
  Bezier handles/tap-to-insert require a separate UI milestone. Current Amp/Pitch
  point editing remains touch-only, 2–16 knots.
- Humanizer was removed at the maintainer's request; use the MPC's own features.
- Selected Sounds → Instruments thumbnail sometimes does not reopen its list;
  the maintainer also observes this in other external plugins. Investigation
  deferred; use bottom preset arrows or the native in-editor preset chooser.

## Release policy

Tutorial, changelog, matching modified source and licence notices accompany
releases. All new binaries need their own final measurements. Published tags/
assets are immutable. Desktop copies only on explicit request. Keep private
presets, logs, device backups and test output out of the public source.

The next port after Geonkick is Fire, followed by Wolf Shaper. BYOD first needs
navigation/usability review; Surge follows that review. See the JC303 roadmap.
