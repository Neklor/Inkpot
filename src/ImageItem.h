#pragma once

#include <QGraphicsPixmapItem>

#include "InkObject.h"

// Ein eingefuegtes oder geoeffnetes Bild als Objekt.
class ImageItem : public QGraphicsPixmapItem, public InkObject
{
public:
    explicit ImageItem(const QPixmap &pixmap = QPixmap());

    QString typeName() const override { return QStringLiteral("image"); }
    QString displayName() const override;
    void write(QJsonObject &json, const QPointF &origin) const override;
    bool read(const QJsonObject &json) override;
};
