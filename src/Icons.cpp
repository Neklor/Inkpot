#include "Icons.h"

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace {

// Every icon is designed on a 64x64 grid and scaled down by QIcon to
// whatever size is needed.
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

    // The classic mouse pointer.
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

    // A brush held at an angle: wooden handle, metal ferrule, coloured bristles.
    painter.translate(33.0, 32.0);
    painter.rotate(-45.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0xB4, 0x7E, 0x48));
    painter.drawRoundedRect(QRectF(-6.5, -28.0, 13.0, 29.0), 6.0, 6.0);

    // Highlight along the handle
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

    // Hint at the end points
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(14.0, 50.0), 7.0, 7.0);
    painter.drawEllipse(QPointF(50.0, 14.0), 7.0, 7.0);

    painter.end();
    return QIcon(pixmap);
}

QIcon eraser(const QColor &outline)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Two-tone block in the style of a classic Pelikan eraser: the end that
    // meets the paper is a pale red, the other end a pale blue.
    const QColor red(0xE4, 0xA3, 0xA3);
    const QColor blue(0xA8, 0xC0, 0xE5);

    painter.save();
    painter.translate(33.0, 27.0);
    painter.rotate(-40.0);

    QPainterPath block;
    block.addRoundedRect(QRectF(-24.0, -12.5, 48.0, 25.0), 5.0, 5.0);

    painter.setPen(Qt::NoPen);
    painter.setClipPath(block);
    painter.setBrush(red);
    painter.drawRect(QRectF(-24.0, -12.5, 24.0, 25.0));
    painter.setBrush(blue);
    painter.drawRect(QRectF(0.0, -12.5, 24.0, 25.0));
    painter.setClipping(false);

    // Seam between the halves, then the outline around the whole block.
    painter.setPen(QPen(outline, 1.8));
    painter.drawLine(QPointF(0.0, -12.5), QPointF(0.0, 12.5));
    painter.setPen(QPen(outline, 2.8));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(block);
    painter.restore();

    // The surface being erased.
    painter.setPen(outlinePen(outline, 4.0));
    painter.drawLine(QPointF(11.0, 56.0), QPointF(55.0, 56.0));

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

    // A capital T
    painter.drawRoundedRect(QRectF(12.0, 12.0, 40.0, 9.0), 2.0, 2.0);
    painter.drawRoundedRect(QRectF(27.5, 12.0, 9.0, 42.0), 2.0, 2.0);

    painter.end();
    return QIcon(pixmap);
}

QIcon bucket(const QColor &accent, const QColor &outline)
{
    QPixmap pixmap = createPixmap();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Puddle and the stream feeding it, both in the chosen colour. They are
    // drawn first so the bucket covers where the stream leaves its mouth.
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);
    painter.drawEllipse(QPointF(32.0, 55.0), 21.0, 6.5);

    QPainterPath stream;
    stream.moveTo(26.0, 33.0);
    stream.quadTo(21.0, 46.0, 24.0, 55.0);
    stream.lineTo(40.0, 55.0);
    stream.quadTo(37.0, 44.0, 40.0, 32.0);
    stream.closeSubpath();
    painter.drawPath(stream);

    // The bucket, tipped over so its mouth points down to the left.
    painter.save();
    painter.translate(42.0, 24.0);
    painter.rotate(140.0);

    QPainterPath body;
    body.moveTo(-17.0, -14.0);
    body.lineTo(17.0, -14.0);
    body.lineTo(12.0, 16.0);
    body.quadTo(11.0, 20.0, 7.0, 20.0);
    body.lineTo(-7.0, 20.0);
    body.quadTo(-11.0, 20.0, -12.0, 16.0);
    body.closeSubpath();

    painter.setPen(QPen(outline, 3.0));
    painter.setBrush(QColor(0xC6, 0xCC, 0xD6));
    painter.drawPath(body);

    // Paint standing in the mouth.
    const QRectF mouth(-17.5, -20.0, 35.0, 12.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);
    painter.drawEllipse(mouth);
    painter.setPen(QPen(outline, 3.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(mouth);
    painter.restore();

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

    // Back panel with tab
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

    // Front flap
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(QPointF(9.0, 32.0), QPointF(55.0, 32.0));

    painter.end();
    return QIcon(pixmap);
}

namespace {

// Body shared by both padlock icons.
void drawLockBody(QPainter &painter, const QColor &color)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(16.0, 30.0, 32.0, 24.0), 4.5, 4.5);

    // The keyhole is a real cut-out so it looks right on any background.
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

    // Shackle: both legs reach down to the body.
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

    // Shackle swung open: the right leg stops above the body.
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
