# Parkhaus mit zwei Raspberry Pi Picos

**Gruppenmitglieder:** Kilian Schoeberl, Noel Merz.

## Projektbeschreibung

Zwei WLAN-fähige Picos bilden ein Parkhausmodell: Kennzeichen eingeben, einfahren, Parkgebühr berechnen, Karte/TWINT als Testzahlung bestätigen und ausfahren. Zwei echte SG90-Servos bewegen die Ein- und Ausfahrtsschranke. Die manuelle Kennzeicheneingabe und simulierte Zahlung entsprechen ausdrücklich dem Testkonzept; eine Kamera oder ein Zahlungsdienstleister wird nicht benötigt.

| Rolle | Aufgabe | Programm |
|---|---|---|
| **Pico 1: Kasse** | Fahrzeugregister, Zeitstempel, Kapazität, Tarif, Gebühren, Zahlung, Kulanz, verbindliche Freigabe | `src/pico1/main.cpp` |
| **Pico 2: Schranken** | Weboberfläche für Ein-/Ausfahrt, Kommunikation mit der Kasse, beide Servos | `src/pico2/main.cpp` |

Alle konkret beschriebenen Testfälle **TC-01 bis TC-20** sind unterstützt, einschließlich Kapazität und Kulanzzeit. Die Fachlogik und Fehlerfälle werden automatisiert auf dem PC getestet; beide Rollen sind für **Pico W (RP2040)** und **Pico 2 W (RP2350)** kompilierbar. Echte WLAN-/Servo- und Usabilitytests am Aufbau stehen noch aus. Der genaue Stand steht in [TESTERGEBNISSE.md](docs/TESTERGEBNISSE.md).

Die PDFs verweisen zusätzlich auf ein **nicht mitgeliefertes Anforderungsdokument mit US-01 bis US-14**. US-03/09 sind nicht beschrieben; US-10–14 werden im TK ausgenommen. Deshalb wird keine vollständige Umsetzung unbekannter Akzeptanzkriterien behauptet. Quellen, Widersprüche und Entscheidungen: [ANFORDERUNGEN.md](docs/ANFORDERUNGEN.md).

## Benötigte Hardware

| Anzahl | Bauteil | Hinweis |
|---|---|---|
| 2 | Raspberry Pi **Pico W/WH** oder **Pico 2 W** | WLAN wurde vom Nutzer bestätigt; genaues Modell vor dem Flashen prüfen. Auch gemischte W-Modelle können die Rollen übernehmen. |
| 2 | USB-Datenkabel passend zum Board, bei Original-Picos Micro-USB | Reine Ladekabel erlauben kein Programmieren. |
| 2 | SG90-Positionsservo, mit Hebel und leichtem Modell-Schrankenarm | A S. 2–3 zeigt einen SG90; zwei Schranken benötigen zwei Antriebe. Keine Continuous-Rotation-Servos. |
| 1 | Geregelte externe 5-V-Servo-Stromversorgung | Für beide Servos gemeinsam auslegen, auch Anlauf-/Blockierstrom berücksichtigen. 5 V / 2 A ist ein Vorschlag für das Modell, kein aus dem PDF belegter Modulwert. Tatsächliche Servodaten prüfen. |
| nach Aufbau | Breadboard/Verteiler, passende Dupont-Kabel, gelötete Stiftleisten | 2 Signalleitungen, 2 Servo-VCC, 2 Servo-GND, Masseverbindung Pico 2–Servoversorgung. |
| 1 | 2,4-GHz-WLAN mit Router/Access Point | Beide Picos und Browser im selben LAN; kein Gastnetz mit Client-Isolation. |
| 1 | PC/Laptop mit Browser und USB | Für Build, Upload und beide Webseiten; kein zusätzlicher Anwendungsserver notwendig. |

Keine zusätzlichen Taster, Lichtschranken, Displays, Kartenleser, RTC oder Kameras sind in den PDFs spezifiziert. Das Programm nimmt solche Bauteile nicht an. Ohne Durchfahrts-/Hindernissensorik ist dies ein beaufsichtigtes **Modell**, keine Schranke für Personen oder echte Fahrzeuge.

### Boardmodell erkennen

