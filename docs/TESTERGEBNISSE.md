# Testergebnisse

## Aktuelle Verifikation vom 20.11.2026: Testdoubles

`python tools/test_host.py` wurde unter Windows mit Python 3.12 und GCC 15.2.0 (MSYS2 UCRT64), C++17, `-Wall -Wextra -Werror -pedantic` ausgeführt.

**Ergebnis: 17 Testgruppen, 11 741 erfolgreiche Prüfungen, Exit-Code 0.** Der Zähler enthält auch wiederholte Assertions und Protokollprüfungen; er entspricht nicht der Zahl unterschiedlicher fachlicher Testfälle. Der ältere Teststand weiter unten ist historisch.

| Gruppe | Prüfung | Testkonzept |
|---|---|---|
| 1 `isolatedStorage` | Echter Kassendienst mit injiziertem `VehicleStoreMock`: Ergebnisse und Parameter, Antwortcache ohne zweiten Speicherzugriff, Abschluss/Storno, falsche Sitzung/Link-ID | TC-01/13/14/17/18 |
| 2 `isolatedInput` | `PlateInputStub` ersetzt Browser/Kamera: fehlende, ungültige und gültige Eingaben; unbezahlte Ausfahrt bleibt geschlossen, bezahlte wird freigegeben | TC-01/02/03/13/14 |
| 3 `httpInput` | Produktiver `HttpPlateInput`: Formulardekodierung und Normalisierung, fehlende/doppelte/fehlerhafte Werte und zu kleiner Zielpuffer | TC-01/02/03 |
| 4 `isolatedExit` | Vorgegebene Kassenantworten, Schranken-Mock und Fake-Uhr: bezahlt/unbezahlt, genau ein Öffnen/Schließen, Grenze 4999/5000 ms | TC-13/14 |
| 5 `inputs` | Kennzeichen und strikte Zahlen | TC-01/02/03 |
| 6 `billing` | Tarifgrenzen, Tagesmaximum, Mehrtage, Tarifwechsel | TC-06/07/08 |
| 7 `lifecycle` | Einfahrtszeit, Duplikate, Kapazität, getrennte Fahrzeuge | TC-01/04/05/19 |
| 8 `payments` | Bestätigung, Abbruch, unbekanntes Kennzeichen, Methoden, alte/doppelte Zahlung | TC-09/10/11/12 |
| 9 `grace` | Zahlungsfreigabe, exakte Kulanzgrenze und Nachzahlung | TC-13/14/15/16 |
| 10 `protocol` | CRC32, Format, Längen, Bitfehler und ungültige Frames | TC-17/18 |
| 11 `replay` | Wiederholte/geänderte/veraltete Anfragen und Sitzungen | TC-17/18 |
| 12 `integration` | Echter Speicher, Kassendienst und Controller über kodierte Frames | TC-01/09/13/17/19 |
| 13 `denied` | Keine Öffnung bei ungültig, Duplikat, voll oder unbezahlt | TC-02/03/04/05/14 |
| 14 `losses` | Verlorene Anfragen/Antworten, Timeout, späte Freigabe und Wiederverbindung | TC-18 |
| 15 `retryAndStale` | Wiederholung vor Frist, unpassende/alte Antworten | TC-17/18 |
| 16 `timeAndRestart` | Zeitüberlauf, 30 Tage, nichtblockierendes Schließen und neue Sitzung | TC-07/18 |
| 17 `http` | Partielle/übergroße HTTP-Anfragen, Framing und Formulardaten | Webeingaben |

Die Änderungen ergänzen `VehicleStore` und `PlateInput` als austauschbare Schnittstellen. Die Körper der bestehenden Speicher-, Gebühren-, Zahlungs-, Kulanz- und Protokolloperationen sowie der Controller und der Servo-Treiber bleiben unverändert. Die Formulardekodierung und Normalisierung verwenden weiterhin dieselben Regeln; ungültige Schrankenformulare antworten weiterhin mit HTTP 400.

