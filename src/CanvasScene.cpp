#include "CanvasScene.h"

#include <algorithm>

#include <QGraphicsItem>
#include <QImage>
#include <QJsonArray>
#include <QPainter>

#include "ImageItem.h"
#include "StrokeItem.h"

namespace {

constexpr qreal kMinimumSide = 16.0;     // kleinste Kantenlaenge in Pixel
constexpr qreal kSceneMargin = 400.0;    // Platz um die Arbeitsflaeche herum

// Erzeugt ein leeres Objekt passend zum Typbezeichner im Projektformat.
QGraphicsItem *createObjectForType(const QString &type)
{
    if (type == QLatin1String("stroke"))
        return new StrokeItem;
    if (type == QLatin1String("image"))
        return new ImageItem;
    return nullptr;
}

}   // namespace

CanvasScene::CanvasScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_pageRect(0.0, 0.0, 1280.0, 720.0)
{
    createHandles();
    updateHandlePositions();
    updateSceneRect();
}

void CanvasScene::createHandles()
{
    const ResizeHandle::Position positions[] = {
        ResizeHandle::TopLeft,  ResizeHandle::Top,    ResizeHandle::TopRight,
        ResizeHandle::Right,    ResizeHandle::BottomRight, ResizeHandle::Bottom,
        ResizeHandle::BottomLeft, ResizeHandle::Left
    };
    for (ResizeHandle::Position position : positions) {
        auto *handle = new ResizeHandle(position, this);
        addItem(handle);
        m_handles.append(handle);
    }
}

void CanvasScene::updateHandlePositions()
{
    const QRectF r = m_pageRect;
    for (ResizeHandle *handle : m_handles) {
        QPointF point;
        switch (handle->handlePosition()) {
        case ResizeHandle::TopLeft:     point = r.topLeft();                        break;
        case ResizeHandle::Top:         point = QPointF(r.center().x(), r.top());    break;
        case ResizeHandle::TopRight:    point = r.topRight();                       break;
        case ResizeHandle::Right:       point = QPointF(r.right(), r.center().y());  break;
        case ResizeHandle::BottomRight: point = r.bottomRight();                    break;
        case ResizeHandle::Bottom:      point = QPointF(r.center().x(), r.bottom()); break;
        case ResizeHandle::BottomLeft:  point = r.bottomLeft();                     break;
        case ResizeHandle::Left:        point = QPointF(r.left(), r.center().y());   break;
        }
        handle->setPos(point);
    }
}

void CanvasScene::setHandlesVisible(bool visible)
{
    for (ResizeHandle *handle : m_handles)
        handle->setVisible(visible);
}

void CanvasScene::updateSceneRect()
{
    QRectF rect = m_pageRect.adjusted(-kSceneMargin, -kSceneMargin, kSceneMargin, kSceneMargin);
    const QRectF itemsRect = itemsBoundingRect();
    if (!itemsRect.isNull())
        rect = rect.united(itemsRect.adjusted(-kSceneMargin / 4.0, -kSceneMargin / 4.0,
                                              kSceneMargin / 4.0, kSceneMargin / 4.0));
    setSceneRect(rect);
}

void CanvasScene::setPageRect(const QRectF &rect)
{
    // Ganzzahlige Groessen halten Export und Druck vorhersagbar.
    const QRectF rounded(qRound(rect.x()), qRound(rect.y()),
                         qMax(kMinimumSide, qreal(qRound(rect.width()))),
                         qMax(kMinimumSide, qreal(qRound(rect.height()))));
    if (rounded == m_pageRect)
        return;

    m_pageRect = rounded;
    updateHandlePositions();
    updateSceneRect();
    update();
    emit pageRectChanged();
    emit edited();
}

void CanvasScene::setPageSize(const QSize &size)
{
    // Die linke obere Ecke bleibt stehen.
    setPageRect(QRectF(m_pageRect.topLeft(), QSizeF(size)));
}

void CanvasScene::setPageWidth(int width)
{
    if (width <= 0)
        return;
    QSize size = pageSize();
    size.setWidth(width);
    if (m_aspectLocked && m_lockedRatio > 0.0)
        size.setHeight(qMax(1, qRound(width / m_lockedRatio)));
    setPageSize(size);
}

void CanvasScene::setPageHeight(int height)
{
    if (height <= 0)
        return;
    QSize size = pageSize();
    size.setHeight(height);
    if (m_aspectLocked && m_lockedRatio > 0.0)
        size.setWidth(qMax(1, qRound(height * m_lockedRatio)));
    setPageSize(size);
}

void CanvasScene::setAspectRatioLocked(bool locked)
{
    m_aspectLocked = locked;
    if (locked && m_pageRect.height() > 0.0)
        m_lockedRatio = m_pageRect.width() / m_pageRect.height();
}

void CanvasScene::beginHandleResize()
{
    if (m_aspectLocked && m_pageRect.height() > 0.0)
        m_lockedRatio = m_pageRect.width() / m_pageRect.height();
}

void CanvasScene::endHandleResize()
{
    emit pageRectChanged();
}

