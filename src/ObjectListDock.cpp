#include "ObjectListDock.h"

#include <algorithm>

#include <QAction>
#include <QGraphicsItem>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QPalette>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "CanvasScene.h"
#include "FillItem.h"
#include "Icons.h"
#include "ImageItem.h"
#include "InkObject.h"
#include "ShapeItem.h"
#include "StrokeItem.h"
#include "TextItem.h"

namespace {

constexpr int kItemRole = Qt::UserRole + 1;

// A given name wins; otherwise a caption is easier to recognise by what it
// says than by its type.
QString rowLabel(QGraphicsItem *item, InkObject *object)
{
    if (!object->name().isEmpty())
        return object->name();

    if (auto *text = dynamic_cast<TextItem *>(item)) {
        QString content = text->toPlainText().simplified();
        if (!content.isEmpty()) {
            if (content.size() > 28)
                content = content.left(27) + QStringLiteral("…");
            return content;
        }
    }
    return object->displayName();
}

}   // namespace

QIcon ObjectListDock::iconFor(QGraphicsItem *item, InkObject *object, const QColor &outline)
{
    const QColor colour = object->color().isValid() ? object->color() : outline;
    QString key = object->typeName() + QLatin1Char('/') + colour.name(QColor::HexArgb);
    if (auto *figure = dynamic_cast<ShapeItem *>(item)) {
        key += QStringLiteral("/%1/%2").arg(int(figure->kind())).arg(int(figure->isFilled()));
    }

    const auto cached = m_iconCache.constFind(key);
    if (cached != m_iconCache.constEnd())
        return cached.value();

    QIcon icon;
    if (dynamic_cast<StrokeItem *>(item))
        icon = Icons::brush(colour, outline);
    else if (dynamic_cast<FillItem *>(item))
        icon = Icons::bucket(colour, outline);
    else if (dynamic_cast<TextItem *>(item))
        icon = Icons::text(colour);
    else if (auto *figure = dynamic_cast<ShapeItem *>(item))
        icon = Icons::fromPath(ShapeItem::pathFor(figure->kind(), QRectF(10, 10, 44, 44)),
                               colour, outline, figure->isFilled());
    else if (dynamic_cast<ImageItem *>(item))
        icon = Icons::picture(outline);

    m_iconCache.insert(key, icon);
    return icon;
}

ObjectListDock::ObjectListDock(CanvasScene *scene, QWidget *parent)
    : QDockWidget(tr("Objects"), parent)
    , m_scene(scene)
    , m_list(new QListWidget(this))
{
    setObjectName(QStringLiteral("objectList"));   // needed to store the layout
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_list->setIconSize(QSize(20, 20));
    m_list->setUniformItemSizes(true);
    m_list->setEditTriggers(QAbstractItemView::DoubleClicked
                            | QAbstractItemView::SelectedClicked);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setDragDropMode(QAbstractItemView::InternalMove);
    m_list->setDefaultDropAction(Qt::MoveAction);

    const QColor symbol = palette().color(QPalette::WindowText);

    // A tooltip that does not name the key leaves the key undiscovered.
    const auto withKey = [](const QString &text, const QAction *action) {
        return QStringLiteral("%1  (%2)")
            .arg(text, action->shortcut().toString(QKeySequence::NativeText));
    };

    m_renameAction = new QAction(tr("&Rename Object"), this);
    m_renameAction->setShortcut(QKeySequence(Qt::Key_F2));
    m_renameAction->setToolTip(withKey(tr("Give the selected object a name"), m_renameAction));
    connect(m_renameAction, &QAction::triggered, this, &ObjectListDock::startRenaming);

    m_forwardAction = new QAction(Icons::stackForward(symbol), tr("Bring &Forward"), this);
    m_forwardAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageUp));
    m_forwardAction->setToolTip(
        withKey(tr("Bring the selected objects one place forward"), m_forwardAction));
    connect(m_forwardAction, &QAction::triggered, this, [this] { restack(true); });

    m_backwardAction = new QAction(Icons::stackBackward(symbol), tr("Send &Backward"), this);
    m_backwardAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    m_backwardAction->setToolTip(
        withKey(tr("Send the selected objects one place back"), m_backwardAction));
    connect(m_backwardAction, &QAction::triggered, this, [this] { restack(false); });

    m_forwardButton = new QToolButton(this);
    m_forwardButton->setDefaultAction(m_forwardAction);
    m_forwardButton->setAutoRaise(true);

    m_backwardButton = new QToolButton(this);
    m_backwardButton->setDefaultAction(m_backwardAction);
    m_backwardButton->setAutoRaise(true);

    auto *buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->addWidget(m_forwardButton);
    buttons->addWidget(m_backwardButton);
    buttons->addStretch();

    auto *body = new QWidget(this);
    auto *column = new QVBoxLayout(body);
    column->setContentsMargins(2, 2, 2, 2);
    column->setSpacing(2);
    column->addWidget(m_list);
    column->addLayout(buttons);
    setWidget(body);

    // A button that does nothing when pressed is worse than one that is
    // visibly out of reach.
    const auto updateButtons = [this] {
        const bool any = !m_scene->selectedItems().isEmpty();
        m_forwardAction->setEnabled(any);
        m_backwardAction->setEnabled(any);
        m_renameAction->setEnabled(m_scene->singleSelectedObject() != nullptr);
    };
    updateButtons();
    connect(m_scene, &QGraphicsScene::selectionChanged, this, updateButtons);

    connect(m_list, &QListWidget::itemChanged, this, &ObjectListDock::renameRow);

    // Dragging a row is carried out as a removal and an insertion, so the
    // order is read once both are done rather than on each half.
    const auto orderMayHaveChanged = [this] {
        if (m_syncing)
            return;
        QTimer::singleShot(0, this, &ObjectListDock::applyListOrder);
    };
    connect(m_list->model(), &QAbstractItemModel::rowsMoved, this, orderMayHaveChanged);
    connect(m_list->model(), &QAbstractItemModel::rowsRemoved, this, orderMayHaveChanged);
    connect(m_list->model(), &QAbstractItemModel::rowsInserted, this, orderMayHaveChanged);
    connect(m_list, &QListWidget::customContextMenuRequested,
            this, &ObjectListDock::showMenu);

    // Only a changed set of objects rebuilds the list. Turning and scaling
    // report through objectPropertiesChanged on every mouse move, and
    // rebuilding there would redraw every row while the mouse is down.
    connect(m_scene, &CanvasScene::objectsChanged, this, &ObjectListDock::rebuild);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &ObjectListDock::showSceneSelection);
    connect(m_list, &QListWidget::itemSelectionChanged, this, &ObjectListDock::applyListSelection);

    rebuild();
}

