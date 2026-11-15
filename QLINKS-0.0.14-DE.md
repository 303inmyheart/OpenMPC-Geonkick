# Geonkick MPC 0.0.14 – Q-Link-Belegung

Bankreihenfolge: Q13–16, Q9–12, Q5–8, Q1–4 (RowTopDown).
Punkte/Schalter/Auswahl/Schieberegler bleiben Touch.

| Seite | Bank1 | Bank2 | Bank3 | Bank4 |
|---|---|---|---|---|
| SOUND L1 | o1_freq_fine, o1_level, —, — | o2_freq_fine, o2_level, —, — | o3_freq_fine, o3_level, —, — | length_ms, volume, —, — |
| SOUND L2 | o4_freq_fine, o4_level, —, — | o5_freq_fine, o5_level, —, — | o6_freq_fine, o6_level, —, — | length_ms, volume, —, — |
| SOUND L3 | o7_freq_fine, o7_level, —, — | o8_freq_fine, o8_level, —, — | o9_freq_fine, o9_level, —, — | length_ms, volume, —, — |
| L1 OSC1 | o1_attack_ms, layer1, —, — | o1_freq_fine, o1_level, o1_decay_ms, o1_sweep | o1_sweep_time_ms, o1_phase, o1_density_pct, o1_fm | —, —, —, — |
| L1 FX1 | o1_cutoff, o1_q, —, — | o1_drive, o1_drive_out, —, — | —, —, —, — | —, —, —, — |
| L1 OSC2 | o2_attack_ms, layer1, —, — | o2_freq_fine, o2_level, o2_decay_ms, o2_sweep | o2_sweep_time_ms, o2_phase, o2_density_pct, — | —, —, —, — |
| L1 FX2 | o2_cutoff, o2_q, —, — | o2_drive, o2_drive_out, —, — | —, —, —, — | —, —, —, — |
| L1 OSC3 | o3_attack_ms, layer1, —, — | o3_freq_fine, o3_level, o3_decay_ms, o3_sweep | o3_sweep_time_ms, o3_phase, o3_density_pct, — | —, —, —, — |
| L1 FX3 | o3_cutoff, o3_q, —, — | o3_drive, o3_drive_out, —, — | —, —, —, — | —, —, —, — |
| L2 OSC1 | o4_attack_ms, layer2, —, — | o4_freq_fine, o4_level, o4_decay_ms, o4_sweep | o4_sweep_time_ms, o4_phase, o4_density_pct, o4_fm | —, —, —, — |
| L2 FX1 | o4_cutoff, o4_q, —, — | o4_drive, o4_drive_out, —, — | —, —, —, — | —, —, —, — |
| L2 OSC2 | o5_attack_ms, layer2, —, — | o5_freq_fine, o5_level, o5_decay_ms, o5_sweep | o5_sweep_time_ms, o5_phase, o5_density_pct, — | —, —, —, — |
| L2 FX2 | o5_cutoff, o5_q, —, — | o5_drive, o5_drive_out, —, — | —, —, —, — | —, —, —, — |
| L2 OSC3 | o6_attack_ms, layer2, —, — | o6_freq_fine, o6_level, o6_decay_ms, o6_sweep | o6_sweep_time_ms, o6_phase, o6_density_pct, — | —, —, —, — |
| L2 FX3 | o6_cutoff, o6_q, —, — | o6_drive, o6_drive_out, —, — | —, —, —, — | —, —, —, — |
| L3 OSC1 | o7_attack_ms, layer3, —, — | o7_freq_fine, o7_level, o7_decay_ms, o7_sweep | o7_sweep_time_ms, o7_phase, o7_density_pct, o7_fm | —, —, —, — |
| L3 FX1 | o7_cutoff, o7_q, —, — | o7_drive, o7_drive_out, —, — | —, —, —, — | —, —, —, — |
| L3 OSC2 | o8_attack_ms, layer3, —, — | o8_freq_fine, o8_level, o8_decay_ms, o8_sweep | o8_sweep_time_ms, o8_phase, o8_density_pct, — | —, —, —, — |
| L3 FX2 | o8_cutoff, o8_q, —, — | o8_drive, o8_drive_out, —, — | —, —, —, — | —, —, —, — |
| L3 OSC3 | o9_attack_ms, layer3, —, — | o9_freq_fine, o9_level, o9_decay_ms, o9_sweep | o9_sweep_time_ms, o9_phase, o9_density_pct, — | —, —, —, — |
| L3 FX3 | o9_cutoff, o9_q, —, — | o9_drive, o9_drive_out, —, — | —, —, —, — | —, —, —, — |
| ENVELOPE | length_ms, —, —, — | —, —, —, — | —, —, —, — | —, —, —, — |
| MASTER MIX | length_ms, —, —, — | layer1, layer2, layer3, volume | tune, —, —, — | —, —, —, — |
| MASTER FX | cutoff, q, —, — | drive, drive_out, —, — | —, —, —, — | —, —, —, — |
| MASTER PRESETS | —, —, —, — | —, —, —, — | —, —, —, — | —, —, —, — |
