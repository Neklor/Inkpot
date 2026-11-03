#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>

#include "InkObject.h"

// An area created with the bucket fill.
//
// The area is worked out on the rendered picture of the whole canvas, so it
// may span several objects, and is kept as a path. Because it is an object
// like any other it can be selected, moved, scaled, recoloured, erased into
// and deleted.
class FillItem : public QGraphicsItem, public InkObject
{
public:
    explicit FillItem(const QPainterPath &region = QPainterPath(),
                      const QColor &color = Qt::black);

    QPainterPath region() const { return m_region; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString typeName() const override { return QStringLiteral("fill"); }
    QString displayName() const override;
    QGraphicsItem *asItem() override { return this; }

    QColor color() const override { return m_color; }
    bool setColor(const QColor &color) override;

    void write(QJsonObject &json) const override;
    bool read(const QJsonObject &json) override;

private:
    QPainterPath m_region;
    QColor m_color;
};
