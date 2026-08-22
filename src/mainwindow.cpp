#include "mainwindow.h"
#include <QDir>
#include <QVBoxLayout>
#include <QToolBar>
#include <QIcon>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    createActions();
    createToolBars();
    
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
}

void MainWindow::createActions()
{
    // TODO: Add menu actions (New, Open, Save, etc)
}

void MainWindow::createToolBars()
{
    QToolBar *mainToolBar = addToolBar(tr("Main Toolbar"));
    mainToolBar->setMovable(false);
    // TODO: Add toolbar actions
}
