#pragma once

#include <QGraphicsScene>
#include <QJsonObject>
#include <QList>
#include <QPainterPath>
#include <QRectF>
#include <QSize>
#include <QTransform>

#include "ResizeHandle.h"

// The scene holds the canvas (the "page") and every object on it.
//
// The canvas is pinned to the top left corner at (0, 0) and grows to the right
// and downwards. It is resized with the three handles on its right and bottom
// edge, or through the fields in the status bar.
class CanvasScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CanvasScene(QObject *parent = nullptr);
    ~CanvasScene() override;

    QRectF pageRect() const { return QRectF(QPointF(0.0, 0.0), QSizeF(m_pageSize)); }
    QSize pageSize() const { return m_pageSize; }

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

    // Managing objects.
    void addObject(QGraphicsItem *item);
    QList<QGraphicsItem *> objects() const;
    int objectCount() const { return objects().size(); }
    void removeSelectedObjects();
    void removeObjectLater(QGraphicsItem *item);
    void clearObjects();
    void selectAllObjects();
    QGraphicsItem *singleSelectedObject() const;

    // Properties of the selection.
    bool setSelectedObjectsColor(const QColor &color);
    QColor selectedObjectColor() const;
    class TextItem *singleSelectedText() const;

    // Erasing inside the objects.
    void eraseWithPath(const QPainterPath &scenePath);

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

    // Starting values while an object is being scaled.
    QGraphicsItem *m_scaleTarget = nullptr;
    QRectF m_scaleStartRect;
    QTransform m_scaleStartTransform;
    qreal m_scaleStartFontSize = 0.0;
};
