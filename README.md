# Inkpot

Ein einfaches, objektbasiertes Malprogramm für Linux — C++ mit Qt 6.

Alles, was auf der Arbeitsfläche entsteht, ist ein eigenständiges **Objekt**:
Pinselstriche und eingefügte Bilder lassen sich auswählen, verschieben und
wieder löschen. Gespeichert wird im Projektformat `*.ipot`, das diese
Objekte erhält; der Export erzeugt ein flaches Bild.

## Stand

**Meilenstein 1 — Grundlegendes Programm**

* Datei-Menü: Neu (Standardgrößen und benutzerdefiniert), Öffnen, Speichern,
  Speichern unter, Exportieren, In die Zwischenablage kopieren, Einfügen,
  Drucken, Beenden
* Arbeitsfläche liegt fest in der linken oberen Ecke und wächst nach rechts
  und unten — Anfasspunkte sitzen an der rechten und unteren Kante. Die Größe
  steht in der Fußzeile und ist dort direkt eingebbar, mit Schalter zum
  Festhalten des Seitenverhältnisses und Anzeige des Verhältnisses
  (z. B. 4:3, 16:9)
* Werkzeuge: Auswahl, Pinsel, Linie, Radiergummi, Text
* Objektfähigkeit: Auswählen, Verschieben, Skalieren über acht eigene
  Anfasspunkte, Mehrfachauswahl per Gummiband, Löschen
* Die Farbwahl wirkt auf die ausgewählten Objekte und gilt zugleich für
  neue; Bilder haben keine Farbe und bleiben unberührt
* Textobjekte haben ein Feld für die Schriftgröße. Zieht man an den
  Anfasspunkten, ändert sich die Schriftgröße selbst statt einer Verzerrung,
  und das Feld zeigt den Wert laufend mit
* Der Radiergummi arbeitet innerhalb der Objekte: er legt eine Maske an,
  statt die Form zu zerstören — die ursprüngliche Geometrie bleibt in der
  Projektdatei erhalten
* Alle Symbole werden zur Laufzeit gezeichnet, das Projekt braucht keine
  Bilddateien

## Bauen

Voraussetzungen (Debian/Ubuntu/Mint):

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
```

Bauen und starten:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/inkpot
```

## Bedienung

| Aktion | Eingabe |
| --- | --- |
| Auswahl | `S` |
| Pinsel | `B` |
| Linie | `L` |
| Radiergummi | `E` |
| Text | `T` |
| Text später ändern | Doppelklick mit dem Auswahlwerkzeug |
| Farbe eines Objekts ändern | Objekt auswählen, dann den Farbknopf |
| Auswahl löschen | `Entf` |
| Alles auswählen | `Strg+A` |
| Zoomen | `Strg+Mausrad`, `Strg++`, `Strg+-` |
| Originalgröße / Einpassen | `Strg+0` / `Strg+9` |
| Ansicht verschieben | Mittlere Maustaste ziehen |

## Projektstruktur

| Datei | Inhalt |
| --- | --- |
| `src/main.cpp` | Programmstart |
| `src/MainWindow.*` | Menüs, Werkzeugleiste, Fußzeile, Datei-Operationen |
| `src/CanvasScene.*` | Arbeitsfläche, Objektverwaltung, Projektformat |
| `src/CanvasView.*` | Ansicht, Werkzeuge, Zoom |
| `src/ResizeHandle.*` | Anfasspunkte für Arbeitsfläche und Objekte |
| `src/StrokeItem.*` | Objekt: Pinselstrich und Linie |
| `src/ImageItem.*` | Objekt: Bild |
| `src/TextItem.*` | Objekt: Beschriftung |
| `src/InkObject.*` | Gemeinsame Objekt-Schnittstelle samt Radier-Maske |
| `src/Icons.*` | Zur Laufzeit gezeichnete Symbole |
| `src/NewCanvasDialog.*` | Dialog für benutzerdefinierte Größe |

## Lizenz

GNU General Public License v3.0 — siehe [LICENSE](LICENSE).
