#include "ShapeItem.h"

#include <cmath>

#include <QCoreApplication>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPolygonF>
#include <QStyleOptionGraphicsItem>

namespace {

// Maps a figure described in the unit square onto the given rectangle, so
// every shape stretches with whatever the pointer spanned.
QPointF at(const QRectF &rect, qreal x, qreal y)
{
    return QPointF(rect.left() + x * rect.width(), rect.top() + y * rect.height());
}

QPainterPath heartPath(const QRectF &rect)
{
    QPainterPath path;
    path.moveTo(at(rect, 0.50, 1.00));
    path.cubicTo(at(rect, 0.10, 0.70), at(rect, 0.00, 0.52), at(rect, 0.00, 0.34));
    path.cubicTo(at(rect, 0.00, 0.14), at(rect, 0.14, 0.00), at(rect, 0.30, 0.00));
    path.cubicTo(at(rect, 0.42, 0.00), at(rect, 0.50, 0.09), at(rect, 0.50, 0.22));
    path.cubicTo(at(rect, 0.50, 0.09), at(rect, 0.58, 0.00), at(rect, 0.70, 0.00));
    path.cubicTo(at(rect, 0.86, 0.00), at(rect, 1.00, 0.14), at(rect, 1.00, 0.34));
    path.cubicTo(at(rect, 1.00, 0.52), at(rect, 0.90, 0.70), at(rect, 0.50, 1.00));
    path.closeSubpath();
    return path;
}

QPainterPath starPath(const QRectF &rect)
{
    // Five points, the inner corners at the ratio that makes a regular star.
    constexpr int kPoints = 5;
    constexpr qreal kInner = 0.382;

    QPolygonF polygon;
    for (int i = 0; i < kPoints * 2; ++i) {
        const qreal radius = (i % 2 == 0) ? 0.5 : 0.5 * kInner;
        const qreal angle = -M_PI / 2.0 + i * M_PI / kPoints;
        polygon << at(rect, 0.5 + radius * std::cos(angle), 0.5 + radius * std::sin(angle));
    }

    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();
    return path;
}

}   // namespace

ShapeItem::ShapeItem(Kind kind, const QColor &color, bool filled, qreal outlineWidth)
    : m_kind(kind)
    , m_color(color)
    , m_filled(filled)
    , m_outlineWidth(qMax(qreal(0.5), outlineWidth))
{
}

QString ShapeItem::nameFor(Kind kind)
{
    switch (kind) {
    case Kind::Ellipse:   return QCoreApplication::translate("ShapeItem", "Ellipse");
    case Kind::Rectangle: return QCoreApplication::translate("ShapeItem", "Rectangle");
    case Kind::Triangle:  return QCoreApplication::translate("ShapeItem", "Triangle");
    case Kind::Heart:     return QCoreApplication::translate("ShapeItem", "Heart");
    case Kind::Star:      return QCoreApplication::translate("ShapeItem", "Star");
    }
    return QCoreApplication::translate("ShapeItem", "Shape");
}

QString ShapeItem::displayName() const
{
    return nameFor(m_kind);
}

QPainterPath ShapeItem::pathFor(Kind kind, const QRectF &rect)
{
    QPainterPath path;
    if (rect.isEmpty())
        return path;

    switch (kind) {
    case Kind::Ellipse:
        path.addEllipse(rect);
        break;
    case Kind::Rectangle:
        path.addRect(rect);
        break;
    case Kind::Triangle: {
        QPolygonF polygon;
        polygon << at(rect, 0.5, 0.0) << at(rect, 1.0, 1.0) << at(rect, 0.0, 1.0);
        path.addPolygon(polygon);
        path.closeSubpath();
        break;
    }
    case Kind::Heart:
        path = heartPath(rect);
        break;
    case Kind::Star:
        path = starPath(rect);
        break;
    }
    return path;
}

