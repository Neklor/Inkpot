#pragma once

#include <QColor>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QUndoCommand>

class CanvasScene;
class QGraphicsItem;
class TextItem;

// Undo for the canvas.
//
// Every command keeps only what it needs to go back and forth: pointers to the
// objects involved and the values before and after. Nothing is copied wholesale,
// so a long history stays cheap even with large pictures on the canvas.

// How an object sits on the canvas. Captions grow through their font size
// rather than a scale, so that value belongs here as well.
struct Placement {
    QPointF position;
    qreal angle = 0.0;
    qreal scaleX = 1.0;
    qreal scaleY = 1.0;
    qreal fontSize = 0.0;   // 0 for everything that is not a caption

    bool operator==(const Placement &other) const;
    bool operator!=(const Placement &other) const { return !(*this == other); }
};

Placement placementOf(QGraphicsItem *item);
void applyPlacement(QGraphicsItem *item, const Placement &placement);

// Objects appear: drawing, filling, pasting, placing a caption.
class AddObjectsCommand : public QUndoCommand
{
public:
    AddObjectsCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                      const QString &text);
    ~AddObjectsCommand() override;

    void undo() override;
    void redo() override;

private:
    CanvasScene *m_scene;
    QList<QGraphicsItem *> m_items;
    bool m_inScene = true;   // the objects are already there when the command is pushed
};

// Objects disappear.
class RemoveObjectsCommand : public QUndoCommand
{
public:
    RemoveObjectsCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                         const QString &text);
    ~RemoveObjectsCommand() override;

    void undo() override;
    void redo() override;

private:
    CanvasScene *m_scene;
    QList<QGraphicsItem *> m_items;
    bool m_inScene = true;
};

// Moving, scaling and turning all end up here.
class TransformCommand : public QUndoCommand
{
public:
    TransformCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                     const QList<Placement> &before, const QList<Placement> &after,
                     const QString &text);

    void undo() override;
    void redo() override;

private:
    void apply(const QList<Placement> &placements);

    CanvasScene *m_scene;
    QList<QGraphicsItem *> m_items;
    QList<Placement> m_before;
    QList<Placement> m_after;
};

class ColorCommand : public QUndoCommand
{
public:
    ColorCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                 const QList<QColor> &before, const QColor &after);

    void undo() override;
    void redo() override;

private:
    CanvasScene *m_scene;
    QList<QGraphicsItem *> m_items;
    QList<QColor> m_before;
    QColor m_after;
};

// One stroke of the eraser, however many objects it reached.
class EraseCommand : public QUndoCommand
{
public:
    EraseCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                 const QList<QPainterPath> &before, const QList<QPainterPath> &after);

    void undo() override;
    void redo() override;

private:
    void apply(const QList<QPainterPath> &masks);

    CanvasScene *m_scene;
    QList<QGraphicsItem *> m_items;
    QList<QPainterPath> m_before;
    QList<QPainterPath> m_after;
};

class PageSizeCommand : public QUndoCommand
{
public:
    PageSizeCommand(CanvasScene *scene, const QSize &before, const QSize &after);

    void undo() override;
    void redo() override;

private:
    CanvasScene *m_scene;
    QSize m_before;
    QSize m_after;
};

// The text of a caption, taken when editing ends.
class TextCommand : public QUndoCommand
{
public:
    TextCommand(CanvasScene *scene, TextItem *item, const QString &before, const QString &after);

    void undo() override;
    void redo() override;

private:
    CanvasScene *m_scene;
    TextItem *m_item;
    QString m_before;
    QString m_after;
};

// Giving an object a name of its own, or taking it away again.
class RenameCommand : public QUndoCommand
{
public:
    RenameCommand(CanvasScene *scene, QGraphicsItem *item,
                  const QString &before, const QString &after);

    void undo() override;
    void redo() override;

private:
    CanvasScene *m_scene;
    QGraphicsItem *m_item;
    QString m_before;
    QString m_after;
};

// Moving objects up or down the stack. The whole order is kept on both sides
// rather than the step itself, which makes undo exact even when several
// objects move at once.
class StackCommand : public QUndoCommand
{
public:
    StackCommand(CanvasScene *scene, const QString &text,
                 const QList<QGraphicsItem *> &items,
                 const QList<qreal> &before, const QList<qreal> &after);

    void undo() override;
    void redo() override;

private:
    void apply(const QList<qreal> &values);

    CanvasScene *m_scene;
    QList<QGraphicsItem *> m_items;
    QList<qreal> m_before;
    QList<qreal> m_after;
};
