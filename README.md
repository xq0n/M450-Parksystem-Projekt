# Parksystem

## Gruppenmitglieder

Kilian Schoeberl, Noel Merz

## Kurze Beschreibung der Umsetzung

Das Parksystem besteht aus zwei WLAN-fähigen Raspberry Pi Picos (Pico W oder Pico 2 W) und ist mit Arduino/C++ umgesetzt. Ein Pico übernimmt die Ein- und Ausfahrt samt gemeinsamer Schranke, der andere die Kasse mit Gebührenberechnung und simuliertem Bezahlvorgang. Die Geräte kommunizieren über WLAN; die Bedienung erfolgt über ihre Webseiten. Ein Servo bewegt die Schranke.

## Umgesetzte User Stories und Anforderungen

Die konkret beschriebenen Anforderungen aus dem Testkonzept (TC-01 bis TC-20) sind umgesetzt:

| Testfälle | Umgesetzte Anforderungen |
|---|---|
| TC-01–05 | Einfahrt, Kennzeichenprüfung, Duplikaterkennung und begrenzte Parkplatzkapazität |
| TC-06–08 | Parkgebühr berechnen, Tagesmaximum und Tarif einstellen |
| TC-09–12 | Zahlung mit Karte oder TWINT simulieren, Zahlung abbrechen und unbekannte Kennzeichen behandeln |
| TC-13–16 | Bezahlte Ausfahrt freigeben, unbezahlte sperren, Kulanz und Nachzahlung |
| TC-17–19 | Status zwischen den Picos abfragen, Kommunikationsfehler behandeln und mehrere Fahrzeuge verwalten |
| TC-20 | Bedienung über die Webseiten |

<details>
<summary>Alle Testfälle TC-01 bis TC-20 anzeigen</summary>

- **TC-01:** Einfahrt mit Zeitstempel
- **TC-02:** Leere Eingabe ablehnen
- **TC-03:** Sonderzeichen ablehnen
- **TC-04:** Doppelte Kennzeichen ablehnen
- **TC-05:** Einfahrt bei vollem Parkhaus ablehnen
- **TC-06:** Mindestgebühr bei kurzer Parkdauer berechnen
- **TC-07:** Tagesmaximum bei langer Parkdauer anwenden
- **TC-08:** Tarif ohne Codeänderung anpassen
- **TC-09:** Zahlung mit Zeitpunkt erfassen
- **TC-10:** Unbekanntes Kennzeichen bei der Zahlung behandeln
- **TC-11:** Zahlung abbrechen
- **TC-12:** Karte oder TWINT auswählen
- **TC-13:** Bezahlte Ausfahrt freigeben
- **TC-14:** Unbezahlte Ausfahrt sperren
- **TC-15:** Ausfahrt innerhalb der Kulanzzeit ermöglichen
- **TC-16:** Nach Ablauf der Kulanzzeit Nachzahlung verlangen
- **TC-17:** Bezahlstatus von Pico 2 bei Pico 1 abfragen
- **TC-18:** Ausfahrt bei Kommunikationsfehler sperren
- **TC-19:** Zwei unabhängige Fahrzeuge verwalten
- **TC-20:** Verständliche Weboberflächen bereitstellen

</details>

Die zugehörigen Hosttests sind vorhanden; die manuellen Hardwaretests müssen gemäß [Hardwarecheckliste](docs/HARDWARETESTS.md) durchgeführt und im [Testprotokoll](docs/TESTPROTOKOLL.md) festgehalten werden.

Die [detaillierten Testfälle mit Prüfschritten und Soll-Ergebnissen](docs/TESTFAELLE.md) orientieren sich am Testkonzept.

## Automatisierte Tests mit Testdoubles

Grundlage sind der Auftrag **A02-01-Testdoubles-Parksystem** und das Theorieblatt **T03-02-Testdoubles**. Die Fachlogik ruft keine Arduino-, Servo-, Browser- oder WLAN-Funktionen auf. Die Firmware und die Hosttests verwenden dieselben Klassen; nur die Umgebungsadapter werden ausgetauscht.

