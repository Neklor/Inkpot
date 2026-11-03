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
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools \
                 qt6-l10n-tools libgl1-mesa-dev
```

`qt6-l10n-tools` liefert `lupdate` und `lrelease` für die Übersetzungen. Fehlt
das Paket, baut Inkpot trotzdem — dann aber ohne Übersetzungen, also auf
Englisch.

Bauen und starten:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/inkpot
```

Systemweit installieren:

```bash
sudo cmake --install build
sudo update-mime-database /usr/local/share/mime
sudo update-desktop-database /usr/local/share/applications
```

## Sprachen

Die Texte im Quelltext sind englisch; Übersetzungen liegen als
Qt-Linguist-Katalog unter [`translations/`](translations/). Das ist das übliche
Verfahren für Qt-Anwendungen und wird von Übersetzungsplattformen wie Weblate
unterstützt.

Eine Übersetzung bearbeiten — entweder mit `linguist6 translations/inkpot_de.ts`
oder direkt im Texteditor, die Dateien sind lesbares XML:

```xml
<message>
    <source>&amp;Save</source>
    <translation>&amp;Speichern</translation>
</message>
```

Eine neue Sprache anlegen:

```bash
cp translations/inkpot_de.ts translations/inkpot_fr.ts   # Sprachkürzel anpassen
# die Zeile <TS ... language="de_DE"> auf die neue Sprache setzen,
# die Datei in CMakeLists.txt bei INKPOT_TS_FILES eintragen
cmake --build build
```

Nach Änderungen am Quelltext holt

```bash
cmake --build build --target update_translations
```

neue und geänderte Texte in alle `.ts`-Dateien.

## Veröffentlichung

Für Flatpak und Distributionspakete liegen bereit:

| Datei | Zweck |
| --- | --- |
| `data/io.github.neklor.Inkpot.desktop` | Eintrag im Anwendungsmenü |
| `data/io.github.neklor.Inkpot.metainfo.xml` | AppStream-Daten für Software-Zentren |
| `data/io.github.neklor.Inkpot.xml` | MIME-Typ für `*.ipot` |
| `data/icons/hicolor/scalable/apps/…svg` | Anwendungssymbol |
| `flatpak/io.github.neklor.Inkpot.yml` | Flatpak-Manifest |

Flatpak lokal bauen:

```bash
flatpak install flathub org.kde.Platform//6.11 org.kde.Sdk//6.11
flatpak-builder --user --install --force-clean build-flatpak \
    flatpak/io.github.neklor.Inkpot.yml
flatpak run io.github.neklor.Inkpot
```

Vor einer Veröffentlichung auf Flathub fehlt noch mindestens ein Bildschirmfoto
unter einer dauerhaften URL; die Stelle dafür ist in der AppStream-Datei
vorbereitet.

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
| `translations/` | Übersetzungskataloge |
| `data/` | Symbol, Desktop-Eintrag, AppStream- und MIME-Daten |
| `flatpak/` | Flatpak-Manifest |

## Mitarbeit

Quelltext, Kommentare und die Texte in der Oberfläche sind englisch, damit das
Projekt für Mitwirkende und Übersetzer offen ist. Diese README bleibt deutsch.
Commit-Nachrichten werden auf Englisch geschrieben.

Änderungen werden in [CHANGELOG.md](CHANGELOG.md) festgehalten; die
Versionsnummer steht in [CMakeLists.txt](CMakeLists.txt) und wird von dort an
Programm und Paketdaten weitergereicht.

## Lizenz

GNU General Public License v3.0 — siehe [LICENSE](LICENSE).
