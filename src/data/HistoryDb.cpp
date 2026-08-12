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
    return true;
}

void HistoryDb::createSchema() {
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS history ("
           "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  page_id INTEGER NOT NULL,"
           "  visited_at INTEGER NOT NULL,"
           "  duration_sec INTEGER,"
           "  ai_interactions INTEGER DEFAULT 0)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_history_visited ON history(visited_at DESC)");
}

void HistoryDb::recordVisit(int pageId) {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("INSERT INTO history (page_id, visited_at, duration_sec, ai_interactions) VALUES (?, ?, 0, 0)");
    q.addBindValue(pageId);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();
}

QList<HistoryItem> HistoryDb::recent(int limit) const {
    QList<HistoryItem> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT h.id, h.page_id, h.visited_at, h.duration_sec, h.ai_interactions, m.name, m.section "
              "FROM history h LEFT JOIN man_page m ON h.page_id = m.id "
              "ORDER BY h.visited_at DESC LIMIT ?");
    q.addBindValue(limit);
    q.exec();
    while (q.next()) {
        HistoryItem item;
        item.id = q.value(0).toInt();
        item.pageId = q.value(1).toInt();
        item.visitedAt = q.value(2).toLongLong();
        item.durationSec = q.value(3).toInt();
        item.aiInteractions = q.value(4).toInt();
        item.pageName = q.value(5).toString();
        item.pageSection = q.value(6).toInt();
        results << item;
    }
    return results;
}

QList<HistoryItem> HistoryDb::since(qint64 timestamp) const {
    QList<HistoryItem> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT h.id, h.page_id, h.visited_at, h.duration_sec, h.ai_interactions, m.name, m.section "
              "FROM history h LEFT JOIN man_page m ON h.page_id = m.id "
              "WHERE h.visited_at >= ? ORDER BY h.visited_at DESC");
    q.addBindValue(timestamp);
    q.exec();
    while (q.next()) {
        HistoryItem item;
        item.id = q.value(0).toInt();
        item.pageId = q.value(1).toInt();
        item.visitedAt = q.value(2).toLongLong();
        item.durationSec = q.value(3).toInt();
        item.aiInteractions = q.value(4).toInt();
        item.pageName = q.value(5).toString();
        item.pageSection = q.value(6).toInt();
        results << item;
    }
    return results;
}
