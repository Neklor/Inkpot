#include "MainWindow.h"

#include <numeric>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QSaveFile>
#include <QSpinBox>
#include <QSettings>
#include <QUndoStack>
#include <QStatusBar>
#include <QToolBar>
#include <QActionGroup>
#include <QProxyStyle>
#include <QToolButton>

#include "CanvasScene.h"
#include "CanvasView.h"
#include "Icons.h"
#include "ImageItem.h"
#include "NewCanvasDialog.h"
#include "ObjectListDock.h"
#include "HeifImage.h"
#include "ShapeItem.h"
#include "ShortcutsDialog.h"
#include "TextItem.h"

namespace {

// The shape tool is the only button in the window with a dropdown, and it is
// the one place that has to be aimed at: the arrow beside the button is a few
// pixels wide and the rows of the menu are the size of a text line. That is
// fine with a mouse and awkward with a finger. This gives that button and
// that menu larger pictures and a wider arrow, and leaves the rest of the
// window as the desktop theme draws it.
class RoomyStyle : public QProxyStyle
{
public:
    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                    const QWidget *widget) const override
    {
        switch (metric) {
        case PM_SmallIconSize:
            return 26;
        case PM_MenuButtonIndicator:
            return 22;
        default:
            break;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};

const char *kProjectSuffix = "ipot";

// Projects from before the move to *.ipot.
const char *kLegacySuffix = "inkpot";

// Does the file carry one of our project extensions?
bool isProjectFile(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix();
    return suffix.compare(QLatin1String(kProjectSuffix), Qt::CaseInsensitive) == 0
           || suffix.compare(QLatin1String(kLegacySuffix), Qt::CaseInsensitive) == 0;
}

// Preset sizes for "File > New".
struct Preset {
    const char *label;
    int width;
    int height;
};

const Preset kPresets[] = {
    { "800 × 600 (4:3)",            800,  600 },
    { "1024 × 768 (4:3)",          1024,  768 },
    { "1280 × 720 (HD, 16:9)",     1280,  720 },
    { "1920 × 1080 (Full HD)",     1920, 1080 },
    { "1080 × 1080 (Quadrat)",     1080, 1080 },
    { "2480 × 3508 (A4, 300 dpi)", 2480, 3508 },
};

}   // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_scene = new CanvasScene(this);
    m_view = new CanvasView(m_scene, this);
    setCentralWidget(m_view);

    createObjectList();
    createActions();
    createToolBar();
    createStatusBar();

    connect(m_scene, &CanvasScene::pageRectChanged, this, &MainWindow::updateSizeControls);
    connect(m_scene, &CanvasScene::objectsChanged, this, &MainWindow::updateObjectInfo);
    connect(m_scene, &CanvasScene::edited, this, [this] { setDirty(true); });
    connect(m_scene, &CanvasScene::selectionChanged, this, &MainWindow::updateObjectInfo);
    connect(m_scene, &CanvasScene::objectPropertiesChanged, this, &MainWindow::updateObjectControls);
    connect(m_view, &CanvasView::zoomChanged, this, &MainWindow::updateZoomInfo);
    connect(m_view, &CanvasView::fillRejected, this, [this] {
        statusBar()->showMessage(tr("There is nothing to fill at this spot"), 3000);
    });

    resize(1200, 820);
    restoreLayout();
    updateSizeControls();
    updateObjectInfo();
    updateZoomInfo(m_view->zoomFactor());
    setCurrentFile(QString());
    setDirty(false);
}

MainWindow::~MainWindow()
{
    // The scene and the view are children of this window and are torn down
    // only after this body has run. While that happens the scene deletes its
    // objects and keeps emitting signals - which must no longer reach us.
    QObject::disconnect(m_scene, nullptr, this, nullptr);
    QObject::disconnect(m_view, nullptr, this, nullptr);
}

void MainWindow::createObjectList()
{
    m_objectList = new ObjectListDock(m_scene, this);
    addDockWidget(Qt::RightDockWidgetArea, m_objectList);
}

// Window size and which docks are shown are remembered between runs.
void MainWindow::restoreLayout()
{
    QSettings settings;
    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    const QByteArray state = settings.value(QStringLiteral("window/state")).toByteArray();

    if (!geometry.isEmpty())
        restoreGeometry(geometry);
    if (!state.isEmpty())
        restoreState(state);
}

