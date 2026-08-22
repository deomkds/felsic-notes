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
#include <QTextCursor>
#include <QStatusBar>
#include <QDateTime>
#include <QVBoxLayout>
#include "pdf_generator.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    createActions();
    createToolBars();
    
    pdfGen = new PdfGenerator(this);
    connect(pdfGen, &PdfGenerator::finished, this, &MainWindow::onPdfGenerated);
    
    // Set up window icon
    setWindowIcon(QIcon(":/window.png"));
    
    // Load global settings
    QSettings settings("Felsic", "FelsicNotes");
    QString lastWorkspace = settings.value("last_workspace", "").toString();
    
    if (!lastWorkspace.isEmpty() && QDir(lastWorkspace).exists()) {
        proxyModel->updateWorkspaceIndex(lastWorkspace);
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(lastWorkspace)));
    } else {
        // Fallback to home dir, but don't auto-index to avoid freezing
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(QDir::homePath())));
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Felsic Notes"));
    resize(1024, 768);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side: Tree view
    treeView = new QTreeView(mainSplitter);
    fileModel = new QFileSystemModel(this);
    
    // Crucial: Filter for directories and files, no . and ..
    fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    fileModel->setNameFilters(QStringList() << "*.md");
    fileModel->setNameFilterDisables(false);
    
    fileModel->setRootPath(""); // Monitor the whole filesystem
    
    proxyModel = new FileFilterProxyModel(this);
    proxyModel->setSourceModel(fileModel);
    
    // Only show Markdown files
    proxyModel->setFilterRegularExpression(QRegularExpression("\\.md$", QRegularExpression::CaseInsensitiveOption));
    
    treeView->setModel(proxyModel);
    
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
}

