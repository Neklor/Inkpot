#pragma once

#include <QColor>
#include <QGraphicsPathItem>
#include <QVector>

#include "InkObject.h"

// Ein gemalter Strich: freihaendig mit dem Pinsel oder gerade mit dem
// Linienwerkzeug.
class StrokeItem : public QGraphicsPathItem, public InkObject
{
public:
    explicit StrokeItem(const QColor &color = Qt::black, qreal width = 4.0);

    // Malvorgang: beginAt() beim Druecken, extendTo() beim Ziehen.
    void beginAt(const QPointF &scenePos);
    void extendTo(const QPointF &scenePos);

    // Linienwerkzeug: zieht eine gerade Strecke vom Startpunkt aus.
    void setLineEnd(const QPointF &scenePos);

    bool isEmpty() const { return m_points.isEmpty(); }
    qreal length() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString typeName() const override { return QStringLiteral("stroke"); }
    QString displayName() const override;
    QGraphicsItem *asItem() override { return this; }
    void write(QJsonObject &json) const override;
    bool read(const QJsonObject &json) override;

private:
    void rebuildPath();
    qreal hitWidth() const;

    QVector<QPointF> m_points;   // in Item-Koordinaten
};
