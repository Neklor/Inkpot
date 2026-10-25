#include "CanvasScene.h"

#include <algorithm>

#include <QBuffer>
#include <QDataStream>
#include <QGraphicsItem>
#include <QImage>
#include <QJsonArray>
#include <QPainter>
#include <QTimer>

#include "ImageItem.h"
#include "StrokeItem.h"
#include "TextItem.h"

namespace {

constexpr qreal kMinimumSide = 16.0;     // kleinste Kantenlaenge der Fläche
constexpr qreal kMinimumObjectSide = 4.0;
constexpr qreal kSceneMargin = 400.0;    // Arbeitsraum rechts und unterhalb

// Erzeugt ein leeres Objekt passend zum Typbezeichner im Projektformat.
QGraphicsItem *createObjectForType(const QString &type)
{
    if (type == QLatin1String("stroke"))
        return new StrokeItem;
    if (type == QLatin1String("image"))
        return new ImageItem;
    if (type == QLatin1String("text"))
        return new TextItem;
    return nullptr;
}

// Radierspuren werden als Pfad gespeichert.
QString pathToBase64(const QPainterPath &path)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << path;
    return QString::fromLatin1(bytes.toBase64());
}

QPainterPath pathFromBase64(const QString &encoded)
{
    QPainterPath path;
    QByteArray bytes = QByteArray::fromBase64(encoded.toLatin1());
    if (bytes.isEmpty())
        return path;
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    stream >> path;
    return path;
}

}   // namespace

CanvasScene::CanvasScene(QObject *parent)
    : QGraphicsScene(parent)
{
    createHandles();
    updateCanvasHandles();
    updateObjectHandles();
    updateSceneRect();

    connect(this, &QGraphicsScene::selectionChanged, this, [this] { updateObjectHandles(); });

    // Verschobene oder veränderte Objekte brauchen neu gesetzte Griffe. Das
    // Flag verhindert, dass das Setzen der Griffe sich selbst auslöst.
    connect(this, &QGraphicsScene::changed, this, [this] {
        if (!m_updatingHandles)
            updateObjectHandles();
    });
}

CanvasScene::~CanvasScene()
{
    // Beim Abbau räumt ~QGraphicsScene noch die Objekte ab und sendet dabei
    // changed(). Zu diesem Zeitpunkt sind die Griff-Listen als Member dieser
    // Klasse bereits zerstört, deshalb hier alle eigenen Verbindungen lösen.
    disconnect(this, nullptr, this, nullptr);
}

void CanvasScene::createHandles()
{
    // Die Arbeitsfläche hängt oben und links fest; sie wächst nur nach
    // rechts und unten.
    const ResizeHandle::Position canvasPositions[] = {
        ResizeHandle::Right, ResizeHandle::BottomRight, ResizeHandle::Bottom
    };
    for (ResizeHandle::Position position : canvasPositions) {
        auto *handle = new ResizeHandle(position, ResizeHandle::Role::Canvas, this);
        addItem(handle);
        m_canvasHandles.append(handle);
    }

    // Objekte lassen sich dagegen in alle Richtungen skalieren.
    const ResizeHandle::Position objectPositions[] = {
        ResizeHandle::TopLeft,  ResizeHandle::Top,         ResizeHandle::TopRight,
        ResizeHandle::Right,    ResizeHandle::BottomRight, ResizeHandle::Bottom,
        ResizeHandle::BottomLeft, ResizeHandle::Left
    };
    for (ResizeHandle::Position position : objectPositions) {
        auto *handle = new ResizeHandle(position, ResizeHandle::Role::Object, this);
        handle->setVisible(false);
        addItem(handle);
        m_objectHandles.append(handle);
    }
}

void CanvasScene::updateCanvasHandles()
{
    const QRectF page = pageRect();
    for (ResizeHandle *handle : m_canvasHandles)
        handle->setPos(ResizeHandle::anchorPoint(handle->handlePosition(), page));
}

void CanvasScene::updateObjectHandles()
{
    QGraphicsItem *target = singleSelectedObject();

    m_updatingHandles = true;
    if (!target) {
        for (ResizeHandle *handle : m_objectHandles)
            handle->setVisible(false);
    } else {
        const QRectF bounds = target->sceneBoundingRect();
        for (ResizeHandle *handle : m_objectHandles) {
            handle->setPos(ResizeHandle::anchorPoint(handle->handlePosition(), bounds));
            handle->setVisible(true);
        }
    }
    m_updatingHandles = false;
}