void ShapeItem::setGeometry(const QPointF &origin, const QPointF &corner, bool keepSquare)
{
    QPointF far = corner;
    if (keepSquare) {
        // Hold Shift and the shape keeps equal sides: a circle, a square.
        const qreal side = qMax(qAbs(corner.x() - origin.x()), qAbs(corner.y() - origin.y()));
        far = QPointF(origin.x() + (corner.x() < origin.x() ? -side : side),
                      origin.y() + (corner.y() < origin.y() ? -side : side));
    }

    prepareGeometryChange();
    m_rect = QRectF(origin, far).normalized();
    applyPlacement();
}

bool ShapeItem::isTooSmall() const
{
    return m_rect.width() < 3.0 && m_rect.height() < 3.0;
}

QRectF ShapeItem::boundingRect() const
{
    const qreal extra = (m_filled ? 1.0 : m_outlineWidth) / 2.0 + 1.0;
    return m_rect.adjusted(-extra, -extra, extra, extra);
}

QPainterPath ShapeItem::shape() const
{
    const QPainterPath outline = pathFor(m_kind, m_rect);
    if (m_filled)
        return withoutErasedArea(outline);

    // Only the outline is drawn, so only the outline can be clicked - but
    // generously, the way a thin stroke is.
    QPainterPathStroker stroker;
    stroker.setWidth(qMax(m_outlineWidth, qreal(10.0)));
    return withoutErasedArea(stroker.createStroke(outline));
}

bool ShapeItem::setColor(const QColor &color)
{
    if (!color.isValid())
        return false;
    m_color = color;
    update();
    return true;
}

void ShapeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (m_rect.isEmpty())
        return;

    painter->save();
    if (hasErasedArea())
        painter->setClipPath(visibleClipPath(boundingRect()), Qt::IntersectClip);

    painter->setRenderHint(QPainter::Antialiasing, true);
    const QPainterPath outline = pathFor(m_kind, m_rect);
    if (m_filled) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_color);
    } else {
        QPen pen(m_color, m_outlineWidth);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
    }
    painter->drawPath(outline);
    painter->restore();

    // A plain QGraphicsItem draws no selection frame of its own.
    if (option->state & QStyle::State_Selected) {
        painter->save();
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Qt::white, 0.0));
        painter->drawRect(boundingRect());
        painter->setPen(QPen(Qt::black, 0.0, Qt::DashLine));
        painter->drawRect(boundingRect());
        painter->restore();
    }
}

void ShapeItem::write(QJsonObject &json) const
{
    json[QStringLiteral("kind")] = int(m_kind);
    json[QStringLiteral("x0")] = m_rect.left();
    json[QStringLiteral("y0")] = m_rect.top();
    json[QStringLiteral("width")] = m_rect.width();
    json[QStringLiteral("height")] = m_rect.height();
    json[QStringLiteral("color")] = m_color.name(QColor::HexArgb);
    json[QStringLiteral("filled")] = m_filled;
    json[QStringLiteral("outlineWidth")] = m_outlineWidth;
}

bool ShapeItem::read(const QJsonObject &json)
{
    const int kind = json.value(QStringLiteral("kind")).toInt(int(Kind::Rectangle));
    if (kind < int(Kind::Ellipse) || kind > int(Kind::Star))
        return false;
    m_kind = Kind(kind);

    m_rect = QRectF(json.value(QStringLiteral("x0")).toDouble(),
                    json.value(QStringLiteral("y0")).toDouble(),
                    json.value(QStringLiteral("width")).toDouble(),
                    json.value(QStringLiteral("height")).toDouble());
    if (m_rect.isEmpty())
        return false;

    const QColor color(json.value(QStringLiteral("color")).toString(QStringLiteral("#ff000000")));
    if (color.isValid())
        m_color = color;
    m_filled = json.value(QStringLiteral("filled")).toBool(true);
    m_outlineWidth = qMax(qreal(0.5), json.value(QStringLiteral("outlineWidth")).toDouble(4.0));

    prepareGeometryChange();
    return true;
}
