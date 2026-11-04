#pragma once

#include <QIcon>
#include <QList>
#include <QMainWindow>
#include <QSize>
#include <QString>

class CanvasScene;
class CanvasView;

class QAction;
class QActionGroup;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;
class QToolBar;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Opens a project or an image, as passed on the command line or handed
    // over by the desktop environment.
    void openPath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // File menu
    void newCanvas(const QSize &size);
    void newCustomCanvas();
    void open();
    bool save();
    bool saveAs();
    void exportImage();
    void copyToClipboard();
    void pasteFromClipboard();
    void print();

    // Edit menu
    void deleteSelection();

    // Help menu
    void showShortcuts();
    void showAbout();

    // Construction
    void createActions();
    void createObjectList();
    void restoreLayout();
    void storeLayout();
    void createToolBar();
    void createStatusBar();

    // State
    bool maybeSave();
    bool loadFile(const QString &path);
    bool loadImageFile(const QString &path);
    bool saveFile(const QString &path);
    void setCurrentFile(const QString &path);
    void setDirty(bool dirty);
    void updateWindowTitle();
    void updateSizeControls();
    void updateObjectInfo();
    void updateZoomInfo(double factor);
    void createShapeTool(QToolBar *toolBar, const QColor &symbolColor);
    void updateShapeAction();
    void chooseToolColor();
    void updateColorButton();
    void updateObjectControls();
    void applyFontSizeToSelection(double pointSize);
    void createToolAction(QToolBar *toolBar, const QString &name, const QIcon &icon,
                          const QString &shortcut, const QString &hint, int tool);

    static QString aspectRatioText(const QSize &size);

    CanvasScene *m_scene = nullptr;
    CanvasView *m_view = nullptr;

    QSpinBox *m_widthSpin = nullptr;
    QSpinBox *m_heightSpin = nullptr;
    QToolButton *m_lockButton = nullptr;
    QLabel *m_ratioLabel = nullptr;
    QLabel *m_objectLabel = nullptr;
    QLabel *m_zoomLabel = nullptr;

    QAction *m_saveAction = nullptr;
    QAction *m_deleteAction = nullptr;
    QAction *m_pasteAction = nullptr;
    QToolButton *m_colorButton = nullptr;
    QDoubleSpinBox *m_toolSizeSpin = nullptr;
    QDoubleSpinBox *m_fontSizeSpin = nullptr;
    QAction *m_brushAction = nullptr;
    QAction *m_fillAction = nullptr;
    QAction *m_shapeAction = nullptr;
    QAction *m_shapeFilledAction = nullptr;
    QList<QAction *> m_shapeKindActions;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QActionGroup *m_toolGroup = nullptr;
    QToolBar *m_toolBar = nullptr;
    class ObjectListDock *m_objectList = nullptr;

    QString m_currentFile;
    bool m_dirty = false;
    bool m_updatingControls = false;
};
