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
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

#include "CanvasScene.h"
#include "CanvasView.h"
#include "Icons.h"
#include "ImageItem.h"
#include "NewCanvasDialog.h"
#include "TextItem.h"

namespace {

const char *kProjectSuffix = "inkpot";
const char *kProjectFilter = "Inkpot-Projekt (*.inkpot)";

// Standardgrößen für "Datei > Neu".
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

    createActions();
    createToolBar();
    createStatusBar();

    connect(m_scene, &CanvasScene::pageRectChanged, this, &MainWindow::updateSizeControls);
    connect(m_scene, &CanvasScene::objectsChanged, this, &MainWindow::updateObjectInfo);
    connect(m_scene, &CanvasScene::edited, this, [this] { setDirty(true); });
    connect(m_scene, &CanvasScene::selectionChanged, this, &MainWindow::updateObjectInfo);
    connect(m_scene, &CanvasScene::objectPropertiesChanged, this, &MainWindow::updateObjectControls);
    connect(m_view, &CanvasView::zoomChanged, this, &MainWindow::updateZoomInfo);

    resize(1200, 820);
    updateSizeControls();
    updateObjectInfo();
    updateZoomInfo(m_view->zoomFactor());
    setCurrentFile(QString());
    setDirty(false);
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&Datei"));

    QMenu *newMenu = fileMenu->addMenu(tr("&Neu"));
    for (const Preset &preset : kPresets) {
        const QSize size(preset.width, preset.height);
        QAction *action = newMenu->addAction(QString::fromUtf8(preset.label));
        connect(action, &QAction::triggered, this, [this, size] { newCanvas(size); });
    }
    newMenu->addSeparator();
    QAction *customAction = newMenu->addAction(tr("&Benutzerdefinierte Größe ..."));
    customAction->setShortcut(QKeySequence::New);
    connect(customAction, &QAction::triggered, this, &MainWindow::newCustomCanvas);

