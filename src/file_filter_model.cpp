#include "file_filter_model.h"
#include <QFileSystemModel>
#include <QDir>
#include <QFileInfo>

FileFilterProxyModel::FileFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent), hideEmptyFolders(false), indexer(nullptr)
{
}

void FileFilterProxyModel::updateWorkspaceIndex(const QString &rootPath)
{
    allMdFiles.clear();
    dirMatchCache.clear();
    invalidateFilter();

    if (indexer) {
        indexer->deleteLater();
    }
    
    indexer = new WorkspaceIndexer(rootPath, this);
    connect(indexer, &WorkspaceIndexer::finishedIndexing, this, &FileFilterProxyModel::onIndexingFinished);
    indexer->start();
}

void FileFilterProxyModel::onIndexingFinished(const QList<QPair<QString, QString>> &mdFiles)
{
    allMdFiles = mdFiles;
    dirMatchCache.clear();
    invalidateFilter();
}

void FileFilterProxyModel::addToIndex(const QString &filepath)
{
    QPair<QString, QString> entry = qMakePair(filepath, QFileInfo(filepath).fileName());
    if (!allMdFiles.contains(entry)) {
        allMdFiles.append(entry);
        dirMatchCache.clear();
        invalidateFilter();
    }
}

void FileFilterProxyModel::removeFromIndex(const QString &filepath)
{
    for (int i = 0; i < allMdFiles.size(); ++i) {
        if (allMdFiles[i].first == filepath) {
            allMdFiles.removeAt(i);
            dirMatchCache.clear();
            invalidateFilter();
            break;
        }
    }
}

void FileFilterProxyModel::renameInIndex(const QString &oldPath, const QString &newPath)
{
    for (int i = 0; i < allMdFiles.size(); ++i) {
        if (allMdFiles[i].first == oldPath) {
            allMdFiles[i] = qMakePair(newPath, QFileInfo(newPath).fileName());
            dirMatchCache.clear();
            invalidateFilter();
            break;
        }
    }
}

void FileFilterProxyModel::renameDirInIndex(const QString &oldDir, const QString &newDir)
{
    QString oldPrefix = oldDir + QDir::separator();
    bool changed = false;
    for (int i = 0; i < allMdFiles.size(); ++i) {
        QString mdPath = allMdFiles[i].first;
        if (mdPath == oldDir || mdPath.startsWith(oldPrefix)) {
            QString newFpath = mdPath;
            newFpath.replace(0, oldDir.length(), newDir);
            allMdFiles[i] = qMakePair(newFpath, QFileInfo(newFpath).fileName());
            changed = true;
        }
    }
    if (changed) {
        dirMatchCache.clear();
        invalidateFilter();
    }
}

void FileFilterProxyModel::removeDirFromIndex(const QString &deadDir)
{
    QString deadPrefix = deadDir + QDir::separator();
    bool changed = false;
    
    QMutableListIterator<QPair<QString, QString>> i(allMdFiles);
    while (i.hasNext()) {
        QString mdPath = i.next().first;
        if (mdPath == deadDir || mdPath.startsWith(deadPrefix)) {
            i.remove();
            changed = true;
        }
    }
    
    if (changed) {
        dirMatchCache.clear();
        invalidateFilter();
    }
}

void FileFilterProxyModel::setHideEmptyFolders(bool hide)
{
    hideEmptyFolders = hide;
    invalidateFilter();
}

void FileFilterProxyModel::setFilterRegularExpression(const QRegularExpression &regex)
{
    dirMatchCache.clear();
    QSortFilterProxyModel::setFilterRegularExpression(regex);
}

void FileFilterProxyModel::setSourceModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel::setSourceModel(model);
    QFileSystemModel *fsModel = qobject_cast<QFileSystemModel*>(model);
    if (fsModel) {
        connect(fsModel, &QFileSystemModel::directoryLoaded, this, &FileFilterProxyModel::clearCache);
        connect(fsModel, &QFileSystemModel::fileRenamed, this, &FileFilterProxyModel::clearCache);
        connect(fsModel, &QFileSystemModel::rowsInserted, this, &FileFilterProxyModel::clearCache);
        connect(fsModel, &QFileSystemModel::rowsRemoved, this, &FileFilterProxyModel::clearCache);
    }
}

void FileFilterProxyModel::clearCache()
{
    dirMatchCache.clear();
}

bool FileFilterProxyModel::hasMatchingFile(const QString &path, const QRegularExpression &regex) const
{
    if (dirMatchCache.contains(path)) {
        return dirMatchCache.value(path);
    }

    bool hasMatch = false;
    QString pathPrefix = path;
    if (!pathPrefix.endsWith(QDir::separator())) {
        pathPrefix += QDir::separator();
    }

    for (const auto &entry : allMdFiles) {
        const QString &mdFile = entry.first;
        const QString &name = entry.second;
        
        if (mdFile == path || mdFile.startsWith(pathPrefix)) {
            if (regex.pattern().isEmpty() || regex.match(name).hasMatch()) {
                hasMatch = true;
                break;
            }
        }
    }

    dirMatchCache.insert(path, hasMatch);
    return hasMatch;
}

QVariant FileFilterProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ForegroundRole) {
        QModelIndex sourceIndex = mapToSource(index);
        QFileSystemModel *model = qobject_cast<QFileSystemModel*>(sourceModel());
        if (model && model->isDir(sourceIndex)) {
            QString path = model->filePath(sourceIndex);
            QRegularExpression regex = filterRegularExpression();
            if (!hasMatchingFile(path, regex)) {
                return QColor(Qt::gray);
            }
        }
    }
    return QSortFilterProxyModel::data(index, role);
}

bool FileFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QFileSystemModel *model = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!model) return true;

    QModelIndex index = model->index(sourceRow, 0, sourceParent);
    
    if (model->isDir(index)) {
        if (hideEmptyFolders) {
            QString path = model->filePath(index);
            QRegularExpression regex = filterRegularExpression();
            return hasMatchingFile(path, regex);
        }
        return true; // Always accept directories to preserve tree expandability if hide is false
    }
    
    // Otherwise, check file name against our regex filter
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
