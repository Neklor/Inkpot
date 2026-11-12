#pragma once

#include <QGraphicsScene>
#include <QJsonObject>
#include <QList>
#include <QPainterPath>
#include <QRectF>
#include <QSize>
#include <QTransform>

#include <QSet>
#include <QUndoStack>

#include "Commands.h"
#include "ResizeHandle.h"

// The scene holds the canvas (the "page") and every object on it.
//
// The canvas is pinned to the top left corner at (0, 0) and grows to the right
// and downwards. It is resized with the three handles on its right and bottom
// edge, or through the fields in the status bar.
class TextItem;

class CanvasScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CanvasScene(QObject *parent = nullptr);
    ~CanvasScene() override;

    QRectF pageRect() const { return QRectF(QPointF(0.0, 0.0), QSizeF(m_pageSize)); }
    QSize pageSize() const { return m_pageSize; }

    // Starts a fresh drawing: no objects, the given size, no history. Setting
    // up a new document is not something one undoes.
    void newDocument(const QSize &size);

    void setPageSize(const QSize &size);
    void setPageWidth(int width);
    void setPageHeight(int height);

    bool aspectRatioLocked() const { return m_aspectLocked; }
    void setAspectRatioLocked(bool locked);
    double lockedRatio() const { return m_lockedRatio; }

    // Dragging a handle of the canvas.
    void beginCanvasResize();
    void resizeCanvasFromHandle(ResizeHandle::Position position, const QPointF &scenePos);
    void endCanvasResize();

    // Dragging a handle of a selected object.
    void beginObjectScale();
    void scaleObjectFromHandle(ResizeHandle::Position position, const QPointF &scenePos);
    void endObjectScale();

    // Turning a selected object with the round handle above it.
    void beginObjectRotate(const QPointF &scenePos);
    void rotateObjectFromHandle(const QPointF &scenePos,
                                Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    void endObjectRotate();

    // Undo. Every change that a command describes goes through the raw
    // operations below, which do the work without recording anything.
    QUndoStack *undoStack() const { return m_undoStack; }
    void insertObject(QGraphicsItem *item);
    void takeObject(QGraphicsItem *item);
    void applyPageSize(const QSize &size);
    void objectsWereChanged();
    void pushTextChange(TextItem *item, const QString &before, const QString &after);

    // Managing objects.
    //
    // addObject() puts the object on the canvas without recording anything;
    // it is meant for a stroke that is still being drawn. Once it is finished,
    // commitAddedObject() records it. Objects that appear in one go, such as a
    // fill or a pasted picture, use addObjectUndoable().
    void addObject(QGraphicsItem *item);
    void commitAddedObject(QGraphicsItem *item, const QString &text = QString());
    void addObjectUndoable(QGraphicsItem *item, const QString &text = QString());
    QList<QGraphicsItem *> objects() const;
    int objectCount() const { return objects().size(); }
    void removeSelectedObjects();
    void removeObjectLater(QGraphicsItem *item);
    void clearObjects();
    void selectAllObjects();
    QGraphicsItem *singleSelectedObject() const;

    // Giving one object a name of its own. An empty name takes it away again
    // and the list falls back to saying what the object is.
    void renameObject(QGraphicsItem *item, const QString &name);

    // Moving the selected objects one place up or down the stack, which is
    // what decides who covers whom. Several at once keep their order among
    // themselves. Returns false when nothing could move.
    bool restackSelection(bool forward);

    // Properties of the selection.
    bool setSelectedObjectsColor(const QColor &color);
    QColor selectedObjectColor() const;
    class TextItem *singleSelectedText() const;

    // Bucket fill. The area is determined on the rendered picture of the
    // whole canvas, so it may span several objects. Returns false when there
    // is nothing to fill at that spot.
    bool fillAt(const QPointF &scenePos, const QColor &color, int tolerance = 24);

    // Erasing inside the objects. One drag of the eraser becomes one entry in
    // the history, however many objects it reached.
    void beginErase();
    void eraseWithPath(const QPainterPath &scenePath);
    void endErase();

    // Moving objects with the mouse, recorded as one entry.
    void beginObjectMove();
    void endObjectMove();

    // Callable from outside, for instance after objects have been moved.
    void notifyEdited() { emit edited(); }

    // Rendering to an image. Handles and selection frames are hidden.
    QImage renderRegion(const QRectF &source,
                        const QList<QGraphicsItem *> &onlyItems = QList<QGraphicsItem *>(),
                        const QColor &background = Qt::white,
                        qreal scale = 1.0);
    QImage renderPage(const QColor &background = Qt::white, qreal scale = 1.0);
    QImage renderSelection();

    // Project format (*.ipot).
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &json, QString *error = nullptr);

signals:
    void pageRectChanged();
    void objectsChanged();
    void objectPropertiesChanged();   // colour or font size of the selection
    void edited();          // there are unsaved changes

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    void createHandles();
    void updateCanvasHandles();
    void updateObjectHandles();
    void setHandlesVisible(bool visible);
    void updateSceneRect();
    void prepareObject(QGraphicsItem *item);

    QSize m_pageSize = QSize(1280, 720);
    bool m_aspectLocked = false;
    double m_lockedRatio = 16.0 / 9.0;
    bool m_renderingToImage = false;
    bool m_updatingHandles = false;
    qreal m_topZ = 0.0;

    QList<ResizeHandle *> m_canvasHandles;
    QList<ResizeHandle *> m_objectHandles;

    // Angle of the pointer around a centre, in degrees.
    static qreal angleTo(const QPointF &centre, const QPointF &point);

    // Records what changed between a begin and an end call.
    void pushTransform(const QString &text);

    QUndoStack *m_undoStack = nullptr;
    QSet<QGraphicsItem *> m_uncommitted;   // drawn but not yet in the history
    bool m_resizingCanvas = false;
    QSize m_pageSizeBeforeResize;

    QList<QGraphicsItem *> m_dragItems;
    QList<Placement> m_dragBefore;
    QList<QPainterPath> m_eraseBefore;
    QList<QGraphicsItem *> m_eraseItems;

    // Starting values while an object is being scaled.
    QGraphicsItem *m_scaleTarget = nullptr;
    QRectF m_scaleStartRect;              // in the object's own coordinates
    QTransform m_scaleStartSceneTransform;
    qreal m_scaleStartAngle = 0.0;
    qreal m_scaleStartScaleX = 1.0;
    qreal m_scaleStartScaleY = 1.0;
    qreal m_scaleStartFontSize = 0.0;

    // Starting values while an object is being turned.
    QGraphicsItem *m_rotateTarget = nullptr;
    QPointF m_rotateCentre;
    qreal m_rotateStartAngle = 0.0;
    qreal m_rotateStartPointer = 0.0;

    ResizeHandle *m_rotateHandle = nullptr;
};
