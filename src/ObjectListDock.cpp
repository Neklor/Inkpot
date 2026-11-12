#include "ObjectListDock.h"

#include <algorithm>

#include <QAction>
#include <QGraphicsItem>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QPalette>
#include <QShortcut>
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

    const QColor symbol = palette().color(QPalette::WindowText);

    m_forwardButton = new QToolButton(this);
    m_forwardButton->setIcon(Icons::stackForward(symbol));
    m_forwardButton->setToolTip(tr("Bring the selected objects one place forward"));
    m_forwardButton->setAutoRaise(true);

    m_backwardButton = new QToolButton(this);
    m_backwardButton->setIcon(Icons::stackBackward(symbol));
    m_backwardButton->setToolTip(tr("Send the selected objects one place back"));
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
        m_forwardButton->setEnabled(any);
        m_backwardButton->setEnabled(any);
    };
    updateButtons();
    connect(m_scene, &QGraphicsScene::selectionChanged, this, updateButtons);

    connect(m_forwardButton, &QToolButton::clicked, this, [this] { restack(true); });
    connect(m_backwardButton, &QToolButton::clicked, this, [this] { restack(false); });
    connect(m_list, &QListWidget::itemChanged, this, &ObjectListDock::renameRow);
    connect(m_list, &QListWidget::customContextMenuRequested,
            this, &ObjectListDock::showMenu);

    // F2 is the usual key for this and is not taken anywhere else.
    auto *rename = new QShortcut(QKeySequence(Qt::Key_F2), m_list);
    rename->setContext(Qt::WidgetShortcut);
    connect(rename, &QShortcut::activated, this, &ObjectListDock::startRenaming);

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

    // An empty name means the object goes back to being called what it is.
    m_scene->renameObject(item, row->text());

    // Whatever the object ends up being called is what the row has to show,
    // which is not the typed text when the name was cleared.
    auto *object = dynamic_cast<InkObject *>(item);
    if (object && row->text() != rowLabel(item, object)) {
        m_syncing = true;
        row->setText(rowLabel(item, object));
        m_syncing = false;
    }
}

void ObjectListDock::startRenaming()
{
    if (QListWidgetItem *row = m_list->currentItem())
        m_list->editItem(row);
}

void ObjectListDock::restack(bool forward)
{
    m_scene->restackSelection(forward);
}

void ObjectListDock::showMenu(const QPoint &where)
{
    QListWidgetItem *row = m_list->itemAt(where);

    QMenu menu(this);
    QAction *rename = menu.addAction(tr("Rename"));
    rename->setShortcut(QKeySequence(Qt::Key_F2));
    rename->setEnabled(row != nullptr);
    menu.addSeparator();
    QAction *forward = menu.addAction(tr("Bring forward"));
    QAction *backward = menu.addAction(tr("Send backward"));

    const bool anySelected = !m_list->selectedItems().isEmpty();
    forward->setEnabled(anySelected);
    backward->setEnabled(anySelected);

    QAction *chosen = menu.exec(m_list->viewport()->mapToGlobal(where));
    if (chosen == rename && row) {
        m_list->setCurrentItem(row);
        m_list->editItem(row);
    } else if (chosen == forward) {
        restack(true);
    } else if (chosen == backward) {
        restack(false);
    }
}
