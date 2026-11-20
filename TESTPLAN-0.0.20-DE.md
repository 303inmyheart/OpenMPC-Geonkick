> Historisch: Der Humanizer wurde mit0.0.21 auf Nutzerwunsch entfernt. Für aktuelle Builds TESTPLAN-0.0.21-DE.md verwenden.

# Geonkick MPC0.0.20 — Humanizer prüfen

1. MASTER → HUMANIZER öffnen. Init/alte Projekte zeigen OFF. Nur VELOCITY und
   TIMING haben Q-Links; ON/OFF ist Touch. Beide Regler per Touch grob und per
   Q-Link/Datenrad fein bewegen, auf Rücksprünge achten.
2. Einen gleichmäßigen Beat mit gleicher Anschlagstärke spielen. ON und
   VELOCITY20–30%, TIMING0: unterschiedliche Lautstärken, gleiche Zeitpunkte.
3. VELOCITY0, TIMING20–30ms: unterschiedlich spät startende Hits. Danach5ms
   für subtilere Abweichung. OFF beziehungsweise beide Werte0 klingen/timen
   exakt wie vorher. Die Wellenform ist der Grundklang, nicht jeder Zufallshit.
4. Gate mit kurzen Noten und hoher Timing-Variation: losgelassene passende Noten
   dürfen nicht später ungewollt erklingen. Andere MIDI-Kanäle bleiben getrennt.
   Panic/Reset dürfen keine verzögerten Hits nachlaufen lassen.
5. CUT + hohe Timing-Variation: der vorherige Hit wird erst beim neuen Start
   abgeschnitten. Poly/Cut/Gate und schnelle Retrigger auf neue Knackser prüfen.
6. Humanizer an/Regler setzen, Projekt speichern/laden. Alte0.0.19-Projekte
   behalten ihre Filter und laden Humanizer OFF. Neue Dateien als eigene Tests
   speichern; neue GKMP7-Zustände benötigen0.0.20+.
7. Presetbrowser, WIDE/CLASSIC, Punkteditor, Übersicht und alle Layer-FX weiter
   bedienbar; MASTER hat jetzt MIX → FX → HUMANIZER → PRESETS.

Native/ARM64-Tests sind Funktionstests, kein formaler Store-CPU-Benchmark.

0.0.20 installiert; MPC-Anwendung neu gestartet, Dienst läuft und installierte
Bibliothek stimmt mit dem geprüften Installer überein. Neuer Praxistest steht aus.