| Abhängigkeit | Rolle und Aufwand im Test | Testansatz |
|---|---|---|
| Gemeinsame Schranke | Öffnet und schliesst per Servo. Der physische Aufbau kann auf dem PC keine Bewegung ausführen. | `BarrierOutput` erhält in der Firmware den Servo-Treiber und im Hosttest einen Mock, der Öffnungs- und Schliessbefehle aufzeichnet. |
| Zeit | Steuert Parkgebühr, Kulanz, Zeitüberschreitungen und Schrankenlaufzeit. Echtzeit würde die Tests verlangsamen. | Die Schrankenstation erhält eine `TickSource`. Auf dem Pico liest `ArduinoTicks` die Laufzeit. Im Hosttest lässt sich die `FakeClock` gezielt vorstellen. Gebühren- und Kulanztests geben Einfahrts-, Zahlungs- und Ausfahrtszeit direkt vor. |
| UDP-Verbindung zur Kasse | Übermittelt Anfragen und Antworten. Paketverluste lassen sich mit realem WLAN schwer reproduzieren. | **Fake:** Die Station erhält einen `CashLink`. `MemoryLink` ersetzt den Transport durch eine Liste ausgehender Nachrichten. Der Testaufbau verarbeitet sie mit dem echten Kassendienst und verwirft gezielt Anfragen oder Antworten. So sind Wiederholungen und Ausfälle reproduzierbar. |
| Bezahlstatus der entfernten Kasse | Liefert Freigabe oder Ablehnung zur Ausfahrt. Echte Zahlungen und zwei Boards würden einen isolierten Schranken-Test erschweren. | **Stub:** `CashReplyStub` liefert vorgegebene Ergebnisse wie `Ok` und `Unpaid` über `CashLink`. Nur die Protokollzuordnung wird aus der Anfrage übernommen; eine echte Gebühren- oder Zahlungsberechnung findet dabei nicht statt. |
| HTTP und Kennzeicheneingabe / simulierte Kamera | Liefert das eingegebene Kennzeichen. Browser und Kamera sind für feste gültige, fehlende oder fehlerhafte Testeingaben unnötig aufwändig. | **Stub:** `GateStation::action` erhält die Eingabe über `PlateInput`. In der Firmware dekodiert `HttpPlateInput` das Formular, im Test liefert `PlateInputStub` einen vorgegebenen Wert oder keine Eingabe. Der echte HTTP-Adapter und Parser werden zusätzlich ohne Browser getestet. |
| Fahrzeugspeicher | Registriert Fahrzeuge und liefert Ausfahrtberechtigungen beziehungsweise Abschluss/Storno. Bestimmte Speicherantworten und Aufrufzahlen sollen unabhängig von echten Parkvorgängen prüfbar sein. | **Mock:** `Service` erhält ein `VehicleStore`-Interface per Konstruktor. Die Firmware verwendet weiterhin den echten RAM-`Store`. `VehicleStoreMock` liefert feste Ergebnisse und protokolliert Kennzeichen, Zeit, Besuchsnummer und Stornoflags. So prüft der Test auch, dass wiederholte Pakete keine zweite Speicheränderung auslösen. Die echte Speicher- und Zahlungslogik wird separat weiter getestet. |

Die Abhängigkeiten werden von außen übergeben: `TickSource` und `CashLink` an `GateStation`, `Barrier` an `BarrierOutput`, `VehicleStore` an `Service` und `PlateInput` an die Aktion. Die produktiven Implementierungen werden in den beiden `main.cpp` zusammengesetzt. Die handgeschriebenen Doubles reichen für diese kleinen Schnittstellen aus; ein Mocking-Framework, Dummy oder Spy ist dafür nicht erforderlich.

Der Schranken-Mock prüft für **TC-13**, dass eine bezahlte Ausfahrt genau einen Öffnungsbefehl und danach genau einen Schliessbefehl auslöst. Für **TC-04, TC-05 und TC-14** prüft er, dass abgelehnte Aktionen keinen Schrankenbefehl auslösen. Die physische Servobewegung und die Verständlichkeit der Weboberflächen (**TC-20**) werden am Aufbau geprüft.

