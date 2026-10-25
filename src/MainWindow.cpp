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
#include "ImageItem.h"
#include "NewCanvasDialog.h"

namespace {

const char *kProjectSuffix = "inkpot";
const char *kProjectFilter = "Inkpot-Projekt (*.inkpot)";

// Standardgroessen fuer "Datei > Neu".
struct Preset {
    const char *label;
    int width;
    int height;
};

const Preset kPresets[] = {
    { "800 \xC3\x97 600 (4:3)",            800,  600 },
    { "1024 \xC3\x97 768 (4:3)",          1024,  768 },
    { "1280 \xC3\x97 720 (HD, 16:9)",     1280,  720 },
    { "1920 \xC3\x97 1080 (Full HD)",     1920, 1080 },
    { "1080 \xC3\x97 1080 (Quadrat)",     1080, 1080 },
    { "2480 \xC3\x97 3508 (A4, 300 dpi)", 2480, 3508 },
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
    QAction *customAction = newMenu->addAction(tr("&Benutzerdefinierte Groesse ..."));
    customAction->setShortcut(QKeySequence::New);
    connect(customAction, &QAction::triggered, this, &MainWindow::newCustomCanvas);

    QAction *openAction = fileMenu->addAction(tr("&Oeffnen ..."));
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
    exportAction->setStatusTip(tr("Die Arbeitsflaeche als Bilddatei speichern"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportImage);

    fileMenu->addSeparator();

    QAction *copyAction = fileMenu->addAction(tr("In die &Zwischenablage kopieren"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copyToClipboard);

    m_pasteAction = fileMenu->addAction(tr("&Einfuegen"));
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

    m_deleteAction = editMenu->addAction(tr("Auswahl &loeschen"));
    m_deleteAction->setShortcut(QKeySequence::Delete);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteSelection);

    QAction *selectAllAction = editMenu->addAction(tr("&Alles auswaehlen"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, [this] { m_scene->selectAllObjects(); });

    QAction *deselectAction = editMenu->addAction(tr("Auswahl auf&heben"));
    deselectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+A")));
    connect(deselectAction, &QAction::triggered, this, [this] { m_scene->clearSelection(); });

    QMenu *viewMenu = menuBar()->addMenu(tr("&Ansicht"));

    QAction *zoomInAction = viewMenu->addAction(tr("Ver&groessern"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, m_view, &CanvasView::zoomIn);

    QAction *zoomOutAction = viewMenu->addAction(tr("Ver&kleinern"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, m_view, &CanvasView::zoomOut);

    QAction *resetZoomAction = viewMenu->addAction(tr("&Originalgroesse"));
    resetZoomAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(resetZoomAction, &QAction::triggered, m_view, &CanvasView::resetZoom);

    QAction *fitAction = viewMenu->addAction(tr("&Einpassen"));
    fitAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+9")));
    connect(fitAction, &QAction::triggered, m_view, &CanvasView::fitPageInView);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Werkzeuge"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);

    QAction *selectAction = toolBar->addAction(tr("Auswahl"));
    selectAction->setCheckable(true);
    selectAction->setChecked(true);
    selectAction->setShortcut(QKeySequence(QStringLiteral("S")));
    selectAction->setStatusTip(tr("Objekte auswaehlen, verschieben und loeschen"));
    connect(selectAction, &QAction::triggered, this,
            [this] { m_view->setTool(CanvasView::Tool::Select); });
    m_toolGroup->addAction(selectAction);

    QAction *brushAction = toolBar->addAction(tr("Pinsel"));
    brushAction->setCheckable(true);
    brushAction->setShortcut(QKeySequence(QStringLiteral("B")));
    brushAction->setStatusTip(tr("Malen - jeder Strich wird ein eigenes Objekt"));
    connect(brushAction, &QAction::triggered, this,
            [this] { m_view->setTool(CanvasView::Tool::Brush); });
    m_toolGroup->addAction(brushAction);

    toolBar->addSeparator();

    m_colorButton = new QToolButton(this);
    m_colorButton->setText(tr("Farbe"));
    m_colorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_colorButton, &QToolButton::clicked, this, &MainWindow::chooseBrushColor);
    toolBar->addWidget(m_colorButton);
    updateColorButton();

    toolBar->addWidget(new QLabel(tr("  Breite:"), this));
    m_widthBrushSpin = new QDoubleSpinBox(this);
    m_widthBrushSpin->setRange(0.5, 200.0);
    m_widthBrushSpin->setSingleStep(0.5);
    m_widthBrushSpin->setDecimals(1);
    m_widthBrushSpin->setValue(m_view->brushWidth());
    m_widthBrushSpin->setSuffix(tr(" px"));
    connect(m_widthBrushSpin, &QDoubleSpinBox::valueChanged, this,
            [this](double value) { m_view->setBrushWidth(value); });
    toolBar->addWidget(m_widthBrushSpin);
}

void MainWindow::createStatusBar()
{
    statusBar()->addWidget(new QLabel(tr("Arbeitsflaeche:"), this));

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(16, 20000);
    m_widthSpin->setSuffix(tr(" px"));
    m_widthSpin->setKeyboardTracking(false);
    m_widthSpin->setToolTip(tr("Breite der Arbeitsflaeche"));
    connect(m_widthSpin, &QSpinBox::valueChanged, this, [this](int value) {
        if (!m_updatingControls)
            m_scene->setPageWidth(value);
    });
    statusBar()->addWidget(m_widthSpin);

    statusBar()->addWidget(new QLabel(QStringLiteral("\xC3\x97"), this));

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(16, 20000);
    m_heightSpin->setSuffix(tr(" px"));
    m_heightSpin->setKeyboardTracking(false);
    m_heightSpin->setToolTip(tr("Hoehe der Arbeitsflaeche"));
    connect(m_heightSpin, &QSpinBox::valueChanged, this, [this](int value) {
        if (!m_updatingControls)
            m_scene->setPageHeight(value);
    });
    statusBar()->addWidget(m_heightSpin);

    m_lockButton = new QToolButton(this);
    m_lockButton->setText(tr("Verhaeltnis halten"));
    m_lockButton->setCheckable(true);
    m_lockButton->setToolTip(tr("Seitenverhaeltnis beim Veraendern der Groesse beibehalten"));
    connect(m_lockButton, &QToolButton::toggled, this, [this](bool checked) {
        m_scene->setAspectRatioLocked(checked);
    });
    statusBar()->addWidget(m_lockButton);

    m_ratioLabel = new QLabel(this);
    m_ratioLabel->setToolTip(tr("Seitenverhaeltnis der Arbeitsflaeche"));
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
    m_scene->setPageRect(QRectF(QPointF(0.0, 0.0), QSizeF(size)));
    m_view->resetZoom();
    m_view->centerOn(m_scene->pageRect().center());
    setCurrentFile(QString());
    setDirty(false);
    statusBar()->showMessage(tr("Neue Arbeitsflaeche %1 \xC3\x97 %2")
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

    const QString path = QFileDialog::getOpenFileName(this, tr("Oeffnen"), QString(), filter);
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
        // Endung aus dem gewaehlten Filter ergaenzen.
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
    // Mit Auswahl: nur die ausgewaehlten Objekte, sonst die ganze Arbeitsflaeche.
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
    statusBar()->showMessage(tr("Bild als neues Objekt eingefuegt"), 3000);
}

void MainWindow::print()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Arbeitsflaeche drucken"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QImage image = m_scene->renderPage(Qt::white);
    if (image.isNull())
        return;

    QPainter painter(&printer);
    if (!painter.isActive())
        return;

    // Seitenfuellend einpassen, Seitenverhaeltnis bleibt erhalten.
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
        statusBar()->showMessage(tr("Kein Objekt ausgewaehlt"), 3000);
        return;
    }
    const int count = m_scene->selectedItems().size();
    m_scene->removeSelectedObjects();
    statusBar()->showMessage(tr("%n Objekt(e) geloescht", nullptr, count), 3000);
}

// ------------------------------------------------------------------- Zustand

bool MainWindow::maybeSave()
{
    if (!m_dirty)
        return true;

    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this, tr("Inkpot"),
        tr("Die Zeichnung wurde geaendert.\nMoechtest Du die Aenderungen speichern?"),
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
        QMessageBox::warning(this, tr("Oeffnen"),
                             tr("Die Datei %1 konnte nicht gelesen werden.")
                                 .arg(QFileInfo(path).fileName()));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, tr("Oeffnen"),
                             tr("Die Datei %1 ist beschaedigt: %2")
                                 .arg(QFileInfo(path).fileName(), parseError.errorString()));
        return false;
    }

    QString error;
    if (!m_scene->fromJson(document.object(), &error)) {
        QMessageBox::warning(this, tr("Oeffnen"), error);
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
    statusBar()->showMessage(tr("%1 geoeffnet").arg(QFileInfo(path).fileName()), 4000);
    return true;
}

bool MainWindow::loadImageFile(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Oeffnen"),
                             tr("Das Bild %1 konnte nicht geladen werden: %2")
                                 .arg(QFileInfo(path).fileName(), reader.errorString()));
        return false;
    }

