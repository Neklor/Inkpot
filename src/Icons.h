#pragma once

#include <QIcon>

class QColor;

// Alle Werkzeug-Symbole werden zur Laufzeit gezeichnet. Dadurch braucht das
// Projekt keine Bilddateien und die Symbole passen sich der Textfarbe des
// jeweiligen Themes an.
namespace Icons {

QIcon pointer(const QColor &color);   // Auswahl: Mauspfeil
QIcon brush(const QColor &color);     // Pinsel
QIcon line(const QColor &color);      // Linie
QIcon eraser(const QColor &color);    // Radiergummi
QIcon text(const QColor &color);      // Text
QIcon plus(const QColor &color);      // Neu
QIcon folder(const QColor &color);    // Öffnen

}   // namespace Icons
