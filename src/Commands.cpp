#include "Commands.h"

#include <QCoreApplication>
#include <QGraphicsItem>

#include "CanvasScene.h"
#include "InkObject.h"
#include "TextItem.h"

bool Placement::operator==(const Placement &other) const
{
    return position == other.position
        && qFuzzyCompare(angle + 1.0, other.angle + 1.0)
        && qFuzzyCompare(scaleX, other.scaleX)
        && qFuzzyCompare(scaleY, other.scaleY)
        && qFuzzyCompare(fontSize + 1.0, other.fontSize + 1.0);
}

Placement placementOf(QGraphicsItem *item)
{
    Placement placement;
    if (!item)
        return placement;

    placement.position = item->pos();
    if (auto *object = dynamic_cast<InkObject *>(item)) {
        placement.angle = object->rotationAngle();
        placement.scaleX = object->scaleX();
        placement.scaleY = object->scaleY();
    }
    if (auto *text = dynamic_cast<TextItem *>(item))
        placement.fontSize = text->fontPointSize();
    return placement;
}

void applyPlacement(QGraphicsItem *item, const Placement &placement)
{
    if (!item)
        return;

    // The font size comes first: it changes the size of the caption, and the
    // matrix is built around the centre of that size.
    if (auto *text = dynamic_cast<TextItem *>(item); text && placement.fontSize > 0.0)
        text->setFontPointSize(placement.fontSize);

    if (auto *object = dynamic_cast<InkObject *>(item))
        object->setPlacement(placement.angle, placement.scaleX, placement.scaleY);

    item->setPos(placement.position);
}

// ------------------------------------------------------------------- Adding

AddObjectsCommand::AddObjectsCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                                     const QString &text)
    : QUndoCommand(text)
    , m_scene(scene)
    , m_items(items)
{
}

AddObjectsCommand::~AddObjectsCommand()
{
    // While the command sits on the undone side of the stack the objects
    // belong to it, and go when it does.
    if (!m_inScene)
        qDeleteAll(m_items);
}

void AddObjectsCommand::redo()
{
    if (m_inScene)
        return;   // pushing the command must not add them a second time
    for (QGraphicsItem *item : m_items)
        m_scene->insertObject(item);
    m_inScene = true;
}

void AddObjectsCommand::undo()
{
    for (QGraphicsItem *item : m_items)
        m_scene->takeObject(item);
    m_inScene = false;
}

// ----------------------------------------------------------------- Removing

RemoveObjectsCommand::RemoveObjectsCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                                           const QString &text)
    : QUndoCommand(text)
    , m_scene(scene)
    , m_items(items)
{
}

RemoveObjectsCommand::~RemoveObjectsCommand()
{
    if (!m_inScene)
        qDeleteAll(m_items);
}

void RemoveObjectsCommand::redo()
{
    for (QGraphicsItem *item : m_items)
        m_scene->takeObject(item);
    m_inScene = false;
}

void RemoveObjectsCommand::undo()
{
    for (QGraphicsItem *item : m_items)
        m_scene->insertObject(item);
    m_inScene = true;
}

// --------------------------------------------------------------- Placement

TransformCommand::TransformCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                                   const QList<Placement> &before, const QList<Placement> &after,
                                   const QString &text)
    : QUndoCommand(text)
    , m_scene(scene)
    , m_items(items)
    , m_before(before)
    , m_after(after)
{
}

void TransformCommand::apply(const QList<Placement> &placements)
{
    for (int i = 0; i < m_items.size() && i < placements.size(); ++i)
        applyPlacement(m_items.at(i), placements.at(i));
    m_scene->objectsWereChanged();
}

void TransformCommand::redo() { apply(m_after); }
void TransformCommand::undo() { apply(m_before); }

// ------------------------------------------------------------------ Colour

ColorCommand::ColorCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                           const QList<QColor> &before, const QColor &after)
    : QUndoCommand(QCoreApplication::translate("Commands", "Change colour"))
    , m_scene(scene)
    , m_items(items)
    , m_before(before)
    , m_after(after)
{
}

