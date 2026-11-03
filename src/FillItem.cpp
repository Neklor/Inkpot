#include "FillItem.h"

#include <QCoreApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

FillItem::FillItem(const QPainterPath &region, const QColor &color)
    : m_region(region)
    , m_color(color)
{
}

QString FillItem::displayName() const
{
    return QCoreApplication::translate("FillItem", "Filled area");
}

QRectF FillItem::boundingRect() const
{
    return m_region.boundingRect().adjusted(-1.0, -1.0, 1.0, 1.0);
}

QPainterPath FillItem::shape() const
{
    return withoutErasedArea(m_region);
}

bool FillItem::setColor(const QColor &color)
{
    if (!color.isValid())
        return false;
    m_color = color;
    update();
    return true;
}

void FillItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    painter->save();
    if (hasErasedArea())
        painter->setClipPath(visibleClipPath(boundingRect()), Qt::IntersectClip);

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawPath(m_region);
    painter->restore();

    // A plain QGraphicsItem draws no selection frame of its own.
    if (option->state & QStyle::State_Selected) {
        painter->save();
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Qt::white, 0.0));
        painter->drawRect(boundingRect());
        QPen marching(Qt::black, 0.0, Qt::DashLine);
        painter->setPen(marching);
        painter->drawRect(boundingRect());
        painter->restore();
    }
}

void FillItem::write(QJsonObject &json) const
{
    json[QStringLiteral("color")] = m_color.name(QColor::HexArgb);
    json[QStringLiteral("region")] = InkPath::toBase64(m_region);
}

bool FillItem::read(const QJsonObject &json)
{
    m_region = InkPath::fromBase64(json.value(QStringLiteral("region")).toString());
    if (m_region.isEmpty())
        return false;

    const QColor color(json.value(QStringLiteral("color")).toString(QStringLiteral("#ff000000")));
    if (color.isValid())
        m_color = color;

    prepareGeometryChange();
    return true;
}