Der aktuelle Stand wurde nicht auf angeschlossene Picos übertragen; bei der Prüfung wurden keine seriellen Ports gefunden. Physische Servo-Bewegung, WLAN am Aufbau und TC-20 bleiben manuell zu bestätigen. Der historische Hardwarebericht unten ist kein Nachweis für einen neuen Hardwaretest.

### Firmware-Builds vom 20.11.2026

Alle vier bestehenden Buildziele wurden mit dem unveränderten `platformio.ini` erfolgreich gebaut. Für diesen Lauf wurden PlatformIO 6.2.0 unter `tmp/build-deps` und die Abhängigkeiten frisch über das Netzwerk unter `.pio-core` installiert. Es waren keine Änderungen an Boardkonfiguration, Arduino-Core, Servotreiber oder WLAN-Protokoll nötig. Im Buildlog finden sich keine Compilerfehler oder -warnungen.

Ausgeführter Build mit dem Python-Interpreter dieser Umgebung und projektlokalen Abhängigkeiten:

```powershell
$env:PYTHONPATH = (Resolve-Path tmp/build-deps).Path
$env:PLATFORMIO_CORE_DIR = Join-Path (Get-Location) '.pio-core'
python -m platformio run -e pico1-kasse -e pico2-schranken -e pico1-kasse-rp2350 -e pico2-schranken-rp2350
```

Bei regulär installiertem PlatformIO baut `python tools/build.py` dieselben vier Ziele. Das lokale vollständige Buildlog liegt unter `tmp/firmware-build.log` (nicht eingecheckt).

| Environment | Ergebnis | Statisches RAM | Flash | UF2 erzeugt |
|---|---|---:|---:|---|
| `pico1-kasse` | SUCCESS | 76 648 B / 262 144 B | 358 452 B / 2 093 056 B | ja |
| `pico2-schranken` | SUCCESS | 73 968 B / 262 144 B | 354 148 B / 2 093 056 B | ja |
| `pico1-kasse-rp2350` | SUCCESS | 76 836 B / 524 288 B | 354 328 B / 4 190 208 B | ja |
| `pico2-schranken-rp2350` | SUCCESS | 74 188 B / 524 288 B | 350 184 B / 4 190 208 B | ja |

Exit-Code 0. Die vier Dateien liegen unter `.pio/build/<Environment>/firmware.uf2`. Gebaut wurde mit den WLAN-Platzhaltern; vor dem Betrieb lokale Zugangsdaten eintragen und neu bauen. Statisches RAM ist ein Linkerwert und keine Messung des Laufzeitverbrauchs.

Versionen: Raspberry-Pi-Platform 1.20.0, Commit `5d4561a05e3b212660ac6fdd3fbfb328d1988aa1`; Arduino-Pico 6.1.0, Commit `fd65f6d4ab168d4bb181a3709738fe353ac93585`; ARM-Toolchain/Picotool/PIOASM `5.160100.260719`, Compiler 16.1.0. Ein erfolgreicher Build belegt die Kompilierbarkeit für beide Boardfamilien, nicht die physische Funktion auf angeschlossenen Geräten.

## Historische Verifikation vom 12.11.2026

> Aktueller Aufbau: ein gemeinsamer SG90 am Schranken-Pico, GP8/Pin 11; fünf Sekunden Öffnungszeit. Beide Picos stellen wie im TK S. 2 eine eigene Webseite bereit: Kasse `192.168.1.127`, Schranke `192.168.1.126`.


## Tatsächlich ausgeführte Tests ohne Hardware

`python tools/test_host.py` wurde unter Windows mit GCC 15.2.0 (MSYS2 UCRT64), C++17, `-Wall -Wextra -Werror -pedantic` ausgeführt.

