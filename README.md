# Inkpot

**A paint program for Linux in which everything you draw stays an object.**

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-2C3E50)](LICENSE)
[![Donate with PayPal](https://img.shields.io/badge/Donate-PayPal-00457C?logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

*(Deutsche Fassung weiter unten — [zur deutschen Fassung](#inkpot-deutsch))*

Inkpot is a small paint program written in C++ with Qt 6. Brush strokes,
straight lines, captions and pasted images are objects in their own right: they
can be selected, moved, scaled, recoloured and deleted at any time, long after
they were drawn. The project format keeps them that way.

> **Early development.** Milestone 1 — the basic program — is in place. The
> project is not released yet, so there are no packages to download.

## Features

**Tools**
* Selection, brush, straight line, eraser and text
* The eraser works *inside* objects: the erased area is masked out while the
  original shape is kept in the project file
* Captions can be edited again with a double click

**Objects**
* Everything drawn or pasted becomes a separate object
* Select, move, scale through eight handles, recolour, delete
* Rubber band selection for several objects at once
* Scaling a caption changes its font size rather than distorting it, and the
  size follows live in the toolbar

**Canvas**
* Pinned to the top left corner, resized with handles on its right and bottom
  edge or by typing the size in the status bar
* Lockable aspect ratio, with the current ratio shown as 4:3, 16:9 and so on

**Files**
* Project format `*.ipot` (JSON) that keeps every object editable
* Export to PNG, JPEG, BMP, TIFF and WebP
* Copy to clipboard, paste, print

## Building

Requirements (Debian, Ubuntu, Linux Mint):

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev \
                 qt6-base-dev-tools qt6-l10n-tools libgl1-mesa-dev
```

`qt6-l10n-tools` provides `lupdate` and `lrelease` for the translations.
Without it Inkpot still builds, it simply runs untranslated.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/inkpot
```

Installing system wide:

```bash
sudo cmake --install build
sudo update-mime-database /usr/local/share/mime
sudo update-desktop-database /usr/local/share/applications
```

This installs the program, the desktop entry, the application icon, the
AppStream metadata, the MIME type for `*.ipot` and the translations.

## Keys and mouse

| Action | Input |
| --- | --- |
| Selection | `S` |
| Brush | `B` |
| Line | `L` |
| Eraser | `E` |
| Text | `T` |
| Edit a caption later | double click with the selection tool |
| Recolour objects | select them, then the colour button |
| Delete selection | `Del` |
| Select all | `Ctrl+A` |
| Zoom | `Ctrl` + mouse wheel, `Ctrl++`, `Ctrl+-` |
| Actual size / fit in window | `Ctrl+0` / `Ctrl+9` |
| Pan the view | drag with the middle mouse button |

## Translations

The strings in the sources are English; translations live in
[`translations/`](translations/) as Qt Linguist catalogues. This is the usual
mechanism for Qt applications and is understood by translation platforms such
as Weblate.

Editing a translation — either with `linguist6 translations/inkpot_de.ts` or in
a text editor, the files are readable XML:

```xml
<message>
    <source>&amp;Save</source>
    <translation>&amp;Speichern</translation>
</message>
```

Adding a language:

```bash
cp translations/inkpot_de.ts translations/inkpot_fr.ts
# set <TS ... language="fr_FR"> and list the file under
# INKPOT_TS_FILES in CMakeLists.txt
cmake --build build
```

After changing the sources,

```bash
cmake --build build --target update_translations
```

pulls new and changed strings into every `.ts` file.

## Packaging

| File | Purpose |
| --- | --- |
| `data/io.github.neklor.Inkpot.desktop` | application menu entry |
| `data/io.github.neklor.Inkpot.metainfo.xml` | AppStream data for software centres |
| `data/io.github.neklor.Inkpot.xml` | MIME type for `*.ipot` |
| `data/icons/hicolor/scalable/apps/…svg` | application icon |
| `flatpak/io.github.neklor.Inkpot.yml` | Flatpak manifest |

Building a Flatpak locally:

```bash
flatpak install flathub org.kde.Platform//6.11 org.kde.Sdk//6.11
flatpak-builder --user --install --force-clean build-flatpak \
    flatpak/io.github.neklor.Inkpot.yml
flatpak run io.github.neklor.Inkpot
```

Before a release on Flathub at least one screenshot under a permanent URL is
still missing; the place for it is prepared in the AppStream file.

## Project structure

| Path | Contents |
| --- | --- |
| `src/main.cpp` | program start, translations, command line |
| `src/MainWindow.*` | menus, toolbar, status bar, file operations |
| `src/CanvasScene.*` | canvas, object handling, project format |
| `src/CanvasView.*` | view, tools, zoom |
| `src/ResizeHandle.*` | handles for canvas and objects |
| `src/StrokeItem.*` | object: brush stroke and line |
| `src/ImageItem.*` | object: image |
| `src/TextItem.*` | object: caption |
| `src/InkObject.*` | shared object interface including the erase mask |
| `src/Icons.*` | icons drawn at run time |
| `src/NewCanvasDialog.*` | dialog for a custom canvas size |
| `translations/` | translation catalogues |
| `data/` | icon, desktop entry, AppStream and MIME data |
| `flatpak/` | Flatpak manifest |

## Contributing

Sources, comments and the strings in the user interface are English so the
project stays open to contributors and translators. Commit messages are
written in English.

Changes are recorded in [CHANGELOG.md](CHANGELOG.md). The version number lives
in [CMakeLists.txt](CMakeLists.txt) and is handed from there to the program and
the package data.

## Supporting the project

Inkpot is written in my spare time and is free. If you find it useful, you can
support its development with a donation:

[![Donate with PayPal](https://img.shields.io/badge/Donate%20with%20PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

The **Sponsor** button at the top of the project page leads to the same link.

Donations are voluntary support for a private spare-time project. They are not
an order and establish no claim to a particular feature or a particular
schedule — but I will say openly what I am working on.

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).

Copyright © 2026 Christoph Krogmann

---

# Inkpot (Deutsch)

**Ein Malprogramm für Linux, in dem alles Gemalte ein Objekt bleibt.**

[![Lizenz: GPL-3.0](https://img.shields.io/badge/Lizenz-GPL--3.0-2C3E50)](LICENSE)
[![Mit PayPal spenden](https://img.shields.io/badge/Spenden-PayPal-00457C?logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

Inkpot ist ein kleines Malprogramm in C++ mit Qt 6. Pinselstriche, gerade
Linien, Beschriftungen und eingefügte Bilder sind eigenständige Objekte: Sie
lassen sich jederzeit auswählen, verschieben, skalieren, umfärben und löschen —
auch lange nachdem sie entstanden sind. Das Projektformat hält sie so.

> **Frühe Entwicklung.** Meilenstein 1 — das Grundprogramm — steht. Das Projekt
> ist noch nicht veröffentlicht, es gibt also noch nichts herunterzuladen.

## Funktionen

**Werkzeuge**
* Auswahl, Pinsel, gerade Linie, Radiergummi und Text
* Der Radiergummi wirkt *innerhalb* von Objekten: Die radierte Stelle wird
  ausgespart, die ursprüngliche Form bleibt in der Projektdatei erhalten
* Beschriftungen lassen sich per Doppelklick wieder bearbeiten

**Objekte**
* Alles Gemalte oder Eingefügte wird ein eigenes Objekt
* Auswählen, verschieben, über acht Anfasspunkte skalieren, umfärben, löschen
* Gummiband-Auswahl für mehrere Objekte auf einmal
* Beim Skalieren einer Beschriftung ändert sich die Schriftgröße statt einer
  Verzerrung, und der Wert läuft live in der Werkzeugleiste mit

**Arbeitsfläche**
* Liegt fest in der linken oberen Ecke, veränderbar über die Anfasspunkte an
  rechter und unterer Kante oder durch Eingabe der Größe in der Fußzeile
* Feststellbares Seitenverhältnis, mit Anzeige als 4:3, 16:9 und so weiter

**Dateien**
* Projektformat `*.ipot` (JSON), das jedes Objekt bearbeitbar hält
* Export nach PNG, JPEG, BMP, TIFF und WebP
* In die Zwischenablage kopieren, einfügen, drucken

## Bauen

Voraussetzungen (Debian, Ubuntu, Linux Mint):

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev \
                 qt6-base-dev-tools qt6-l10n-tools libgl1-mesa-dev
```

`qt6-l10n-tools` liefert `lupdate` und `lrelease` für die Übersetzungen. Ohne
das Paket baut Inkpot trotzdem, läuft dann aber unübersetzt.

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

Damit kommen Programm, Menüeintrag, Programmsymbol, AppStream-Daten, der
MIME-Typ für `*.ipot` und die Übersetzungen mit.

## Tasten und Maus

| Aktion | Eingabe |
| --- | --- |
| Auswahl | `S` |
| Pinsel | `B` |
| Linie | `L` |
| Radiergummi | `E` |
| Text | `T` |
| Beschriftung später ändern | Doppelklick mit dem Auswahlwerkzeug |
| Objekte umfärben | auswählen, dann der Farbknopf |
| Auswahl löschen | `Entf` |
| Alles auswählen | `Strg+A` |
| Zoomen | `Strg` + Mausrad, `Strg++`, `Strg+-` |
| Originalgröße / Einpassen | `Strg+0` / `Strg+9` |
| Ansicht verschieben | Ziehen mit der mittleren Maustaste |

## Übersetzungen

Die Texte im Quelltext sind englisch; Übersetzungen liegen unter
[`translations/`](translations/) als Qt-Linguist-Kataloge. Das ist das übliche
Verfahren für Qt-Anwendungen und wird von Übersetzungsplattformen wie Weblate
unterstützt.

Eine Übersetzung bearbeiten — entweder mit `linguist6 translations/inkpot_de.ts`
oder im Texteditor, die Dateien sind lesbares XML:

```xml
<message>
    <source>&amp;Save</source>
    <translation>&amp;Speichern</translation>
</message>
```

Eine neue Sprache anlegen:

```bash
cp translations/inkpot_de.ts translations/inkpot_fr.ts
# <TS ... language="fr_FR"> setzen und die Datei in CMakeLists.txt
# bei INKPOT_TS_FILES eintragen
cmake --build build
```

Nach Änderungen am Quelltext holt

```bash
cmake --build build --target update_translations
```

neue und geänderte Texte in alle `.ts`-Dateien.

## Veröffentlichung

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

## Projektstruktur

| Pfad | Inhalt |
| --- | --- |
| `src/main.cpp` | Programmstart, Übersetzungen, Kommandozeile |
| `src/MainWindow.*` | Menüs, Werkzeugleiste, Fußzeile, Dateioperationen |
| `src/CanvasScene.*` | Arbeitsfläche, Objektverwaltung, Projektformat |
| `src/CanvasView.*` | Ansicht, Werkzeuge, Zoom |
| `src/ResizeHandle.*` | Anfasspunkte für Arbeitsfläche und Objekte |
| `src/StrokeItem.*` | Objekt: Pinselstrich und Linie |
| `src/ImageItem.*` | Objekt: Bild |
| `src/TextItem.*` | Objekt: Beschriftung |
| `src/InkObject.*` | Gemeinsame Objekt-Schnittstelle samt Radier-Maske |
| `src/Icons.*` | Zur Laufzeit gezeichnete Symbole |
| `src/NewCanvasDialog.*` | Dialog für eine eigene Flächengröße |
| `translations/` | Übersetzungskataloge |
| `data/` | Symbol, Desktop-Eintrag, AppStream- und MIME-Daten |
| `flatpak/` | Flatpak-Manifest |

## Mitarbeit

Quelltext, Kommentare und die Texte in der Oberfläche sind englisch, damit das
Projekt für Mitwirkende und Übersetzer offen bleibt. Commit-Nachrichten werden
auf Englisch geschrieben.

Änderungen werden in [CHANGELOG.md](CHANGELOG.md) festgehalten. Die
Versionsnummer steht in [CMakeLists.txt](CMakeLists.txt) und wird von dort an
Programm und Paketdaten weitergereicht.

## Projekt unterstützen

Inkpot entsteht in meiner Freizeit und ist kostenlos. Wenn es euch gefällt,
könnt ihr die Weiterentwicklung mit einer Spende unterstützen:

[![Mit PayPal spenden](https://img.shields.io/badge/Mit%20PayPal%20spenden-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

Über die Schaltfläche **Sponsor** oben auf der Projektseite geht es zum
gleichen Link.

Spenden sind freiwillige Unterstützung eines privaten Freizeitprojekts. Sie
sind keine Bestellung und begründen keinen Anspruch auf eine bestimmte Funktion
oder einen Zeitplan — ich sage aber offen, woran ich arbeite.

## Lizenz

GNU General Public License v3.0 — siehe [LICENSE](LICENSE).

Copyright © 2026 Christoph Krogmann
