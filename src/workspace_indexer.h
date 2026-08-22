#ifndef WORKSPACE_INDEXER_H
#define WORKSPACE_INDEXER_H

#include <QThread>
#include <QString>
#include <QList>
#include <QPair>

class WorkspaceIndexer : public QThread
{
    Q_OBJECT

public:
    explicit WorkspaceIndexer(const QString &rootPath, QObject *parent = nullptr);

signals:
    void finishedIndexing(const QList<QPair<QString, QString>> &mdFiles);

protected:
    void run() override;

private:
    QString rootPath;
};

#endif // WORKSPACE_INDEXER_H
