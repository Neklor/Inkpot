#include "Icons.h"

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace {

// Alle Symbole werden in einem 64x64-Raster entworfen und von QIcon auf die
// jeweils benötigte Größe verkleinert.
constexpr int kGrid = 64;

QPixmap createPixmap()
{
    QPixmap pixmap(kGrid, kGrid);
    pixmap.fill(Qt::transparent);
    return pixmap;
}

QPen outlinePen(const QColor &color, qreal width = 5.0)
{
    QPen pen(color, width);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

}   // namespace

namespace Icons {

QIcon pointer(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Klassischer Mauspfeil.
    QPainterPath arrow;
    arrow.moveTo(16.0, 8.0);
    arrow.lineTo(16.0, 50.0);
    arrow.lineTo(26.5, 39.5);
    arrow.lineTo(34.0, 54.0);
    arrow.lineTo(41.0, 50.5);
    arrow.lineTo(34.0, 36.5);
    arrow.lineTo(47.0, 36.5);
    arrow.closeSubpath();

    painter.setPen(outlinePen(color, 3.0));
    painter.setBrush(color);
    painter.drawPath(arrow);

    painter.end();
    return QIcon(pixmap);
}

QIcon brush(const QColor &accent, const QColor &outline)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Schräg liegender Pinsel: Holzstiel, Metallzwinge, farbige Borsten.
    painter.translate(33.0, 32.0);
    painter.rotate(-45.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0xB4, 0x7E, 0x48));
    painter.drawRoundedRect(QRectF(-6.5, -28.0, 13.0, 29.0), 6.0, 6.0);

    // Lichtkante auf dem Stiel
    painter.setBrush(QColor(0xD6, 0xA5, 0x70));
    painter.drawRoundedRect(QRectF(-4.5, -26.0, 4.0, 24.0), 2.0, 2.0);

    painter.setBrush(QColor(0xC2, 0xC7, 0xCE));
    painter.drawRoundedRect(QRectF(-8.5, -4.0, 17.0, 12.0), 2.5, 2.5);
    painter.setPen(QPen(QColor(0x8B, 0x90, 0x99), 1.4));
    painter.drawLine(QPointF(-8.5, 2.0), QPointF(8.5, 2.0));

    QPainterPath bristles;
    bristles.moveTo(-8.5, 8.0);
    bristles.lineTo(8.5, 8.0);
    bristles.lineTo(5.5, 21.0);
    bristles.quadTo(0.0, 27.5, -5.5, 21.0);
    bristles.closeSubpath();

    painter.setPen(QPen(outline, 1.3));
    painter.setBrush(accent);
    painter.drawPath(bristles);

    painter.end();
    return QIcon(pixmap);
}

QIcon line(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(outlinePen(color, 5.0));
    painter.drawLine(QPointF(14.0, 50.0), QPointF(50.0, 14.0));

    // Endpunkte andeuten
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(14.0, 50.0), 7.0, 7.0);
    painter.drawEllipse(QPointF(50.0, 14.0), 7.0, 7.0);

    painter.end();
    return QIcon(pixmap);
}

QIcon eraser(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.save();
    painter.translate(30.0, 28.0);
    painter.rotate(-42.0);

    QColor fill = color;
    fill.setAlpha(70);
    painter.setPen(outlinePen(color, 4.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(QRectF(-21.0, -11.0, 42.0, 22.0), 4.0, 4.0);

    // Trennlinie zwischen Griff und Radierteil
    painter.drawLine(QPointF(1.0, -11.0), QPointF(1.0, 11.0));
    painter.restore();

    // Unterlage, auf der radiert wird
    painter.setPen(outlinePen(color, 4.0));
    painter.drawLine(QPointF(12.0, 55.0), QPointF(54.0, 55.0));

    painter.end();
    return QIcon(pixmap);
}

QIcon text(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    // Grosses T
    painter.drawRoundedRect(QRectF(12.0, 12.0, 40.0, 9.0), 2.0, 2.0);
    painter.drawRoundedRect(QRectF(27.5, 12.0, 9.0, 42.0), 2.0, 2.0);

    painter.end();
    return QIcon(pixmap);
}

QIcon plus(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(12.0, 28.0, 40.0, 9.0), 4.0, 4.0);
    painter.drawRoundedRect(QRectF(27.5, 12.5, 9.0, 40.0), 4.0, 4.0);

    painter.end();
    return QIcon(pixmap);
}

QIcon folder(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor fill = color;
    fill.setAlpha(55);

    // Rückwand mit Reiter
    QPainterPath body;
    body.moveTo(9.0, 50.0);
    body.lineTo(9.0, 17.0);
    body.lineTo(25.0, 17.0);
    body.lineTo(31.0, 24.0);
    body.lineTo(55.0, 24.0);
    body.lineTo(55.0, 50.0);
    body.closeSubpath();

    painter.setPen(outlinePen(color, 4.5));
    painter.setBrush(fill);
    painter.drawPath(body);

    // Vorderklappe
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(QPointF(9.0, 32.0), QPointF(55.0, 32.0));

    painter.end();
    return QIcon(pixmap);
}

namespace {

// Gemeinsamer Korpus beider Schloss-Symbole.
void drawLockBody(QPainter &painter, const QColor &color)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(16.0, 30.0, 32.0, 24.0), 4.5, 4.5);

    // Schlüsselloch als echte Aussparung, damit es auf jedem Grund stimmt.
    painter.save();
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setBrush(Qt::black);
    painter.drawEllipse(QPointF(32.0, 40.0), 3.6, 3.6);
    painter.drawRoundedRect(QRectF(30.4, 40.0, 3.2, 8.0), 1.6, 1.6);
    painter.restore();
}

QPen shacklePen(const QColor &color)
{
    QPen pen(color, 5.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

}   // namespace

QIcon lockClosed(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Bügel: beide Schenkel reichen bis zum Korpus.
    QPainterPath shackle;
    shackle.moveTo(23.0, 31.0);
    shackle.lineTo(23.0, 26.0);
    shackle.arcTo(QRectF(23.0, 15.0, 18.0, 18.0), 180.0, -180.0);
    shackle.lineTo(41.0, 31.0);

    painter.setPen(shacklePen(color));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shackle);

    drawLockBody(painter, color);

    painter.end();
    return QIcon(pixmap);
}

QIcon lockOpen(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Bügel aufgeklappt: der rechte Schenkel endet über dem Korpus.
    QPainterPath shackle;
    shackle.moveTo(23.0, 31.0);
    shackle.lineTo(23.0, 24.0);
    shackle.arcTo(QRectF(23.0, 13.0, 18.0, 18.0), 180.0, -180.0);
    shackle.lineTo(41.0, 24.0);

    painter.setPen(shacklePen(color));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shackle);

    drawLockBody(painter, color);

    painter.end();
    return QIcon(pixmap);
}

}   // namespace Icons
