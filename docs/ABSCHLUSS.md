# Abschlussbericht

> Aktueller Aufbau: **Eine gemeinsame Schranke** am Schranken-Pico, GP8/Pin 11, übernimmt beide Richtungen; GP9 entfällt. Nach dem physischen Boardtausch stellt die Kasse ihre eigene Webseite auf `192.168.1.127` bereit, die Schranke ihre Webseite auf `192.168.1.126`. Der Servo öffnet nach Freigabe fünf Sekunden.


Aktualisierung 20.11.2026: Für A02 wurden Speicher und Kennzeicheneingabe zusätzlich hinter austauschbare Schnittstellen gelegt und mit Testdoubles geprüft. Die bestehende Gebühren-, Zahlungs-, Protokoll- und Servologik bleibt erhalten. Der Hardwarebericht vom 12.11.2026 beschreibt den damaligen Stand; für diese Aktualisierung wurde kein neuer Hardwaretest durchgeführt. Aktuelle Nachweise: [TESTERGEBNISSE.md](TESTERGEBNISSE.md).

## Ergebnis

Zwei eigenständig baubare Arduino/C++-Programme, zusätzlich je eine Boardvariante für RP2040 Pico W und RP2350 Pico 2 W. Pico 1 übernimmt Kasse/Register/Gebühren/Zahlung/Kulanz/Kapazität und liefert die Kassen-Webseite; Pico 2 liefert die Schranken-Webseite und steuert den einen Servo über `Gate::open()` und `Gate::update()` nach dem bereitgestellten Beispiel. Das WLAN-Protokoll verwendet CRC32, Boot-Sitzungen, Sequenzen, Besuchsnummern, Quittierungen, Wiederholungen, Heartbeats, Timeout und Storno verspäteter Freigaben.

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

- **17 Host-Testgruppen / 11 741 Prüfungen bestanden (20.11.2026).** Zusätzlich zu Fachfunktionen, Zuständen, kodiertem Austausch, Paketverlusten und Parser werden der isolierte Kassendienst mit Speicher-Mock sowie die Kennzeicheneingabe mit Stub geprüft.
- **Vier Firmware-Builds erfolgreich (20.11.2026)**, jeweils UF2 erzeugt, keine Compilerfehler oder -warnungen. Maximal rund 29,2 % statisches RAM auf RP2040, Flash deutlich unter dem Boardlimit. Die Buildkonfiguration und die Hardwaretreiber wurden beibehalten.
- Hardware-/WiFi-APIs wurden durch den echten Cross-Compiler geprüft. Test-/Buildskripte syntaxgeprüft, Änderungen und Dokumentationsverweise geprüft.
- **Keine neuen Hardwaretests bestanden behauptet:** Am 20.11.2026 wurden keine seriellen Ports erkannt und keine Boards geflasht. Die manuelle Checkliste und das noch offene Testprotokoll liegen bereit.

Details, Versionen und Grenzen der Messungen: [TESTERGEBNISSE.md](TESTERGEBNISSE.md).

## Anforderungsabdeckung und noch offen

Alle **konkret in beiden PDFs beschriebenen Prototypfunktionen und TC-01–20** sind implementiert. Die Erfüllung aller 14 ursprünglich referenzierten User Stories ist nicht belegbar: deren Anforderungsdokument fehlt, US-03/09 sind nicht beschrieben, US-10–14 sind im TK ausgeschlossen. Diese Unklarheiten wurden ausdrücklich festgehalten, nicht durch erfundene Anforderungen ersetzt.

Vor dem Betrieb noch nötig: Boardmodell am Aufdruck bestimmen, WLAN-Daten eintragen, passende Firmware neu bauen/flashen, gemäß README verkabeln und Servoendlagen am tatsächlichen Aufbau prüfen. Der vollständige manuelle TK-Durchlauf, besonders unbezahlte Ausfahrt/Netzwerkfehler und Usability, muss danach ausgeführt werden.

Bewusste Prototypgrenzen: RAM-Daten gehen beim Neustart verloren; keine reale Kamera, kein echter Zahlungsdienst, keine Durchfahrts-/Hindernissensorik. Eine Passage während Stromausfall kann nicht automatisch rekonstruiert werden; keine neue Öffnung bei unklarem Zustand. Unverschlüsseltes HTTP/UDP ist für das isolierte Übungsnetz bestimmt.
