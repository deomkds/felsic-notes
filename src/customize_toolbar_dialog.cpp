#include "customize_toolbar_dialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidgetItem>

CustomizeToolbarDialog::CustomizeToolbarDialog(
    const QMap<QString, QPair<QString, QAction*>> &catalog,
    const QStringList &currentLayout,
    QWidget *parent)
    : QDialog(parent), catalog(catalog), currentLayout(currentLayout)
{
    setupUi();
    populateLists();
}

void CustomizeToolbarDialog::setupUi()
{
    setWindowTitle(tr("Customize Toolbar"));
    resize(500, 400);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // Left side: Available actions
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(new QLabel(tr("Available Actions:")));
    availList = new QListWidget(this);
    leftLayout->addWidget(availList);
    mainLayout->addLayout(leftLayout);

    // Middle buttons
    QVBoxLayout *btnLayout = new QVBoxLayout();
    btnLayout->addStretch();
    btnAdd = new QPushButton(tr("Add ->"), this);
    btnRemove = new QPushButton(tr("<- Remove"), this);
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnRemove);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // Right side: Current toolbar
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(new QLabel(tr("Current Toolbar:")));
    currList = new QListWidget(this);
    rightLayout->addWidget(currList);

    // Up/Down buttons
    QHBoxLayout *udLayout = new QHBoxLayout();
    btnUp = new QPushButton(tr("Up"), this);
    btnDown = new QPushButton(tr("Down"), this);
    udLayout->addWidget(btnUp);
    udLayout->addWidget(btnDown);
    rightLayout->addLayout(udLayout);

    // Apply/Cancel buttons
    QHBoxLayout *acLayout = new QHBoxLayout();
    btnApply = new QPushButton(tr("Apply"), this);
    btnCancel = new QPushButton(tr("Cancel"), this);
    acLayout->addWidget(btnApply);
    acLayout->addWidget(btnCancel);
    rightLayout->addLayout(acLayout);

    mainLayout->addLayout(rightLayout);

    // Connections
    connect(btnAdd, &QPushButton::clicked, this, &CustomizeToolbarDialog::addItem);
    connect(btnRemove, &QPushButton::clicked, this, &CustomizeToolbarDialog::removeItem);
    connect(btnUp, &QPushButton::clicked, this, &CustomizeToolbarDialog::moveUp);
    connect(btnDown, &QPushButton::clicked, this, &CustomizeToolbarDialog::moveDown);
    connect(btnApply, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void CustomizeToolbarDialog::populateLists()
{
    // Populate current layout
    for (const QString &itemId : currentLayout) {
        QString name = itemId;
        if (catalog.contains(itemId)) {
            name = catalog.value(itemId).first;
        }
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, itemId);
        currList->addItem(item);
    }

    // Populate available catalog
    QMapIterator<QString, QPair<QString, QAction*>> i(catalog);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item = new QListWidgetItem(i.value().first);
        item->setData(Qt::UserRole, i.key());
        availList->addItem(item);
    }
}

void CustomizeToolbarDialog::addItem()
{
    int row = availList->currentRow();
    if (row >= 0) {
        QListWidgetItem *item = availList->item(row);
        QString itemId = item->data(Qt::UserRole).toString();
        QListWidgetItem *newItem = new QListWidgetItem(item->text());
        newItem->setData(Qt::UserRole, itemId);
        currList->addItem(newItem);
    }
}

void CustomizeToolbarDialog::removeItem()
{
    int row = currList->currentRow();
    if (row >= 0) {
        delete currList->takeItem(row);
    }
}

void CustomizeToolbarDialog::moveUp()
{
    int row = currList->currentRow();
    if (row > 0) {
        QListWidgetItem *item = currList->takeItem(row);
        currList->insertItem(row - 1, item);
        currList->setCurrentRow(row - 1);
    }
}

void CustomizeToolbarDialog::moveDown()
{
    int row = currList->currentRow();
    if (row >= 0 && row < currList->count() - 1) {
        QListWidgetItem *item = currList->takeItem(row);
        currList->insertItem(row + 1, item);
        currList->setCurrentRow(row + 1);
    }
}

QStringList CustomizeToolbarDialog::getLayout() const
{
    QStringList newLayout;
    for (int i = 0; i < currList->count(); ++i) {
        newLayout.append(currList->item(i)->data(Qt::UserRole).toString());
    }
    return newLayout;
}
