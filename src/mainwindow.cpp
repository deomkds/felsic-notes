#include "mainwindow.h"
#include <QDir>
#include <QVBoxLayout>
#include <QToolBar>
#include <QIcon>
#include <QSettings>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QDateTime>
#include <QVBoxLayout>
#include <QSettings>
#include <QIcon>
#include <QStyle>
#include <QPair>
#include <QAction>
#include <QCloseEvent>
#include <QMenu>
#include <QInputDialog>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include "file_filter_model.h"
#include "pdf_generator.h"
#include "customize_toolbar_dialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    createActions();
    
    // Default toolbar layout
    currentToolbarLayout = QStringList{
        "save_file", "save_as", "export_pdf", "spacer",
        "zoom_in", "zoom_out", "separator", "bold", "italic", "link", "code", "spacer",
        "preview", "wrap_text"
    };
    buildToolbar();
    
    pdfGen = new PdfGenerator(this);
    connect(pdfGen, &PdfGenerator::finished, this, &MainWindow::onPdfGenerated);
    
    // Set up window icon
    setWindowIcon(QIcon(":/window.png"));
    
    // Load global settings
    QSettings globalSettings("Felsic", "FelsicNotes");
    QString lastWorkspace = globalSettings.value("last_workspace", "").toString();
    
    if (!lastWorkspace.isEmpty() && QDir(lastWorkspace).exists()) {
        fileModel->setRootPath(lastWorkspace);
        proxyModel->updateWorkspaceIndex(lastWorkspace);
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(lastWorkspace)));
        loadWorkspaceSettings(lastWorkspace);
    } else {
        // Fallback to home dir
        QString home = QDir::homePath();
        fileModel->setRootPath(home);
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(home)));
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Felsic Notes"));
    
    // Toolbar
    mainToolBar = addToolBar(tr("Main Toolbar"));
    mainToolBar->setObjectName("MainToolBar");
    mainToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    mainToolBar->setMovable(false);
    
    // --- Layout setup ---
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side container (Search + Tree)
    QWidget *leftContainer = new QWidget(mainSplitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Search Box
    searchBox = new QLineEdit(leftContainer);
    searchBox->setPlaceholderText(tr("Search notes..."));
    searchBox->setClearButtonEnabled(true);
    searchBox->setStyleSheet("QLineEdit { padding: 5px; border-radius: 4px; border: 1px solid #ccc; margin: 4px; }");
    connect(searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchChanged);
    leftLayout->addWidget(searchBox);
    
    // Tree view
    treeView = new QTreeView(leftContainer);
    leftLayout->addWidget(treeView);
    fileModel = new QFileSystemModel(this);
    
    // Crucial: Filter for directories and files, no . and ..
    fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    fileModel->setNameFilters(QStringList() << "*.md");
    fileModel->setNameFilterDisables(false);
    
    // We will setRootPath later when a workspace is loaded, to avoid watching the whole OS
    
    proxyModel = new FileFilterProxyModel(this);
    proxyModel->setSourceModel(fileModel);
    
    // Only show Markdown files
    proxyModel->setFilterRegularExpression(QRegularExpression("\\.md$", QRegularExpression::CaseInsensitiveOption));
    
    treeView->setModel(proxyModel);
    treeView->setColumnHidden(1, true); // hide size
    treeView->setColumnHidden(2, true); // hide type
    treeView->setColumnHidden(3, true); // hide date modified
    
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showTreeContextMenu);
    
    // Hide standard file system columns except Name
    for (int col = 1; col < 4; ++col) {
        treeView->hideColumn(col);
    }
    treeView->setHeaderHidden(true);

    // Right side container
    QWidget *rightContainer = new QWidget(mainSplitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    // Title Box
    titleBox = new QLineEdit(rightContainer);
    titleBox->setPlaceholderText(tr("Note Title..."));
    QFont titleFont = titleBox->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleBox->setFont(titleFont);
    titleBox->setStyleSheet("border: none; padding: 10px; background-color: transparent;");
    connect(titleBox, &QLineEdit::textChanged, this, &MainWindow::onTitleChanged);
    
    rightLayout->addWidget(titleBox);

    // Right side: Editor & Preview Stacked Widget
    stackedWidget = new QStackedWidget(rightContainer);
    
    // Page 0: Editor
    editor = new QPlainTextEdit(stackedWidget);
    QFont font("Consolas", 11);
    editor->setFont(font);

    // Page 1: Preview
    preview = new QTextBrowser(stackedWidget);
    
    // Page 2: Multi-Select (Empty placeholder for now)
    multiSelectView = new QWidget(stackedWidget);

    stackedWidget->addWidget(editor);
    stackedWidget->addWidget(preview);
    stackedWidget->addWidget(multiSelectView);

    stackedWidget->setCurrentIndex(0); // Start with editor
    rightLayout->addWidget(stackedWidget);

    setCentralWidget(mainSplitter);
    
    // Connect Editor text changed to Markdown parser
    connect(editor, &QPlainTextEdit::textChanged, this, &MainWindow::onEditorTextChanged);
    
    // Connect Tree View selection to file opener
    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelected);
    
    // Status Bar
    statsLabel = new QLabel(this);
    statusBar()->addPermanentWidget(statsLabel);
    
    statsTimer = new QTimer(this);
    statsTimer->setSingleShot(true);
    statsTimer->setInterval(500);
    connect(statsTimer, &QTimer::timeout, this, &MainWindow::updateStats);
    
    connect(editor, &QPlainTextEdit::textChanged, statsTimer, qOverload<>(&QTimer::start));
    updateStats();
    
    // Search Timer
    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(300);
    connect(searchTimer, &QTimer::timeout, this, &MainWindow::applySearch);
}

