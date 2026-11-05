# Parkhaus mit automatischer Schrankensteuerung

**Gruppenmitglieder:** Kilian Schoeberl, Noel Merz

## Umsetzung

Das Projekt baut zwei unabhaengige Arduino-Firmwares fuer Raspberry Pi Pico W mit PlatformIO:

- **Pico 1 – Kasse:** fuehrt die aktiven Parkvorgaenge, Einfahrts- und Zahlungszeitpunkte sowie den Bezahlstatus. Er stellt die Kassen-Weboberflaeche, Tarifkonfiguration und HTTP-Schnittstelle bereit.
- **Pico 2 – Schranken:** stellt die Weboberflaeche fuer die manuelle Kennzeicheneingabe (Kamera-Simulation) bereit, registriert Einfahrten bei Pico 1, fragt vor der Ausfahrt den Bezahlstatus ab und steuert die beiden SG90-Servos.

Pico 1 ist die zentrale Datenquelle. Pico 2 oeffnet die Ausfahrt nur bei einer positiven Antwort von Pico 1. Bei WLAN- oder HTTP-Fehlern bleibt die Ausfahrt geschlossen. Nach dem Oeffnen meldet Pico 2 die erfolgte Ausfahrt an Pico 1; fehlgeschlagene Meldungen werden wiederholt.

## Umgesetzte Anforderungen und User Stories

Das Hauptdokument enthaelt den minimalen Umfang (Ein-/Ausfahrt, Gebuehrenberechnung, Bezahlung), aber nicht die referenzierten Akzeptanzkriterien der User Stories. Die untenstehende Abdeckung leitet sich daher aus den konkreten Testfaellen TC-01 bis TC-20 im Testkonzept ab.

- **US-01 – Einfahrt:** Kennzeichenvalidierung, Duplikatpruefung und Einfahrtszeitpunkt; Pico 2 oeffnet erst nach erfolgreicher Registrierung bei Pico 1.
- **US-02 – Mehrfach-/Kapazitaetspruefung:** doppelte Einfahrten werden abgelehnt. Eine fachliche Parkplatzkapazitaet ist nicht konfiguriert; TC-05 ist daher nicht anwendbar. Der feste RAM-Speicher kann 32 gleichzeitige Datensaetze halten.
- **US-04 – Gebuehr:** Abrechnung je angefangener Zeiteinheit mit Mindestgebuehr und Tagesmaximum. Der Tarif ist ueber die Kassen-Weboberflaeche ohne Programmierung aenderbar.
- **US-05/US-06 – Bezahlung:** Karte oder TWINT werden als Testzahlung simuliert. Der Status und Zahlungszeitpunkt werden erst nach ausdruecklicher Bestaetigung gesetzt. Ohne Bestaetigung bleibt der Vorgang unbezahlt.
- **US-07 – Ausfahrt:** Pico 2 fragt Pico 1 ueber HTTP ab und oeffnet nur bei Status „bezahlt“. Unbezahlte, unbekannte Kennzeichen und Kommunikationsfehler fuehren zu einer geschlossenen Schranke.
- **US-08 – Kulanzzeit:** nicht umgesetzt; TC-15 und TC-16 sind gemaess Testkonzept nicht anwendbar.

Betreiber-Dashboard, Tagesumsatzbericht, Dauerparkkarten, Notfalloeffnung und Datenloeschung (US-10 bis US-14) sind im Testkonzept ausdruecklich ausserhalb des Prototyp-Umfangs.

## Tarif und Zeit

Die PDFs geben keine konkreten Geldbetraege vor. Die konfigurierbaren Demo-Startwerte sind deshalb **CHF 0.50 je angefangene 30 Minuten** und **CHF 15.00 Maximum je 24-Stunden-Abschnitt**. Die Formel ist `angefangene Intervalle × Intervallpreis`, pro 24-Stunden-Abschnitt auf das Tagesmaximum begrenzt. Der Tarif wird am Kassen-Pico unter „Tarif einstellen“ geaendert. Fuer TC-06/TC-07 gibt es auf der Vorgangsseite eine als Testhilfe markierte Eingabe, mit der die verstrichenen Minuten simuliert werden koennen.

