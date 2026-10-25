#pragma once

#include <QGraphicsPixmapItem>

#include "InkObject.h"

// Ein eingefügtes oder geöffnetes Bild als Objekt.
class ImageItem : public QGraphicsPixmapItem, public InkObject
{
public:
    explicit ImageItem(const QPixmap &pixmap = QPixmap());

    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString typeName() const override { return QStringLiteral("image"); }
    QString displayName() const override;
    QGraphicsItem *asItem() override { return this; }
    void write(QJsonObject &json) const override;
    bool read(const QJsonObject &json) override;
};
