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

QIcon brush(const QColor &color)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    // Stiel
    painter.drawRoundedRect(QRectF(25.0, 4.0, 14.0, 24.0), 5.0, 5.0);

    // Zwinge
    painter.drawRoundedRect(QRectF(20.0, 27.0, 24.0, 11.0), 2.5, 2.5);

    // Borsten, nach unten zulaufend
    QPainterPath bristles;
    bristles.moveTo(21.0, 39.0);
    bristles.lineTo(43.0, 39.0);
    bristles.lineTo(38.0, 55.0);
    bristles.quadTo(32.0, 60.0, 26.0, 55.0);
    bristles.closeSubpath();
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

}   // namespace Icons