void MainWindow::storeLayout()
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QMenu *newMenu = fileMenu->addMenu(tr("&New"));
    for (const Preset &preset : kPresets) {
        const QSize size(preset.width, preset.height);
        QAction *action = newMenu->addAction(QString::fromUtf8(preset.label));
        connect(action, &QAction::triggered, this, [this, size] { newCanvas(size); });
    }
    newMenu->addSeparator();
    QAction *customAction = newMenu->addAction(tr("&Custom Size..."));
    customAction->setShortcut(QKeySequence::New);
    connect(customAction, &QAction::triggered, this, &MainWindow::newCustomCanvas);

    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::open);

    m_saveAction = fileMenu->addAction(tr("&Save"));
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::save);

    QAction *saveAsAction = fileMenu->addAction(tr("Save &As..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAs);

    QAction *exportAction = fileMenu->addAction(tr("&Export..."));
    exportAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    exportAction->setStatusTip(tr("Save the canvas as an image file"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportImage);

    fileMenu->addSeparator();

    QAction *copyAction = fileMenu->addAction(tr("&Copy to Clipboard"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copyToClipboard);

    m_pasteAction = fileMenu->addAction(tr("&Paste"));
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, this, &MainWindow::pasteFromClipboard);

    fileMenu->addSeparator();

    QAction *printAction = fileMenu->addAction(tr("&Print..."));
    printAction->setShortcut(QKeySequence::Print);
    connect(printAction, &QAction::triggered, this, &MainWindow::print);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    const QColor symbolColour = palette().color(QPalette::ButtonText);

    m_undoAction = m_scene->undoStack()->createUndoAction(this, tr("&Undo"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setIcon(Icons::undo(symbolColour));
    editMenu->addAction(m_undoAction);

    m_redoAction = m_scene->undoStack()->createRedoAction(this, tr("&Redo"));
    m_redoAction->setShortcuts({ QKeySequence::Redo, QKeySequence(QStringLiteral("Ctrl+Y")) });
    m_redoAction->setIcon(Icons::redo(symbolColour));
    editMenu->addAction(m_redoAction);

    editMenu->addSeparator();

    m_deleteAction = editMenu->addAction(tr("&Delete Selection"));
    m_deleteAction->setShortcut(QKeySequence::Delete);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteSelection);

    QAction *selectAllAction = editMenu->addAction(tr("Select &All"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, [this] { m_scene->selectAllObjects(); });

    QAction *deselectAction = editMenu->addAction(tr("Deselect A&ll"));
    deselectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+A")));
    connect(deselectAction, &QAction::triggered, this, [this] { m_scene->clearSelection(); });

    editMenu->addSeparator();

    // Naming an object and deciding which one covers which belong to the
    // object list, which owns these three. The menu shows the same actions,
    // which is also how they reach the shortcut help.
    editMenu->addAction(m_objectList->renameAction());
    editMenu->addSeparator();
    editMenu->addAction(m_objectList->forwardAction());
    editMenu->addAction(m_objectList->backwardAction());

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    QAction *zoomInAction = viewMenu->addAction(tr("Zoom &In"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, m_view, &CanvasView::zoomIn);

    QAction *zoomOutAction = viewMenu->addAction(tr("Zoom &Out"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, m_view, &CanvasView::zoomOut);

    QAction *resetZoomAction = viewMenu->addAction(tr("&Actual Size"));
    resetZoomAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(resetZoomAction, &QAction::triggered, m_view, &CanvasView::resetZoom);

    QAction *fitAction = viewMenu->addAction(tr("&Fit in Window"));
    fitAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+9")));
    connect(fitAction, &QAction::triggered, m_view, &CanvasView::fitPageInView);

    viewMenu->addSeparator();

    // The dock supplies its own show and hide entry, which also carries the
    // right checked state.
    QAction *listAction = m_objectList->toggleViewAction();
    listAction->setText(tr("&Object List"));
    viewMenu->addAction(listAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *shortcutsAction = helpMenu->addAction(tr("&Keyboard Shortcuts"));
    connect(shortcutsAction, &QAction::triggered, this, &MainWindow::showShortcuts);

    helpMenu->addSeparator();

    QAction *aboutAction = helpMenu->addAction(tr("&About Inkpot"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    QAction *aboutQtAction = helpMenu->addAction(tr("About &Qt"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::showShortcuts()
{
    ShortcutsDialog dialog(menuBar(), m_toolBar, this);
    dialog.exec();
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this, tr("About Inkpot"),
        tr("<h3>Inkpot %1</h3>"
           "<p>A small paint program in which everything you draw stays an "
           "object of its own.</p>"
           "<p>Copyright \u00a9 %2 %3</p>"
           "<p>Licensed under the GNU General Public License, version 3.</p>"
           "<p><a href=\"%4\">%4</a></p>")
            .arg(QStringLiteral(INKPOT_VERSION),
                 QStringLiteral(INKPOT_COPYRIGHT_YEARS),
                 QStringLiteral(INKPOT_AUTHOR),
                 QStringLiteral(INKPOT_HOMEPAGE)));
}

void MainWindow::createToolAction(QToolBar *toolBar, const QString &name, const QIcon &icon,
                                  const QString &shortcut, const QString &hint, int tool)
{
    QAction *action = toolBar->addAction(icon, name);
    action->setCheckable(true);
    action->setShortcut(QKeySequence(shortcut));
    action->setToolTip(QStringLiteral("%1 (%2)\n%3").arg(name, shortcut, hint));
    action->setStatusTip(hint);
    connect(action, &QAction::triggered, this, [this, tool] {
        m_view->setTool(static_cast<CanvasView::Tool>(tool));
    });
    m_toolGroup->addAction(action);
}

// The shape tool is one button with a menu: clicking it draws the shape that
// is currently chosen, the arrow beside it picks another.
void MainWindow::createShapeTool(QToolBar *toolBar, const QColor &symbolColor)
{
    m_shapeAction = toolBar->addAction(tr("Shape"));
    m_shapeAction->setCheckable(true);
    m_shapeAction->setShortcut(QKeySequence(QStringLiteral("R")));
    m_shapeAction->setStatusTip(tr("Draw a shape - hold Shift to keep it even"));
    connect(m_shapeAction, &QAction::triggered, this,
            [this] { m_view->setTool(CanvasView::Tool::Shape); });
    m_toolGroup->addAction(m_shapeAction);

    auto *menu = new QMenu(this);
    auto *kinds = new QActionGroup(this);
    kinds->setExclusive(true);

    const QList<ShapeItem::Kind> order = {
        ShapeItem::Kind::Rectangle, ShapeItem::Kind::Ellipse, ShapeItem::Kind::Triangle,
        ShapeItem::Kind::Heart, ShapeItem::Kind::Star
    };

    for (ShapeItem::Kind kind : order) {
        QAction *entry = menu->addAction(ShapeItem::nameFor(kind));
        entry->setCheckable(true);
        entry->setChecked(kind == m_view->shapeKind());
        entry->setData(int(kind));
        entry->setIcon(Icons::fromPath(ShapeItem::pathFor(kind, QRectF(10, 10, 44, 44)),
                                       m_view->toolColor(), symbolColor, true));
        kinds->addAction(entry);
        m_shapeKindActions.append(entry);

        connect(entry, &QAction::triggered, this, [this, kind] {
            m_view->setShapeKind(kind);
            m_view->setTool(CanvasView::Tool::Shape);
            m_shapeAction->setChecked(true);
            updateShapeAction();
        });
    }

    menu->addSeparator();
    m_shapeFilledAction = menu->addAction(tr("Filled"));
    m_shapeFilledAction->setCheckable(true);
    m_shapeFilledAction->setChecked(m_view->shapesFilled());
    connect(m_shapeFilledAction, &QAction::toggled, this, [this](bool filled) {
        m_view->setShapesFilled(filled);
        updateShapeAction();
    });

    auto *roomy = new RoomyStyle;
    roomy->setParent(this);
    menu->setStyle(roomy);

    if (auto *button = qobject_cast<QToolButton *>(toolBar->widgetForAction(m_shapeAction))) {
        button->setMenu(menu);
        button->setPopupMode(QToolButton::MenuButtonPopup);
        button->setStyle(roomy);
    }

    updateShapeAction();
}

// The button shows the shape it would draw, in the colour it would use.
void MainWindow::updateShapeAction()
{
    if (!m_shapeAction)
        return;

    const ShapeItem::Kind kind = m_view->shapeKind();
    m_shapeAction->setIcon(Icons::fromPath(ShapeItem::pathFor(kind, QRectF(10, 10, 44, 44)),
                                           m_view->toolColor(),
                                           palette().color(QPalette::ButtonText),
                                           m_view->shapesFilled()));
    m_shapeAction->setToolTip(tr("%1 (R)\n%2")
                                  .arg(ShapeItem::nameFor(kind),
                                       tr("Draw a shape - hold Shift to keep it even")));
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Tools"));
    m_toolBar = toolBar;
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar->setIconSize(QSize(24, 24));

    // Icons follow the text colour of the current theme.
    const QColor symbolColor = palette().color(QPalette::ButtonText);

    QAction *newAction = toolBar->addAction(Icons::plus(symbolColor), tr("New"));
    newAction->setToolTip(tr("New canvas (Ctrl+N)"));
    newAction->setStatusTip(tr("Create a new canvas"));
    connect(newAction, &QAction::triggered, this, &MainWindow::newCustomCanvas);

    QAction *openAction = toolBar->addAction(Icons::folder(symbolColor), tr("Open"));
    openAction->setToolTip(tr("Open (Ctrl+O)"));
    openAction->setStatusTip(tr("Open a project or an image"));
    connect(openAction, &QAction::triggered, this, &MainWindow::open);

    toolBar->addSeparator();

    if (m_undoAction)
        toolBar->addAction(m_undoAction);
    if (m_redoAction)
        toolBar->addAction(m_redoAction);

    toolBar->addSeparator();

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);

    createToolAction(toolBar, tr("Selection"), Icons::pointer(symbolColor), QStringLiteral("S"),
                     tr("Select, move, scale and delete objects"),
                     int(CanvasView::Tool::Select));
    createToolAction(toolBar, tr("Brush"), Icons::brush(m_view->toolColor(), symbolColor),
                     QStringLiteral("B"),
                     tr("Draw freehand - every stroke becomes an object of its own"),
                     int(CanvasView::Tool::Brush));
    m_brushAction = m_toolGroup->actions().last();
    createToolAction(toolBar, tr("Line"), Icons::line(symbolColor), QStringLiteral("L"),
                     tr("Drag a straight line"),
                     int(CanvasView::Tool::Line));
    createToolAction(toolBar, tr("Eraser"), Icons::eraser(symbolColor), QStringLiteral("E"),
                     tr("Erase inside objects"),
                     int(CanvasView::Tool::Eraser));
    createToolAction(toolBar, tr("Fill"), Icons::bucket(m_view->toolColor(), symbolColor),
                     QStringLiteral("F"),
                     tr("Fill an enclosed area - works across objects"),
                     int(CanvasView::Tool::Fill));
    m_fillAction = m_toolGroup->actions().last();

    createShapeTool(toolBar, symbolColor);

    createToolAction(toolBar, tr("Text"), Icons::text(symbolColor), QStringLiteral("T"),
                     tr("Place a caption - double-click edits it later"),
                     int(CanvasView::Tool::Text));

    m_toolGroup->actions().first()->setChecked(true);

    toolBar->addSeparator();

    m_colorButton = new QToolButton(this);
    m_colorButton->setToolTip(tr("Choose a colour — recolours the selected objects and "
                                 "applies to new ones"));
    m_colorButton->setAutoRaise(true);
    m_colorButton->setIconSize(QSize(24, 24));
    connect(m_colorButton, &QToolButton::clicked, this, &MainWindow::chooseToolColor);
    toolBar->addWidget(m_colorButton);
    updateColorButton();

    toolBar->addWidget(new QLabel(tr(" Size: "), this));
    m_toolSizeSpin = new QDoubleSpinBox(this);
    m_toolSizeSpin->setRange(0.5, 200.0);
    m_toolSizeSpin->setSingleStep(0.5);
    m_toolSizeSpin->setDecimals(1);
    m_toolSizeSpin->setValue(m_view->toolSize());
    m_toolSizeSpin->setSuffix(tr(" px"));
    m_toolSizeSpin->setToolTip(tr("Stroke width of brush and line, diameter of the eraser"));
    connect(m_toolSizeSpin, &QDoubleSpinBox::valueChanged, this,
            [this](double value) { m_view->setToolSize(value); });
    toolBar->addWidget(m_toolSizeSpin);

    toolBar->addWidget(new QLabel(tr("  Font: "), this));
    m_fontSizeSpin = new QDoubleSpinBox(this);
    m_fontSizeSpin->setRange(1.0, 2000.0);
    m_fontSizeSpin->setSingleStep(1.0);
    m_fontSizeSpin->setDecimals(1);
    m_fontSizeSpin->setValue(m_view->fontPointSize());
    m_fontSizeSpin->setSuffix(tr(" pt"));
    m_fontSizeSpin->setToolTip(tr("Font size of the selected text; without a selection, "
                                  "the size for new captions"));
    connect(m_fontSizeSpin, &QDoubleSpinBox::valueChanged, this,
            &MainWindow::applyFontSizeToSelection);
    toolBar->addWidget(m_fontSizeSpin);
}

void MainWindow::createStatusBar()
{
    statusBar()->addWidget(new QLabel(tr("Canvas:"), this));

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(16, 20000);
    m_widthSpin->setSuffix(tr(" px"));
    m_widthSpin->setKeyboardTracking(false);
    m_widthSpin->setToolTip(tr("Width of the canvas"));
    connect(m_widthSpin, &QSpinBox::valueChanged, this, [this](int value) {
        if (!m_updatingControls)
            m_scene->setPageWidth(value);
    });
    statusBar()->addWidget(m_widthSpin);

    statusBar()->addWidget(new QLabel(QStringLiteral("×"), this));

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(16, 20000);
    m_heightSpin->setSuffix(tr(" px"));
    m_heightSpin->setKeyboardTracking(false);
    m_heightSpin->setToolTip(tr("Height of the canvas"));
    connect(m_heightSpin, &QSpinBox::valueChanged, this, [this](int value) {
        if (!m_updatingControls)
            m_scene->setPageHeight(value);
    });
    statusBar()->addWidget(m_heightSpin);

    m_lockButton = new QToolButton(this);
    m_lockButton->setCheckable(true);
    m_lockButton->setAutoRaise(true);
    m_lockButton->setIconSize(QSize(18, 18));
    connect(m_lockButton, &QToolButton::toggled, this, [this](bool checked) {
        m_scene->setAspectRatioLocked(checked);
        const QColor symbolColor = palette().color(QPalette::ButtonText);
        m_lockButton->setIcon(checked ? Icons::lockClosed(symbolColor)
                                      : Icons::lockOpen(symbolColor));
        m_lockButton->setToolTip(checked
                                     ? tr("Aspect ratio is locked — click to unlock")
                                     : tr("Lock the aspect ratio"));
    });
    m_lockButton->setIcon(Icons::lockOpen(palette().color(QPalette::ButtonText)));
    m_lockButton->setToolTip(tr("Lock the aspect ratio"));
    statusBar()->addWidget(m_lockButton);

    m_ratioLabel = new QLabel(this);
    m_ratioLabel->setToolTip(tr("Aspect ratio of the canvas"));
    statusBar()->addWidget(m_ratioLabel);

    m_objectLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_objectLabel);

    m_zoomLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_zoomLabel);
}

// -------------------------------------------------------------- File menu

void MainWindow::newCanvas(const QSize &size)
{
    if (!maybeSave())
        return;

    m_scene->newDocument(size);
    m_view->resetZoom();
    setCurrentFile(QString());
    setDirty(false);
    statusBar()->showMessage(tr("New canvas %1 × %2")
                                 .arg(size.width()).arg(size.height()), 4000);
}

void MainWindow::newCustomCanvas()
{
    NewCanvasDialog dialog(m_scene->pageSize(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    newCanvas(dialog.canvasSize());
    if (dialog.aspectRatioLocked())
        m_lockButton->setChecked(true);
}

void MainWindow::open()
{
    if (!maybeSave())
        return;

    QStringList imagePatterns;
    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    for (const QByteArray &format : formats)
        imagePatterns.append(QStringLiteral("*.%1").arg(QString::fromLatin1(format)));

    // Qt brings no reader for HEIF; libheif does that part.
    if (HeifImage::isSupported())
        imagePatterns << QStringLiteral("*.heic") << QStringLiteral("*.heif")
                       << QStringLiteral("*.hif");

    const QString filter = QStringLiteral("%1;;%2;;%3")
                               .arg(tr("Inkpot project (*.ipot *.inkpot)"),
                                    tr("Images (%1)").arg(imagePatterns.join(QLatin1Char(' '))),
                                    tr("All files (*)"));

    const QString path = QFileDialog::getOpenFileName(this, tr("Open"), QString(), filter);
    if (path.isEmpty())
        return;

    if (isProjectFile(path))
        loadFile(path);
    else
        loadImageFile(path);
}

void MainWindow::openPath(const QString &path)
{
    if (path.isEmpty() || !maybeSave())
        return;

    if (isProjectFile(path))
        loadFile(path);
    else
        loadImageFile(path);
}

bool MainWindow::save()
{
    if (m_currentFile.isEmpty())
        return saveAs();
    return saveFile(m_currentFile);
}

bool MainWindow::saveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                m_currentFile.isEmpty() ? tr("Untitled.ipot")
                                                                        : m_currentFile,
                                                tr("Inkpot project (*.ipot)"));
    if (path.isEmpty())
        return false;

    if (QFileInfo(path).suffix().isEmpty())
        path += QLatin1Char('.') + QLatin1String(kProjectSuffix);

    return saveFile(path);
}

void MainWindow::exportImage()
{
    // Only offer what this build of Qt can actually write. Which formats are
    // there depends on the installed image plugins, so the list is asked for
    // rather than assumed.
    const QList<QPair<QString, QString>> known = {
        { tr("PNG image (*.png)"),         QStringLiteral("png")  },
        { tr("JPEG image (*.jpg *.jpeg)"), QStringLiteral("jpg")  },
        { tr("BMP image (*.bmp)"),         QStringLiteral("bmp")  },
        { tr("TIFF image (*.tif *.tiff)"), QStringLiteral("tif")  },
        { tr("WebP image (*.webp)"),       QStringLiteral("webp") },
    };

    const QList<QByteArray> writable = QImageWriter::supportedImageFormats();
    QList<QPair<QString, QString>> formats;
    for (const auto &candidate : known) {
        const QByteArray suffix = candidate.second.toLatin1();
        const QByteArray alias = suffix == "jpg" ? QByteArray("jpeg")
                               : suffix == "tif" ? QByteArray("tiff")
                                                 : suffix;
        if (writable.contains(suffix) || writable.contains(alias))
            formats.append(candidate);
    }
    if (formats.isEmpty())
        formats.append(known.first());

    QStringList filters;
    for (const auto &format : formats)
        filters.append(format.first);

    QString selectedFilter = filters.first();
    QString path = QFileDialog::getSaveFileName(this, tr("Export"), tr("Untitled.png"),
                                                filters.join(QStringLiteral(";;")), &selectedFilter);
    if (path.isEmpty())
        return;

    if (QFileInfo(path).suffix().isEmpty()) {
        // Append the extension that goes with the chosen filter.
        QString suffix = formats.first().second;
        for (const auto &format : formats) {
            if (format.first == selectedFilter) {
                suffix = format.second;
                break;
            }
        }
        path += QLatin1Char('.') + suffix;
    }

    const QImage image = m_scene->renderPage(Qt::white);
    if (image.isNull() || !image.save(path)) {
        QMessageBox::warning(this, tr("Export"),
                             tr("The file %1 could not be written.")
                                 .arg(QFileInfo(path).fileName()));
        return;
    }
    statusBar()->showMessage(tr("Exported to %1").arg(QFileInfo(path).fileName()), 4000);
}

void MainWindow::copyToClipboard()
{
    // With a selection: only the selected objects, otherwise the whole canvas.
    QImage image = m_scene->renderSelection();
    if (image.isNull())
        image = m_scene->renderPage(Qt::white);

    if (image.isNull())
        return;

    QApplication::clipboard()->setImage(image);
    statusBar()->showMessage(tr("Copied to the clipboard"), 3000);
}

void MainWindow::pasteFromClipboard()
{
    const QImage image = QApplication::clipboard()->image();
    if (image.isNull()) {
        statusBar()->showMessage(tr("The clipboard does not contain an image"), 4000);
        return;
    }

    auto *item = new ImageItem(QPixmap::fromImage(image));
    const QRectF page = m_scene->pageRect();
    item->setPos(page.center() - QPointF(image.width() / 2.0, image.height() / 2.0));
    m_scene->addObjectUndoable(item, tr("Paste image"));
    m_scene->clearSelection();
    item->setSelected(true);
    statusBar()->showMessage(tr("Image pasted as a new object"), 3000);
}

void MainWindow::print()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Canvas"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QImage image = m_scene->renderPage(Qt::white);
    if (image.isNull())
        return;

    QPainter painter(&printer);
    if (!painter.isActive())
        return;

    // Fit to the page while keeping the aspect ratio.
    const QRect target = painter.viewport();
    QSize size = image.size();
    size.scale(target.size(), Qt::KeepAspectRatio);
    painter.setViewport(target.x() + (target.width() - size.width()) / 2,
                        target.y() + (target.height() - size.height()) / 2,
                        size.width(), size.height());
    painter.setWindow(image.rect());
    painter.drawImage(0, 0, image);
}

// -------------------------------------------------------------- Edit menu

void MainWindow::deleteSelection()
{
    if (m_scene->selectedItems().isEmpty()) {
        statusBar()->showMessage(tr("No object selected"), 3000);
        return;
    }
    const int count = m_scene->selectedItems().size();
    m_scene->removeSelectedObjects();
    statusBar()->showMessage(tr("%n object(s) deleted", nullptr, count), 3000);
}

// ---------------------------------------------------------------- State

bool MainWindow::maybeSave()
{
    if (!m_dirty)
        return true;

    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this, tr("Inkpot"),
        tr("The drawing has been changed.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (answer) {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Discard:
        return true;
    default:
        return false;
    }
}

bool MainWindow::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Open"),
                             tr("The file %1 could not be read.")
                                 .arg(QFileInfo(path).fileName()));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, tr("Open"),
                             tr("The file %1 is damaged: %2")
                                 .arg(QFileInfo(path).fileName(), parseError.errorString()));
        return false;
    }

    QString error;
    if (!m_scene->fromJson(document.object(), &error)) {
        QMessageBox::warning(this, tr("Open"), error);
        return false;
    }

    m_updatingControls = true;
    m_lockButton->setChecked(m_scene->aspectRatioLocked());
    m_updatingControls = false;

    m_view->fitPageInView();
    setCurrentFile(path);
    setDirty(false);
    updateSizeControls();
    updateObjectInfo();
    statusBar()->showMessage(tr("Opened %1").arg(QFileInfo(path).fileName()), 4000);
    return true;
}

bool MainWindow::loadImageFile(const QString &path)
{
    QImage image;
    QString problem;

    if (HeifImage::looksLikeHeif(path)) {
        image = HeifImage::load(path, &problem);
    } else {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        image = reader.read();
        problem = reader.errorString();
    }

    if (image.isNull()) {
        QMessageBox::warning(this, tr("Open"),
                             tr("The image %1 could not be loaded: %2")
                                 .arg(QFileInfo(path).fileName(), problem));
        return false;
    }

    // The image becomes an object on a canvas sized to match.
    m_scene->newDocument(image.size());

    auto *item = new ImageItem(QPixmap::fromImage(image));
    item->setPos(0.0, 0.0);
    m_scene->addObjectUndoable(item, tr("Import image"));
    m_scene->undoStack()->clear();

    m_view->fitPageInView();
    setCurrentFile(QString());   // not saved as a project yet
    setDirty(true);
    statusBar()->showMessage(tr("Imported %1 as an object").arg(QFileInfo(path).fileName()), 4000);
    return true;
}

bool MainWindow::saveFile(const QString &path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Save"),
                             tr("The file %1 could not be written.")
                                 .arg(QFileInfo(path).fileName()));
        return false;
    }

    const QJsonDocument document(m_scene->toJson());
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Save"),
                             tr("The file %1 could not be written.")
                                 .arg(QFileInfo(path).fileName()));
        return false;
    }

    setCurrentFile(path);
    setDirty(false);
    statusBar()->showMessage(tr("Saved as %1").arg(QFileInfo(path).fileName()), 4000);
    return true;
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = path;
    updateWindowTitle();
}

