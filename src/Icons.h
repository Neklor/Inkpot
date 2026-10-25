#pragma once

#include <QIcon>

class QColor;

// Alle Werkzeug-Symbole werden zur Laufzeit gezeichnet. Dadurch braucht das
// Projekt keine Bilddateien und die Symbole passen sich der Textfarbe des
// jeweiligen Themes an.
namespace Icons {

QIcon pointer(const QColor &color);   // Auswahl: Mauspfeil

// Pinsel mit Borsten in der aktuell gewählten Malfarbe. Der Umriss sorgt
// dafür, dass auch sehr helle Farben auf hellem Grund sichtbar bleiben.
QIcon brush(const QColor &accent, const QColor &outline);
QIcon line(const QColor &color);      // Linie
QIcon eraser(const QColor &color);    // Radiergummi
QIcon text(const QColor &color);      // Text
QIcon plus(const QColor &color);      // Neu
QIcon folder(const QColor &color);    // Öffnen

// Seitenverhältnis-Sperre: geschlossenes und offenes Schloss.
QIcon lockClosed(const QColor &color);
QIcon lockOpen(const QColor &color);

}   // namespace Icons
