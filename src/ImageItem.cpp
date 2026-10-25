#include "ImageItem.h"

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QPainter>

ImageItem::ImageItem(const QPixmap &pixmap)
    : QGraphicsPixmapItem(pixmap)
{
    setTransformationMode(Qt::SmoothTransformation);
    setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
}

QString ImageItem::displayName() const
{
    return QCoreApplication::translate("ImageItem", "Bild");
}

QPainterPath ImageItem::shape() const
{
    return withoutErasedArea(QGraphicsPixmapItem::shape());
}

void ImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (!hasErasedArea()) {
        QGraphicsPixmapItem::paint(painter, option, widget);
        return;
    }

    painter->save();
    painter->setClipPath(visibleClipPath(boundingRect()), Qt::IntersectClip);
    QGraphicsPixmapItem::paint(painter, option, widget);
    painter->restore();
}

void ImageItem::write(QJsonObject &json) const
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap().toImage().save(&buffer, "PNG");
    buffer.close();
    json[QStringLiteral("png")] = QString::fromLatin1(bytes.toBase64());
}

bool ImageItem::read(const QJsonObject &json)
{
    const QByteArray bytes =
        QByteArray::fromBase64(json.value(QStringLiteral("png")).toString().toLatin1());
    QImage image;
    if (bytes.isEmpty() || !image.loadFromData(bytes, "PNG"))
        return false;

    setPixmap(QPixmap::fromImage(image));
    return true;
}