void ColorCommand::redo()
{
    for (QGraphicsItem *item : m_items) {
        if (auto *object = dynamic_cast<InkObject *>(item))
            object->setColor(m_after);
    }
    m_scene->objectsWereChanged();
}

void ColorCommand::undo()
{
    for (int i = 0; i < m_items.size() && i < m_before.size(); ++i) {
        if (auto *object = dynamic_cast<InkObject *>(m_items.at(i)))
            object->setColor(m_before.at(i));
    }
    m_scene->objectsWereChanged();
}

// ------------------------------------------------------------------ Erasing

EraseCommand::EraseCommand(CanvasScene *scene, const QList<QGraphicsItem *> &items,
                           const QList<QPainterPath> &before, const QList<QPainterPath> &after)
    : QUndoCommand(QCoreApplication::translate("Commands", "Erase"))
    , m_scene(scene)
    , m_items(items)
    , m_before(before)
    , m_after(after)
{
}

void EraseCommand::apply(const QList<QPainterPath> &masks)
{
    for (int i = 0; i < m_items.size() && i < masks.size(); ++i) {
        if (auto *object = dynamic_cast<InkObject *>(m_items.at(i)))
            object->setErasedMask(masks.at(i));
    }
    m_scene->objectsWereChanged();
}

void EraseCommand::redo() { apply(m_after); }
void EraseCommand::undo() { apply(m_before); }

// -------------------------------------------------------------- Canvas size

PageSizeCommand::PageSizeCommand(CanvasScene *scene, const QSize &before, const QSize &after)
    : QUndoCommand(QCoreApplication::translate("Commands", "Resize canvas"))
    , m_scene(scene)
    , m_before(before)
    , m_after(after)
{
}

void PageSizeCommand::redo() { m_scene->applyPageSize(m_after); }
void PageSizeCommand::undo() { m_scene->applyPageSize(m_before); }

// --------------------------------------------------------------------- Text

TextCommand::TextCommand(CanvasScene *scene, TextItem *item,
                         const QString &before, const QString &after)
    : QUndoCommand(QCoreApplication::translate("Commands", "Edit caption"))
    , m_scene(scene)
    , m_item(item)
    , m_before(before)
    , m_after(after)
{
}

void TextCommand::redo()
{
    m_item->setPlainText(m_after);
    m_item->applyPlacement();
    m_scene->objectsWereChanged();
}

void TextCommand::undo()
{
    m_item->setPlainText(m_before);
    m_item->applyPlacement();
    m_scene->objectsWereChanged();
}

RenameCommand::RenameCommand(CanvasScene *scene, QGraphicsItem *item,
                             const QString &before, const QString &after)
    : QUndoCommand(QCoreApplication::translate("Commands", "Rename object"))
    , m_scene(scene)
    , m_item(item)
    , m_before(before)
    , m_after(after)
{
}

void RenameCommand::undo()
{
    if (auto *object = dynamic_cast<InkObject *>(m_item))
        object->setName(m_before);
    m_scene->objectsWereChanged();
}

void RenameCommand::redo()
{
    if (auto *object = dynamic_cast<InkObject *>(m_item))
        object->setName(m_after);
    m_scene->objectsWereChanged();
}

StackCommand::StackCommand(CanvasScene *scene, const QString &text,
                           const QList<QGraphicsItem *> &items,
                           const QList<qreal> &before, const QList<qreal> &after)
    : QUndoCommand(text)
    , m_scene(scene)
    , m_items(items)
    , m_before(before)
    , m_after(after)
{
}

void StackCommand::apply(const QList<qreal> &values)
{
    for (int i = 0; i < m_items.size() && i < values.size(); ++i)
        m_items.at(i)->setZValue(values.at(i));
    m_scene->objectsWereChanged();
}

void StackCommand::undo()
{
    apply(m_before);
}

void StackCommand::redo()
{
    apply(m_after);
}
