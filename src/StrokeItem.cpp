#include "StrokeItem.h"

#include <cmath>

#include <QCoreApplication>
#include <QPainter>
#include <QJsonArray>
#include <QPainterPathStroker>
#include <QPen>

namespace {
// Mindestbreite für die Trefferflaeche, damit sich auch dünne Striche
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

bool StrokeItem::setColor(const QColor &color)
{
    if (!color.isValid())
        return false;
    QPen newPen = pen();
    newPen.setColor(color);
    setPen(newPen);
    update();
    return true;
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

void StrokeItem::setLineEnd(const QPointF &scenePos)
{
    const QPointF local = mapFromScene(scenePos);
    if (m_points.isEmpty())
        m_points.append(QPointF(0.0, 0.0));

    // Eine Gerade besteht genau aus Start- und Endpunkt.
    if (m_points.size() < 2)
        m_points.append(local);
    else
        m_points[1] = local;
    while (m_points.size() > 2)
        m_points.removeLast();

    rebuildPath();
}

qreal StrokeItem::length() const
{
    qreal total = 0.0;
    for (int i = 1; i < m_points.size(); ++i) {
        const QPointF delta = m_points.at(i) - m_points.at(i - 1);
        total += std::hypot(delta.x(), delta.y());
    }
    return total;
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
        // Über die Mittelpunkte interpolieren ergibt weiche Linien.
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
    return withoutErasedArea(stroker.createStroke(path()));
}

void StrokeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (!hasErasedArea()) {
        QGraphicsPathItem::paint(painter, option, widget);
        return;
    }

    // Radierte Stellen werden ausgeblendet, der Strich selbst bleibt erhalten.
    painter->save();
    painter->setClipPath(visibleClipPath(boundingRect()), Qt::IntersectClip);
    QGraphicsPathItem::paint(painter, option, widget);
    painter->restore();
}

void StrokeItem::write(QJsonObject &json) const
{
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

    rebuildPath();
    return true;
}