void MainWindow::createActions()
{
    // File Menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    QIcon newIcon = QIcon::fromTheme("document-new", style()->standardIcon(QStyle::SP_FileIcon));
    newAction = new QAction(newIcon, tr("&New File"), this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    fileMenu->addAction(newAction);
    
    QIcon openIcon = QIcon::fromTheme("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton));
    openAction = new QAction(openIcon, tr("&Open File..."), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addAction(openAction);
    
    QIcon folderIcon = QIcon::fromTheme("folder-open", style()->standardIcon(QStyle::SP_DirOpenIcon));
    openFolderAction = new QAction(folderIcon, tr("Open &Folder..."), this);
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::openFolder);
    fileMenu->addAction(openFolderAction);
    
    fileMenu->addSeparator();
    
    QIcon saveIcon = QIcon::fromTheme("document-save", style()->standardIcon(QStyle::SP_DialogSaveButton));
    saveAction = new QAction(saveIcon, tr("&Save"), this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    fileMenu->addAction(saveAction);
    
    QIcon saveAsIcon = QIcon::fromTheme("document-save-as", style()->standardIcon(QStyle::SP_DialogSaveButton));
    saveAsAction = new QAction(saveAsIcon, tr("Save &As..."), this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    fileMenu->addAction(saveAsAction);
    
    QIcon pdfIcon = QIcon::fromTheme("application-pdf-symbolic", QIcon::fromTheme("application-pdf", style()->standardIcon(QStyle::SP_DriveFDIcon)));
    exportPdfAction = new QAction(pdfIcon, tr("Export to &PDF"), this);
    connect(exportPdfAction, &QAction::triggered, this, &MainWindow::exportToPdf);
    fileMenu->addAction(exportPdfAction);
    
    fileMenu->addSeparator();
    
    QIcon exitIcon = QIcon::fromTheme("application-exit", style()->standardIcon(QStyle::SP_DialogCloseButton));
    exitAction = new QAction(exitIcon, tr("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    // Edit Menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    
    QIcon boldIcon = QIcon::fromTheme("format-text-bold");
    boldAction = new QAction(boldIcon, tr("&Bold"), this);
    boldAction->setShortcut(QKeySequence::Bold);
    connect(boldAction, &QAction::triggered, this, &MainWindow::insertBold);
    editMenu->addAction(boldAction);
    
    QIcon italicIcon = QIcon::fromTheme("format-text-italic");
    italicAction = new QAction(italicIcon, tr("&Italic"), this);
    italicAction->setShortcut(QKeySequence::Italic);
    connect(italicAction, &QAction::triggered, this, &MainWindow::insertItalic);
    editMenu->addAction(italicAction);
    
    QIcon linkIcon = QIcon::fromTheme("insert-link", style()->standardIcon(QStyle::SP_ArrowRight));
    linkAction = new QAction(linkIcon, tr("&Link"), this);
    linkAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    connect(linkAction, &QAction::triggered, this, &MainWindow::insertLink);
    editMenu->addAction(linkAction);
    
    QIcon codeIcon = QIcon::fromTheme("format-text-code");
    if (codeIcon.isNull()) codeIcon = QIcon::fromTheme("text-x-script", style()->standardIcon(QStyle::SP_FileIcon));
    codeAction = new QAction(codeIcon, tr("&Code"), this);
    codeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(codeAction, &QAction::triggered, this, &MainWindow::insertCode);
    editMenu->addAction(codeAction);
    
    editMenu->addSeparator();
    
    upperAction = new QAction(tr("UPPER CASE"), this);
    upperAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    connect(upperAction, &QAction::triggered, this, &MainWindow::changeCaseUpper);
    editMenu->addAction(upperAction);
    
    lowerAction = new QAction(tr("lower case"), this);
    lowerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_U));
    connect(lowerAction, &QAction::triggered, this, &MainWindow::changeCaseLower);
    editMenu->addAction(lowerAction);
    
    titleAction = new QAction(tr("Title Case"), this);
    titleAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_U));
    connect(titleAction, &QAction::triggered, this, &MainWindow::changeCaseTitle);
    editMenu->addAction(titleAction);
    
    sentenceAction = new QAction(tr("Sentence case"), this);
    connect(sentenceAction, &QAction::triggered, this, &MainWindow::changeCaseSentence);
    editMenu->addAction(sentenceAction);
    
    // View Menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    
    QIcon previewIcon = QIcon::fromTheme("view-preview", style()->standardIcon(QStyle::SP_DesktopIcon));
    togglePreviewAction = new QAction(previewIcon, tr("Toggle &Preview"), this);
    togglePreviewAction->setCheckable(true);
    togglePreviewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(togglePreviewAction, &QAction::toggled, this, &MainWindow::togglePreview);
    viewMenu->addAction(togglePreviewAction);
    
    QIcon wrapIcon = QIcon::fromTheme("format-text-wrap", style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    wrapTextAction = new QAction(wrapIcon, tr("&Word Wrap"), this);
    wrapTextAction->setCheckable(true);
    wrapTextAction->setChecked(true);
    connect(wrapTextAction, &QAction::toggled, this, [this](bool checked) {
        editor->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    });
    viewMenu->addAction(wrapTextAction);
    
    hideEmptyAction = new QAction(tr("&Hide Empty Folders"), this);
    hideEmptyAction->setCheckable(true);
    connect(hideEmptyAction, &QAction::toggled, this, [this](bool checked) {
        proxyModel->setHideEmptyFolders(checked);
    });
    viewMenu->addAction(hideEmptyAction);
    
    viewMenu->addSeparator();
    
    QIcon zoomInIcon = QIcon::fromTheme("zoom-in");
    zoomInAction = new QAction(zoomInIcon, tr("Zoom &In"), this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    viewMenu->addAction(zoomInAction);
    
    QIcon zoomOutIcon = QIcon::fromTheme("zoom-out");
    zoomOutAction = new QAction(zoomOutIcon, tr("Zoom &Out"), this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    viewMenu->addAction(zoomOutAction);
    
    viewMenu->addSeparator();
    
    QAction *customizeToolbarAction = new QAction(tr("Customize &Toolbar..."), this);
    connect(customizeToolbarAction, &QAction::triggered, this, &MainWindow::customizeToolbar);
    viewMenu->addAction(customizeToolbarAction);

    // Help Menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    QIcon aboutIcon = QIcon::fromTheme("help-about");
    aboutAction = new QAction(aboutIcon, tr("&About Felsic Notes"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAction);
    
    // Populate Catalog
    catalog.insert("new_file", qMakePair(tr("New File"), newAction));
    catalog.insert("open_file", qMakePair(tr("Open File..."), openAction));
    catalog.insert("open_folder", qMakePair(tr("Open Folder..."), openFolderAction));
    catalog.insert("exit", qMakePair(tr("Quit"), exitAction));
    catalog.insert("save_file", qMakePair(tr("Save"), saveAction));
    catalog.insert("save_as", qMakePair(tr("Save As..."), saveAsAction));
    catalog.insert("export_pdf", qMakePair(tr("Export to PDF"), exportPdfAction));
    
    catalog.insert("zoom_in", qMakePair(tr("Zoom In"), zoomInAction));
    catalog.insert("zoom_out", qMakePair(tr("Zoom Out"), zoomOutAction));
    catalog.insert("bold", qMakePair(tr("Bold"), boldAction));
    catalog.insert("italic", qMakePair(tr("Italic"), italicAction));
    catalog.insert("link", qMakePair(tr("Link"), linkAction));
    catalog.insert("code", qMakePair(tr("Code"), codeAction));
    
    catalog.insert("upper_case", qMakePair(tr("UPPER CASE"), upperAction));
    catalog.insert("lower_case", qMakePair(tr("lower case"), lowerAction));
    catalog.insert("title_case", qMakePair(tr("Title Case"), titleAction));
    catalog.insert("sentence_case", qMakePair(tr("Sentence case"), sentenceAction));
    
    catalog.insert("preview", qMakePair(tr("Toggle Preview"), togglePreviewAction));
    catalog.insert("wrap_text", qMakePair(tr("Toggle Wrap"), wrapTextAction));
    catalog.insert("hide_empty_folders", qMakePair(tr("Hide Empty Folders"), hideEmptyAction));
    catalog.insert("about", qMakePair(tr("About"), aboutAction));
    
    catalog.insert("spacer", qMakePair(tr("Space (Align Right)"), nullptr));
    catalog.insert("separator", qMakePair(tr("Vertical Separator"), nullptr));
}

void MainWindow::buildToolbar()
{
    mainToolBar->clear();
    for (const QString &itemId : currentToolbarLayout) {
        if (itemId == "spacer") {
            QWidget *spacer = new QWidget(this);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            mainToolBar->addWidget(spacer);
        } else if (itemId == "separator") {
            mainToolBar->addSeparator();
        } else {
            if (catalog.contains(itemId)) {
                QAction *act = catalog.value(itemId).second;
                if (act) {
                    mainToolBar->addAction(act);
                }
            }
        }
    }
}

void MainWindow::onEditorTextChanged()
{
    // Qt's QTextBrowser has built-in Markdown support!
    preview->setMarkdown(editor->toPlainText());
}

void MainWindow::exportToPdf()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export PDF"), QDir::homePath(), tr("PDF Files (*.pdf)"));
    if (filePath.isEmpty()) return;
    
    // Convert current Markdown to HTML for PDF generation
    QString html = preview->toHtml();
    pdfGen->generatePdf(html, filePath);
}

void MainWindow::onPdfGenerated(bool success, const QString &outputPath)
{
    if (success) {
        QMessageBox::information(this, tr("Success"), tr("PDF exported successfully to:\n") + outputPath);
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to export PDF to:\n") + outputPath);
    }
}

void MainWindow::onFileSelected(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (selected.indexes().isEmpty()) return;
    
    // Reset toggle preview if it's checked
    if (togglePreviewAction->isChecked()) {
        togglePreviewAction->setChecked(false);
        stackedWidget->setCurrentIndex(0);
    }
    
    QModelIndex index = selected.indexes().first();
    QFileSystemModel *model = qobject_cast<QFileSystemModel*>(proxyModel->sourceModel());
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    
    if (model->isDir(sourceIndex)) return;
    
    QString path = model->filePath(sourceIndex);
    if (path.isEmpty()) return;
    
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        currentFilePath = path;
        
        titleBox->blockSignals(true);
        titleBox->setText(QFileInfo(path).baseName());
        titleBox->blockSignals(false);
        
        editor->setPlainText(QString::fromUtf8(file.readAll()));
        file.close();
        updateStats();
    }
}

void MainWindow::newFile()
{
    if (editor->document()->isModified()) {
        QMessageBox::StandardButton res = QMessageBox::warning(this, tr("Unsaved Changes"),
            tr("You have unsaved changes. Do you want to save before creating a new file?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (res == QMessageBox::Yes) {
            saveFile();
        } else if (res == QMessageBox::Cancel) {
            return;
        }
    }
    
    currentFilePath.clear();
    titleBox->blockSignals(true);
    titleBox->setText("");
    titleBox->blockSignals(false);
    
    editor->clear();
    editor->document()->setModified(false);
    
    // Unselect anything in tree
    treeView->clearSelection();
    
    updateStats();
}

void MainWindow::saveFile()
{
    QString newTitle = titleBox->text().trimmed();
    if (newTitle.isEmpty()) newTitle = "Untitled";

    if (currentFilePath.isEmpty()) {
        // Not working on a file yet. Target the workspace folder.
        QSettings settings("Felsic", "FelsicNotes");
        QString lastWorkspace = settings.value("last_workspace", "").toString();
        
        if (!lastWorkspace.isEmpty()) {
            QString destPath = QDir(lastWorkspace).filePath(newTitle + ".md");
            if (QFile::exists(destPath)) {
                QMessageBox::warning(this, tr("Error"), tr("A note with this name already exists in the workspace."));
                return;
            }
            currentFilePath = destPath;
        } else {
            saveFileAs();
            return;
        }
    } else {
        // Working on an existing file
        QString currentBasename = QFileInfo(currentFilePath).baseName();
        if (newTitle != currentBasename) {
            // Name changed visually
            QString destPath = QFileInfo(currentFilePath).dir().filePath(newTitle + ".md");
            if (QFile::exists(destPath)) {
                QMessageBox::warning(this, tr("Error"), tr("A note with this name already exists. Choose another title."));
                return;
            }
            
            // Try to rename on disk
            if (QFile::rename(currentFilePath, destPath)) {
                currentFilePath = destPath;
            } else {
                QMessageBox::warning(this, tr("Error"), tr("Could not rename the file on disk."));
                return;
            }
        }
    }

    QFile file(currentFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(editor->toPlainText().toUtf8());
        file.close();
        editor->document()->setModified(false);
        updateStats();
        // Update tree view index
        proxyModel->addToIndex(currentFilePath);
        
        // Save config with note
        saveWorkspaceSettings();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not save the file."));
    }
}

void MainWindow::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Workspace Folder"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        // Save current workspace settings before switching
        QSettings globalSettings("Felsic", "FelsicNotes");
        QString lastWorkspace = globalSettings.value("last_workspace", "").toString();
        if (!lastWorkspace.isEmpty()) {
            saveWorkspaceSettings();
        }

        // Save to QSettings global
        globalSettings.setValue("last_workspace", dir);
        
        // Update models
        fileModel->setRootPath(dir);
        proxyModel->updateWorkspaceIndex(dir);
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(dir)));
        
        // Load new settings
        loadWorkspaceSettings(dir);
    }
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Markdown File"), QDir::homePath(), tr("Markdown Files (*.md);;All Files (*)"));
    if (path.isEmpty()) return;
    
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        currentFilePath = path;
        
        titleBox->blockSignals(true);
        titleBox->setText(QFileInfo(path).baseName());
        titleBox->blockSignals(false);
        
        editor->setPlainText(QString::fromUtf8(file.readAll()));
        file.close();
        updateStats();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not open the file."));
    }
}

void MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save File As"), QDir::homePath(), tr("Markdown Files (*.md);;All Files (*)"));
    if (path.isEmpty()) return;
    
    currentFilePath = path;
    saveFile();
    proxyModel->addToIndex(path);
}

void MainWindow::zoomIn()
{
    if (currentFontSize < 48) {
        currentFontSize += 1;
        applyFontSize();
    }
}

void MainWindow::zoomOut()
{
    if (currentFontSize > 6) {
        currentFontSize -= 1;
        applyFontSize();
    }
}

void MainWindow::applyFontSize()
{
    QFont f = editor->font();
    f.setPointSize(currentFontSize);
    editor->setFont(f);
    
    QFont fPreview = preview->font();
    fPreview.setPointSize(currentFontSize);
    preview->setFont(fPreview);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About Felsic Notes"),
        tr("<h3>Felsic Notes</h3>"
           "<p>A fast, portable, and lightweight Markdown note-taking app.</p>"
           "<p>Built with Qt6 and copious amounts of AI.</p>"
           "<p><a href=\"https://github.com/deomkds/felsic-notes\">GitHub Repository</a></p>"));
}

void MainWindow::customizeToolbar()
{
    CustomizeToolbarDialog dialog(catalog, currentToolbarLayout, this);
    if (dialog.exec() == QDialog::Accepted) {
        currentToolbarLayout = dialog.getLayout();
        buildToolbar();
    }
}

