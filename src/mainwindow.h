#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QFileSystemModel>
#include <QItemSelection>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QLineEdit>
#include <QMap>
#include <QPair>
#include <QAction>
#include <QCloseEvent>
#include "file_filter_model.h"

class PdfGenerator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onEditorTextChanged();
    void exportToPdf();
    void onPdfGenerated(bool success, const QString &outputPath);
    void onFileSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void newFile();
    void saveFile();
    void saveFileAs();
    void openFile();
    void openFolder();
    void togglePreview(bool checked);
    void zoomIn();
    void zoomOut();
    void showAbout();
    void customizeToolbar();
    
    // Context Menu slots
    void showTreeContextMenu(const QPoint &pos);
    void createNewNote(const QString &baseDir);
    void createNewFolder(const QString &baseDir);
    void renameNote(const QString &sourcePath);
    void moveNote(const QString &sourcePath);
    void duplicateNote(const QString &sourcePath);
    void deleteNote(const QString &sourcePath);
    void revealInExplorer(const QString &path);
    void renameFolder(const QString &sourcePath);
    void moveFolder(const QString &sourcePath);
    void deleteFolder(const QString &sourcePath);
    void expandAll(const QModelIndex &index);
    void collapseAll(const QModelIndex &index);

    // Batch operations
    void batchGroup();
    void batchMove();
    void batchCopy();
    void batchDelete();

    void updateStats();
    void onTitleChanged();
    void onSearchChanged(const QString &text);
    void applySearch();
    
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
    void buildToolbar();
    void applyFontSize();

    
    int currentFontSize = 14;
    
    void loadWorkspaceSettings(const QString &workspacePath);
    void saveWorkspaceSettings();
    bool removeDirectoryRecursively(const QString &dirName);
    qint64 getDirectorySize(const QString &path);

    // Toolbar logic
    QMap<QString, QPair<QString, QAction*>> catalog;
    QStringList currentToolbarLayout;

    // UI Elements
    QToolBar *mainToolBar;
    QSplitter *mainSplitter;
    QLineEdit *searchBox;
    QTreeView *treeView;
    QLineEdit *titleBox;
    QStackedWidget *stackedWidget;
    QPlainTextEdit *editor;
    QTextBrowser *preview;
    
    // Multi-select UI
    QWidget *multiSelectView;
    QLabel *msLabel;
    QPushButton *btnGroupNewFolder;
    QPushButton *btnMove;
    QPushButton *btnCopy;
    QPushButton *btnDelete;
    
    // Models & Helpers
    QFileSystemModel *fileModel;
    FileFilterProxyModel *proxyModel;
    PdfGenerator *pdfGen;
    
    // UI Action Pointers
    QAction *newAction;
    QAction *openAction;
    QAction *openFolderAction;
    QAction *exitAction;
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *exportPdfAction;
    
    QAction *boldAction;
    QAction *italicAction;
    QAction *linkAction;
    QAction *codeAction;
    QAction *upperAction;
    QAction *lowerAction;
    QAction *titleAction;
    QAction *sentenceAction;
    
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *togglePreviewAction;
    QAction *wrapTextAction;
    QAction *hideEmptyAction;
    QAction *aboutAction;

    // Status Bar Elements
    QLabel *statsLabel;
    QTimer *statsTimer;
    
    // Search Elements
    QTimer *searchTimer;
    
    // State
    QString currentFilePath;
    QStringList selectedBatchFiles;
};

#endif // MAINWINDOW_H
