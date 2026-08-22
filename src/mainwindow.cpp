#include "mainwindow.h"
#include <QDir>
#include <QVBoxLayout>
#include <QToolBar>
#include <QIcon>
#include <QSettings>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
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
    fileModel->setRootPath(QDir::homePath()); // Default path for now
    
    proxyModel = new FileFilterProxyModel(this);
    proxyModel->setSourceModel(fileModel);
    
    // Only show Markdown files
    proxyModel->setFilterRegularExpression(QRegularExpression("\\.md$", QRegularExpression::CaseInsensitiveOption));
    proxyModel->updateWorkspaceIndex(QDir::homePath());
    
    treeView->setModel(proxyModel);
    treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(QDir::homePath())));

    // Right side: Editor & Preview Splitter
    editorSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    
    editor = new QPlainTextEdit(editorSplitter);
    QFont font("Consolas", 11);
    editor->setFont(font);

    preview = new QTextBrowser(editorSplitter);

    setCentralWidget(mainSplitter);
    
    // Connect Editor text changed to Markdown parser
    connect(editor, &QPlainTextEdit::textChanged, this, &MainWindow::onEditorTextChanged);
    
    // Connect Tree View selection to file opener
    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelected);
}

void MainWindow::createActions()
{
    actionNew = new QAction(QIcon::fromTheme("document-new"), tr("&New"), this);
    actionNew->setShortcut(QKeySequence::New);
    
    actionSave = new QAction(QIcon::fromTheme("document-save"), tr("&Save"), this);
    actionSave->setShortcut(QKeySequence::Save);
    connect(actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    
    actionExportPdf = new QAction(QIcon::fromTheme("document-print"), tr("Export to &PDF..."), this);
    connect(actionExportPdf, &QAction::triggered, this, &MainWindow::exportToPdf);
}

void MainWindow::createToolBars()
{
    QToolBar *mainToolBar = addToolBar(tr("Main Toolbar"));
    mainToolBar->setMovable(false);
    
    mainToolBar->addAction(actionNew);
    mainToolBar->addAction(actionSave);
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
    
    QModelIndex index = selected.indexes().first();
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    QString path = fileModel->filePath(sourceIndex);
    
    if (QFileInfo(path).isFile()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            currentFilePath = path;
            editor->setPlainText(QString::fromUtf8(file.readAll()));
            file.close();
        }
    }
}

void MainWindow::saveFile()
{
    if (currentFilePath.isEmpty()) return;
    
    QFile file(currentFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(editor->toPlainText().toUtf8());
        file.close();
        // Optional: show a small status update or saved indicator
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not save the file."));
    }
}

