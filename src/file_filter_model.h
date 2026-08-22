#ifndef FILE_FILTER_MODEL_H
#define FILE_FILTER_MODEL_H

#include <QSortFilterProxyModel>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QColor>
#include "workspace_indexer.h"

class FileFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FileFilterProxyModel(QObject *parent = nullptr);

    void updateWorkspaceIndex(const QString &rootPath);
    void setHideEmptyFolders(bool hide);
    void setFilterRegularExpression(const QRegularExpression &regex);
    void setSourceModel(QAbstractItemModel *model) override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void addToIndex(const QString &filepath);
    void removeFromIndex(const QString &filepath);
    void renameInIndex(const QString &oldPath, const QString &newPath);
    void renameDirInIndex(const QString &oldDir, const QString &newDir);
    void removeDirFromIndex(const QString &deadDir);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private slots:
    void onIndexingFinished(const QList<QPair<QString, QString>> &mdFiles);
    void clearCache();

private:
    bool hasMatchingFile(const QString &path, const QRegularExpression &regex) const;

    mutable QHash<QString, bool> dirMatchCache;
    bool hideEmptyFolders;
    QList<QPair<QString, QString>> allMdFiles;
    WorkspaceIndexer *indexer;
};

#endif // FILE_FILTER_MODEL_H
