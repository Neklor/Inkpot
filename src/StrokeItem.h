#pragma once

#include <QColor>
#include <QGraphicsPathItem>
#include <QPen>
#include <QVector>

#include "InkObject.h"

// A drawn stroke: freehand with the brush or straight with the line tool.
class StrokeItem : public QGraphicsPathItem, public InkObject
{
public:
    explicit StrokeItem(const QColor &color = Qt::black, qreal width = 4.0);

    // Drawing: beginAt() on press, extendTo() while dragging.
    void beginAt(const QPointF &scenePos);
    void extendTo(const QPointF &scenePos);

    // Line tool: drags a straight segment away from the starting point.
    void setLineEnd(const QPointF &scenePos);

    bool isEmpty() const { return m_points.isEmpty(); }
    qreal length() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QColor color() const override { return pen().color(); }
    bool setColor(const QColor &color) override;

    QString typeName() const override { return QStringLiteral("stroke"); }
    QString displayName() const override;
    QGraphicsItem *asItem() override { return this; }
    void write(QJsonObject &json) const override;
    bool read(const QJsonObject &json) override;

private:
    void rebuildPath();
    qreal hitWidth() const;

    QVector<QPointF> m_points;   // in item coordinates
};
