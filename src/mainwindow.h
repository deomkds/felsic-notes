#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QFileSystemModel>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QLineEdit>
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
    void saveFileAs();
    void openFile();
    void openFolder();
    void togglePreview(bool checked);
    void toggleWordWrap(bool checked);
    void toggleHideEmpty(bool checked);
    void zoomIn();
    void zoomOut();
    void showAbout();
    void customizeToolbar();
    void updateStats();
    void onTitleChanged();
    
    // Formatting Slots
    void insertBold();
    void insertItalic();
    void insertLink();
    void insertCode();
    void changeCaseUpper();
    void changeCaseLower();
    void changeCaseTitle();
    void changeCaseSentence();

private:
    void setupUi();
    void createActions();
    void createToolBars();

    // UI Elements
    QSplitter *mainSplitter;
    QTreeView *treeView;
    QLineEdit *titleBox;
    QStackedWidget *stackedWidget;
    QPlainTextEdit *editor;
    QTextBrowser *preview;
    QWidget *multiSelectView; // Placeholder for now

    // Actions
    QAction *actionNew;
    QAction *actionOpen;
    QAction *actionOpenFolder;
    QAction *actionSave;
    QAction *actionSaveAs;
    QAction *actionExportPdf;
    QAction *actionExit;
    
    QAction *actionTogglePreview;
    QAction *actionToggleWrap;
    QAction *actionToggleHideEmpty;
    QAction *actionZoomIn;
    QAction *actionZoomOut;
    QAction *actionCustomizeToolbar;
    
    QAction *actionAbout;
    
    // Formatting Actions
    QAction *actionBold;
    QAction *actionItalic;
    QAction *actionLink;
    QAction *actionCode;
    QAction *actionUpper;
    QAction *actionLower;
    QAction *actionTitle;
    QAction *actionSentence;

    // Models & Helpers
    QFileSystemModel *fileModel;
    FileFilterProxyModel *proxyModel;
    PdfGenerator *pdfGen;
    
    // Status Bar Elements
    QLabel *statsLabel;
    QTimer *statsTimer;
    
    // State
    QString currentFilePath;
};

#endif // MAINWINDOW_H