**Ergebnis: 13 Testgruppen, 11 639 erfolgreiche Prüfungen, Exit-Code 0.** Die Anzahl enthält wiederholte Integrationsprüfungen, alle einzelnen CRC-Bitfehler und 10 000 zufällig erzeugte ungültige Frames; es sind nicht 11 639 unterschiedliche fachliche Testfälle. Das Testprogramm benutzt unveränderte Fach-, Protokoll-, Controller- und Parserheader aus der Firmware, keine Python-Nachimplementierung der Gebührenformel.

| Gruppe | Geprüft | TK-Bezug |
|---|---|---|
| 1 `inputs` | Normalisierung, leer, Sonderzeichen, Grenzen, nicht-ASCII, strikte Zahlen/Überlauf | TC-01/02/03 |
| 2 `billing` | Mindestgebühr, Einheitsgrenze, Tagesgrenze, Mehrtage, Tagespreis unter Maximum, Tarifwechsel, ungültiger Tarif, Monotonie | TC-06/07/08 |
| 3 `lifecycle` | Zeitstempel, Duplikat, Kapazität, zwei Fahrzeuge, Wiederverwendung Kennzeichen mit neuer Besuchs-ID | TC-01/04/05/19 |
| 4 `payments` | Unbekannt, Angebot, Abbruch, ungültige Methode, Angebotsablauf, TWINT, doppelte Bestätigung | TC-09/10/11/12 |
| 5 `grace` | Unbezahlt gesperrt, Kulanz exakt/überschritten, Nachzahlung mit Anrechnung, wiederholte Autorisierung, Storno, Tagesmaximum bleibt wirksam | TC-13/14/15/16 |
| 6 `protocol` | Referenz-CRC, Roundtrip, alle 384 Einzelbitfehler, kurze/lange Frames, unbekannte Operation, unterminierte/unsaubere Strings, Zufallsframes | TC-17/18 + Robustheit |
| 7 `replay` | Identische Wiederholung, gleiche Sequenz anderer Inhalt, veraltete Anfrage, falscher Boot, falsche Link-ID | TC-17/18 |
| 8 `integration` | Controller und Kassendienst über wirklich kodierte/dekodierte Frames: zwei Einfahrten, Zahlungen, Ausfahrten, Abschluss | TC-01/09/13/17/19 |
| 9 `denied` | Kein Gate-Öffnen bei ungültig, Duplikat, voll oder unbezahlt | TC-02/03/04/05/14 |
| 10 `losses` | Verlorene Entry-Antwort, 1800-ms-Timeout, späte Freigabe storniert, Offline-Kasse, verlorene Depart-Bestätigung, Wiederaufnahme | TC-18 |
| 11 `retryAndStale` | Wiederholung innerhalb Frist öffnet einmal, alte/unpassende Antworten unwirksam, Frist exakt erreicht | TC-17/18 |
| 12 `timeAndRestart` | Millis-Überlauf, 30 Tage, überlaufender Gate-Timer, neue Server-Sitzung ohne Freigabe | TC-07/18 |
| 13 `http` | Partielle Requests, Formularkodierung, doppelte Parameter, ungültige/mehrfache Content-Length, Chunked/Übergröße abgelehnt | Webeingaben/Fehlerbehandlung |

Beide Methoden werden ausgeführt: Karte in Integration/Lifecycle, TWINT in Payments. Die Prüfergebnisse sind **automatisierte Softwaretests**, keine manuelle Abnahme der 20 TK-Fälle.

## Tatsächlich erzeugte Firmware-Builds

Befehl: `python tools/build.py --offline`. Verwendet wurde der bereits im Repository vorhandene PlatformIO-Cache nach Prüfung des Platform-Commits. Das temporäre Konfigurationsfile ersetzt nur die Platform-Downloadadresse durch denselben lokalen Commit; Boarddefinitionen, Framework, Compiler, Quellfilter und Anwendung bleiben unverändert.

