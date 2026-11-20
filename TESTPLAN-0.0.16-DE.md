# Geonkick MPC 0.0.16 – Auswahl und Presets prüfen

- SOUND: oben nacheinander L1/1 bis L3/3 antippen. Die Auswahl muss erhalten
  bleiben, auch nach einer kurzen Wartezeit und während des Spielens.
- AMPLITUDE/PITCH wechseln: Oszillatorwahl muss bleiben, Kurve passend wechseln.
- Frequency/Attack/Decay ändern und danach einen anderen Ziel-Oszillator wählen:
  kein Rücksprung auf den zuletzt bearbeiteten Oszillator durch Host-Rückmeldung.
- Feine Q-Link-Schritte und direkte Touch-Bewegung müssen weiter funktionieren.
- MASTER → PRESETS: alle acht Tasten liegen vollständig und mittig im Feld.
  Jeden Klang und RESET INIT per Touch prüfen. Es gibt weiterhin keine Preset-
  Q-Links. Die übrigen Menü-/Punkt- und Audio-Prüfungen aus 0.0.14 gelten weiter.

Native Sanitizer, echte ARM64-Bibliothek unter QEMU, Host-Echo-Regressions- und
Geometrieprüfungen bestanden. Die tatsächliche Touch-Prüfung auf der MPC steht
noch aus; kein formaler CPU-/Store-Benchmark dieses Entwicklungsbuilds.
