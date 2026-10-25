#pragma once

#include <QColor>
#include <QGraphicsPathItem>
#include <QVector>

#include "InkObject.h"

// Ein freihaendig gemalter Pinselstrich.
class StrokeItem : public QGraphicsPathItem, public InkObject
{
public:
    explicit StrokeItem(const QColor &color = Qt::black, qreal width = 4.0);

    // Malvorgang: beginAt() beim Druecken, extendTo() beim Ziehen.
    void beginAt(const QPointF &scenePos);
    void extendTo(const QPointF &scenePos);

    bool isEmpty() const { return m_points.isEmpty(); }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    QString typeName() const override { return QStringLiteral("stroke"); }
    QString displayName() const override;
    void write(QJsonObject &json, const QPointF &origin) const override;
    bool read(const QJsonObject &json) override;

private:
    void rebuildPath();
    qreal hitWidth() const;

    QVector<QPointF> m_points;   // in Item-Koordinaten
};