void ObjectListDock::rebuild()
{
    if (m_syncing)
        return;

    m_syncing = true;

    // Topmost object first, the way such a list is usually read.
    QList<QGraphicsItem *> items = m_scene->objects();
    std::sort(items.begin(), items.end(), [](QGraphicsItem *a, QGraphicsItem *b) {
        return a->zValue() > b->zValue();
    });

    const QColor outline = palette().color(QPalette::WindowText);

    m_list->clear();
    for (QGraphicsItem *item : items) {
        auto *object = dynamic_cast<InkObject *>(item);
        if (!object)
            continue;

        auto *row = new QListWidgetItem(iconFor(item, object, outline), rowLabel(item, object));
        row->setFlags(row->flags() | Qt::ItemIsEditable);
        row->setData(kItemRole, QVariant::fromValue(reinterpret_cast<quintptr>(item)));
        m_list->addItem(row);
        row->setSelected(item->isSelected());
    }

    m_syncing = false;
}

void ObjectListDock::showSceneSelection()
{
    if (m_syncing)
        return;

    m_syncing = true;
    for (int row = 0; row < m_list->count(); ++row) {
        QListWidgetItem *entry = m_list->item(row);
        auto *item = reinterpret_cast<QGraphicsItem *>(entry->data(kItemRole).value<quintptr>());
        entry->setSelected(item && item->isSelected());
    }
    m_syncing = false;
}

void ObjectListDock::applyListSelection()
{
    if (m_syncing)
        return;

    m_syncing = true;
    for (int row = 0; row < m_list->count(); ++row) {
        QListWidgetItem *entry = m_list->item(row);
        auto *item = reinterpret_cast<QGraphicsItem *>(entry->data(kItemRole).value<quintptr>());
        if (item)
            item->setSelected(entry->isSelected());
    }
    m_syncing = false;
}

QGraphicsItem *ObjectListDock::itemOf(const QListWidgetItem *row) const
{
    if (!row)
        return nullptr;
    return reinterpret_cast<QGraphicsItem *>(row->data(kItemRole).value<quintptr>());
}

void ObjectListDock::renameRow(QListWidgetItem *row)
{
    if (m_syncing)
        return;

    QGraphicsItem *item = itemOf(row);
    if (!item)
        return;

    // This also fires while a dragged row is being filled in, field by
    // field, and renaming from here would rebuild the list under Qt's feet
    // while it is still writing to the row. So the object and the typed text
    // are taken along and the work is done once that is over.
    const QString typed = row->text();
    QTimer::singleShot(0, this, [this, item, typed] {
        if (!m_scene->objects().contains(item))
            return;   // the object went away in the meantime

        // An empty name means the object goes back to being called what it
        // is. A rename of its own rebuilds the list through the scene; only
        // when nothing changed does the row have to be put right here.
        m_scene->renameObject(item, typed);
        if (auto *object = dynamic_cast<InkObject *>(item)) {
            if (object->label() != typed)
                rebuild();
        }
    });
}

void ObjectListDock::startRenaming()
{
    // The key works wherever the window has focus, so the row is looked up
    // from what is selected on the canvas rather than from the list.
    QListWidgetItem *row = nullptr;
    if (QGraphicsItem *only = m_scene->singleSelectedObject()) {
        for (int i = 0; i < m_list->count() && !row; ++i) {
            if (itemOf(m_list->item(i)) == only)
                row = m_list->item(i);
        }
    }
    if (!row)
        row = m_list->currentItem();
    if (!row)
        return;

    if (isHidden())
        show();   // renaming in a list nobody can see would be a riddle

    m_list->setCurrentItem(row);
    m_list->editItem(row);
}

void ObjectListDock::restack(bool forward)
{
    m_scene->restackSelection(forward);
}

void ObjectListDock::showMenu(const QPoint &where)
{
    QListWidgetItem *row = m_list->itemAt(where);

    if (row)
        m_list->setCurrentItem(row);

    QMenu menu(this);
    menu.addAction(m_renameAction);
    menu.addSeparator();
    menu.addAction(m_forwardAction);
    menu.addAction(m_backwardAction);
    menu.exec(m_list->viewport()->mapToGlobal(where));
}

void ObjectListDock::applyListOrder()
{
    if (m_syncing)
        return;

    QList<QGraphicsItem *> topFirst;
    topFirst.reserve(m_list->count());
    for (int row = 0; row < m_list->count(); ++row) {
        if (QGraphicsItem *item = itemOf(m_list->item(row)))
            topFirst << item;
    }

    // A half finished move would name fewer objects than there are. Leaving
    // it alone is right: the next signal arrives with the list complete.
    if (topFirst.size() != m_scene->objectCount())
        return;

    if (!m_scene->setStackOrder(topFirst))
        rebuild();   // nothing changed, so put the rows back where they belong
}
