# Geonkick MPC 0.0.34 — kurzer Gerätetest

Die Optimierung ändert die Parameterverwaltung, nicht die Syntheseformeln.
Native Sanitizer, ARM64/QEMU, Preset-/Kit-Rundläufe und Layoutprüfungen sind bestanden.

1. Frequency am Q-Link langsam vorwärts/rückwärts drehen, dann am Touch grob ändern.
   Anschließend am Q-Link fein weiterdrehen: keine Sprünge oder Gegenbewegungen.
2. Zwischen Layern und Pads wechseln und den ersten Q-Link-Schritt prüfen.
3. Amp-/Pitch-Punkte verschieben, Seite wechseln und zur Kurve zurückkehren.
4. Ein vorhandenes Kit laden, verändern, nativ speichern und erneut laden.
5. Während des Spielens Filter/Drive und mehrere Regler bedienen: auf Knacken achten.

Die exakten Live-III-Leistungsberichte liegen unter `benchmarks/0.0.34`.
Der Standardtest besteht, der zusätzliche Vollparameter-Stresstest noch nicht.
Diese Messungen ersetzen keinen Hör-/Touch-Test im MPC-Projekt.