void MainWindow::setDirty(bool dirty)
{
    if (m_dirty == dirty)
        return;
    m_dirty = dirty;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    const QString name = m_currentFile.isEmpty() ? tr("Untitled")
                                                 : QFileInfo(m_currentFile).fileName();
    setWindowTitle(QStringLiteral("%1%2 - Inkpot").arg(m_dirty ? QStringLiteral("*") : QString(), name));
}

void MainWindow::updateSizeControls()
{
    const QSize size = m_scene->pageSize();

    m_updatingControls = true;
    m_widthSpin->setValue(size.width());
    m_heightSpin->setValue(size.height());
    m_updatingControls = false;

    m_ratioLabel->setText(tr("Ratio: %1").arg(aspectRatioText(size)));
}

void MainWindow::updateObjectInfo()
{
    const int total = m_scene->objectCount();
    const int selected = m_scene->selectedItems().size();
    m_objectLabel->setText(selected > 0 ? tr("Objects: %1 (%2 selected)").arg(total).arg(selected)
                                        : tr("Objects: %1").arg(total));
}

void MainWindow::updateZoomInfo(double factor)
{
    m_zoomLabel->setText(tr("Zoom: %1 %").arg(qRound(factor * 100.0)));
}

void MainWindow::chooseToolColor()
{
    // With something selected the dialog starts from that object's colour.
    const QColor current = m_scene->selectedObjectColor().isValid()
                               ? m_scene->selectedObjectColor()
                               : m_view->toolColor();

    const QColor color = QColorDialog::getColor(current, this, tr("Choose Colour"),
                                                QColorDialog::ShowAlphaChannel);
    if (!color.isValid())
        return;

    // From now on the colour applies to new objects and to the selection.
    m_view->setToolColor(color);
    if (m_scene->setSelectedObjectsColor(color)) {
        const int count = m_scene->selectedItems().size();
        statusBar()->showMessage(tr("Changed the colour of %n object(s)", nullptr, count), 3000);
    }
    updateColorButton();
}

