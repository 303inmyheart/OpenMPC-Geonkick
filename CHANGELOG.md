# Changelog

## 1.0.1 — 2026-11-27

> **PRESET COMPATIBILITY NOTICE:** IMPORTANT: Manufacturer changed from OpenMPC to 303inmyheart. Older personal MPC presets may be rejected as "not for this plugin". Keep originals and use migrated copies; updating the plugin does not convert your user presets.

Corrects the displayed manufacturer and retires the old matching-id plugin folder. The remembered sample-folder setting is carried over. Plugin UID, parameters and audio engine are unchanged.

The manufacturer change affects MPC preset metadata, not just the displayed
folder name. Keeping the plugin UID alone does not guarantee host acceptance.
User presets are not automatically converted by the standard installer.
Read [Preset migration](PRESET-MIGRATION.md) **before updating**.

The maintainer confirmed loading an older Geonkick project after the update.
An English PRESET INFO window explains the transition; the common Python
converter creates compatible JC303/Geonkick preset copies on a computer.
It never runs automatically and does not rewrite project files.

Exact-binary Live III results: standard PASS 1.1% p99 / 2.0% max; full-stress
FAIL 77.0% / 97.2%; separately paced audio PASS 1.9% / 4.0% with background
synthesis up to 77.5% of one core. See benchmarks/1.0.1 for scope and raw data.
The tutorial, converter script and instructions are included in the ZIP; no
converter runs automatically. Obsolete manual test plans were removed and
current controls, licences and validation evidence consolidated under docs/.