void CanvasScene::resizeFromHandle(ResizeHandle::Position position, const QPointF &scenePos)
{
    QRectF rect = m_pageRect;

    // Die gezogene Kante folgt dem Mauszeiger, die uebrigen bleiben stehen.
    switch (position) {
    case ResizeHandle::TopLeft:
        rect.setTop(qMin(scenePos.y(), rect.bottom() - kMinimumSide));
        rect.setLeft(qMin(scenePos.x(), rect.right() - kMinimumSide));
        break;
    case ResizeHandle::Top:
        rect.setTop(qMin(scenePos.y(), rect.bottom() - kMinimumSide));
        break;
    case ResizeHandle::TopRight:
        rect.setTop(qMin(scenePos.y(), rect.bottom() - kMinimumSide));
        rect.setRight(qMax(scenePos.x(), rect.left() + kMinimumSide));
        break;
    case ResizeHandle::Right:
        rect.setRight(qMax(scenePos.x(), rect.left() + kMinimumSide));
        break;
    case ResizeHandle::BottomRight:
        rect.setBottom(qMax(scenePos.y(), rect.top() + kMinimumSide));
        rect.setRight(qMax(scenePos.x(), rect.left() + kMinimumSide));
        break;
    case ResizeHandle::Bottom:
        rect.setBottom(qMax(scenePos.y(), rect.top() + kMinimumSide));
        break;
    case ResizeHandle::BottomLeft:
        rect.setBottom(qMax(scenePos.y(), rect.top() + kMinimumSide));
        rect.setLeft(qMin(scenePos.x(), rect.right() - kMinimumSide));
        break;
    case ResizeHandle::Left:
        rect.setLeft(qMin(scenePos.x(), rect.right() - kMinimumSide));
        break;
    }

    if (m_aspectLocked && m_lockedRatio > 0.0) {
        qreal width = rect.width();
        qreal height = rect.height();

        switch (position) {
        case ResizeHandle::Left:
        case ResizeHandle::Right:
            // Nur die Breite wurde gezogen, die Hoehe folgt.
            height = width / m_lockedRatio;
            break;
        case ResizeHandle::Top:
        case ResizeHandle::Bottom:
            width = height * m_lockedRatio;
            break;
        default:
            // Eckgriff: die staerker veraenderte Achse gibt den Ton an.
            if (width / height > m_lockedRatio)
                height = width / m_lockedRatio;
            else
                width = height * m_lockedRatio;
            break;
        }

        width = qMax(width, kMinimumSide);
        height = qMax(height, kMinimumSide);

        // Neu verankern: die gegenueberliegende Kante darf sich nicht bewegen.
        switch (position) {
        case ResizeHandle::TopLeft:
            rect = QRectF(rect.right() - width, rect.bottom() - height, width, height);
            break;
        case ResizeHandle::Top:
        case ResizeHandle::TopRight:
            rect = QRectF(rect.left(), rect.bottom() - height, width, height);
            break;
        case ResizeHandle::BottomLeft:
        case ResizeHandle::Left:
            rect = QRectF(rect.right() - width, rect.top(), width, height);
            break;
        default:
            rect = QRectF(rect.left(), rect.top(), width, height);
            break;
        }
    }

    setPageRect(rect);
}

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
    updateSceneRect();
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
        updateSceneRect();
        emit objectsChanged();
        emit edited();
    }
}

void CanvasScene::clearObjects()
{
    const QList<QGraphicsItem *> all = objects();
    for (QGraphicsItem *item : all) {
        removeItem(item);
        delete item;
    }
    m_topZ = 0.0;
    if (!all.isEmpty()) {
        updateSceneRect();
        emit objectsChanged();
    }
}

void CanvasScene::selectAllObjects()
{
    const QList<QGraphicsItem *> all = objects();
    for (QGraphicsItem *item : all)
        item->setSelected(true);
}

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
    return renderRegion(m_pageRect, QList<QGraphicsItem *>(), background, scale);
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
    painter->drawRect(m_pageRect.translated(5.0, 5.0));
    painter->setBrush(Qt::white);
    painter->setPen(QPen(QColor(0x5a, 0x5c, 0x62), 0.0));
    painter->drawRect(m_pageRect);
    painter->restore();
}

QJsonObject CanvasScene::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("application")] = QStringLiteral("Inkpot");
    root[QStringLiteral("formatVersion")] = 1;

    QJsonObject canvas;
    canvas[QStringLiteral("width")] = pageSize().width();
    canvas[QStringLiteral("height")] = pageSize().height();
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
        object->write(json, m_pageRect.topLeft());
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
    const int width = canvas.value(QStringLiteral("width")).toInt(1280);
    const int height = canvas.value(QStringLiteral("height")).toInt(720);
    m_lockedRatio = canvas.value(QStringLiteral("lockedRatio")).toDouble(16.0 / 9.0);
    m_aspectLocked = canvas.value(QStringLiteral("aspectLocked")).toBool(false);

    // Geladene Projekte beginnen immer im Ursprung.
    setPageRect(QRectF(0.0, 0.0, width, height));

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