void MainWindow::togglePreview(bool checked)
{
    if (checked) {
        // Convert to markdown and switch to preview page
        preview->setMarkdown(editor->toPlainText());
        stackedWidget->setCurrentIndex(1);
    } else {
        // Switch back to editor
        stackedWidget->setCurrentIndex(0);
    }
}

void MainWindow::onTitleChanged()
{
    if (!editor->document()->isModified()) {
        editor->document()->setModified(true);
        updateStats();
    }
}

void MainWindow::updateStats()
{
    QString text = editor->toPlainText();
    int chars = text.length();
    int words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    
    QString status = editor->document()->isModified() ? tr("Unsaved") : tr("Saved");
    QString sizeStr = "";
    QString dateInfo = "";
    
    if (!currentFilePath.isEmpty() && QFile::exists(currentFilePath)) {
        QFileInfo info(currentFilePath);
        qint64 sizeBytes = info.size();
        if (sizeBytes < 1024) sizeStr = QString("  |  %1: %2 B").arg(tr("Size")).arg(sizeBytes);
        else if (sizeBytes < 1024 * 1024) sizeStr = QString("  |  %1: %2 KB").arg(tr("Size")).arg(sizeBytes / 1024.0, 0, 'f', 1);
        else sizeStr = QString("  |  %1: %2 MB").arg(tr("Size")).arg(sizeBytes / (1024.0 * 1024.0), 0, 'f', 2);
        
        QString cTime = info.birthTime().toString("dd/MM/yyyy HH:mm");
        QString mTime = info.lastModified().toString("dd/MM/yyyy HH:mm");
        dateInfo = QString("  |  %1: %2  |  %3: %4").arg(tr("Created")).arg(cTime).arg(tr("Modified")).arg(mTime);
    }
    
    QString wordText = (words == 1) ? tr("1 word") : tr("%1 words").arg(words);
    QString charText = (chars == 1) ? tr("1 character") : tr("%1 characters").arg(chars);
    
    statsLabel->setText(QString("%1%2  |  %3  |  %4%5").arg(status, dateInfo, wordText, charText, sizeStr));
}