| Environment | Board | Ergebnis | Statisches RAM | Flash | UF2 erzeugt |
|---|---|---|---:|---:|---|
| `pico1-kasse` | RP2040 Pico W | SUCCESS | 76 640 B / 262 144 B (29,2 %) | 357 796 B (17,1 %) | ja |
| `pico2-schranken` | RP2040 Pico W | SUCCESS | 73 956 B / 262 144 B (28,2 %) | 352 252 B (16,8 %) | ja |
| `pico1-kasse-rp2350` | RP2350 Pico 2 W | SUCCESS | 76 828 B / 524 288 B (14,7 %) | 353 680 B (8,4 %) | ja |
| `pico2-schranken-rp2350` | RP2350 Pico 2 W | SUCCESS | 74 172 B / 524 288 B (14,1 %) | 348 256 B (8,3 %) | ja |

Vier erfolgreiche Builds, Exit-Code 0; im finalen Buildlog keine Compilerfehler oder -warnungen. Die RAM-Angaben sind Linkerwerte, **keine gemessenen Heap-/Stack-Spitzen im WLAN-Betrieb**. Alle vier Dateien liegen unter `.pio/build/<Environment>/firmware.uf2`. Sie wurden mit den Zugangsdaten-Platzhaltern gebaut: vor einem echten Upload Secrets ausfüllen und neu bauen.

Verwendete Versionen:

- Platform `1.20.0`, Git `5d4561a05e3b212660ac6fdd3fbfb328d1988aa1`.
- Arduino-Pico `1.60100.0` (Core 6.1.0), Git `fd65f6d4ab168d4bb181a3709738fe353ac93585`.
- ARM-Toolchain/PIOASM/Picotool `5.160100.260719`, Compiler 16.1.0 laut PlatformIO.
- WiFi und Servo aus diesem Core; keine zusätzlich heruntergeladene Anwendungsbibliothek.

Umgebungsbesonderheiten: Der erste Online-Build scheiterte an DNS in der eingeschränkten Umgebung. Der lokale ARM-Compiler meldete innerhalb der Sandbox einen Windows-DLL-Adressfehler. Außerhalb der Sandbox lief derselbe Compiler erfolgreich. Dies waren Umgebungsfehler, keine übergangenen C++-Buildfehler. Ein frischer Online-Download aller Abhängigkeiten wurde nicht separat nachgewiesen; der Quellstand der lokal verwendeten Abhängigkeiten ist bekannt.

## Statisch / durch Kompilierung geprüft

- API-Kompatibilität von `WiFi.beginNoBlock`, WiFiUDP, begrenztem WiFiClient-Schreiben und Servo-Attach mit initialer Schließposition für beide Chipfamilien.
- Pinbelegung mit PDF-Pinout und vorhandener Codebelegung verglichen; Winkel/Pulse zentral konfigurierbar.
- Vollständige Webseiten-/Handlerimplementierung, keine Pflichtfunktions-Platzhalter oder TODOs.
- Begrenzte HTTP-/UDP-Arbeit pro loop, keine selbst geschriebenen blockierenden Netzwerk-Warteschleifen.
- Boot-/Sequenz-/Besuchszuordnung und DHCP-Wiederentdeckung bei ausstehender Transaktion im Code geprüft; tatsächlicher DHCP-/UDP-Treiber noch auf Hardware zu testen.
- Quell- und Dokumentationsprüfung: `git diff --check`, Python-Syntaxprüfung der Werkzeuge; lokale Dokumentationslinks und TC-01–20-Zuordnung geprüft.

## Auf den angeschlossenen Picos geprüft