void CanvasScene::setHandlesVisible(bool visible)
{
    m_updatingHandles = true;
    for (ResizeHandle *handle : m_canvasHandles)
        handle->setVisible(visible);
    for (ResizeHandle *handle : m_objectHandles)
        handle->setVisible(visible && singleSelectedObject() != nullptr);
    m_updatingHandles = false;
}

void CanvasScene::updateSceneRect()
{
    // Links und oben schließt die Szene bündig mit der Arbeitsfläche ab,
    // damit diese in der Ansicht fest in der Ecke liegt.
    setSceneRect(QRectF(0.0, 0.0,
                        m_pageSize.width() + kSceneMargin,
                        m_pageSize.height() + kSceneMargin));
}

void CanvasScene::setPageSize(const QSize &size)
{
    const QSize clamped(qMax(int(kMinimumSide), size.width()),
                        qMax(int(kMinimumSide), size.height()));
    if (clamped == m_pageSize)
        return;

    m_pageSize = clamped;
    updateCanvasHandles();
    updateSceneRect();
    update();
    emit pageRectChanged();
    emit edited();
}

void CanvasScene::setPageWidth(int width)
{
    if (width <= 0)
        return;
    QSize size = m_pageSize;
    size.setWidth(width);
    if (m_aspectLocked && m_lockedRatio > 0.0)
        size.setHeight(qMax(1, qRound(width / m_lockedRatio)));
    setPageSize(size);
}

void CanvasScene::setPageHeight(int height)
{
    if (height <= 0)
        return;
    QSize size = m_pageSize;
    size.setHeight(height);
    if (m_aspectLocked && m_lockedRatio > 0.0)
        size.setWidth(qMax(1, qRound(height * m_lockedRatio)));
    setPageSize(size);
}

void CanvasScene::setAspectRatioLocked(bool locked)
{
    m_aspectLocked = locked;
    if (locked && m_pageSize.height() > 0)
        m_lockedRatio = double(m_pageSize.width()) / m_pageSize.height();
}

// ------------------------------------------------- Größe der Arbeitsfläche

void CanvasScene::beginCanvasResize()
{
    if (m_aspectLocked && m_pageSize.height() > 0)
        m_lockedRatio = double(m_pageSize.width()) / m_pageSize.height();
}

void CanvasScene::endCanvasResize()
{
    emit pageRectChanged();
}

void CanvasScene::resizeCanvasFromHandle(ResizeHandle::Position position, const QPointF &scenePos)
{
    // Die linke obere Ecke bleibt immer bei (0, 0); nur die gezogene Kante
    // folgt dem Mauszeiger.
    qreal width = m_pageSize.width();
    qreal height = m_pageSize.height();

    switch (position) {
    case ResizeHandle::Right:
        width = scenePos.x();
        break;
    case ResizeHandle::Bottom:
        height = scenePos.y();
        break;
    case ResizeHandle::BottomRight:
        width = scenePos.x();
        height = scenePos.y();
        break;
    default:
        return;   // oben und links gibt es keine Griffe
    }

    width = qMax(width, kMinimumSide);
    height = qMax(height, kMinimumSide);

    if (m_aspectLocked && m_lockedRatio > 0.0) {
        switch (position) {
        case ResizeHandle::Right:
            height = width / m_lockedRatio;
            break;
        case ResizeHandle::Bottom:
            width = height * m_lockedRatio;
            break;
        default:
            // Eckgriff: die stärker veränderte Achse gibt den Ton an.
            if (width / height > m_lockedRatio)
                height = width / m_lockedRatio;
            else
                width = height * m_lockedRatio;
            break;
        }
        width = qMax(width, kMinimumSide);
        height = qMax(height, kMinimumSide);
    }

    setPageSize(QSize(qRound(width), qRound(height)));
}

// ------------------------------------------------------- Objekte skalieren

void CanvasScene::beginObjectScale()
{
    m_scaleTarget = singleSelectedObject();
    if (!m_scaleTarget)
        return;
    m_scaleStartRect = m_scaleTarget->sceneBoundingRect();
    m_scaleStartTransform = m_scaleTarget->transform();
}

