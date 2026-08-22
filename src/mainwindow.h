#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QFileSystemModel>
#include "file_filter_model.h"

class PdfGenerator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onEditorTextChanged();
    void exportToPdf();
    void onPdfGenerated(bool success, const QString &outputPath);
    void onFileSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void saveFile();

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

    // Actions
    QAction *actionNew;
    QAction *actionSave;
    QAction *actionExportPdf;

    // Models & Helpers
    QFileSystemModel *fileModel;
    FileFilterProxyModel *proxyModel;
    PdfGenerator *pdfGen;
    
    // State
    QString currentFilePath;
};

#endif // MAINWINDOW_H