    QAction *openAction = fileMenu->addAction(tr("&Öffnen ..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::open);

    m_saveAction = fileMenu->addAction(tr("&Speichern"));
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::save);

    QAction *saveAsAction = fileMenu->addAction(tr("Speichern &unter ..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAs);

    QAction *exportAction = fileMenu->addAction(tr("&Exportieren ..."));
    exportAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    exportAction->setStatusTip(tr("Die Arbeitsfläche als Bilddatei speichern"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportImage);

    fileMenu->addSeparator();

    QAction *copyAction = fileMenu->addAction(tr("In die &Zwischenablage kopieren"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copyToClipboard);

    m_pasteAction = fileMenu->addAction(tr("&Einfügen"));
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, this, &MainWindow::pasteFromClipboard);

    fileMenu->addSeparator();

    QAction *printAction = fileMenu->addAction(tr("&Drucken ..."));
    printAction->setShortcut(QKeySequence::Print);
    connect(printAction, &QAction::triggered, this, &MainWindow::print);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction(tr("&Beenden"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu(tr("&Bearbeiten"));

    m_deleteAction = editMenu->addAction(tr("Auswahl &löschen"));
    m_deleteAction->setShortcut(QKeySequence::Delete);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteSelection);

    QAction *selectAllAction = editMenu->addAction(tr("&Alles auswählen"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, [this] { m_scene->selectAllObjects(); });

    QAction *deselectAction = editMenu->addAction(tr("Auswahl auf&heben"));
    deselectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+A")));
    connect(deselectAction, &QAction::triggered, this, [this] { m_scene->clearSelection(); });

    QMenu *viewMenu = menuBar()->addMenu(tr("&Ansicht"));

    QAction *zoomInAction = viewMenu->addAction(tr("Ver&größern"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, m_view, &CanvasView::zoomIn);

    QAction *zoomOutAction = viewMenu->addAction(tr("Ver&kleinern"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, m_view, &CanvasView::zoomOut);

    QAction *resetZoomAction = viewMenu->addAction(tr("&Originalgröße"));
    resetZoomAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(resetZoomAction, &QAction::triggered, m_view, &CanvasView::resetZoom);

    QAction *fitAction = viewMenu->addAction(tr("&Einpassen"));
    fitAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+9")));
    connect(fitAction, &QAction::triggered, m_view, &CanvasView::fitPageInView);
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

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Werkzeuge"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar->setIconSize(QSize(24, 24));

    // Symbole folgen der Textfarbe des Themes.
    const QColor symbolColor = palette().color(QPalette::ButtonText);

    QAction *newAction = toolBar->addAction(Icons::plus(symbolColor), tr("Neu"));
    newAction->setToolTip(tr("Neue Arbeitsfläche (Strg+N)"));
    newAction->setStatusTip(tr("Neue Arbeitsfläche anlegen"));
    connect(newAction, &QAction::triggered, this, &MainWindow::newCustomCanvas);

    QAction *openAction = toolBar->addAction(Icons::folder(symbolColor), tr("Öffnen"));
    openAction->setToolTip(tr("Öffnen (Strg+O)"));
    openAction->setStatusTip(tr("Projekt oder Bild öffnen"));
    connect(openAction, &QAction::triggered, this, &MainWindow::open);

    toolBar->addSeparator();

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);

    createToolAction(toolBar, tr("Auswahl"), Icons::pointer(symbolColor), QStringLiteral("S"),
                     tr("Objekte auswählen, verschieben, skalieren und löschen"),
                     int(CanvasView::Tool::Select));
    createToolAction(toolBar, tr("Pinsel"), Icons::brush(m_view->toolColor(), symbolColor),
                     QStringLiteral("B"),
                     tr("Freihand malen - jeder Strich wird ein eigenes Objekt"),
                     int(CanvasView::Tool::Brush));
    m_brushAction = m_toolGroup->actions().last();
    createToolAction(toolBar, tr("Linie"), Icons::line(symbolColor), QStringLiteral("L"),
                     tr("Gerade Linie ziehen"),
                     int(CanvasView::Tool::Line));
    createToolAction(toolBar, tr("Radiergummi"), Icons::eraser(symbolColor), QStringLiteral("E"),
                     tr("Innerhalb von Objekten radieren"),
                     int(CanvasView::Tool::Eraser));
    createToolAction(toolBar, tr("Text"), Icons::text(symbolColor), QStringLiteral("T"),
                     tr("Beschriftung setzen - Doppelklick bearbeitet sie später"),
                     int(CanvasView::Tool::Text));

    m_toolGroup->actions().first()->setChecked(true);

    toolBar->addSeparator();

    m_colorButton = new QToolButton(this);
    m_colorButton->setToolTip(tr("Farbe wählen — färbt die ausgewählten Objekte um und gilt "
                                 "für neue"));
    m_colorButton->setAutoRaise(true);
    m_colorButton->setIconSize(QSize(24, 24));
    connect(m_colorButton, &QToolButton::clicked, this, &MainWindow::chooseToolColor);
    toolBar->addWidget(m_colorButton);
    updateColorButton();

    toolBar->addWidget(new QLabel(tr(" Größe: "), this));
    m_toolSizeSpin = new QDoubleSpinBox(this);
    m_toolSizeSpin->setRange(0.5, 200.0);
    m_toolSizeSpin->setSingleStep(0.5);
    m_toolSizeSpin->setDecimals(1);
    m_toolSizeSpin->setValue(m_view->toolSize());
    m_toolSizeSpin->setSuffix(tr(" px"));
    m_toolSizeSpin->setToolTip(tr("Strichstärke von Pinsel und Linie, Durchmesser des Radiergummis"));
    connect(m_toolSizeSpin, &QDoubleSpinBox::valueChanged, this,
            [this](double value) { m_view->setToolSize(value); });
    toolBar->addWidget(m_toolSizeSpin);

    toolBar->addWidget(new QLabel(tr("  Schrift: "), this));
    m_fontSizeSpin = new QDoubleSpinBox(this);
    m_fontSizeSpin->setRange(1.0, 2000.0);
    m_fontSizeSpin->setSingleStep(1.0);
    m_fontSizeSpin->setDecimals(1);
    m_fontSizeSpin->setValue(m_view->fontPointSize());
    m_fontSizeSpin->setSuffix(tr(" pt"));
    m_fontSizeSpin->setToolTip(tr("Schriftgröße des ausgewählten Textes; ohne Auswahl die "
                                  "Größe für neue Beschriftungen"));
    connect(m_fontSizeSpin, &QDoubleSpinBox::valueChanged, this,
            &MainWindow::applyFontSizeToSelection);
    toolBar->addWidget(m_fontSizeSpin);
}

void MainWindow::createStatusBar()
{
    statusBar()->addWidget(new QLabel(tr("Arbeitsfläche:"), this));

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(16, 20000);
    m_widthSpin->setSuffix(tr(" px"));
    m_widthSpin->setKeyboardTracking(false);
    m_widthSpin->setToolTip(tr("Breite der Arbeitsfläche"));
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
    m_heightSpin->setToolTip(tr("Höhe der Arbeitsfläche"));
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
                                     ? tr("Seitenverhältnis ist festgehalten — zum Lösen klicken")
                                     : tr("Seitenverhältnis festhalten"));
    });
    m_lockButton->setIcon(Icons::lockOpen(palette().color(QPalette::ButtonText)));
    m_lockButton->setToolTip(tr("Seitenverhältnis festhalten"));
    statusBar()->addWidget(m_lockButton);

    m_ratioLabel = new QLabel(this);
    m_ratioLabel->setToolTip(tr("Seitenverhältnis der Arbeitsfläche"));
    statusBar()->addWidget(m_ratioLabel);

    m_objectLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_objectLabel);

    m_zoomLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_zoomLabel);
}

// ---------------------------------------------------------------- Datei-Menue

void MainWindow::newCanvas(const QSize &size)
{
    if (!maybeSave())
        return;

    m_scene->clearObjects();
    m_scene->setPageSize(size);
    m_view->resetZoom();
    setCurrentFile(QString());
    setDirty(false);
    statusBar()->showMessage(tr("Neue Arbeitsfläche %1 × %2")
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

    const QString filter = QStringLiteral("%1;;%2;;%3")
                               .arg(QString::fromLatin1(kProjectFilter),
                                    tr("Bilder (%1)").arg(imagePatterns.join(QLatin1Char(' '))),
                                    tr("Alle Dateien (*)"));

    const QString path = QFileDialog::getOpenFileName(this, tr("Öffnen"), QString(), filter);
    if (path.isEmpty())
        return;

    if (QFileInfo(path).suffix().compare(QLatin1String(kProjectSuffix), Qt::CaseInsensitive) == 0)
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
    QString path = QFileDialog::getSaveFileName(this, tr("Speichern unter"),
                                                m_currentFile.isEmpty() ? tr("Unbenannt.inkpot")
                                                                        : m_currentFile,
                                                QString::fromLatin1(kProjectFilter));
    if (path.isEmpty())
        return false;

    if (QFileInfo(path).suffix().isEmpty())
        path += QLatin1Char('.') + QLatin1String(kProjectSuffix);

    return saveFile(path);
}

void MainWindow::exportImage()
{
    // Filtertext -> Standardendung
    const QVector<QPair<QString, QString>> formats = {
        { tr("PNG-Bild (*.png)"),          QStringLiteral("png")  },
        { tr("JPEG-Bild (*.jpg *.jpeg)"),  QStringLiteral("jpg")  },
        { tr("BMP-Bild (*.bmp)"),          QStringLiteral("bmp")  },
        { tr("TIFF-Bild (*.tif *.tiff)"),  QStringLiteral("tif")  },
        { tr("WebP-Bild (*.webp)"),        QStringLiteral("webp") },
    };

    QStringList filters;
    for (const auto &format : formats)
        filters.append(format.first);

    QString selectedFilter = filters.first();
    QString path = QFileDialog::getSaveFileName(this, tr("Exportieren"), tr("Unbenannt.png"),
                                                filters.join(QStringLiteral(";;")), &selectedFilter);
    if (path.isEmpty())
        return;

    if (QFileInfo(path).suffix().isEmpty()) {
        // Endung aus dem gewählten Filter ergaenzen.
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
        QMessageBox::warning(this, tr("Exportieren"),
                             tr("Die Datei %1 konnte nicht geschrieben werden.")
                                 .arg(QFileInfo(path).fileName()));
        return;
    }
    statusBar()->showMessage(tr("Exportiert nach %1").arg(QFileInfo(path).fileName()), 4000);
}

void MainWindow::copyToClipboard()
{
    // Mit Auswahl: nur die ausgewählten Objekte, sonst die ganze Arbeitsfläche.
    QImage image = m_scene->renderSelection();
    if (image.isNull())
        image = m_scene->renderPage(Qt::white);

    if (image.isNull())
        return;

    QApplication::clipboard()->setImage(image);
    statusBar()->showMessage(tr("In die Zwischenablage kopiert"), 3000);
}

void MainWindow::pasteFromClipboard()
{
    const QImage image = QApplication::clipboard()->image();
    if (image.isNull()) {
        statusBar()->showMessage(tr("Die Zwischenablage enthaelt kein Bild"), 4000);
        return;
    }

    auto *item = new ImageItem(QPixmap::fromImage(image));
    const QRectF page = m_scene->pageRect();
    item->setPos(page.center() - QPointF(image.width() / 2.0, image.height() / 2.0));
    m_scene->addObject(item);
    m_scene->clearSelection();
    item->setSelected(true);
    statusBar()->showMessage(tr("Bild als neues Objekt eingefügt"), 3000);
}

void MainWindow::print()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Arbeitsfläche drucken"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QImage image = m_scene->renderPage(Qt::white);
    if (image.isNull())
        return;

    QPainter painter(&printer);
    if (!painter.isActive())
        return;

    // Seitenfuellend einpassen, Seitenverhältnis bleibt erhalten.
    const QRect target = painter.viewport();
    QSize size = image.size();
    size.scale(target.size(), Qt::KeepAspectRatio);
    painter.setViewport(target.x() + (target.width() - size.width()) / 2,
                        target.y() + (target.height() - size.height()) / 2,
                        size.width(), size.height());
    painter.setWindow(image.rect());
    painter.drawImage(0, 0, image);
}

// ----------------------------------------------------------- Bearbeiten-Menue

void MainWindow::deleteSelection()
{
    if (m_scene->selectedItems().isEmpty()) {
        statusBar()->showMessage(tr("Kein Objekt ausgewählt"), 3000);
        return;
    }
    const int count = m_scene->selectedItems().size();
    m_scene->removeSelectedObjects();
    statusBar()->showMessage(tr("%n Objekt(e) gelöscht", nullptr, count), 3000);
}

// ------------------------------------------------------------------- Zustand

bool MainWindow::maybeSave()
{
    if (!m_dirty)
        return true;

    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this, tr("Inkpot"),
        tr("Die Zeichnung wurde geändert.\nMöchtest Du die Änderungen speichern?"),
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
        QMessageBox::warning(this, tr("Öffnen"),
                             tr("Die Datei %1 konnte nicht gelesen werden.")
                                 .arg(QFileInfo(path).fileName()));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, tr("Öffnen"),
                             tr("Die Datei %1 ist beschädigt: %2")
                                 .arg(QFileInfo(path).fileName(), parseError.errorString()));
        return false;
    }

