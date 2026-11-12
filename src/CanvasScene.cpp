#include "CanvasScene.h"

#include <algorithm>
#include <cmath>

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QImage>
#include <QJsonArray>
#include <QPainter>
#include <QTimer>

#include "FillItem.h"
#include "ShapeItem.h"
#include "FloodFill.h"
#include "ImageItem.h"
#include "StrokeItem.h"
#include "TextItem.h"

namespace {

constexpr qreal kMinimumSide = 16.0;     // smallest edge length of the canvas
constexpr qreal kMinimumObjectSide = 4.0;
constexpr qreal kSceneMargin = 400.0;    // room to work right of and below it

// Creates an empty object matching the type identifier in the project format.
QGraphicsItem *createObjectForType(const QString &type)
{
    if (type == QLatin1String("stroke"))
        return new StrokeItem;
    if (type == QLatin1String("image"))
        return new ImageItem;
    if (type == QLatin1String("text"))
        return new TextItem;
    if (type == QLatin1String("fill"))
        return new FillItem;
    if (type == QLatin1String("shape"))
        return new ShapeItem;
    return nullptr;
}

}   // namespace

CanvasScene::CanvasScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_undoStack(new QUndoStack(this))
{
    createHandles();
    updateCanvasHandles();
    updateObjectHandles();
    updateSceneRect();

    connect(this, &QGraphicsScene::selectionChanged, this, [this] {
        updateObjectHandles();
        emit objectPropertiesChanged();
    });

    // Objects that were moved or changed need their handles repositioned.
    // The flag keeps placing the handles from triggering itself.
    connect(this, &QGraphicsScene::changed, this, [this] {
        if (!m_updatingHandles)
            updateObjectHandles();
    });
}

CanvasScene::~CanvasScene()
{
    // While tearing down, ~QGraphicsScene still deletes the objects and emits
    // changed() along the way. By then the handle lists, being members of this
    // class, are already gone, so drop all of our own connections here.
    disconnect(this, nullptr, this, nullptr);
}

void CanvasScene::createHandles()
{
    // The canvas is pinned at the top and on the left; it only grows to the
    // right and downwards.
    const ResizeHandle::Position canvasPositions[] = {
        ResizeHandle::Right, ResizeHandle::BottomRight, ResizeHandle::Bottom
    };
    for (ResizeHandle::Position position : canvasPositions) {
        auto *handle = new ResizeHandle(position, ResizeHandle::Role::Canvas, this);
        addItem(handle);
        m_canvasHandles.append(handle);
    }

    // Objects, in contrast, can be scaled in every direction.
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

    // The turning handle sits above the top edge of the selected object.
    m_rotateHandle = new ResizeHandle(ResizeHandle::Top, ResizeHandle::Role::Rotate, this);
    m_rotateHandle->setVisible(false);
    addItem(m_rotateHandle);
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
        m_rotateHandle->setVisible(false);
    } else {
        // The handles follow the object's own rectangle, so they turn with it.
        const QRectF local = target->boundingRect();
        for (ResizeHandle *handle : m_objectHandles) {
            handle->setPos(target->mapToScene(
                ResizeHandle::anchorPoint(handle->handlePosition(), local)));
            handle->setVisible(true);
        }

        // Keep the turning handle a constant distance above the top edge on
        // screen, whatever the zoom.
        qreal viewScale = 1.0;
        if (!views().isEmpty())
            viewScale = qMax(qreal(0.01), views().first()->transform().m11());

        const QPointF topCentre = target->mapToScene(
            ResizeHandle::anchorPoint(ResizeHandle::Top, local));
        const QPointF up = target->mapToScene(QPointF(0.0, -1.0))
                           - target->mapToScene(QPointF(0.0, 0.0));
        const qreal length = std::hypot(up.x(), up.y());
        const QPointF direction = length > 0.0 ? up / length : QPointF(0.0, -1.0);

        m_rotateHandle->setPos(topCentre + direction * (30.0 / viewScale));
        m_rotateHandle->setVisible(true);
    }
    m_updatingHandles = false;
}

