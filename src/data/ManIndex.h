// src/data/ManIndex.h
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>

struct ManPage {
    int id = -1;
    QString name;
    int section = 0;
    QString sectionName;
    QString sourcePath;
    QString title;
    qint64 sourceMtime = 0;
    qint64 indexedAt = 0;
};

class ManIndex : public QObject {
    Q_OBJECT
public:
    explicit ManIndex(const QString& dbPath, QObject* parent = nullptr);
    ~ManIndex();

    bool open();
    bool tableExists(const QString& name);
    int scanManPages(const QString& manRoot);
    int pageCount() const;

    QList<ManPage> findByName(const QString& name) const;
    ManPage findById(int id) const;
    QList<ManPage> fullTextSearch(const QString& query, int limit = 20) const;

signals:
    void scanProgress(int current, int total);
    void scanFinished(int totalPages);

private:
    QString m_dbPath;
    QSqlDatabase m_db;
    void createSchema();
    QStringList findManFiles(const QString& manRoot) const;
    ManPage parseManPath(const QString& path) const;
    QString sectionName(int section) const;
    QString extractTitle(const QString& gzPath) const;
    QString extractPlainText(const QString& gzPath) const;
};