| Testkonzept | Automatisierte Prüfung auf dem PC |
|---|---|
| TC-01 bis TC-05, TC-19 | Kennzeichen, Einfahrtszeit, Duplikate, Kapazität und unabhängige Fahrzeugdaten |
| TC-01 bis TC-03 | Kennzeicheneingabe über `PlateInputStub`; zusätzlich derselbe `HttpPlateInput` wie in der Firmware mit gültigen, fehlenden und fehlerhaften Formularwerten |
| TC-06 bis TC-08 | Tarifgrenzen, Tagesmaximum und Tarifänderungen |
| TC-09 bis TC-12 | Zahlung, unbekanntes Fahrzeug, Abbruch und Zahlungsmethode |
| TC-13 bis TC-16 | Ausfahrt mit und ohne Freigabe, Kulanzgrenze und Nachzahlung. Für TC-13/14 liefert ein Kassen-Stub vorgegebene Zahlungsantworten. Fake-Uhr und Schranken-Mock prüfen die Schliessgrenze bei 4999 und 5000 ms. |
| TC-17 und TC-18 | Statusabfrage, verlorene Nachrichten, verspätete Freigabe und Wiederverbindung |
| TC-01, TC-13/14 und TC-17/18 | Kassendienst isoliert mit `VehicleStoreMock`: Ergebnisse weitergeben, Parameter übergeben, doppelte Speicherung bei Paketwiederholung vermeiden und falsche Sitzungen vor dem Speicherzugriff abweisen |

**Grenzen der Automatisierung.** Für TC-01 und TC-13 kann der Mock die Befehlsfolge prüfen. Ob der Servo seine Endposition erreicht, erfordert eine Beobachtung am Aufbau. Bei TC-17 und TC-18 prüft der PC die Protokollreaktion. Funkverbindung und Wiederanmeldung der realen Boards sind Hardwaretests. TC-20 erfordert eine Testperson, weil erst die Bedienung ohne Hilfestellung zeigt, ob die Rückmeldungen verständlich sind.

**Verifikation vom 20.11.2026:** 17 Host-Testgruppen mit 11 741 erfolgreichen Prüfungen; alle vier Firmware-Builds für Pico W und Pico 2 W erfolgreich, jeweils UF2 erzeugt. Einzelne Zählerwerte enthalten wiederholte Assertions und Protokollprüfungen und entsprechen nicht der Zahl fachlicher Testfälle. Aktuelle Buildnachweise und die Abgrenzung zur Hardwareprüfung stehen in [TESTERGEBNISSE.md](docs/TESTERGEBNISSE.md).

Das ursprüngliche Dokument mit den User Stories US-01 bis US-14 liegt nicht vor. US-03 und US-09 sind in den verfügbaren Unterlagen nicht beschrieben; US-10 bis US-14 sind im Testkonzept ausgenommen. Für diese Anforderungen wird daher keine Umsetzung behauptet. Details dazu stehen im [Anforderungsabgleich](docs/ANFORDERUNGEN.md).

## Bauen und testen

Die ausführliche [Anleitung zum Bauen und Übertragen der Firmware auf die Picos](docs/ANLEITUNG.md) beschreibt Voraussetzungen, Boardauswahl und Upload.

Für die Hosttests werden Python 3 und GCC (`g++`) oder Clang (`clang++`) mit C++17 benötigt. Der Compiler muss im `PATH` stehen; alternativ legt die Umgebungsvariable `CXX` seinen Pfad fest. Die Tests benötigen weder PlatformIO noch angeschlossene Picos. Für Firmware-Builds wird zusätzlich PlatformIO benötigt.

Zuerst die WLAN-Konfiguration anlegen und Zugangsdaten eintragen:

```powershell
Copy-Item include\parkhaus_secrets.h.example include\parkhaus_secrets.h
```

Danach Firmware bauen und Hosttests ausführen:

```powershell
python tools/build.py
python tools/test_host.py
```

Weitere Informationen zu Aufbau und Testdurchführung: [Hardwarecheckliste](docs/HARDWARETESTS.md). Architektur und Testergebnisse: [ARCHITEKTUR.md](docs/ARCHITEKTUR.md) und [TESTERGEBNISSE.md](docs/TESTERGEBNISSE.md).

## Abgabe

Der für A02 vorgesehene Branch heißt `testdoubles`. Quellcode, Testdoubles, automatisierte Tests und diese Dokumentation gehören gemeinsam in den Branch. Nach erfolgreicher Verifikation wird der Abgabestand committed und mit `git push origin testdoubles` veröffentlicht. Ein erfolgreicher Build ersetzt nicht die Prüfung der Servo-Bewegung und des WLAN-Ablaufs am Aufbau.
