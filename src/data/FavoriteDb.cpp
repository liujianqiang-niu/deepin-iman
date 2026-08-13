// src/data/FavoriteDb.cpp
#include "FavoriteDb.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

FavoriteDb::FavoriteDb(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath)
{
}

FavoriteDb::~FavoriteDb() {
    if (m_db.isOpen()) {
        QSqlDatabase::removeDatabase(m_db.connectionName());
        m_db.close();
    }
}

bool FavoriteDb::open() {
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbPath);
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) return false;
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    createSchema();
    return true;
}

void FavoriteDb::createSchema() {
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS favorite ("
           "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  page_id INTEGER NOT NULL,"
           "  page_name TEXT,"
           "  page_section INTEGER,"
           "  note TEXT,"
           "  tags TEXT,"
           "  created_at INTEGER NOT NULL)");
    q.exec("ALTER TABLE favorite ADD COLUMN page_name TEXT");
    q.exec("ALTER TABLE favorite ADD COLUMN page_section INTEGER");
}

bool FavoriteDb::add(int pageId, const QString& pageName, int pageSection, const QString& note, const QString& tags) {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("INSERT INTO favorite (page_id, page_name, page_section, note, tags, created_at) VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(pageId);
    q.addBindValue(pageName);
    q.addBindValue(pageSection);
    q.addBindValue(note);
    q.addBindValue(tags);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    return q.exec();
}

bool FavoriteDb::remove(int pageId) {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("DELETE FROM favorite WHERE page_id = ?");
    q.addBindValue(pageId);
    return q.exec();
}

void FavoriteDb::clearAll() {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.exec("DELETE FROM favorite");
}

bool FavoriteDb::isFavorite(int pageId) const {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT 1 FROM favorite WHERE page_id = ?");
    q.addBindValue(pageId);
    return q.exec() && q.next();
}

QList<FavoriteItem> FavoriteDb::list() const {
    QList<FavoriteItem> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.exec("SELECT id, page_id, page_name, page_section, note, tags, created_at "
           "FROM favorite ORDER BY created_at DESC");
    while (q.next()) {
        FavoriteItem item;
        item.id = q.value(0).toInt();
        item.pageId = q.value(1).toInt();
        item.pageName = q.value(2).toString();
        item.pageSection = q.value(3).toInt();
        item.note = q.value(4).toString();
        item.tags = q.value(5).toString();
        item.createdAt = q.value(6).toLongLong();
        results << item;
    }
    return results;
}
