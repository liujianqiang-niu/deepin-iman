// src/data/HistoryDb.cpp
#include "HistoryDb.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

HistoryDb::HistoryDb(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath)
{
}

HistoryDb::~HistoryDb() {
    if (m_db.isOpen()) {
        QSqlDatabase::removeDatabase(m_db.connectionName());
        m_db.close();
    }
}

bool HistoryDb::open() {
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbPath);
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) return false;
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    createSchema();
    cleanup(100);
    return true;
}

void HistoryDb::createSchema() {
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS history ("
           "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  page_id INTEGER NOT NULL,"
           "  page_name TEXT,"
           "  page_section INTEGER,"
           "  visited_at INTEGER NOT NULL,"
           "  duration_sec INTEGER,"
           "  ai_interactions INTEGER DEFAULT 0)");
    q.exec("ALTER TABLE history ADD COLUMN page_name TEXT");
    q.exec("ALTER TABLE history ADD COLUMN page_section INTEGER");
    q.exec("CREATE INDEX IF NOT EXISTS idx_history_visited ON history(visited_at DESC)");
}

void HistoryDb::recordVisit(int pageId, const QString& pageName, int pageSection) {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("INSERT INTO history (page_id, page_name, page_section, visited_at, duration_sec, ai_interactions) VALUES (?, ?, ?, ?, 0, 0)");
    q.addBindValue(pageId);
    q.addBindValue(pageName);
    q.addBindValue(pageSection);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();
}

QList<HistoryItem> HistoryDb::recent(int limit) const {
    QList<HistoryItem> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT id, page_id, page_name, page_section, visited_at, duration_sec, ai_interactions "
              "FROM history ORDER BY visited_at DESC LIMIT ?");
    q.addBindValue(limit);
    q.exec();
    while (q.next()) {
        HistoryItem item;
        item.id = q.value(0).toInt();
        item.pageId = q.value(1).toInt();
        item.pageName = q.value(2).toString();
        item.pageSection = q.value(3).toInt();
        item.visitedAt = q.value(4).toLongLong();
        item.durationSec = q.value(5).toInt();
        item.aiInteractions = q.value(6).toInt();
        results << item;
    }
    return results;
}

QList<HistoryItem> HistoryDb::since(qint64 timestamp) const {
    QList<HistoryItem> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT id, page_id, page_name, page_section, visited_at, duration_sec, ai_interactions "
              "FROM history WHERE visited_at >= ? ORDER BY visited_at DESC");
    q.addBindValue(timestamp);
    q.exec();
    while (q.next()) {
        HistoryItem item;
        item.id = q.value(0).toInt();
        item.pageId = q.value(1).toInt();
        item.pageName = q.value(2).toString();
        item.pageSection = q.value(3).toInt();
        item.visitedAt = q.value(4).toLongLong();
        item.durationSec = q.value(5).toInt();
        item.aiInteractions = q.value(6).toInt();
        results << item;
    }
    return results;
}

void HistoryDb::cleanup(int keepCount) {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("DELETE FROM history WHERE id NOT IN (SELECT id FROM history ORDER BY visited_at DESC LIMIT ?)");
    q.addBindValue(keepCount);
    q.exec();
}