void CanvasScene::scaleObjectFromHandle(ResizeHandle::Position position, const QPointF &scenePos)
{
    if (!m_scaleTarget || m_scaleStartRect.isEmpty())
        return;

    // Gewuenschtes neues Rechteck: die gezogene Kante folgt dem Zeiger, die
    // gegenüberliegende bleibt stehen.
    QRectF target = m_scaleStartRect;
    switch (position) {
    case ResizeHandle::TopLeft:
        target.setTop(qMin(scenePos.y(), target.bottom() - kMinimumObjectSide));
        target.setLeft(qMin(scenePos.x(), target.right() - kMinimumObjectSide));
        break;
    case ResizeHandle::Top:
        target.setTop(qMin(scenePos.y(), target.bottom() - kMinimumObjectSide));
        break;
    case ResizeHandle::TopRight:
        target.setTop(qMin(scenePos.y(), target.bottom() - kMinimumObjectSide));
        target.setRight(qMax(scenePos.x(), target.left() + kMinimumObjectSide));
        break;
    case ResizeHandle::Right:
        target.setRight(qMax(scenePos.x(), target.left() + kMinimumObjectSide));
        break;
    case ResizeHandle::BottomRight:
        target.setBottom(qMax(scenePos.y(), target.top() + kMinimumObjectSide));
        target.setRight(qMax(scenePos.x(), target.left() + kMinimumObjectSide));
        break;
    case ResizeHandle::Bottom:
        target.setBottom(qMax(scenePos.y(), target.top() + kMinimumObjectSide));
        break;
    case ResizeHandle::BottomLeft:
        target.setBottom(qMax(scenePos.y(), target.top() + kMinimumObjectSide));
        target.setLeft(qMin(scenePos.x(), target.right() - kMinimumObjectSide));
        break;
    case ResizeHandle::Left:
        target.setLeft(qMin(scenePos.x(), target.right() - kMinimumObjectSide));
        break;
    }

    const qreal factorX = target.width() / m_scaleStartRect.width();
    const qreal factorY = target.height() / m_scaleStartRect.height();

    // Die neue Skalierung wird auf die ursprüngliche Transformation gelegt,
    // damit mehrfaches Ziehen sich nicht aufschaukelt.
    const QTransform scaled = m_scaleStartTransform * QTransform::fromScale(factorX, factorY);
    m_scaleTarget->setTransform(scaled);

    // Position so nachführen, dass die stehende Kante wirklich stehen bleibt.
    const QRectF mapped = scaled.mapRect(m_scaleTarget->boundingRect());
    m_scaleTarget->setPos(target.topLeft() - mapped.topLeft());

    updateObjectHandles();
}

void CanvasScene::endObjectScale()
{
    if (!m_scaleTarget)
        return;
    m_scaleTarget = nullptr;
    updateObjectHandles();
    emit edited();
}

// --------------------------------------------------------------- Objekte