„Pico 1“ und „Pico 2“ bezeichnen hier **Rollen**, nicht die Hardwaregeneration. Auf der Platine steht „Raspberry Pi Pico W“ beziehungsweise „Raspberry Pi Pico 2 W“; auch der Chip ist mit RP2040 beziehungsweise RP2350 beschriftet. In BOOTSEL kann `INFO_UF2.TXT` weitere Boardinformationen zeigen. Vor dem Upload die passende Zeile auswählen:

| Boardmodell | Kassen-Firmware | Schranken-Firmware |
|---|---|---|
| Pico W / Pico WH, RP2040 | `pico1-kasse` | `pico2-schranken` |
| Pico 2 W, RP2350 | `pico1-kasse-rp2350` | `pico2-schranken-rp2350` |

Ein Pico ohne WLAN passt ohne zusätzliche, hier nicht spezifizierte Netzwerkhardware nicht zu diesem Aufbau.

## Verkabelungsanleitung

**Vor dem Verdrahten alle Stromquellen trennen.** Die Angaben „Pin“ sind physische Nummern der 40-poligen Leiste; `GP8`/`GP9` sind GPIO-Nummern. Board-Aufdruck und Pinout abgleichen.

### Pico 1: Kasse

| Anschluss | Verbindung |
|---|---|
| USB | PC oder geeignete USB-Stromversorgung |
| WLAN | Gemeinsames 2,4-GHz-Netz |
| GPIO / VBUS / 3V3 | Für die Kasse keine externen Anschlüsse erforderlich |

### Pico 2: Schranken und Servos

| Pico 2 / Versorgung | Ziel | Kabelfarbe bei typischem SG90 |
|---|---|---|
| **GP8, physischer Pin 11** | Signal Einfahrtservo | Orange/Gelb |
| **GP9, physischer Pin 12** | Signal Ausfahrtservo | Orange/Gelb |
| **GND, physischer Pin 8** | Minus/GND der externen Servoversorgung | Gemeinsame Masse |
| Externes **+5 V** | VCC beider Servos | Rot |
| Externes **GND** | GND beider Servos | Braun/Schwarz |
| USB Pico 2 | PC oder USB-Stromversorgung für Pico 2 | USB-Datenkabel beim Flashen |

GP8 folgt dem Beispiel auf A S. 3. GP9 stammt aus der bestehenden Codebase und ist eine **konfigurierbare Projektbelegung**, keine zweite Pinvorgabe des PDFs. Beide W-Boardgenerationen verwenden für diese Anschlüsse dieselben physischen Pinnummern.

```text
Pico 1 (Kasse)  <--- gemeinsames WLAN --->  Pico 2 (Schranken)
                                              GP8 ---- Signal Einfahrt
                                              GP9 ---- Signal Ausfahrt
                                              GND ----+---- GND Servo 1
                                                      +---- GND Servo 2
                                                      +---- Minus 5-V-Netzteil
                          Plus 5-V-Netzteil ------------+---- VCC Servo 1
                                                      +---- VCC Servo 2
```

Die beiden Picos brauchen **keine Datenleitung untereinander**. Gemeinsame Masse ist für **Pico 2 und beide Servos** zwingend; eine zusätzliche Masseleitung zu Pico 1 ist für WLAN nicht erforderlich.

GPIOs führen **3,3-V-Logik**. Keine 5 V an GPIO oder `3V3(OUT)` anschließen. Servos nicht aus `3V3(OUT)` speisen. Beim beschriebenen Aufbau bleibt der Pluspol der externen Servoversorgung von Pico-VBUS/VSYS getrennt; nur die Masse wird verbunden. So werden USB und externe Versorgung nicht über ihre Pluspole miteinander verbunden. A S. 3 zeigt VBUS/Pin 40 für **einen** Servo als Beispiel; hier wird für zwei Servos eine getrennte Versorgung verwendet.

### Servos ausrichten

1. Schrankenarme zunächst abnehmen. Signale und Masse wie oben anschließen.
2. Firmware starten: beide Ausgänge starten unmittelbar mit der konfigurierten Schließposition.
3. Strom ausschalten, Hebel in geschlossener Position montieren, wieder einschalten.
4. Einfahrt/Ausfahrt testen; bei Anschlag, Brummen oder falscher Richtung abschalten und Montage/Winkel anpassen.
5. Alle Werte stehen in `include/parkhaus_config.h`: getrennte Offen-/Geschlossenwinkel, `SERVO_MIN_US`, `SERVO_MAX_US`, `GATE_OPEN_MS`, beide GPIOs. Startwerte aus dem Bestand: 0°/90°, 2500 ms, Pulse 540–2400 µs. Die passenden mechanischen Endlagen müssen am tatsächlichen Servo überprüft werden.