    // Das Bild wird ein Objekt auf einer passend grossen Arbeitsflaeche.
    m_scene->clearObjects();
    m_scene->setPageRect(QRectF(QPointF(0.0, 0.0), QSizeF(image.size())));

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

    m_ratioLabel->setText(tr("Verhaeltnis: %1").arg(aspectRatioText(size)));
}

void MainWindow::updateObjectInfo()
{
    const int total = m_scene->objectCount();
    const int selected = m_scene->selectedItems().size();
    m_objectLabel->setText(selected > 0 ? tr("Objekte: %1 (%2 ausgewaehlt)").arg(total).arg(selected)
                                        : tr("Objekte: %1").arg(total));
}

void MainWindow::updateZoomInfo(double factor)
{
    m_zoomLabel->setText(tr("Zoom: %1 %").arg(qRound(factor * 100.0)));
}

void MainWindow::chooseBrushColor()
{
    const QColor color = QColorDialog::getColor(m_view->brushColor(), this, tr("Pinselfarbe"));
    if (!color.isValid())
        return;
    m_view->setBrushColor(color);
    updateColorButton();
}

void MainWindow::updateColorButton()
{
    QPixmap pixmap(16, 16);
    pixmap.fill(m_view->brushColor());
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(0x40, 0x40, 0x44), 1.0));
    painter.drawRect(0, 0, 15, 15);
    painter.end();
    m_colorButton->setIcon(QIcon(pixmap));
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

    // Gekuerzt zu unuebersichtlich: an gaengige Verhaeltnisse annaehern.
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
