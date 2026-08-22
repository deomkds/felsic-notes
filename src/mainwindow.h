#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QFileSystemModel>
#include "file_filter_model.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void createActions();
    void createToolBars();

    // UI Elements
    QSplitter *mainSplitter;
    QTreeView *treeView;
    QSplitter *editorSplitter;
    QPlainTextEdit *editor;
    QTextBrowser *preview;

    // Models
    QFileSystemModel *fileModel;
    FileFilterProxyModel *proxyModel;
};

#endif // MAINWINDOW_H