## Softwareinstallation

Verwendet werden **C++ mit Arduino-Pico und PlatformIO**, wie bereits im Repository und in A S. 1 vorgeschrieben. MicroPython muss nicht installiert werden. Ein UF2-Upload ersetzt eine eventuell vorhandene MicroPython-Firmware.

1. Git, Python 3 und VS Code installieren; in VS Code die Erweiterung **PlatformIO IDE** öffnen. Alternativ PlatformIO Core über `python -m pip install platformio` installieren.
2. Ein **PlatformIO-Terminal** öffnen, damit `pio` verfügbar ist. Prüfen:

   ```powershell
   pio --version
   git --version
   python --version
   ```

3. Der erste Build lädt Arduino-Pico, Compiler und Uploadtools; Internetzugang ist dafür nötig. WiFi und Servo sind im Core enthalten, **keine zusätzlichen Bibliotheken** sind zu installieren.

Die Platform-Version ist auf Commit `5d4561a05e3b212660ac6fdd3fbfb328d1988aa1` fixiert. Dieser verwendet Arduino-Pico `fd65f6d4ab168d4bb181a3709738fe353ac93585` (6.1.0) und die zugehörige Toolchain. Bei Windows-Pfadproblemen einen kurzen Repositorypfad wählen; die [offizielle PlatformIO-Anleitung des Cores](https://arduino-pico.readthedocs.io/en/latest/platformio.html) erläutert lange Pfade.

## Schritt für Schritt: aufbauen, programmieren und starten

### 1. Repository vorbereiten

Für dieses vorhandene Repository:

```powershell
Set-Location 'C:\Users\NMerz\Documents\m450-projekt'
Copy-Item include/parkhaus_secrets.h.example include/parkhaus_secrets.h
```

Beim erneuten Einrichten eine bereits ausgefüllte Secrets-Datei **nicht überschreiben**. `include/parkhaus_secrets.h` bearbeiten:

```cpp
static constexpr char PARKING_WIFI_SSID[] = "DEIN_WLAN";
static constexpr char PARKING_WIFI_PASSWORD[] = "DEIN_PASSWORT";
#define PARKING_ADMIN_PIN "4500"
```

SSID und Passwort gelten für beide Picos; die Datei ist von Git ausgeschlossen. Ohne echte WLAN-Daten baut die Firmware, verbindet sich aber nicht. Der PIN schützt die lokalen Tarif-/Testformulare vor versehentlichen Änderungen; er ist kein Schutz für einen öffentlichen Internetdienst.

Standardwerte in `include/parkhaus_config.h` prüfen. Für den vollständigen TK-Durchlauf bleibt `PARKING_TEST_MODE = true`, damit Zeit vorsetzbar ist. Für normalen Modellbetrieb kann diese Testhilfe auf `false` gesetzt werden.

### 2. Pico 1 anschließen

Board als **„Kasse“** beschriften. Zunächst nur diesen Pico per USB-Datenkabel anschließen. Für den ersten Upload BOOTSEL beim Einstecken gedrückt halten, dann loslassen. Es erscheint ein USB-Laufwerk; dessen Name hängt vom Modell ab.

### 3. Code auf Pico 1 übertragen

Für Pico W/RP2040:

```powershell
pio run -e pico1-kasse
pio run -e pico1-kasse -t upload
```

Für Pico 2 W/RP2350 stattdessen:

```powershell
pio run -e pico1-kasse-rp2350
pio run -e pico1-kasse-rp2350 -t upload
```

Bei Uploadproblemen nach dem erfolgreichen Build die passende `firmware.uf2` manuell auf das BOOTSEL-Laufwerk kopieren, beispielsweise für Pico W (Laufwerksbuchstaben zuerst im Explorer prüfen):

```powershell
Copy-Item .pio/build/pico1-kasse/firmware.uf2 E:\
```

Für RP2350 entsprechend `.pio/build/pico1-kasse-rp2350/firmware.uf2` verwenden. Das Board startet nach dem Kopieren automatisch; das Laufwerk verschwindet.

Seriellen Port ermitteln und Monitor öffnen:

```powershell
pio device list
pio device monitor --port COM5 --baud 115200
```

`COM5` durch den tatsächlich angezeigten Port ersetzen. Bei erfolgreichem WLAN erscheint alle 15 Sekunden `Weboberflaeche: http://...`. IP notieren und im Browser öffnen. Den Monitor mit `Ctrl+C` beenden, bevor derselbe Port erneut geflasht wird.

### 4. Pico 2 anschließen

Kasse vom Programmier-PC trennen oder klar getrennte Ports verwenden. Zweites Board **„Schranken“** beschriften. Servoversorgung noch ausschalten, Verkabelung prüfen und nur Pico 2 per USB anschließen. BOOTSEL wie oben verwenden.

### 5. Code auf Pico 2 übertragen

Für Pico W/RP2040:

```powershell
pio run -e pico2-schranken
pio run -e pico2-schranken -t upload
```

Für Pico 2 W/RP2350:

```powershell
pio run -e pico2-schranken-rp2350
pio run -e pico2-schranken-rp2350 -t upload
```

Alternativ das entsprechende `.pio/build/<Environment>/firmware.uf2` auf das BOOTSEL-Laufwerk kopieren. Port mit `pio device list` ermitteln, Monitor öffnen und zweite IP notieren. Nie die Kassen-Firmware auf beide Boards kopieren.

### 6. Picos verbinden

Beide mit demselben WLAN starten. Die Firmware entdeckt die Kasse über UDP im lokalen Subnetz. **Kein UART/I2C/SPI-Kabel** ist notwendig. Beide Webseiten sind per IP erreichbar; mDNS/`.local` wird nicht vorausgesetzt. In der Schrankenansicht erscheint nach dem Handshake „Kasse: verbunden“ und ein Link zur Kasse.

Blockiert das WLAN Broadcasts, der direkte Verkehr ist aber erlaubt: Kassen-IP im Router reservieren und `PARKING_CASH_IP` in `include/parkhaus_config.h` auf diese numerische Adresse setzen, beispielsweise `"192.168.1.42"` (nur Beispiel). Schranken-Firmware neu bauen/übertragen. UDP-Port **4210** und HTTP-Port **80** müssen zwischen den betreffenden Geräten erreichbar sein; bei Client-Isolation hilft eine feste IP allein nicht.

### 7. Gesamtsystem starten

Bewegungsbereich freihalten. Pico 1 starten, dann Pico 2. Servoversorgung mit angeschlossener gemeinsamer Masse einschalten. Beide Schranken müssen geschlossen sein. Beide Webseiten öffnen und Status aktualisieren. Bei getrennten Stromquellen Signal nicht ohne Servomasse betreiben.

### 8. Grundfunktion kontrollieren

Auf Pico 2 `LU 123456` einfahren lassen: Einfahrt öffnet etwa 2,5 s. Pico 1 zeigt das Kennzeichen mit Zeitstempel. Vor Zahlung eine Ausfahrt versuchen: Ausfahrt bleibt zu. An Pico 1 „Gebuehr berechnen“, Zahlungsmethode wählen und ausdrücklich bestätigen. Danach auf Pico 2 ausfahren: Ausfahrt öffnet etwa 2,5 s und der Datensatz verschwindet nach bestätigter Abschlussmeldung.

Die Schranken-Webseite aktualisiert einen laufenden Vorgang automatisch alle 2 s; sonst „Status aktualisieren“ verwenden. Ein zweiter Vorgang wird während einer Bewegung oder eines ungeklärten Abgleichs abgewiesen.

### 9. Vollständiges TK demonstrieren

Die [Hardwarecheckliste](docs/HARDWARETESTS.md) enthält für **alle 20 Fälle** konkrete Schritte und Sollwerte. Für Langzeittests auf der Kasse unter „Testmodus aktiv“ die globale Testzeit vorsetzen (Admin-PIN). Für TC-05 Kapazität auf 2 setzen; für TC-16 nach Zahlung 31 Minuten vorsetzen. Danach Standardwerte wiederherstellen oder beide Boards neu starten.

Ergebnisse in [TESTPROTOKOLL.md](docs/TESTPROTOKOLL.md) eintragen. Kritische Fälle TC-14 und TC-18 zuerst absichern. Für eine Vorführung: Einfahrt → unbezahlte Ausfahrt abgelehnt → Gebühr → Abbruch → Zahlung → Ausfahrt → zweites Fahrzeug → Kapazitäts-/Kulanz-/Verbindungsfehler zeigen. Geplanter Unterrichtsablauf und Rollen stehen in der Checkliste.

## Tarif, Zeit und gespeicherte Daten

- Demo: **CHF 0.50 / angefangene 30 min**, höchstens **CHF 15.00 / 24-h-Abschnitt**, **15 min Kulanz**, **10 Parkplätze**. Über die Kassenoberfläche ohne Codeänderung einstellbar.
- 0–30 Minuten kosten eine Einheit; bei 30 Minuten + 1 ms beginnt die zweite. Ein ganzer 24-h-Abschnitt kostet `min(angefangene Tagesintervalle × Preis, Tagesmaximum)`; weitere Tage werden ebenso berechnet. Ein angebrochener Folgetag wird separat gekappt.
- Ein Zahlungsangebot ist 120 Sekunden gültig. Erst „Zahlung verbindlich simulieren“ speichert Betrag, Methode und Zahlungszeitpunkt. „Abbrechen“ entwertet das Angebot. Alte Formulare oder doppelte Bestätigungen können keine zweite Zahlung oder neue Kulanz auslösen.
- Exakt am Ende der Kulanz ist die Ausfahrt noch möglich. Danach muss an der Kasse neu bestätigt werden. Nachzahlung = aktuelle Gesamtgebühr minus bisherige Zahlungen, mindestens 0. Innerhalb eines schon bezahlten Intervalls oder am Tagesmaximum kann der Rest 0 sein; die abgelaufene Ausfahrtberechtigung erneuert sich trotzdem erst durch Bestätigung. Tarifänderungen entwerten offene Angebote, ändern aber keine bereits bestätigte Kulanzfrist.
- Zeitstempel sind Laufzeit seit Kassenstart, keine Kalenderzeit. 64-Bit-Erweiterung behandelt den `millis()`-Überlauf. Die Testuhr verändert Gebühren/Kulanz für alle Datensätze; Servo-/Netzwerkfristen laufen unverändert in Echtzeit.
- Fahrzeug- und Tarifdaten liegen im RAM. Bei Neustart der Kasse sind sie weg. Ein Neustart beider Picos stellt den Grundzustand her; keine Wiederherstellung produktiver Parkvorgänge.

## Kommunikation und Fehlerverhalten

Pico 2 sendet strukturierte 48-Byte-UDP-Nachrichten mit Version, Operation, Link-ID, beiden Boot-Sitzungen, Sequenz, Besuchsnummer, Kennzeichen und CRC32. Pico 1 bestätigt dieselbe Anfrage; doppelte Anfragen erhalten die gecachte Antwort und verändern den Zustand nicht nochmals. Fremde/veraltete/fehlerhafte Antworten öffnen keine Schranke.

Wiederholung alle 300 ms, Freigabefrist 1800 ms. Nach Fristablauf bleibt die Schranke geschlossen; der identische Vorgang wird im Hintergrund abgeglichen. Eine erst danach eingetroffene positive Antwort wird storniert, **nicht nachträglich geöffnet**. Ein vollzogener Ausgang wird nach Schließen mit Besuchsnummer bestätigt und bis zur Antwort wiederholt. Im Leerlauf läuft ein Heartbeat. WLAN-Verbindung und Sockets werden nach Ausfall wieder aufgebaut. Browser und Servo-Timer warten nicht auf eine Kommunikationsantwort.

Bereits rechtmäßig geöffnete Schranken schließen nach ihrer normalen Haltezeit. Ohne Sensor ist das Ende dieser Zeit die angenommene Durchfahrt. Bei Stromausfall des Schranken-Picos während einer Bewegung kann die reale Passage nicht rekonstruiert werden; mechanischen Aufbau prüfen und beide Boards kontrolliert neu starten. CRC und Link-ID sind **keine kryptografische Authentifizierung**; Betrieb im isolierten Übungsnetz entsprechend TK S. 4. Protokoll und Zustandsabläufe: [ARCHITEKTUR.md](docs/ARCHITEKTUR.md).

## Testanleitung

### Automatisierte Tests ohne Hardware

Python 3 und GCC (`g++`) oder Clang (`clang++`) im PATH bereitstellen. Unter Windows kann GCC beispielsweise aus MSYS2 UCRT64 kommen; dessen `bin`-Verzeichnis muss im PATH stehen. Dann:

```powershell
python tools/test_host.py
```

Es werden die **gleichen C++-Klassen wie in der Firmware** kompiliert und ausgeführt: Kennzeichen, Gebühren, Kapazität, Zahlung/Abbruch, Kulanz, Protokoll, Controller, Paketverlust/Wiederholung, Überläufe, HTTP-Parser. Keine Arduino-Hardware wird dabei behauptet. Optional auf einem Compiler mit Sanitizer-Runtime:

```sh
python tools/test_host.py --sanitize
```

Alle vier Firmware-Ziele bauen:

```powershell
python tools/build.py
```

Alternativ `pio run` für die zwei Standardziele oder einzelne `pio run -e ...` aus der Tabelle. Nur auf dieser Arbeitskopie mit bereits vorhandenem passendem `.pio-core`-Cache ist zusätzlich möglich:

```powershell
python tools/build.py --offline
```

Der Offline-Modus prüft den Platform-Git-Commit, bevor er den Cache verwendet. Er ist keine Voraussetzung für einen frischen Checkout. Build-Ausgaben liegen unter `.pio/build/`, Host-Binaries unter `build/host/`; beides wird nicht eingecheckt.

### Echte Hardware und Bedienbarkeit

[HARDWARETESTS.md](docs/HARDWARETESTS.md) vollständig durchführen. Automatisch bestandene Fachtests sind **kein** bestandenes manuelles TK-Protokoll. WLAN, Stromversorgung, mechanische Winkel, tatsächliches Schließen bei langsamem Browser/Netzfehler und Bedienbarkeit mit einer unvorbereiteten Testperson sind offen, bis sie am Aufbau überprüft wurden.

## Anforderungsmatrix

Quellen: **A** = [Umsetzungsauftrag](docs/sources/umsetzung-parksystem.pdf), **TK** = [Testkonzept](docs/sources/testkonzept.pdf); jeweils PDF-Seiten ab 1. **I/H** = implementiert, Hosttest bestanden; Hardware-/Browser-Systemtest noch offen. **I/B** = implementiert und Firmware gebaut; physische/Usability-Prüfung offen. Detaillierte Testgruppen stehen im Ergebnisbericht.

| Anforderung | Quelle und Seite | Pico | Implementierungsdatei | Testmethode | Status |
|---|---|---|---|---|---|
| Genau 2 Picos, Arduino, Quellcode/README, Gruppenmitglieder | A 1; TK 2/4 | beide | `platformio.ini`, beide `main.cpp`, README | Vier Firmware-Builds, Aufbaukontrolle | I/B |
| SG90-Ein-/Ausfahrt, reales Servosignal | A 1–3; TK 2 | 2 | `src/pico2/main.cpp`, `parkhaus_config.h` | GateTimer-Hosttest; Servo-/Stromtest | I/B |
| TC-01 Einfahrt mit Zeitstempel | TK 9–10, US-01 | 1+2 | `parking/core.h`, `parking/controller.h` | Host `lifecycle/integration`, Hardware TC-01 | I/H |
| TC-02 Leere Eingabe ablehnen | TK 10, US-01 | beide | `parking_common.h`, beide `main.cpp` | Host `inputs/denied`, Hardware TC-02 | I/H |
| TC-03 Sonderzeichen ablehnen | TK 10, US-01 | beide | `parking_common.h` | Host `inputs/denied`, Hardware TC-03 | I/H |
| TC-04 Duplikate ablehnen | TK 10–11, US-01/02 | 1+2 | `parking/core.h` | Host `lifecycle/denied`, Hardware TC-04 | I/H |
| TC-05 Volles Parkhaus | TK 11, US-02 | 1+2 | `parking/core.h`, `src/pico1/main.cpp` | Host Kapazitätsgrenzen, Hardware TC-05 | I/H |
| TC-06 Kurze Parkdauer/Mindestgebühr | TK 11, US-04 | 1 | `parking/core.h` | Host `billing`, Hardware TC-06 | I/H |
| TC-07 Lange Dauer/Tagesmaximum | TK 11–12, US-04 | 1 | `parking/core.h` | Host Tages-/Mehrtagesgrenzen, Hardware TC-07 | I/H |
| TC-08 Tarifänderung ohne Codeänderung | TK 12, US-04 | 1 | `parking/core.h`, `src/pico1/main.cpp` | Host Tarifwechsel, Webformular TC-08 | I/H |
| TC-09 Zahlung mit Zeitpunkt | TK 12, US-05 | 1 | `parking/core.h`, `src/pico1/main.cpp` | Host `payments`, Hardware TC-09 | I/H |
| TC-10 Unbekanntes Kennzeichen | TK 12, US-05 | 1 | `parking/core.h` | Host `payments`, Hardware TC-10 | I/H |
| TC-11 Zahlung abbrechen | TK 13, US-05/06 | 1 | `parking/core.h`, `src/pico1/main.cpp` | Host `payments`, Hardware TC-11 | I/H |
| TC-12 Karte/TWINT auswählen | TK 13, US-06 | 1 | `parking/core.h`, `src/pico1/main.cpp` | Host beide Methoden, Hardware TC-12 | I/H |
| TC-13 Bezahlte Ausfahrt | TK 13, US-07 | 1+2 | `parking/core.h`, `parking/controller.h` | Host `integration/grace`, Hardware TC-13 | I/H |
| TC-14 Unbezahlte Ausfahrt sperren | TK 13–14, US-07 | 1+2 | `parking/core.h`, `parking/controller.h` | Host `denied/grace`, kritischer Hardwaretest | I/H |
| TC-15 Innerhalb Kulanz ausfahren | TK 14, US-08 | 1+2 | `parking/core.h` | Host Fristgrenze, Hardware TC-15 | I/H |
| TC-16 Nach Kulanz nachzahlen | TK 14, US-08 | 1+2 | `parking/core.h`, `src/pico1/main.cpp` | Host Ablauf/Nachzahlung, Hardware TC-16 | I/H |
| TC-17 Statusabfrage Pico 2 → 1 | TK 14, US-07 | beide | `parking/protocol.h`, `drivers/runtime.h` | Kodierte Integration, UART-Logs am Aufbau | I/H |
| TC-18 Kommunikationsfehler sperrt | TK 14–15, US-07 | beide | `parking/controller.h`, `parking/protocol.h` | Verlust/Timeout/Neustart-Hosttests; WLAN-Test | I/H |
| TC-19 Zwei unabhängige Fahrzeuge | TK 15, US-01/07 | 1+2 | `parking/core.h` | Host `lifecycle/integration`, Hardware TC-19 | I/H |
| TC-20 Verständliche Weboberflächen | TK 15 | beide | beide `main.cpp`, `web/ui.h` | Firmware-Build; Testperson ohne Erklärung | I/B |
| Testumgebung, Rollen, Reset, Protokoll, Abnahme | TK 4–8/16–18 | beide/Team | `docs/HARDWARETESTS.md`, `TESTPROTOKOLL.md` | Manuelle Durchführung und Auswertung | vorbereitet, ausstehend |
| Automatisierte Tests und Fehlerfälle | Nutzerauftrag; zusätzlich zu TK 3/5 | Host | `test/test_parking.cpp`, `tools/test_host.py` | Tatsächlich ausgeführter Hosttest | bestanden |
| US-03/09 | Nicht beschrieben; Verweis TK 1/18 | offen | keine erfundene Umsetzung | Originalanforderungen fehlen | nicht beurteilbar |
| US-10–14 | TK 3 ausdrücklich ausgenommen | offen | keine vollständige Umsetzung behauptet | Akzeptanzkriterien fehlen | außerhalb TK-Prototyp |

Pfade der gemeinsamen Header in der Tabelle sind relativ zu `include/`.

## Fehlerbehebung

| Problem | Prüfung / Lösung |
|---|---|
| `pio` nicht gefunden | PlatformIO-Terminal benutzen oder PlatformIO Core installieren. VS-Code-Installation unter Windows typischerweise `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" ...`. |
| Build lädt nicht / DNS-Fehler | Internet-/Proxyzugriff auf GitHub und PlatformIO prüfen. Vorhandener korrekter Projektcache: `python tools/build.py --offline`. |
| Falsche Firmware / kein Laufwerk | Boardmodell prüfen, BOOTSEL beim USB-Einstecken halten, USB-Datenkabel verwenden. Richtiges UF2 für Rolle und Chip kopieren. |
| Port belegt | Serial Monitor mit Ctrl+C schließen, andere IDE-Monitore schließen, `pio device list` erneut ausführen. |
| WLAN verbindet nicht | Echte Secrets vor dem Build eintragen, danach beide Boards neu flashen; 2,4 GHz und gemeinsames LAN prüfen. Wiederholversuch alle 15 s. |
| IP-Ausgabe verpasst | Monitor 115200 Baud öffnen und bis zu 15 s warten; alternativ DHCP-Liste des Routers nach Hostname prüfen. |
| Webseite geht, Kasse wird nicht gefunden | Client-Isolation/Firewall/UDP 4210/Broadcast prüfen; gegebenenfalls `PARKING_CASH_IP` und DHCP-Reservierung verwenden. Beide Programme müssen dieselbe Link-ID haben. |
| „Stoerung … abgeglichen“ | Kasse/Verbindung wiederherstellen und warten. Nach Timeout wird keine alte Freigabe ausgeführt. Nach erfolgreichem Abgleich Vorgang neu eingeben. |
| „Zustandsabgleich“/Ausfahrt blockiert nach Neustart | Physische Passage kann ohne Sensor nicht rekonstruiert werden. Modell prüfen, beide Picos ausschalten und für einen neuen Test neu starten. |
| „Vorgang abgelaufen“ bei Zahlung | Angebot älter als 120 s, abgebrochen, schon verwendet oder durch Tarif-/Testzeitänderung entwertet. Gebühr neu berechnen. |
| Nach Kulanz Restbetrag CHF 0.00 | Bereits bezahltes Intervall oder Tagesmaximum deckt die Gebühr; Frist dennoch abgelaufen. Neue Bestätigung erforderlich. |
| Servo bewegt sich nicht / Pico startet neu | Servo-5-V, gemeinsame Masse, richtige Signalpins, genügend Strom, tatsächlicher Positionsservo und mechanische Blockierung prüfen. |
| Servo läuft falsch herum / brummt | Hebel abnehmen, Winkel/Pulsgrenzen in Konfiguration anpassen und neu flashen. Nicht gegen mechanischen Anschlag betreiben. |
| Daten nach Neustart weg | Erwartetes RAM-Verhalten im TK-Prototyp. Neuer Test beginnt mit leerem Register und Standardtarif. |
| Browser zeigt alten Status | „Status aktualisieren“ bzw. Kassenansicht neu öffnen. Kein Browsercache; keine permanente Liveanzeige im Leerlauf. |

## Projektstruktur und technische Quellen

```text
src/pico1/main.cpp             Kassen-Webablauf und UDP-Dienst
src/pico2/main.cpp             Schranken-Webablauf und Servo-Anbindung
include/parkhaus_config.h      Hardware-/Netz-/Demo-Konfiguration
include/parkhaus_secrets.h.example
include/parking_common.h      Kennzeichen, Zahlen, Uhr, Schranken-Timer
include/parking/core.h        Gebühren und Parkvorgänge
include/parking/protocol.h    Frameformat, CRC, Dienst, Wiederholungen
include/parking/controller.h  Schranken-Zustandsautomat
include/parking/http_parser.h Begrenzter HTTP-/Formularparser
include/drivers/              Echte WLAN-/UDP-/HTTP-Anbindung
include/web/ui.h              Gemeinsame HTML-Ausgabe
test/test_parking.cpp         Hosttests derselben C++-Logik
tools/                       Build- und Teststarter
docs/                        Quellenabgleich, Architektur, Tests, Bericht
```

Offizielle technische Referenzen: [Raspberry-Pi-Boarddokumentation](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html), [Pico-W-Datenblatt und 3,3-V-GPIOs](https://datasheets.raspberrypi.com/picow/pico-w-datasheet.pdf), [Arduino-Pico WLAN](https://arduino-pico.readthedocs.io/en/stable/wifi.html), [UDP](https://arduino-pico.readthedocs.io/en/latest/wifiudp.html), [PIO-Servo und Pulsgrenzen](https://arduino-pico.readthedocs.io/en/latest/servo.html). Für die konkrete API wurde zusätzlich der lokal installierte und im Build verwendete Core geprüft.
