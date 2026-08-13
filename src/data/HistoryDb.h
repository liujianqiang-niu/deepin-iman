// src/data/HistoryDb.h
#pragma once
#include <QObject>
#include <QString>
#include <QSqlDatabase>

struct HistoryItem {
    int id = -1;
    int pageId = -1;
    qint64 visitedAt = 0;
    int durationSec = 0;
    int aiInteractions = 0;
    QString pageName;
    int pageSection = 0;
};

class HistoryDb : public QObject {
    Q_OBJECT
public:
    explicit HistoryDb(const QString& dbPath, QObject* parent = nullptr);
    ~HistoryDb();

    bool open();
    void recordVisit(int pageId, const QString& pageName, int pageSection);
    QList<HistoryItem> recent(int limit = 50) const;
    QList<HistoryItem> since(qint64 timestamp) const;
    void cleanup(int keepCount = 100);
    void clearAll();

private:
    QString m_dbPath;
    QSqlDatabase m_db;
    void createSchema();
};