    QString error;
    if (!m_scene->fromJson(document.object(), &error)) {
        QMessageBox::warning(this, tr("Öffnen"), error);
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
    statusBar()->showMessage(tr("%1 geöffnet").arg(QFileInfo(path).fileName()), 4000);
    return true;
}

bool MainWindow::loadImageFile(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Öffnen"),
                             tr("Das Bild %1 konnte nicht geladen werden: %2")
                                 .arg(QFileInfo(path).fileName(), reader.errorString()));
        return false;
    }

    // Das Bild wird ein Objekt auf einer passend grossen Arbeitsfläche.
    m_scene->clearObjects();
    m_scene->setPageSize(image.size());

    auto *item = new ImageItem(QPixmap::fromImage(image));
    item->setPos(0.0, 0.0);
    m_scene->addObject(item);

    m_view->fitPageInView();
    setCurrentFile(QString());   // als Projekt noch nicht gespeichert
    setDirty(true);
    statusBar()->showMessage(tr("%1 als Objekt importiert").arg(QFileInfo(path).fileName()), 4000);
    return true;
}

bool MainWindow::saveFile(const QString &path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Speichern"),
                             tr("Die Datei %1 konnte nicht geschrieben werden.")
                                 .arg(QFileInfo(path).fileName()));
        return false;
    }

    const QJsonDocument document(m_scene->toJson());
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Speichern"),
                             tr("Die Datei %1 konnte nicht geschrieben werden.")
                                 .arg(QFileInfo(path).fileName()));
        return false;
    }

    setCurrentFile(path);
    setDirty(false);
    statusBar()->showMessage(tr("Gespeichert als %1").arg(QFileInfo(path).fileName()), 4000);
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
    const QString name = m_currentFile.isEmpty() ? tr("Unbenannt")
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

    m_ratioLabel->setText(tr("Verhältnis: %1").arg(aspectRatioText(size)));
}

