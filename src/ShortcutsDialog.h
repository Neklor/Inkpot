#pragma once

#include <QDialog>

class QMenuBar;
class QToolBar;
class QVBoxLayout;

// Lists the keyboard shortcuts.
//
// The entries are gathered from the menus and the toolbar at the moment the
// dialog opens, so the list cannot fall out of step with the program. Only
// what the mouse does has to be written down by hand.
class ShortcutsDialog : public QDialog
{
    Q_OBJECT

public:
    ShortcutsDialog(QMenuBar *menuBar, QToolBar *toolBar, QWidget *parent = nullptr);
};