void MainWindow::applyFontSizeToSelection(double pointSize)
{
    if (m_updatingControls)
        return;

    if (TextItem *text = m_scene->singleSelectedText()) {
        text->setFontPointSize(pointSize);
        m_scene->notifyEdited();
        return;
    }

    // With no text selected the size applies to the next caption.
    m_view->setFontPointSize(pointSize);
}

void MainWindow::updateObjectControls()
{
    m_updatingControls = true;

    // The colour button shows the colour of the selection. The drawing colour
    // itself stays untouched: clicking an object must not recolour the brush.
    updateColorButton();

    // Show the font size of the selected text, including while dragging.
    if (TextItem *text = m_scene->singleSelectedText())
        m_fontSizeSpin->setValue(text->fontPointSize());
    else
        m_fontSizeSpin->setValue(m_view->fontPointSize());

    m_updatingControls = false;
}

void MainWindow::updateColorButton()
{
    // With something selected the button shows its colour, otherwise the
    // colour new objects are created with.
    const QColor selected = m_scene->selectedObjectColor();
    const QColor shown = selected.isValid() ? selected : m_view->toolColor();

    QPixmap pixmap(24, 24);
    pixmap.fill(shown);
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(0x40, 0x40, 0x44), 1.0));
    painter.drawRect(0, 0, 23, 23);
    painter.end();
    m_colorButton->setIcon(QIcon(pixmap));

    // The bristles of the brush icon always show the drawing colour, not the
    // colour of the selection - that is what you actually paint with.
    const QColor symbolColor = palette().color(QPalette::ButtonText);
    if (m_brushAction)
        m_brushAction->setIcon(Icons::brush(m_view->toolColor(), symbolColor));
    if (m_fillAction)
        m_fillAction->setIcon(Icons::bucket(m_view->toolColor(), symbolColor));
    updateShapeAction();
}

QString MainWindow::aspectRatioText(const QSize &size)
{
    if (size.width() <= 0 || size.height() <= 0)
        return QStringLiteral("-");

    const int divisor = std::gcd(size.width(), size.height());
    const int width = size.width() / divisor;
    const int height = size.height() / divisor;
    if (width <= 64 && height <= 64)
        return QStringLiteral("%1:%2").arg(width).arg(height);

    // Too unwieldy when reduced: approximate a common ratio instead.
    struct CommonRatio { int w; int h; };
    static const CommonRatio common[] = {
        {1, 1}, {5, 4}, {4, 3}, {3, 2}, {16, 10}, {16, 9}, {21, 9}, {2, 1},
        {4, 5}, {3, 4}, {2, 3}, {10, 16}, {9, 16}, {1, 2}
    };

    const double ratio = double(size.width()) / size.height();
    for (const CommonRatio &candidate : common) {
        const double value = double(candidate.w) / candidate.h;
        if (qAbs(ratio - value) / ratio < 0.01)
            return QStringLiteral("~%1:%2").arg(candidate.w).arg(candidate.h);
    }

    return QStringLiteral("%1:1").arg(QString::number(ratio, 'f', 2));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    storeLayout();
    event->accept();
}
