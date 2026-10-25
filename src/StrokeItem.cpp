#include "StrokeItem.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QPainterPathStroker>
#include <QPen>

namespace {
// Mindestbreite fuer die Trefferflaeche, damit sich auch duenne Striche
// bequem anklicken lassen.
constexpr qreal kMinimumHitWidth = 10.0;
}

StrokeItem::StrokeItem(const QColor &color, qreal width)
{
    QPen pen(color, width);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    setPen(pen);
    setBrush(Qt::NoBrush);
}

QString StrokeItem::displayName() const
{
    return QCoreApplication::translate("StrokeItem", "Pinselstrich");
}

void StrokeItem::beginAt(const QPointF &scenePos)
{
    setPos(scenePos);
    m_points.clear();
    m_points.append(QPointF(0.0, 0.0));
    rebuildPath();
}

void StrokeItem::extendTo(const QPointF &scenePos)
{
    const QPointF local = mapFromScene(scenePos);
    if (!m_points.isEmpty()) {
        const QPointF delta = local - m_points.last();
        // Sehr dichte Punkte bringen nichts und kosten nur Speicher.
        if (delta.manhattanLength() < 1.0)
            return;
    }
    m_points.append(local);
    rebuildPath();
}

void StrokeItem::rebuildPath()
{
    QPainterPath path;
    if (m_points.isEmpty()) {
        setPath(path);
        return;
    }

    path.moveTo(m_points.first());
    if (m_points.size() == 1) {
        // Einzelner Klick: winziges Segment, damit der runde Pinselkopf einen
        // sichtbaren Punkt setzt.
        path.lineTo(m_points.first() + QPointF(0.01, 0.0));
    } else {
        // Ueber die Mittelpunkte interpolieren ergibt weiche Linien.
        for (int i = 1; i < m_points.size() - 1; ++i) {
            const QPointF mid = (m_points.at(i) + m_points.at(i + 1)) / 2.0;
            path.quadTo(m_points.at(i), mid);
        }
        path.lineTo(m_points.last());
    }
    setPath(path);
}

qreal StrokeItem::hitWidth() const
{
    return qMax(pen().widthF(), kMinimumHitWidth);
}

QRectF StrokeItem::boundingRect() const
{
    const qreal extra = hitWidth() / 2.0 + 1.0;
    return path().controlPointRect().adjusted(-extra, -extra, extra, extra);
}

QPainterPath StrokeItem::shape() const
{
    QPainterPathStroker stroker;
    stroker.setWidth(hitWidth());
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(path());
}

void StrokeItem::write(QJsonObject &json, const QPointF &origin) const
{
    const QPointF relative = pos() - origin;
    json[QStringLiteral("x")] = relative.x();
    json[QStringLiteral("y")] = relative.y();
    json[QStringLiteral("z")] = zValue();
    json[QStringLiteral("color")] = pen().color().name(QColor::HexArgb);
    json[QStringLiteral("width")] = pen().widthF();

    QJsonArray points;
    for (const QPointF &point : m_points) {
        QJsonArray pair;
        pair.append(point.x());
        pair.append(point.y());
        points.append(pair);
    }
    json[QStringLiteral("points")] = points;
}

bool StrokeItem::read(const QJsonObject &json)
{
    const QJsonArray points = json.value(QStringLiteral("points")).toArray();
    if (points.isEmpty())
        return false;

    QPen newPen = pen();
    const QColor color(json.value(QStringLiteral("color")).toString(QStringLiteral("#ff000000")));
    if (color.isValid())
        newPen.setColor(color);
    newPen.setWidthF(json.value(QStringLiteral("width")).toDouble(4.0));
    setPen(newPen);

    m_points.clear();
    m_points.reserve(points.size());
    for (const QJsonValue &value : points) {
        const QJsonArray pair = value.toArray();
        if (pair.size() != 2)
            continue;
        m_points.append(QPointF(pair.at(0).toDouble(), pair.at(1).toDouble()));
    }
    if (m_points.isEmpty())
        return false;

    setPos(json.value(QStringLiteral("x")).toDouble(), json.value(QStringLiteral("y")).toDouble());
    rebuildPath();
    return true;
}