void MainWindow::onSearchChanged(const QString &text)
{
    Q_UNUSED(text);
    searchTimer->start();
}

void MainWindow::applySearch()
{
    QString text = searchBox->text();
    // In C++, the proxy model is currently matching BOTH extension (.md) and search text.
    // However, our QFileSystemModel already strictly filters out non .md files.
    // So we can just safely search everything against the filename.
    QRegularExpression regex(text, QRegularExpression::CaseInsensitiveOption);
    proxyModel->setFilterRegularExpression(regex);
}

// --- Formatting Slots ---

void MainWindow::insertBold()
{
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    cursor.insertText("**" + text + "**");
    if (text.isEmpty()) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
        editor->setTextCursor(cursor);
    }
}

void MainWindow::insertItalic()
{
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    cursor.insertText("*" + text + "*");
    if (text.isEmpty()) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        editor->setTextCursor(cursor);
    }
}

void MainWindow::insertLink()
{
    QTextCursor cursor = editor->textCursor();
    QString url = cursor.selectedText();
    cursor.insertText("[](" + url + ")");
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, url.length() + 3);
    editor->setTextCursor(cursor);
}

void MainWindow::insertCode()
{
    QTextCursor cursor = editor->textCursor();
    QString text = cursor.selectedText();
    if (text.contains("\u2029")) { // Multiline selection has paragraph separators in Qt
        cursor.insertText("```\n" + text.replace("\u2029", "\n") + "\n```");
    } else {
        cursor.insertText("`" + text + "`");
        if (text.isEmpty()) {
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
            editor->setTextCursor(cursor);
        }
    }
}

void MainWindow::changeCaseUpper()
{
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection()) return;
    cursor.insertText(cursor.selectedText().toUpper());
}

void MainWindow::changeCaseLower()
{
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection()) return;
    cursor.insertText(cursor.selectedText().toLower());
}

void MainWindow::changeCaseTitle()
{
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection()) return;
    
    QString text = cursor.selectedText();
    QString titleCase;
    bool nextUpper = true;
    for (int i = 0; i < text.length(); ++i) {
        if (text[i].isSpace()) {
            nextUpper = true;
            titleCase += text[i];
        } else if (nextUpper) {
            titleCase += text[i].toUpper();
            nextUpper = false;
        } else {
            titleCase += text[i].toLower();
        }
    }
    cursor.insertText(titleCase);
}

