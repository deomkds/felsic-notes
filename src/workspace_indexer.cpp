#include "workspace_indexer.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>

WorkspaceIndexer::WorkspaceIndexer(const QString &rootPath, QObject *parent)
    : QThread(parent), rootPath(rootPath)
{
}

void WorkspaceIndexer::run()
{
    QList<QPair<QString, QString>> allMd;
    
    try {
        QDirIterator it(rootPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            // Ignore hidden folders
            if (it.filePath().contains("/.")) {
                continue;
            }
            if (it.fileName().endsWith(".md", Qt::CaseInsensitive)) {
                allMd.append(qMakePair(it.filePath(), it.fileName()));
            }
        }
    } catch (const std::exception &e) {
        qWarning() << "Error during workspace indexing:" << e.what();
    }
    
    emit finishedIndexing(allMd);
}
