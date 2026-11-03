# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-11-03

First development release: milestone 1, the basic program.

### Added

- Canvas pinned to the top left corner, resized with handles on its right and
  bottom edge or through the fields in the status bar, with a lockable aspect
  ratio and a live ratio display.
- Object handling: brush strokes, straight lines, captions and pasted images
  are objects of their own and can be selected, moved, scaled through eight
  handles, recoloured and deleted.
- Tools for selection, brush, line, eraser and text. The eraser works inside
  objects by masking the erased area, so the original shape is preserved in
  the project file.
- Font size field that follows the selected caption; dragging its handles
  changes the font size instead of distorting the text.
- File menu with new (preset and custom sizes), open, save, save as, export to
  PNG, JPEG, BMP, TIFF and WebP, copy to clipboard, paste, print and quit.
- Project format `*.ipot` that keeps every object editable.
- Tool icons drawn at run time, so the project ships no image files.
- Desktop integration: desktop entry, AppStream metadata, application icon and
  a MIME type for `*.ipot`, plus a Flatpak manifest.
- German translation through Qt Linguist catalogues. When the LinguistTools
  CMake package is absent, the build falls back to calling `lrelease` directly,
  so the smaller qt6-l10n-tools package is enough.
- Help menu with an about dialog carrying version, author, licence and
  homepage.

[Unreleased]: https://github.com/Neklor/Inkpot/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Neklor/Inkpot/releases/tag/v0.1.0