Zeitstempel und Dauern basieren auf der monotonen Laufzeit des Kassen-Picos. Ohne Echtzeituhr und ohne Internetzeitdienst wird keine Kalenderzeit angezeigt. Fahrzeug- und Tarifdaten liegen im RAM; ein Neustart setzt sie zurueck. Das entspricht dem im Testkonzept vorgesehenen Zuruecksetzen zwischen Testfaellen.

## WLAN und Verkabelung

Beide Picos und der Browser muessen mit demselben **2,4-GHz-WLAN** verbunden sein. Vor dem ersten Flashen `include/parkhaus_secrets.h.example` nach `include/parkhaus_secrets.h` kopieren und dort `PARKING_WIFI_SSID` sowie `PARKING_WIFI_PASSWORD` eintragen. Die echte Zugangsdaten-Datei wird nicht in Git aufgenommen; ohne sie bleibt der Quellcode mit Platzhaltern kompilierbar. Pico 1 ist unter `parkhaus-kasse.local` erreichbar; seine IP-Adresse wird zusaetzlich im Serial Monitor ausgegeben. Pico 2 gibt seine IP ebenfalls im Serial Monitor aus. Falls das WLAN mDNS blockiert, `PARKING_CASH_HOST` in [parkhaus_config.h](include/parkhaus_config.h) auf die IP-Adresse von Pico 1 setzen und Pico 2 erneut flashen.

Beide Servo-Signale sind an Pico 2 angeschlossen: **Einfahrt GP8**, **Ausfahrt GP9**. Das Hauptdokument zeigt beim SG90 Signal GP8, GND an GND (Pin 8) und VCC an VBUS (Pin 40). Bei zwei Servos eine stabile 5-V-Versorgung mit gemeinsamer Masse verwenden, falls die USB-Versorgung des Pico nicht ausreicht. Die Servos sind bei 0 Grad geschlossen und werden beim Freigeben fuer 2.5 Sekunden auf 90 Grad gestellt.

## In VS Code bauen und flashen

1. Den Projektordner in VS Code oeffnen und die empfohlene Erweiterung **PlatformIO IDE** installieren. PlatformIO laedt die im `platformio.ini` festgelegte Plattform und benoetigten Build-Werkzeuge beim ersten Build.
2. `include/parkhaus_secrets.h.example` nach `include/parkhaus_secrets.h` kopieren und WLAN-Zugangsdaten eintragen.
3. Das passende Environment in der PlatformIO-Seitenleiste auswaehlen:
   - `pico1-kasse` fuer Pico 1
   - `pico2-schranken` fuer Pico 2
4. **Build** ausfuehren. Zum Flashen den jeweiligen Pico per USB anschliessen, bei Bedarf BOOTSEL halten, dann **Upload** fuer das ausgewaehlte Environment ausfuehren.
5. Den Serial Monitor mit **115200 Baud** oeffnen und die ausgegebene IP-Adresse notieren. Beide Weboberflaechen koennen danach im Browser ueber ihre IP-Adresse geoeffnet werden.

In der PlatformIO CLI lauten die entsprechenden Befehle:

```sh
pio run -e pico1-kasse
pio run -e pico2-schranken
pio run -e pico1-kasse -t upload
pio run -e pico2-schranken -t upload
```

## Testfaelle aus tk.pdf

Die Weboberflaechen unterstuetzen die manuellen Schritte fuer TC-01 bis TC-04, TC-06 bis TC-14 sowie TC-17 bis TC-20. TC-05 (Parkplatzkapazitaet) sowie TC-15/TC-16 (Kulanzzeit) werden als **n. a.** protokolliert, wie im Testkonzept vorgesehen, wenn die Funktionen nicht Teil des Prototyps sind. Eine echte Kamera, ein Zahlungsdienstleister und Tests der physischen Schranken sind nicht enthalten.

## Offene Hardwarepruefungen

Die Firmwares und die PlatformIO-Builds sind fuer die zwei genannten Pico-W-Environments vorgesehen. In dieser Umgebung kann kein WLAN- oder Servotest an realer Hardware durchgefuehrt werden. Vor der Vorfuehrung bitte WLAN-Zugang, Erreichbarkeit beider Webseiten, Servo-Winkel, Stromversorgung sowie die Fail-safe-Faelle TC-14 und TC-18 am Aufbau pruefen.

