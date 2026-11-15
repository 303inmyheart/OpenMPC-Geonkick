# Geonkick MPC 0.0.14 – Menü und Audio prüfen

## Menü und Q-Links

1. SOUND: drei Layer-Übersichten mit Wellenform und Schnellzugriff.
2. LAYER1–3: OSC1 → FX1 → OSC2 → FX2 → OSC3 → FX3. Die Effekte gehören jeweils
   zum vorherigen Oszillator. Die Layer-Lautstärke bleibt auf den OSC-Seiten.
3. ENVELOPE: Wellenform und Punkte gemeinsam; Amp/Pitch, Oszillator und
   Simple/Points per Touch wählen. Punkte, Punktwahl und Punktwerte haben keine
   Q-Links. Nur LENGTH ist hier auf einem Q-Link.
4. MASTER: MIX → FX → PRESETS. MIX enthält die drei Layer-Pegel und OUTPUT,
   darunter TUNE sowie Key Track, Playback und Audition per Touch.
5. Q-Links steuern ausschließlich Drehregler. Keine Schalter, Wellenformwahl,
   Filter-/Drive-Typen, Presetwahl oder Schieberegler. Die Rahmen müssen die
   vollständigen Regler samt Namen und Wert umschließen. Touch darf keinen
   fremden Hardware-Rahmen auswählen.

Auf OSC-Seiten: erste Bank Attack/Layer-Pegel; zweite Bank Frequency/Level/Decay/
Pitch Drop; dritte Bank Pitch Time/Phase/Noise Density/ggf. FM. Bei Noise oder
abgeschaltetem FM-Ziel sind die entsprechenden Regler ausgeblendet; ihre
Hardware-Plätze bleiben stabil. Touch bleibt direkt/grob, Q-Links fein.

## Knacken

- Bei einer Kick LENGTH auf etwa 30–80 ms verkürzen, Decay bewusst länger lassen.
  Hören, ob das zusätzliche Knacken am Ende verschwindet; Attack soll bleiben.
- MASTER → MIX: CUT per Touch auswählen und die Kick wiederholt, auch schnell
  und mit verschiedenen Anschlagstärken, spielen. Der vorherige Schlag soll
  weich auslaufen, der neue sofort starten. Es gibt eine kurze 5 ms Überblendung.
- POLY muss weiter überlappende Schläge erlauben; GATE bleibt notengesteuert.
- Parameter/Q-Links bewegen, anschließend zum Punkteditor zurück: Wellenform
  und Kurve müssen weiter aktualisieren. Keine Rückkehr zu alten Redraw-Pfaden.
- Projekt speichern/laden; Klang, Modi und Punkte müssen erhalten bleiben.

Native Sanitizer-, ARM64/QEMU- und isolierte Signaltests auf Live III bestanden.
Das ersetzt die Hör- und Touch-Prüfung in der MPC-Anwendung nicht. Kein formaler
CPU-/Store-Benchmark dieses Entwicklungsbuilds und keine öffentliche Freigabe.