void CanvasScene::setHandlesVisible(bool visible)
{
    m_updatingHandles = true;
    for (ResizeHandle *handle : m_canvasHandles)
        handle->setVisible(visible);
    const bool withObject = visible && singleSelectedObject() != nullptr;
    for (ResizeHandle *handle : m_objectHandles)
        handle->setVisible(withObject);
    m_rotateHandle->setVisible(withObject);
    m_updatingHandles = false;
}

void CanvasScene::updateSceneRect()
{
    // On the left and at the top the scene ends flush with the canvas, which
    // is what keeps the canvas pinned to the corner of the view.
    setSceneRect(QRectF(0.0, 0.0,
                        m_pageSize.width() + kSceneMargin,
                        m_pageSize.height() + kSceneMargin));
}

void CanvasScene::newDocument(const QSize &size)
{
    clearObjects();
    applyPageSize(QSize(qMax(int(kMinimumSide), size.width()),
                        qMax(int(kMinimumSide), size.height())));
    m_undoStack->clear();
    m_uncommitted.clear();
}

void CanvasScene::setPageSize(const QSize &size)
{
    const QSize clamped(qMax(int(kMinimumSide), size.width()),
                        qMax(int(kMinimumSide), size.height()));
    if (clamped == m_pageSize)
        return;

    // While a handle is being dragged the size changes continuously; one entry
    // in the history is recorded when the drag ends.
    if (!m_resizingCanvas)
        m_undoStack->push(new PageSizeCommand(this, m_pageSize, clamped));
    else
        applyPageSize(clamped);
}