void MainWindow::updateObjectInfo()
{
    const int total = m_scene->objectCount();
    const int selected = m_scene->selectedItems().size();
    m_objectLabel->setText(selected > 0 ? tr("Objekte: %1 (%2 ausgewählt)").arg(total).arg(selected)
                                        : tr("Objekte: %1").arg(total));
}

void MainWindow::updateZoomInfo(double factor)
{
    m_zoomLabel->setText(tr("Zoom: %1 %").arg(qRound(factor * 100.0)));
}

void MainWindow::chooseToolColor()
{
    // Ist etwas ausgewählt, zeigt der Dialog dessen Farbe als Ausgangswert.
    const QColor current = m_scene->selectedObjectColor().isValid()
                               ? m_scene->selectedObjectColor()
                               : m_view->toolColor();

    const QColor color = QColorDialog::getColor(current, this, tr("Farbe wählen"),
                                                QColorDialog::ShowAlphaChannel);
    if (!color.isValid())
        return;

    // Die Farbe gilt ab jetzt für neue Objekte und zugleich für die Auswahl.
    m_view->setToolColor(color);
    if (m_scene->setSelectedObjectsColor(color)) {
        const int count = m_scene->selectedItems().size();
        statusBar()->showMessage(tr("Farbe von %n Objekt(en) geändert", nullptr, count), 3000);
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

    // Ohne ausgewählten Text gilt die Größe für die nächste Beschriftung.
    m_view->setFontPointSize(pointSize);
}

void MainWindow::updateObjectControls()
{
    m_updatingControls = true;

    // Der Farbknopf zeigt die Farbe der Auswahl. Die Malfarbe selbst bleibt
    // unberührt: ein Objekt anzuklicken soll den Pinsel nicht umfärben.
    updateColorButton();

    // Schriftgröße des ausgewählten Textes anzeigen, auch während des Ziehens.
    if (TextItem *text = m_scene->singleSelectedText())
        m_fontSizeSpin->setValue(text->fontPointSize());
    else
        m_fontSizeSpin->setValue(m_view->fontPointSize());

    m_updatingControls = false;
}

void MainWindow::updateColorButton()
{
    // Ist etwas ausgewählt, zeigt der Knopf dessen Farbe, sonst die Farbe,
    // mit der neue Objekte entstehen.
    const QColor selected = m_scene->selectedObjectColor();
    const QColor shown = selected.isValid() ? selected : m_view->toolColor();

    QPixmap pixmap(24, 24);
    pixmap.fill(shown);
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(0x40, 0x40, 0x44), 1.0));
    painter.drawRect(0, 0, 23, 23);
    painter.end();
    m_colorButton->setIcon(QIcon(pixmap));

    // Die Borsten des Pinselsymbols zeigen immer die Malfarbe, nicht die
    // Farbe der Auswahl - gemalt wird schließlich mit ihr.
    if (m_brushAction)
        m_brushAction->setIcon(Icons::brush(m_view->toolColor(),
                                            palette().color(QPalette::ButtonText)));
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

    // Gekürzt zu unübersichtlich: an gängige Verhältnisse annaehern.
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
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}
