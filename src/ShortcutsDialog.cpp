#include "ShortcutsDialog.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QScrollArea>
#include <QToolBar>
#include <QVBoxLayout>

namespace {

struct Entry {
    QString action;
    QString keys;
};

// Menu titles and action names carry the markers for their access keys.
//
// Undo and redo say what they would undo, so their text changes with every
// step and would read "Undo Reorder objects" here. Those two carry a plain
// name of their own in the icon text, which is what this list wants.
QString plainTitle(const QAction *action)
{
    QString text = action->property("shortcutTitle").toString();
    if (text.isEmpty())
        text = action->text();
    text.remove(QLatin1Char('&'));
    return text;
}

void collect(const QMenu *menu, QList<Entry> &entries)
{
    const QList<QAction *> actions = menu->actions();
    for (const QAction *action : actions) {
        if (action->isSeparator())
            continue;
        if (action->menu()) {
            collect(action->menu(), entries);
            continue;
        }
        if (action->shortcut().isEmpty())
            continue;
        entries.append({ plainTitle(action),
                         action->shortcut().toString(QKeySequence::NativeText) });
    }
}

// framed marks real key combinations, which get a keycap look. The mouse
// entries are whole sentences and are set as plain, wrapping text instead.
void addSection(QVBoxLayout *layout, const QString &title, const QList<Entry> &entries,
                bool framed)
{
    if (entries.isEmpty())
        return;

    auto *heading = new QLabel(title);
    QFont headingFont = heading->font();
    headingFont.setBold(true);
    heading->setFont(headingFont);
    layout->addSpacing(8);
    layout->addWidget(heading);

    auto *grid = new QGridLayout;
    grid->setColumnStretch(0, 3);
    grid->setColumnStretch(1, 2);
    grid->setHorizontalSpacing(16);

    int row = 0;
    for (const Entry &entry : entries) {
        grid->addWidget(new QLabel(entry.action), row, 0);

        auto *keys = new QLabel(entry.keys);
        keys->setWordWrap(true);
        keys->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (framed) {
            keys->setFrameShape(QFrame::StyledPanel);
            keys->setAlignment(Qt::AlignCenter);
            keys->setMargin(3);
        } else {
            // Wrapped text set flush right leaves a ragged left edge, which
            // is what makes a two line entry hard to read.
            keys->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
        grid->addWidget(keys, row, 1);
        ++row;
    }
    layout->addLayout(grid);
}

}   // namespace

ShortcutsDialog::ShortcutsDialog(QMenuBar *menuBar, QToolBar *toolBar, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Keyboard Shortcuts"));

    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(4, 4, 4, 4);

    if (menuBar) {
        const QList<QAction *> menus = menuBar->actions();
        for (const QAction *action : menus) {
            if (!action->menu())
                continue;
            QList<Entry> entries;
            collect(action->menu(), entries);
            addSection(layout, plainTitle(action), entries, true);
        }
    }

    if (toolBar) {
        QList<Entry> tools;
        const QList<QAction *> actions = toolBar->actions();
        for (const QAction *action : actions) {
            if (action->isSeparator() || action->shortcut().isEmpty())
                continue;
            tools.append({ plainTitle(action),
                           action->shortcut().toString(QKeySequence::NativeText) });
        }
        addSection(layout, tr("Tools"), tools, true);
    }

    addSection(layout, tr("Mouse"), {
        { tr("Zoom"),                   tr("Ctrl + mouse wheel") },
        { tr("Pan the view"),           tr("Drag with the middle mouse button") },
        { tr("Edit a caption"),         tr("Double click with the selection tool") },
        { tr("Select several objects"), tr("Drag a rubber band with the selection tool") },
    }, false);

    layout->addStretch();

    auto *scroll = new QScrollArea(this);
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(scroll);
    outer->addWidget(buttons);

    resize(520, 580);
}