void CanvasScene::applyPageSize(const QSize &size)
{
    if (size == m_pageSize)
        return;

    m_pageSize = size;
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

// ------------------------------------------------------------- Canvas size

void CanvasScene::beginCanvasResize()
{
    if (m_aspectLocked && m_pageSize.height() > 0)
        m_lockedRatio = double(m_pageSize.width()) / m_pageSize.height();
    m_resizingCanvas = true;
    m_pageSizeBeforeResize = m_pageSize;
}

void CanvasScene::endCanvasResize()
{
    m_resizingCanvas = false;
    if (m_pageSize != m_pageSizeBeforeResize)
        m_undoStack->push(new PageSizeCommand(this, m_pageSizeBeforeResize, m_pageSize));
    emit pageRectChanged();
}

void CanvasScene::resizeCanvasFromHandle(ResizeHandle::Position position, const QPointF &scenePos)
{
    // The top left corner always stays at (0, 0); only the dragged edge
    // follows the mouse pointer.
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
        return;   // there are no handles at the top or on the left
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
            // Corner handle: the axis that changed more sets the tone.
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

// --------------------------------------------------------- Scaling objects

void CanvasScene::pushTransform(const QString &text)
{
    QList<QGraphicsItem *> changed;
    QList<Placement> before;
    QList<Placement> after;

    for (int i = 0; i < m_dragItems.size() && i < m_dragBefore.size(); ++i) {
        QGraphicsItem *item = m_dragItems.at(i);
        const Placement now = placementOf(item);
        if (now == m_dragBefore.at(i))
            continue;
        changed.append(item);
        before.append(m_dragBefore.at(i));
        after.append(now);
    }

    m_dragItems.clear();
    m_dragBefore.clear();

    if (!changed.isEmpty())
        m_undoStack->push(new TransformCommand(this, changed, before, after, text));
}

void CanvasScene::beginObjectMove()
{
    m_dragItems.clear();
    m_dragBefore.clear();
    const QList<QGraphicsItem *> selection = selectedItems();
    for (QGraphicsItem *item : selection) {
        if (!dynamic_cast<InkObject *>(item))
            continue;
        m_dragItems.append(item);
        m_dragBefore.append(placementOf(item));
    }
}

void CanvasScene::endObjectMove()
{
    pushTransform(tr("Move objects"));
}

qreal CanvasScene::angleTo(const QPointF &centre, const QPointF &point)
{
    const QPointF delta = point - centre;
    return std::atan2(delta.y(), delta.x()) * 180.0 / M_PI;
}

void CanvasScene::beginObjectScale()
{
    m_scaleTarget = singleSelectedObject();
    auto *object = dynamic_cast<InkObject *>(m_scaleTarget);
    if (!object) {
        m_scaleTarget = nullptr;
        return;
    }

    // Everything is measured in the object's own coordinates, so a turned
    // object still grows along its own axes.
    m_scaleStartRect = m_scaleTarget->boundingRect();
    m_scaleStartSceneTransform = m_scaleTarget->sceneTransform();
    m_scaleStartAngle = object->rotationAngle();
    m_scaleStartScaleX = object->scaleX();
    m_scaleStartScaleY = object->scaleY();

    m_scaleStartFontSize = 0.0;
    if (auto *text = dynamic_cast<TextItem *>(m_scaleTarget))
        m_scaleStartFontSize = text->fontPointSize();

    m_dragItems = { m_scaleTarget };
    m_dragBefore = { placementOf(m_scaleTarget) };
}

void CanvasScene::scaleObjectFromHandle(ResizeHandle::Position position, const QPointF &scenePos)
{
    auto *object = dynamic_cast<InkObject *>(m_scaleTarget);
    if (!object || m_scaleStartRect.isEmpty())
        return;

    bool invertible = false;
    const QTransform toLocal = m_scaleStartSceneTransform.inverted(&invertible);
    if (!invertible)
        return;

    const QPointF local = toLocal.map(scenePos);
    const QRectF base = m_scaleStartRect;

    // The dragged edge follows the pointer, the opposite one stays put.
    const QPointF edge = ResizeHandle::anchorPoint(position, base);
    const QPointF anchor = ResizeHandle::anchorPoint(ResizeHandle::opposite(position), base);

    const bool drivesX = position != ResizeHandle::Top && position != ResizeHandle::Bottom;
    const bool drivesY = position != ResizeHandle::Left && position != ResizeHandle::Right;

    qreal factorX = 1.0;
    qreal factorY = 1.0;
    if (drivesX && !qFuzzyCompare(edge.x(), anchor.x()))
        factorX = (local.x() - anchor.x()) / (edge.x() - anchor.x());
    if (drivesY && !qFuzzyCompare(edge.y(), anchor.y()))
        factorY = (local.y() - anchor.y()) / (edge.y() - anchor.y());

    // No flipping and nothing that collapses to nothing.
    factorX = qMax(qreal(0.02), factorX);
    factorY = qMax(qreal(0.02), factorY);

    QPointF anchorNow = anchor;

    if (auto *text = dynamic_cast<TextItem *>(m_scaleTarget)) {
        // Text is not stretched but grows through its font size. Side handles
        // follow the width, all others the height.
        const bool widthDrives = position == ResizeHandle::Left
                                 || position == ResizeHandle::Right;
        text->setFontPointSize(m_scaleStartFontSize * (widthDrives ? factorX : factorY));
        object->applyPlacement();   // the centre moved with the new size
        anchorNow = ResizeHandle::anchorPoint(ResizeHandle::opposite(position),
                                              text->boundingRect());
    } else {
        object->setPlacement(m_scaleStartAngle,
                             m_scaleStartScaleX * factorX,
                             m_scaleStartScaleY * factorY);
    }

    // Put the object back so the stationary edge really does stay where it was.
    const QPointF before = m_scaleStartSceneTransform.map(anchor);
    const QPointF after = m_scaleTarget->sceneTransform().map(anchorNow);
    m_scaleTarget->setPos(m_scaleTarget->pos() + (before - after));

    updateObjectHandles();
    emit objectPropertiesChanged();
}

void CanvasScene::endObjectScale()
{
    if (!m_scaleTarget)
        return;
    m_scaleTarget = nullptr;
    pushTransform(tr("Scale object"));
    updateObjectHandles();
    emit objectPropertiesChanged();
    emit edited();
}

// --------------------------------------------------------- Turning objects

void CanvasScene::beginObjectRotate(const QPointF &scenePos)
{
    m_rotateTarget = singleSelectedObject();
    auto *object = dynamic_cast<InkObject *>(m_rotateTarget);
    if (!object) {
        m_rotateTarget = nullptr;
        return;
    }

    // Turning happens about the centre of the object, which stays put.
    m_rotateCentre = m_rotateTarget->mapToScene(m_rotateTarget->boundingRect().center());
    m_rotateStartAngle = object->rotationAngle();
    m_rotateStartPointer = angleTo(m_rotateCentre, scenePos);

    m_dragItems = { m_rotateTarget };
    m_dragBefore = { placementOf(m_rotateTarget) };
}

void CanvasScene::rotateObjectFromHandle(const QPointF &scenePos,
                                         Qt::KeyboardModifiers modifiers)
{
    auto *object = dynamic_cast<InkObject *>(m_rotateTarget);
    if (!object)
        return;

    qreal angle = m_rotateStartAngle + (angleTo(m_rotateCentre, scenePos) - m_rotateStartPointer);

    if (modifiers & Qt::ShiftModifier)
        angle = qRound(angle / 15.0) * 15.0;

    while (angle >= 180.0)
        angle -= 360.0;
    while (angle < -180.0)
        angle += 360.0;

    object->setPlacement(angle, object->scaleX(), object->scaleY());

    updateObjectHandles();
    emit objectPropertiesChanged();
}

void CanvasScene::endObjectRotate()
{
    if (!m_rotateTarget)
        return;
    m_rotateTarget = nullptr;
    pushTransform(tr("Turn object"));
    updateObjectHandles();
    emit objectPropertiesChanged();
    emit edited();
}

// ----------------------------------------------------------------- Objects

void CanvasScene::prepareObject(QGraphicsItem *item)
{
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
    item->setFlag(QGraphicsItem::ItemIsMovable, true);
    item->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

void CanvasScene::insertObject(QGraphicsItem *item)
{
    if (!item)
        return;
    prepareObject(item);
    addItem(item);
    objectsWereChanged();
}

void CanvasScene::takeObject(QGraphicsItem *item)
{
    if (!item || item->scene() != this)
        return;
    item->setSelected(false);
    removeItem(item);
    updateObjectHandles();
    objectsWereChanged();
}

void CanvasScene::objectsWereChanged()
{
    updateObjectHandles();
    emit objectsChanged();
    emit edited();
}

void CanvasScene::pushTextChange(TextItem *item, const QString &before, const QString &after)
{
    if (item && before != after)
        m_undoStack->push(new TextCommand(this, item, before, after));
}

void CanvasScene::commitAddedObject(QGraphicsItem *item, const QString &text)
{
    if (!item || !m_uncommitted.remove(item))
        return;
    m_undoStack->push(new AddObjectsCommand(
        this, { item }, text.isEmpty() ? tr("Add object") : text));
}

void CanvasScene::addObjectUndoable(QGraphicsItem *item, const QString &text)
{
    addObject(item);
    commitAddedObject(item, text);
}

void CanvasScene::addObject(QGraphicsItem *item)
{
    if (!item)
        return;
    prepareObject(item);
    m_topZ += 1.0;
    item->setZValue(m_topZ);
    addItem(item);
    m_uncommitted.insert(item);
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
            return nullptr;   // several objects at once are not scaled
        found = item;
    }
    return found;
}

void CanvasScene::removeSelectedObjects()
{
    QList<QGraphicsItem *> doomed;
    const QList<QGraphicsItem *> selection = selectedItems();
    for (QGraphicsItem *item : selection) {
        if (dynamic_cast<InkObject *>(item))
            doomed.append(item);
    }
    if (doomed.isEmpty())
        return;

    // Anything still waiting to be recorded is simply forgotten.
    for (QGraphicsItem *item : doomed)
        m_uncommitted.remove(item);

    m_undoStack->push(new RemoveObjectsCommand(this, doomed, tr("Delete objects")));
}

void CanvasScene::removeObjectLater(QGraphicsItem *item)
{
    if (!item || !dynamic_cast<InkObject *>(item))
        return;

    // Never recorded, so it can simply go.
    m_uncommitted.remove(item);

    // Called from an event of the object itself, hence the delay.
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
    // The history belongs to the drawing that is going away.
    m_undoStack->clear();
    m_uncommitted.clear();

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

bool CanvasScene::setSelectedObjectsColor(const QColor &color)
{
    if (!color.isValid())
        return false;

    QList<QGraphicsItem *> changed;
    QList<QColor> before;

    const QList<QGraphicsItem *> selection = selectedItems();
    for (QGraphicsItem *item : selection) {
        auto *object = dynamic_cast<InkObject *>(item);
        if (!object || !object->color().isValid() || object->color() == color)
            continue;
        changed.append(item);
        before.append(object->color());
    }

    if (changed.isEmpty())
        return false;

    m_undoStack->push(new ColorCommand(this, changed, before, color));
    emit objectPropertiesChanged();
    return true;
}

QColor CanvasScene::selectedObjectColor() const
{
    const QList<QGraphicsItem *> selection = selectedItems();
    for (QGraphicsItem *item : selection) {
        auto *object = dynamic_cast<InkObject *>(item);
        if (object && object->color().isValid())
            return object->color();
    }
    return QColor();
}

TextItem *CanvasScene::singleSelectedText() const
{
    return dynamic_cast<TextItem *>(singleSelectedObject());
}

void CanvasScene::renameObject(QGraphicsItem *item, const QString &name)
{
    auto *object = dynamic_cast<InkObject *>(item);
    if (!object)
        return;

    const QString wanted = name.trimmed();
    if (wanted == object->name())
        return;

    m_undoStack->push(new RenameCommand(this, item, object->name(), wanted));
}

bool CanvasScene::restackSelection(bool forward)
{
    QList<QGraphicsItem *> all = objects();
    std::sort(all.begin(), all.end(), [](QGraphicsItem *a, QGraphicsItem *b) {
        return a->zValue() < b->zValue();
    });
    if (all.size() < 2)
        return false;

    QList<qreal> before;
    before.reserve(all.size());
    for (QGraphicsItem *item : all)
        before << item->zValue();

    // Each selected object trades places with its unselected neighbour. Going
    // through the list from the far end means a block of selected objects
    // travels as one and keeps its own order.
    QList<QGraphicsItem *> order = all;
    if (forward) {
        for (int i = order.size() - 2; i >= 0; --i) {
            if (order.at(i)->isSelected() && !order.at(i + 1)->isSelected())
                order.swapItemsAt(i, i + 1);
        }
    } else {
        for (int i = 1; i < order.size(); ++i) {
            if (order.at(i)->isSelected() && !order.at(i - 1)->isSelected())
                order.swapItemsAt(i, i - 1);
        }
    }
    if (order == all)
        return false;   // already at the end of the stack, or nothing selected

    // The places keep their numbers and the objects move between them, so the
    // values stay as tidy as they were.
    QList<qreal> after;
    after.reserve(all.size());
    for (QGraphicsItem *item : all)
        after << before.at(order.indexOf(item));

    m_undoStack->push(new StackCommand(this,
                                       forward ? tr("Bring forward") : tr("Send backward"),
                                       all, before, after));
    return true;
}

bool CanvasScene::fillAt(const QPointF &scenePos, const QColor &color, int tolerance)
{
    const QRectF page = pageRect();
    if (!color.isValid() || !page.contains(scenePos))
        return false;

    // The area is found on a picture drawn larger than the canvas. The outline
    // follows the pixels of that picture, so the finer they are the smaller
    // the steps it is made of - which is what keeps the edges clean once the
    // filled area is scaled up. The factor is capped so a large canvas cannot
    // ask for an unreasonable amount of memory.
    constexpr int wanted = 3;
    constexpr qreal budget = 32.0e6;   // pixels in the enlarged picture
    int sampling = wanted;
    while (sampling > 1 && page.width() * page.height() * sampling * sampling > budget)
        --sampling;

    const QImage picture = renderPage(Qt::white, sampling);
    if (picture.isNull())
        return false;

    const QPoint start(qRound((scenePos.x() - page.left()) * sampling),
                       qRound((scenePos.y() - page.top()) * sampling));
    if (!picture.rect().contains(start))
        return false;

    // Clicking a spot that already has the colour would change nothing.
    if (QColor::fromRgba(picture.pixel(start)) == color)
        return false;

    // Growing by one canvas pixel tucks the fill under the antialiased edges
    // that bound it, which is why the step counts in the enlarged picture.
    QPainterPath region = FloodFill::regionAt(picture, start, tolerance, sampling);
    if (region.isEmpty())
        return false;

    QTransform toCanvas;
    toCanvas.scale(1.0 / sampling, 1.0 / sampling);
    region = toCanvas.map(region);

    // What is left of the steps is below half a canvas pixel, and straightening
    // at that size costs no visible accuracy while it removes most of the
    // points: fewer to store, fewer to draw. Tracing the result with curves
    // then takes out the last of the steps, which would otherwise show once
    // the filled area is made much larger.
    region = FloodFill::straightened(region, 0.4);

    // The points that are left sit either on a corner or on a curve. Tracing
    // runs curves through the ones on a curve and leaves the corners alone,
    // which is what finally takes the steps out of a round edge. Measured on
    // a filled star and heart, the outline changes direction sixty times less
    // abruptly than it did before.
    region = FloodFill::traced(region, 60.0);

    auto *item = new FillItem(region.translated(page.topLeft()), color);
    addObjectUndoable(item, tr("Fill area"));
    return true;
}

void CanvasScene::beginErase()
{
    m_eraseItems.clear();
    m_eraseBefore.clear();
    const QList<QGraphicsItem *> all = objects();
    for (QGraphicsItem *item : all) {
        auto *object = dynamic_cast<InkObject *>(item);
        if (!object)
            continue;
        m_eraseItems.append(item);
        m_eraseBefore.append(object->erasedMask());
    }
}

void CanvasScene::endErase()
{
    QList<QGraphicsItem *> changed;
    QList<QPainterPath> before;
    QList<QPainterPath> after;

    for (int i = 0; i < m_eraseItems.size() && i < m_eraseBefore.size(); ++i) {
        auto *object = dynamic_cast<InkObject *>(m_eraseItems.at(i));
        if (!object || object->erasedMask() == m_eraseBefore.at(i))
            continue;
        changed.append(m_eraseItems.at(i));
        before.append(m_eraseBefore.at(i));
        after.append(object->erasedMask());
    }

    m_eraseItems.clear();
    m_eraseBefore.clear();

    if (!changed.isEmpty())
        m_undoStack->push(new EraseCommand(this, changed, before, after));
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

// ----------------------------------------------------------------- Rendering

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

    // Hide handles, selection frames and unwanted objects before drawing,
    // and restore them afterwards.
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
        return;   // when exporting draw only the objects: no frame, no shadow

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

// ------------------------------------------------------------ Project format

QJsonObject CanvasScene::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("application")] = QStringLiteral("Inkpot");
    root[QStringLiteral("formatVersion")] = 3;

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
        if (!object->name().isEmpty())
            json[QStringLiteral("name")] = object->name();

        // Angle and scale are written instead of the raw matrix: that keeps
        // the file readable and lets the matrix be rebuilt in the right order.
        if (!qFuzzyIsNull(object->rotationAngle()))
            json[QStringLiteral("rotation")] = object->rotationAngle();
        if (!qFuzzyCompare(object->scaleX(), qreal(1.0)))
            json[QStringLiteral("scaleX")] = object->scaleX();
        if (!qFuzzyCompare(object->scaleY(), qreal(1.0)))
            json[QStringLiteral("scaleY")] = object->scaleY();

        if (object->hasErasedArea())
            json[QStringLiteral("erased")] = InkPath::toBase64(object->erasedMask());

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
            *error = tr("The file is not an Inkpot project.");
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

        qreal scaleX = objectJson.value(QStringLiteral("scaleX")).toDouble(1.0);
        qreal scaleY = objectJson.value(QStringLiteral("scaleY")).toDouble(1.0);

        // Files written before objects could be turned carry the raw matrix,
        // which only ever held a scale.
        const QJsonArray legacy = objectJson.value(QStringLiteral("transform")).toArray();
        if (legacy.size() == 6) {
            scaleX = legacy.at(0).toDouble(1.0);
            scaleY = legacy.at(3).toDouble(1.0);
        }

        object->setPlacement(objectJson.value(QStringLiteral("rotation")).toDouble(0.0),
                             scaleX, scaleY);

        object->setName(objectJson.value(QStringLiteral("name")).toString());

        const QString erased = objectJson.value(QStringLiteral("erased")).toString();
        if (!erased.isEmpty())
            object->setErasedMask(InkPath::fromBase64(erased));

        prepareObject(item);
        const qreal z = objectJson.value(QStringLiteral("z")).toDouble(m_topZ + 1.0);
        item->setZValue(z);
        m_topZ = qMax(m_topZ, z);
        addItem(item);
    }

    updateSceneRect();

    // An opened drawing starts with a clean history.
    m_undoStack->clear();
    m_uncommitted.clear();

    emit pageRectChanged();
    emit objectsChanged();
    return true;
}