void MainWindow::changeCaseSentence()
{
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection()) return;
    
    QString text = cursor.selectedText().toLower();
    if (text.length() > 0) {
        text[0] = text[0].toUpper();
    }
    cursor.insertText(text);
}

void MainWindow::loadWorkspaceSettings(const QString &workspacePath)
{
    QDir workspaceDir(workspacePath);
    QString configPath = workspaceDir.filePath(".felsic/config.ini");
    if (!QFile::exists(configPath)) {
        return; // Use defaults
    }
    
    QSettings localSettings(configPath, QSettings::IniFormat);
    
    // Load geometry
    QByteArray geometry = localSettings.value("geometry").toByteArray();
    if (!geometry.isEmpty()) restoreGeometry(geometry);
    
    // Load window state
    QByteArray state = localSettings.value("windowState").toByteArray();
    if (!state.isEmpty()) restoreState(state);
    
    // Load splitter state
    QByteArray splitterState = localSettings.value("splitterState").toByteArray();
    if (!splitterState.isEmpty()) mainSplitter->restoreState(splitterState);
    
    // Load toolbar
    QStringList savedLayout = localSettings.value("toolbar_layout").toStringList();
    if (!savedLayout.isEmpty()) {
        currentToolbarLayout = savedLayout;
        buildToolbar();
    }
    
    // Load font size
    int savedFontSize = localSettings.value("font_size", 0).toInt();
    if (savedFontSize > 0) {
        currentFontSize = savedFontSize;
        applyFontSize();
    }
}

