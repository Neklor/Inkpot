#include "ObjectListDock.h"

#include <algorithm>

#include <QGraphicsItem>
#include <QListWidget>
#include <QPalette>
#include <QVariant>

#include "CanvasScene.h"
#include "FillItem.h"
#include "Icons.h"
#include "ImageItem.h"
#include "InkObject.h"
#include "StrokeItem.h"
#include "TextItem.h"

namespace {

constexpr int kItemRole = Qt::UserRole + 1;

// A caption is easier to recognise by what it says than by its type.
QString rowLabel(QGraphicsItem *item, InkObject *object)
{
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
    const QString key = object->typeName() + QLatin1Char('/') + colour.name(QColor::HexArgb);

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
    setWidget(m_list);

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
