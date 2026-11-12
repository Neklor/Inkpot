#pragma once

#include <QDockWidget>
#include <QHash>
#include <QIcon>

class CanvasScene;
class QGraphicsItem;
class QColor;
class QListWidget;
class QListWidgetItem;
class QToolButton;

// The list of objects on the canvas, topmost first.
//
// Selection runs both ways: picking a row selects the object, and selecting on
// the canvas highlights the row. Several rows can be picked with Ctrl, and a
// block of them with Shift, which the list widget handles on its own.
//
// A row can be renamed in place, by double click, by F2 or from the context
// menu. The two buttons below move the selection through the stack, and a
// row can also be dragged to its place.
class ObjectListDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit ObjectListDock(CanvasScene *scene, QWidget *parent = nullptr);

private:
    void rebuild();
    void showSceneSelection();
    void applyListSelection();
    void renameRow(QListWidgetItem *row);
    void startRenaming();
    void showMenu(const QPoint &where);
    void restack(bool forward);
    void applyListOrder();

    QGraphicsItem *itemOf(const QListWidgetItem *row) const;

    QIcon iconFor(QGraphicsItem *item, class InkObject *object, const QColor &outline);

    CanvasScene *m_scene;
    QListWidget *m_list;
    QToolButton *m_forwardButton;
    QToolButton *m_backwardButton;

    // Drawing an icon means painting a pixmap, so each kind and colour is
    // drawn once and then reused.
    QHash<QString, QIcon> m_iconCache;
    bool m_syncing = false;   // keeps the two selections from chasing each other
};
