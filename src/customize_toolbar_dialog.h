#ifndef CUSTOMIZE_TOOLBAR_DIALOG_H
#define CUSTOMIZE_TOOLBAR_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QAction>
#include <QPair>

class CustomizeToolbarDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomizeToolbarDialog(
        const QMap<QString, QPair<QString, QAction*>> &catalog,
        const QStringList &currentLayout,
        QWidget *parent = nullptr);

    QStringList getLayout() const;

private slots:
    void addItem();
    void removeItem();
    void moveUp();
    void moveDown();

private:
    void setupUi();
    void populateLists();

    QMap<QString, QPair<QString, QAction*>> catalog;
    QStringList currentLayout;

    QListWidget *availList;
    QListWidget *currList;

    QPushButton *btnAdd;
    QPushButton *btnRemove;
    QPushButton *btnUp;
    QPushButton *btnDown;
    QPushButton *btnApply;
    QPushButton *btnCancel;
};

#endif // CUSTOMIZE_TOOLBAR_DIALOG_H
