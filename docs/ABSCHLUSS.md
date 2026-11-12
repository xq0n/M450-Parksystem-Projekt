# Abschlussbericht

Stand 12.11.2026. Implementierung direkt im Repository; keine Commits, Uploads auf Hardware oder Veröffentlichungen durchgeführt.

## Ergebnis

Zwei eigenständig baubare Arduino/C++-Programme, zusätzlich je eine Boardvariante für RP2040 Pico W und RP2350 Pico 2 W. Pico 1 übernimmt Kasse/Register/Gebühren/Zahlung/Kulanz/Kapazität; Pico 2 übernimmt Webeingabe und beide echten PIO-Servos. Das WLAN-Protokoll verwendet CRC32, Boot-Sitzungen, Sequenzen, Besuchsnummern, Quittierungen, Wiederholungen, Heartbeats, Timeout und Storno verspäteter Freigaben.

Implementiert: TC-01–20 einschließlich vormals fehlender Kapazitätsprüfung und Kulanz/Nachzahlung; expliziter Zahlungsabbruch, Karte/TWINT, Tarifänderung ohne Codeänderung, begrenzter HTTP-Parser, Wiederverbindung, Zeitüberlaufbehandlung, Testzeitsprung und nichtblockierende Schranken-Zustandslogik. Kamera und Geldfluss sind TK-gemäß simuliert; Servos werden über Hardwaretreiber angesteuert.

## Dateien

Geändert/neu strukturiert:

- `src/pico1/main.cpp`, `src/pico2/main.cpp` – vollständige neue Integration der Rollen.
- `include/parking_common.h` – hardwareunabhängige Grundfunktionen.
- `include/parkhaus_config.h`, `include/parkhaus_secrets.h.example` – zentrale Hardware-, Netz-, Tarif-, Kapazitäts- und Testkonfiguration.
- `platformio.ini` – reproduzierbarer Platform-Commit und vier Board-/Rollen-Ziele.
- `README.md` – Aufbau, Boardwahl, Verkabelung, Software, Flashbefehle, Demo, Fehlerbehebung, Tests und komplette Anforderungsmatrix.
- `.gitignore` – lokale Zwischenstände und Testbinaries ausgeschlossen.

Neu:

- `include/parking/core.h`, `protocol.h`, `controller.h`, `http_parser.h` – Fachlogik, Protokoll, Steuerung, HTTP-Eingaben.
- `include/drivers/runtime.h`, `http_server.h` – echte WLAN-/UDP-/HTTP-Anbindung.
- `include/web/ui.h` – gemeinsame HTML-Ausgabe und Formularhelfer.
- `test/test_parking.cpp`, `tools/test_host.py`, `tools/build.py` – ausführbare Tests und Buildwerkzeug.
- `docs/ANFORDERUNGEN.md`, `ARCHITEKTUR.md`, `HARDWARETESTS.md`, `TESTPROTOKOLL.md`, `TESTERGEBNISSE.md`, `ABSCHLUSS.md` – Quellenabgleich, Architektur, Abnahme und Ergebnisbericht.
- `docs/sources/umsetzung-parksystem.pdf`, `testkonzept.pdf` – unveränderte Kopien der zwei bereitgestellten PDFs für nachvollziehbare Seitenreferenzen.

Entfernt: die drei unbenutzten PlatformIO-Vorlagentexte `include/README`, `lib/README`, `test/README`. Erzeugt, aber nicht eingecheckt: vier UF2/ELF-Builds unter `.pio/build/`, Host-Testbinary unter `build/host/`, Build-/Testlogs unter `tmp/`.

## Verifikation

- **13 Host-Testgruppen / 11 639 Prüfungen bestanden.** Fachfunktionen, Zustände, kodierter Austausch zwischen Controller und Kassendienst, Paketverluste, verspätete Antworten, Fehler-/Grenzfälle und Parser werden ausgeführt.
- **Vier Firmware-Builds erfolgreich**, jeweils UF2 erzeugt. Maximal rund 29,2 % statisches RAM auf RP2040, Flash deutlich unter dem Boardlimit.
- Hardware-/WiFi-APIs wurden durch den echten Cross-Compiler geprüft. Test-/Buildskripte syntaxgeprüft, Änderungen und Dokumentationsverweise geprüft.
- **Keine Hardwaretests bestanden behauptet:** Die Boards waren laut Nutzer nicht angeschlossen. Die manuelle Checkliste und ein unverfälschtes leeres Testprotokoll liegen bereit.

Details, Versionen und Grenzen der Messungen: [TESTERGEBNISSE.md](TESTERGEBNISSE.md).

## Anforderungsabdeckung und noch offen

Alle **konkret in beiden PDFs beschriebenen Prototypfunktionen und TC-01–20** sind implementiert. Die Erfüllung aller 14 ursprünglich referenzierten User Stories ist nicht belegbar: deren Anforderungsdokument fehlt, US-03/09 sind nicht beschrieben, US-10–14 sind im TK ausgeschlossen. Diese Unklarheiten wurden ausdrücklich festgehalten, nicht durch erfundene Anforderungen ersetzt.

Vor dem Betrieb noch nötig: Boardmodell am Aufdruck bestimmen, WLAN-Daten eintragen, passende Firmware neu bauen/flashen, gemäß README verkabeln und Servoendlagen am tatsächlichen Aufbau prüfen. Der vollständige manuelle TK-Durchlauf, besonders unbezahlte Ausfahrt/Netzwerkfehler und Usability, muss danach ausgeführt werden.

Bewusste Prototypgrenzen: RAM-Daten gehen beim Neustart verloren; keine reale Kamera, kein echter Zahlungsdienst, keine Durchfahrts-/Hindernissensorik. Eine Passage während Stromausfall kann nicht automatisch rekonstruiert werden; keine neue Öffnung bei unklarem Zustand. Unverschlüsseltes HTTP/UDP ist für das isolierte Übungsnetz bestimmt.