- Die RP2040 Kassen-Firmware wurde als `firmware.uf2` auf COM6 (USB-ID `E663A837CB54A92D`) übertragen. Die Schranken-Firmware wurde als eigene `firmware.uf2` auf COM5 (USB-ID `E663AC91D337542C`) übertragen. Beide Webseiten sind in der jeweiligen Firmware enthalten.
- Beide Boards starteten zurück als serielle USB-Geräte. HTTP GET an beiden IP-Adressen lieferte HTTP 200. `/api/config` der Kasse meldete `http://192.168.1.126` als Schranken-Pico.
- Aktueller Stand: Beide Webseiten sind direkt in der jeweiligen Firmware enthalten. Live-HTTP-GET auf `192.168.1.127/` ergab den Titel „Pico 1 · Parkhaus-Kasse“; `192.168.1.126/` ergab „Pico 2 · Einfahrt und Ausfahrt“. Beide antworteten mit HTTP 200; die Schranken-Seite meldete die Kasse als verbunden.
- Ein temporärer Einfahrt-/Zahlungs-/Ausfahrtablauf über die HTTP-Endpunkte lieferte Einfahrt 202, Zahlung 200 und Ausfahrt 202. Eine anschließende Gebührenanfrage für das Kennzeichen meldete „nicht gefunden“, damit blieb kein Testvorgang im Kassenregister.
- Die Servo-Steuerzyklen wurden per HTTP ausgelöst, die physische Bewegung und Endlagen aber nicht visuell beobachtet. Ein kompletter manueller Durchlauf aller TK-Fälle steht noch aus.

Offen bleiben: manuelle TC-01–20 als Systemtest, genaue Boardgeneration, physische Servo-Bewegung/Endlagen/Strombedarf, Browser-Usability TC-20, WLAN-Ausfall/Wiederkehr und Verhalten bei Spannungseinbrüchen. Verfahren: `HARDWARETESTS.md`; Protokollvorlage: `TESTPROTOKOLL.md`.

Automatisierte Tests und erfolgreiche Cross-Compilation begründen den Softwarestand. Sie begründen keine Behauptung, dass das vollständige TK am echten Aufbau bereits bestanden wurde.

## Nachtrag: gemeinsame Schranke tatsächlich aufgespielt

Am 12.11.2026 auf Nutzerangabe auf einen SG90 an GP8 des Schranken-Picos umgestellt. Nach dem später gewünschten physischen Rollentausch liegen die Programme nun auf den Boards ohne Servo (Kasse COM6) und mit Servo (Schranke COM5). Aktuelle WLAN-/HTTP-Prüfung siehe Abschnitt „Auf den angeschlossenen Picos geprüft“.

Live über WLAN geprüft: neue Einfahrt, simulierte Kartenzahlung, Ausfahrt und Datensatzabschluss. Für beide Richtungen meldeten Webseite und serielle Ausgabe `Gemeinsame Schranke GP8: offen` und danach `geschlossen`. Beide Öffnungen verwenden denselben Servo-Ausgang. Der Testdatensatz wurde durch die Ausfahrt entfernt. Tatsächliche mechanische Bewegung muss der Nutzer vor Ort bestätigen; die Softwareausgaben messen keine Servoposition.

Nach dem Nutzerfoto am 12.11.2026: Die abgebildete Breadboard-Verdrahtung entspricht wegen der um 90° gedrehten Board-Ausrichtung nicht den physischen Pico-Pins im Schaltbild; insbesondere liegt Servo-Rot links oben statt an VBUS rechts oben und Servo-Orange rechts statt an GP8 links. Auf dem Foto sind keine eingelöteten Stiftleisten erkennbar. Diese Beobachtung erklärt eine ausbleibende Bewegung trotz erfolgreicher Software-RPCs. Ein PIN-geschützter direkter Servotest wurde ergänzt, für den Schranken-Pico gebaut und als UF2 auf COM5 geflasht. Die neue Webseite mit Testformular antwortete über WLAN mit HTTP 200; der eigentliche Servotest wurde wegen der sichtbaren Fehlverdrahtung noch nicht ausgelöst. Eine mechanische Bestätigung ist nach Korrektur der Kontakte erforderlich.
