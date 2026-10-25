#include "ImageItem.h"

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QImage>

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

void ImageItem::write(QJsonObject &json, const QPointF &origin) const
{
    const QPointF relative = pos() - origin;
    json[QStringLiteral("x")] = relative.x();
    json[QStringLiteral("y")] = relative.y();
    json[QStringLiteral("z")] = zValue();

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
    setPos(json.value(QStringLiteral("x")).toDouble(), json.value(QStringLiteral("y")).toDouble());
    return true;
}