void MainWindow::saveWorkspaceSettings()
{
    QSettings globalSettings("Felsic", "FelsicNotes");
    QString currentWorkspace = globalSettings.value("last_workspace", "").toString();
    if (currentWorkspace.isEmpty()) return;
    
    QDir workspaceDir(currentWorkspace);
    if (!workspaceDir.exists(".felsic")) {
        workspaceDir.mkdir(".felsic");
    }
    
    QString configPath = workspaceDir.filePath(".felsic/config.ini");
    QSettings localSettings(configPath, QSettings::IniFormat);
    
    localSettings.setValue("geometry", saveGeometry());
    localSettings.setValue("windowState", saveState());
    localSettings.setValue("splitterState", mainSplitter->saveState());
    localSettings.setValue("toolbar_layout", currentToolbarLayout);
    localSettings.setValue("font_size", currentFontSize);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Check if there are unsaved changes
    if (editor->document()->isModified()) {
        QMessageBox::StandardButton res = QMessageBox::warning(this, tr("Unsaved Changes"),
            tr("You have unsaved changes. Do you want to save before closing?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        
        if (res == QMessageBox::Yes) {
            saveFile();
        } else if (res == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    saveWorkspaceSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::showTreeContextMenu(const QPoint &pos)
{
    QModelIndex proxyIndex = treeView->indexAt(pos);
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    
    QSettings globalSettings("Felsic", "FelsicNotes");
    QString lastWorkspace = globalSettings.value("last_workspace", "").toString();
    if (lastWorkspace.isEmpty()) return;
    
    QMenu menu;
    
    if (sourceIndex.isValid() && !fileModel->isDir(sourceIndex)) {
        // File context menu
        QString filePath = fileModel->filePath(sourceIndex);
        QString baseDir = QFileInfo(filePath).path();
        
        QAction *newNoteAction = menu.addAction(tr("New Note..."));
        connect(newNoteAction, &QAction::triggered, this, [=]() { createNewNote(baseDir); });
        
        QAction *newFolderAction = menu.addAction(tr("New Folder..."));
        connect(newFolderAction, &QAction::triggered, this, [=]() { createNewFolder(baseDir); });
        
        menu.addSeparator();
        
        QAction *renameAction = menu.addAction(tr("Rename..."));
        connect(renameAction, &QAction::triggered, this, [=]() { renameNote(filePath); });
        
        QAction *moveAction = menu.addAction(tr("Move To..."));
        connect(moveAction, &QAction::triggered, this, [=]() { moveNote(filePath); });
        
        QAction *dupAction = menu.addAction(tr("Duplicate"));
        connect(dupAction, &QAction::triggered, this, [=]() { duplicateNote(filePath); });
        
        menu.addSeparator();
        
        QAction *deleteAction = menu.addAction(tr("Delete"));
        connect(deleteAction, &QAction::triggered, this, [=]() { deleteNote(filePath); });
        
        menu.addSeparator();
        
        QAction *revealAction = menu.addAction(tr("Reveal in File Explorer"));
        connect(revealAction, &QAction::triggered, this, [=]() { revealInExplorer(filePath); });
        
    } else {
        // Folder or empty space context menu
        QString baseDir = lastWorkspace;
        if (sourceIndex.isValid() && fileModel->isDir(sourceIndex)) {
            baseDir = fileModel->filePath(sourceIndex);
        }
        
        QAction *newNoteAction = menu.addAction(tr("New Note..."));
        connect(newNoteAction, &QAction::triggered, this, [=]() { createNewNote(baseDir); });
        
        QAction *newFolderAction = menu.addAction(tr("New Folder..."));
        connect(newFolderAction, &QAction::triggered, this, [=]() { createNewFolder(baseDir); });
        
        menu.addSeparator();
        
        if (sourceIndex.isValid() && fileModel->isDir(sourceIndex)) {
            QAction *renameDirAction = menu.addAction(tr("Rename Folder..."));
            connect(renameDirAction, &QAction::triggered, this, [=]() { renameFolder(baseDir); });
            
            QAction *moveDirAction = menu.addAction(tr("Move Folder To..."));
            connect(moveDirAction, &QAction::triggered, this, [=]() { moveFolder(baseDir); });
            
            menu.addSeparator();
            
            QAction *expandAction = menu.addAction(tr("Expand All"));
            connect(expandAction, &QAction::triggered, this, [=]() { expandAll(proxyIndex); });
            
            QAction *collapseAction = menu.addAction(tr("Collapse All"));
            connect(collapseAction, &QAction::triggered, this, [=]() { collapseAll(proxyIndex); });
            
            menu.addSeparator();
            
            QAction *deleteDirAction = menu.addAction(tr("Delete Folder"));
            connect(deleteDirAction, &QAction::triggered, this, [=]() { deleteFolder(baseDir); });
        }
    }
    
    menu.exec(treeView->viewport()->mapToGlobal(pos));
}

void MainWindow::createNewNote(const QString &baseDir)
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("New Note"), tr("Note Name:"), QLineEdit::Normal, "", &ok);
    if (ok && !text.trimmed().isEmpty()) {
        QString filename = text.trimmed();
        if (!filename.endsWith(".md") && !filename.contains('.')) {
            filename += ".md";
        }
        
        QString filepath = QDir(baseDir).filePath(filename);
        if (QFile::exists(filepath)) {
            QMessageBox::warning(this, tr("Error"), tr("A file with this name already exists."));
            return;
        }
        
        QFile file(filepath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write("");
            file.close();
            proxyModel->addToIndex(filepath);
            // Load if current editor is clear or unsaved is handled
            if (!editor->document()->isModified() || QMessageBox::question(this, tr("Unsaved Changes"), tr("Save current file?")) == QMessageBox::Yes) {
                if (editor->document()->isModified()) saveFile();
                
                // load file
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    editor->setPlainText(file.readAll());
                    file.close();
                    currentFilePath = filepath;
                    editor->document()->setModified(false);
                    onTitleChanged();
                }
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to create note."));
        }
    }
}

void MainWindow::createNewFolder(const QString &baseDir)
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("New Folder"), tr("Folder Name:"), QLineEdit::Normal, "", &ok);
    if (ok && !text.trimmed().isEmpty()) {
        QString foldername = text.trimmed();
        QString filepath = QDir(baseDir).filePath(foldername);
        
        if (QFile::exists(filepath)) {
            QMessageBox::warning(this, tr("Error"), tr("A folder with this name already exists."));
            return;
        }
        
        QDir dir;
        if (dir.mkpath(filepath)) {
            proxyModel->addToIndex(filepath);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to create folder."));
        }
    }
}

void MainWindow::renameNote(const QString &sourcePath)
{
    QFileInfo info(sourcePath);
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Note"), tr("New Name:"), QLineEdit::Normal, info.fileName(), &ok);
    
    if (ok && !newName.trimmed().isEmpty() && newName != info.fileName()) {
        newName = newName.trimmed();
        if (!newName.endsWith(".md") && !newName.contains('.')) {
            newName += ".md";
        }
        
        QString destPath = info.dir().filePath(newName);
        if (QFile::exists(destPath)) {
            QMessageBox::warning(this, tr("Error"), tr("A file with this name already exists."));
            return;
        }
        
        if (QFile::rename(sourcePath, destPath)) {
            if (currentFilePath == sourcePath) {
                currentFilePath = destPath;
                onTitleChanged();
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to rename note."));
        }
    }
}

void MainWindow::moveNote(const QString &sourcePath)
{
    QSettings globalSettings("Felsic", "FelsicNotes");
    QString lastWorkspace = globalSettings.value("last_workspace", QDir::homePath()).toString();
    QString destFolder = QFileDialog::getExistingDirectory(this, tr("Select Destination Folder"), lastWorkspace);
    
    if (!destFolder.isEmpty()) {
        QFileInfo info(sourcePath);
        QString destPath = QDir(destFolder).filePath(info.fileName());
        
        if (QFile::exists(destPath)) {
            if (destPath != sourcePath) {
                QMessageBox::warning(this, tr("Error"), tr("A file with this name already exists in the destination."));
            }
            return;
        }
        
        if (QFile::rename(sourcePath, destPath)) {
            if (currentFilePath == sourcePath) {
                currentFilePath = destPath;
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to move note."));
        }
    }
}

void MainWindow::duplicateNote(const QString &sourcePath)
{
    QFileInfo info(sourcePath);
    QString baseDir = info.dir().path();
    QString baseName = info.completeBaseName();
    QString ext = info.suffix().isEmpty() ? "" : "." + info.suffix();
    
    int counter = 1;
    QString suffix = " (copy)";
    QString newName;
    QString destPath;
    
    while (true) {
        newName = baseName + suffix + ext;
        destPath = QDir(baseDir).filePath(newName);
        if (!QFile::exists(destPath)) break;
        counter++;
        suffix = QString(" (copy %1)").arg(counter);
    }
    
    if (QFile::copy(sourcePath, destPath)) {
        // Auto indexed by QFileSystemModel
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to duplicate note."));
    }
}

void MainWindow::deleteNote(const QString &sourcePath)
{
    QMessageBox::StandardButton reply = QMessageBox::warning(this, tr("Confirm Delete"), 
        tr("Are you sure you want to permanently delete:\n%1?").arg(QFileInfo(sourcePath).fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
    if (reply == QMessageBox::Yes) {
        if (QFile::remove(sourcePath)) {
            if (currentFilePath == sourcePath) {
                editor->clear();
                titleBox->clear();
                currentFilePath.clear();
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to delete note."));
        }
    }
}

void MainWindow::revealInExplorer(const QString &path)
{
#if defined(Q_OS_WIN)
    QString param = QString("/select,") + QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer.exe", QStringList() << param);
#elif defined(Q_OS_MAC)
    QProcess::startDetached("open", QStringList() << "-R" << path);
#else
    // Linux
    QFileInfo info(path);
    QString dir = info.isDir() ? path : info.path();
    QProcess::startDetached("xdg-open", QStringList() << dir);
#endif
}

void MainWindow::renameFolder(const QString &sourcePath)
{
    QFileInfo info(sourcePath);
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Folder"), tr("New Name:"), QLineEdit::Normal, info.fileName(), &ok);
    
    if (ok && !newName.trimmed().isEmpty() && newName != info.fileName()) {
        newName = newName.trimmed();
        QString destPath = info.dir().filePath(newName);
        
        if (QFile::exists(destPath)) {
            QMessageBox::warning(this, tr("Error"), tr("A folder with this name already exists."));
            return;
        }
        
        QDir dir;
        if (dir.rename(sourcePath, destPath)) {
            proxyModel->renameDirInIndex(sourcePath, destPath);
            if (!currentFilePath.isEmpty() && (currentFilePath == sourcePath || currentFilePath.startsWith(sourcePath + "/"))) {
                currentFilePath.replace(0, sourcePath.length(), destPath);
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to rename folder."));
        }
    }
}

void MainWindow::moveFolder(const QString &sourcePath)
{
    QSettings globalSettings("Felsic", "FelsicNotes");
    QString lastWorkspace = globalSettings.value("last_workspace", QDir::homePath()).toString();
    QString destFolder = QFileDialog::getExistingDirectory(this, tr("Select Destination Folder"), lastWorkspace);
    
    if (!destFolder.isEmpty()) {
        QFileInfo info(sourcePath);
        QString destPath = QDir(destFolder).filePath(info.fileName());
        
        if (QFile::exists(destPath)) {
            if (destPath != sourcePath) {
                QMessageBox::warning(this, tr("Error"), tr("A folder with this name already exists in the destination."));
            }
            return;
        }
        
        QDir dir;
        if (dir.rename(sourcePath, destPath)) {
            proxyModel->renameDirInIndex(sourcePath, destPath);
            if (!currentFilePath.isEmpty() && (currentFilePath == sourcePath || currentFilePath.startsWith(sourcePath + "/"))) {
                currentFilePath.replace(0, sourcePath.length(), destPath);
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to move folder."));
        }
    }
}

bool MainWindow::removeDirectoryRecursively(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);
    
    if (dir.exists()) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDirectoryRecursively(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }
            
            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}

void MainWindow::deleteFolder(const QString &sourcePath)
{
    QMessageBox::StandardButton reply = QMessageBox::warning(this, tr("Confirm Delete"), 
        tr("Are you sure you want to permanently delete this folder and ALL ITS CONTENTS:\n%1?").arg(QFileInfo(sourcePath).fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
    if (reply == QMessageBox::Yes) {
        if (removeDirectoryRecursively(sourcePath)) {
            proxyModel->removeDirFromIndex(sourcePath);
            if (!currentFilePath.isEmpty() && (currentFilePath == sourcePath || currentFilePath.startsWith(sourcePath + "/"))) {
                editor->clear();
                titleBox->clear();
                currentFilePath.clear();
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to delete folder completely."));
        }
    }
}

void MainWindow::expandAll(const QModelIndex &index)
{
    if (index.isValid()) {
        treeView->expandRecursively(index);
    }
}

void MainWindow::collapseAll(const QModelIndex &index)
{
    if (index.isValid()) {
        // Recursive collapse
        treeView->collapse(index);
        int rows = proxyModel->rowCount(index);
        for (int i = 0; i < rows; ++i) {
            QModelIndex child = proxyModel->index(i, 0, index);
            if (proxyModel->hasChildren(child)) {
                collapseAll(child);
            }
        }
    }
}