void MainWindow::createActions()
{
    // File Actions
    actionNew = new QAction(QIcon::fromTheme("document-new"), tr("&New"), this);
    actionNew->setShortcut(QKeySequence::New);
    
    actionOpen = new QAction(QIcon::fromTheme("document-open"), tr("&Open..."), this);
    actionOpen->setShortcut(QKeySequence::Open);
    connect(actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    
    actionOpenFolder = new QAction(QIcon::fromTheme("folder-open"), tr("Open &Folder..."), this);
    connect(actionOpenFolder, &QAction::triggered, this, &MainWindow::openFolder);
    
    actionSave = new QAction(QIcon::fromTheme("document-save"), tr("&Save"), this);
    actionSave->setShortcut(QKeySequence::Save);
    connect(actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    
    actionSaveAs = new QAction(QIcon::fromTheme("document-save-as"), tr("Save &As..."), this);
    actionSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    
    actionExportPdf = new QAction(QIcon::fromTheme("document-print"), tr("Export to &PDF..."), this);
    connect(actionExportPdf, &QAction::triggered, this, &MainWindow::exportToPdf);
    
    actionExit = new QAction(QIcon::fromTheme("application-exit"), tr("E&xit"), this);
    actionExit->setShortcut(QKeySequence::Quit);
    connect(actionExit, &QAction::triggered, this, &QWidget::close);
    
    // View Actions
    actionTogglePreview = new QAction(QIcon::fromTheme("view-preview"), tr("Toggle &Preview"), this);
    actionTogglePreview->setCheckable(true);
    actionTogglePreview->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(actionTogglePreview, &QAction::triggered, this, &MainWindow::togglePreview);
    
    actionToggleWrap = new QAction(tr("Word &Wrap"), this);
    actionToggleWrap->setCheckable(true);
    actionToggleWrap->setChecked(true); // Default
    connect(actionToggleWrap, &QAction::triggered, this, &MainWindow::toggleWordWrap);
    
    actionToggleHideEmpty = new QAction(tr("&Hide Empty Folders"), this);
    actionToggleHideEmpty->setCheckable(true);
    connect(actionToggleHideEmpty, &QAction::triggered, this, &MainWindow::toggleHideEmpty);
    
    actionZoomIn = new QAction(QIcon::fromTheme("zoom-in"), tr("Zoom &In"), this);
    actionZoomIn->setShortcut(QKeySequence::ZoomIn);
    connect(actionZoomIn, &QAction::triggered, this, &MainWindow::zoomIn);
    
    actionZoomOut = new QAction(QIcon::fromTheme("zoom-out"), tr("Zoom &Out"), this);
    actionZoomOut->setShortcut(QKeySequence::ZoomOut);
    connect(actionZoomOut, &QAction::triggered, this, &MainWindow::zoomOut);
    
    actionCustomizeToolbar = new QAction(tr("&Customize Toolbar..."), this);
    connect(actionCustomizeToolbar, &QAction::triggered, this, &MainWindow::customizeToolbar);
    
    // Help Actions
    actionAbout = new QAction(QIcon::fromTheme("help-about"), tr("&About"), this);
    connect(actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    
    // Formatting Actions
    actionBold = new QAction(QIcon::fromTheme("format-text-bold"), tr("&Bold"), this);
    connect(actionBold, &QAction::triggered, this, &MainWindow::insertBold);
    
    actionItalic = new QAction(QIcon::fromTheme("format-text-italic"), tr("&Italic"), this);
    connect(actionItalic, &QAction::triggered, this, &MainWindow::insertItalic);
    
    actionLink = new QAction(QIcon::fromTheme("insert-link"), tr("Insert &Link"), this);
    connect(actionLink, &QAction::triggered, this, &MainWindow::insertLink);
    
    actionCode = new QAction(QIcon::fromTheme("text-x-generic"), tr("&Code"), this);
    connect(actionCode, &QAction::triggered, this, &MainWindow::insertCode);
    
    actionUpper = new QAction(tr("&UPPERCASE"), this);
    connect(actionUpper, &QAction::triggered, this, &MainWindow::changeCaseUpper);
    
    actionLower = new QAction(tr("&lowercase"), this);
    connect(actionLower, &QAction::triggered, this, &MainWindow::changeCaseLower);
    
    actionTitle = new QAction(tr("&Title Case"), this);
    connect(actionTitle, &QAction::triggered, this, &MainWindow::changeCaseTitle);
    
    actionSentence = new QAction(tr("&Sentence case"), this);
    connect(actionSentence, &QAction::triggered, this, &MainWindow::changeCaseSentence);
    
    // Menus
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(actionNew);
    fileMenu->addAction(actionOpen);
    fileMenu->addAction(actionOpenFolder);
    fileMenu->addSeparator();
    fileMenu->addAction(actionSave);
    fileMenu->addAction(actionSaveAs);
    fileMenu->addAction(actionExportPdf);
    fileMenu->addSeparator();
    fileMenu->addAction(actionExit);
    
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(actionBold);
    editMenu->addAction(actionItalic);
    editMenu->addAction(actionLink);
    editMenu->addAction(actionCode);
    editMenu->addSeparator();
    editMenu->addAction(actionUpper);
    editMenu->addAction(actionLower);
    editMenu->addAction(actionTitle);
    editMenu->addAction(actionSentence);
    
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(actionTogglePreview);
    viewMenu->addAction(actionToggleWrap);
    viewMenu->addAction(actionToggleHideEmpty);
    viewMenu->addSeparator();
    viewMenu->addAction(actionZoomIn);
    viewMenu->addAction(actionZoomOut);
    viewMenu->addSeparator();
    viewMenu->addAction(actionCustomizeToolbar);
    
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(actionAbout);
}

void MainWindow::createToolBars()
{
    QToolBar *mainToolBar = addToolBar(tr("Main Toolbar"));
    mainToolBar->setMovable(false);
    
    mainToolBar->addAction(actionNew);
    mainToolBar->addAction(actionOpenFolder);
    mainToolBar->addAction(actionSave);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionTogglePreview);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionExportPdf);
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

void MainWindow::onFileSelected(const QItemSelection &selected, const QItemSelection &)
{
    if (selected.indexes().isEmpty()) return;
    
    // Reset toggle preview if it's checked
    if (actionTogglePreview->isChecked()) {
        actionTogglePreview->setChecked(false);
        stackedWidget->setCurrentIndex(0);
    }
    
    QModelIndex index = selected.indexes().first();
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    QString path = fileModel->filePath(sourceIndex);
    
    if (QFileInfo(path).isFile()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            currentFilePath = path;
            
            // Set title box without triggering modification
            titleBox->blockSignals(true);
            titleBox->setText(QFileInfo(path).baseName());
            titleBox->blockSignals(false);
            
            editor->setPlainText(QString::fromUtf8(file.readAll()));
            file.close();
            updateStats();
        }
    }
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
        // Save to QSettings
        QSettings settings("Felsic", "FelsicNotes");
        settings.setValue("last_workspace", dir);
        
        // Update models
        proxyModel->updateWorkspaceIndex(dir);
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(dir)));
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

void MainWindow::toggleWordWrap(bool checked)
{
    editor->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void MainWindow::toggleHideEmpty(bool checked)
{
    proxyModel->setHideEmptyFolders(checked);
}

void MainWindow::zoomIn()
{
    QFont f = editor->font();
    f.setPointSize(f.pointSize() + 1);
    editor->setFont(f);
}

void MainWindow::zoomOut()
{
    QFont f = editor->font();
    if (f.pointSize() > 6) {
        f.setPointSize(f.pointSize() - 1);
        editor->setFont(f);
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About Felsic Notes"),
                       tr("<h2>Felsic Notes C++ Port</h2>"
                          "<p>A lightning-fast markdown notes manager built with Qt6 and C++.</p>"));
}

void MainWindow::customizeToolbar()
{
    QMessageBox::information(this, tr("Coming Soon"), tr("Toolbar customization dialog will be implemented in the next step!"));
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