void CanvasScene::prepareObject(QGraphicsItem *item)
{
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
    item->setFlag(QGraphicsItem::ItemIsMovable, true);
    item->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

void CanvasScene::addObject(QGraphicsItem *item)
{
    if (!item)
        return;
    prepareObject(item);
    m_topZ += 1.0;
    item->setZValue(m_topZ);
    addItem(item);
    emit objectsChanged();
    emit edited();
}

QList<QGraphicsItem *> CanvasScene::objects() const
{
    QList<QGraphicsItem *> result;
    const QList<QGraphicsItem *> all = items();
    result.reserve(all.size());
    for (QGraphicsItem *item : all) {
        if (dynamic_cast<InkObject *>(item))
            result.append(item);
    }
    return result;
}

QGraphicsItem *CanvasScene::singleSelectedObject() const
{
    QGraphicsItem *found = nullptr;
    const QList<QGraphicsItem *> selection = selectedItems();
    for (QGraphicsItem *item : selection) {
        if (!dynamic_cast<InkObject *>(item))
            continue;
        if (found)
            return nullptr;   // bei Mehrfachauswahl wird nicht skaliert
        found = item;
    }
    return found;
}

void CanvasScene::removeSelectedObjects()
{
    const QList<QGraphicsItem *> selection = selectedItems();
    bool removedAny = false;
    for (QGraphicsItem *item : selection) {
        if (!dynamic_cast<InkObject *>(item))
            continue;
        removeItem(item);
        delete item;
        removedAny = true;
    }
    if (removedAny) {
        updateObjectHandles();
        emit objectsChanged();
        emit edited();
    }
}

void CanvasScene::removeObjectLater(QGraphicsItem *item)
{
    if (!item || !dynamic_cast<InkObject *>(item))
        return;

    // Aufgerufen aus einem Ereignis des Objekts selbst, daher verzögert.
    QTimer::singleShot(0, this, [this, item] {
        if (item->scene() != this)
            return;
        removeItem(item);
        delete item;
        updateObjectHandles();
        emit objectsChanged();
    });
}

void CanvasScene::clearObjects()
{
    const QList<QGraphicsItem *> all = objects();
    for (QGraphicsItem *item : all) {
        removeItem(item);
        delete item;
    }
    m_topZ = 0.0;
    updateObjectHandles();
    if (!all.isEmpty())
        emit objectsChanged();
}

void CanvasScene::selectAllObjects()
{
    const QList<QGraphicsItem *> all = objects();
    for (QGraphicsItem *item : all)
        item->setSelected(true);
}

void CanvasScene::eraseWithPath(const QPainterPath &scenePath)
{
    if (scenePath.isEmpty())
        return;

    const QRectF area = scenePath.boundingRect();
    bool erasedAny = false;

    const QList<QGraphicsItem *> all = objects();
    for (QGraphicsItem *item : all) {
        if (!item->sceneBoundingRect().intersects(area))
            continue;
        auto *object = dynamic_cast<InkObject *>(item);
        if (!object)
            continue;
        object->eraseScenePath(scenePath);
        erasedAny = true;
    }

    if (erasedAny)
        emit edited();
}

// ------------------------------------------------------------------ Ausgabe

QImage CanvasScene::renderRegion(const QRectF &source, const QList<QGraphicsItem *> &onlyItems,
                                 const QColor &background, qreal scale)
{
    if (source.isEmpty() || scale <= 0.0)
        return QImage();

    const QSize imageSize(qMax(1, qRound(source.width() * scale)),
                          qMax(1, qRound(source.height() * scale)));
    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return QImage();
    image.fill(background);

    // Anfasspunkte, Auswahlrahmen und nicht gewuenschte Objekte vor dem
    // Zeichnen ausblenden und danach wiederherstellen.
    const QList<QGraphicsItem *> selection = selectedItems();
    QList<QGraphicsItem *> hidden;

    setHandlesVisible(false);
    for (QGraphicsItem *item : selection)
        item->setSelected(false);

    if (!onlyItems.isEmpty()) {
        const QList<QGraphicsItem *> all = objects();
        for (QGraphicsItem *item : all) {
            if (!onlyItems.contains(item) && item->isVisible()) {
                item->setVisible(false);
                hidden.append(item);
            }
        }
    }

    m_renderingToImage = true;
    {
        QPainter painter(&image);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(imageSize)), source, Qt::IgnoreAspectRatio);
    }
    m_renderingToImage = false;

    for (QGraphicsItem *item : hidden)
        item->setVisible(true);
    for (QGraphicsItem *item : selection)
        item->setSelected(true);
    setHandlesVisible(true);

    return image;
}

QImage CanvasScene::renderPage(const QColor &background, qreal scale)
{
    return renderRegion(pageRect(), QList<QGraphicsItem *>(), background, scale);
}

QImage CanvasScene::renderSelection()
{
    QList<QGraphicsItem *> selection;
    QRectF bounds;
    const QList<QGraphicsItem *> items = selectedItems();
    for (QGraphicsItem *item : items) {
        if (!dynamic_cast<InkObject *>(item))
            continue;
        selection.append(item);
        bounds = bounds.isNull() ? item->sceneBoundingRect()
                                 : bounds.united(item->sceneBoundingRect());
    }
    if (selection.isEmpty() || bounds.isEmpty())
        return QImage();

    return renderRegion(bounds, selection, Qt::transparent, 1.0);
}

void CanvasScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    if (m_renderingToImage)
        return;   // beim Export nur die Objekte, kein Rahmen, kein Schatten

    painter->fillRect(rect, QColor(0x2b, 0x2d, 0x31));

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 70));
    painter->drawRect(pageRect().translated(5.0, 5.0));
    painter->setBrush(Qt::white);
    painter->setPen(QPen(QColor(0x5a, 0x5c, 0x62), 0.0));
    painter->drawRect(pageRect());
    painter->restore();
}

// ------------------------------------------------------------ Projektformat

QJsonObject CanvasScene::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("application")] = QStringLiteral("Inkpot");
    root[QStringLiteral("formatVersion")] = 2;

    QJsonObject canvas;
    canvas[QStringLiteral("width")] = m_pageSize.width();
    canvas[QStringLiteral("height")] = m_pageSize.height();
    canvas[QStringLiteral("aspectLocked")] = m_aspectLocked;
    canvas[QStringLiteral("lockedRatio")] = m_lockedRatio;
    root[QStringLiteral("canvas")] = canvas;

    QList<QGraphicsItem *> all = objects();
    std::sort(all.begin(), all.end(), [](QGraphicsItem *a, QGraphicsItem *b) {
        return a->zValue() < b->zValue();
    });

    QJsonArray array;
    for (QGraphicsItem *item : all) {
        auto *object = dynamic_cast<InkObject *>(item);
        if (!object)
            continue;

        QJsonObject json;
        json[QStringLiteral("type")] = object->typeName();
        json[QStringLiteral("x")] = item->pos().x();
        json[QStringLiteral("y")] = item->pos().y();
        json[QStringLiteral("z")] = item->zValue();

        const QTransform transform = item->transform();
        if (!transform.isIdentity()) {
            QJsonArray values;
            values.append(transform.m11());
            values.append(transform.m12());
            values.append(transform.m21());
            values.append(transform.m22());
            values.append(transform.dx());
            values.append(transform.dy());
            json[QStringLiteral("transform")] = values;
        }

        if (object->hasErasedArea())
            json[QStringLiteral("erased")] = pathToBase64(object->erasedMask());

        object->write(json);
        array.append(json);
    }
    root[QStringLiteral("objects")] = array;

    return root;
}

bool CanvasScene::fromJson(const QJsonObject &json, QString *error)
{
    if (json.value(QStringLiteral("application")).toString() != QLatin1String("Inkpot")) {
        if (error)
            *error = tr("Die Datei ist kein Inkpot-Projekt.");
        return false;
    }

    clearObjects();

    const QJsonObject canvas = json.value(QStringLiteral("canvas")).toObject();
    m_lockedRatio = canvas.value(QStringLiteral("lockedRatio")).toDouble(16.0 / 9.0);
    m_aspectLocked = canvas.value(QStringLiteral("aspectLocked")).toBool(false);
    setPageSize(QSize(canvas.value(QStringLiteral("width")).toInt(1280),
                      canvas.value(QStringLiteral("height")).toInt(720)));

    const QJsonArray array = json.value(QStringLiteral("objects")).toArray();
    for (const QJsonValue &value : array) {
        const QJsonObject objectJson = value.toObject();
        QGraphicsItem *item = createObjectForType(objectJson.value(QStringLiteral("type")).toString());
        if (!item)
            continue;

        auto *object = dynamic_cast<InkObject *>(item);
        if (!object || !object->read(objectJson)) {
            delete item;
            continue;
        }

        item->setPos(objectJson.value(QStringLiteral("x")).toDouble(),
                     objectJson.value(QStringLiteral("y")).toDouble());

        const QJsonArray transformValues = objectJson.value(QStringLiteral("transform")).toArray();
        if (transformValues.size() == 6) {
            item->setTransform(QTransform(transformValues.at(0).toDouble(),
                                          transformValues.at(1).toDouble(),
                                          transformValues.at(2).toDouble(),
                                          transformValues.at(3).toDouble(),
                                          transformValues.at(4).toDouble(),
                                          transformValues.at(5).toDouble()));
        }

        const QString erased = objectJson.value(QStringLiteral("erased")).toString();
        if (!erased.isEmpty())
            object->setErasedMask(pathFromBase64(erased));

        prepareObject(item);
        const qreal z = objectJson.value(QStringLiteral("z")).toDouble(m_topZ + 1.0);
        item->setZValue(z);
        m_topZ = qMax(m_topZ, z);
        addItem(item);
    }

    updateSceneRect();
    emit pageRectChanged();
    emit objectsChanged();
    return true;
}
