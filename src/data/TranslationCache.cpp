// src/data/TranslationCache.cpp
#include "TranslationCache.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

TranslationCache::TranslationCache(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath)
{
}

TranslationCache::~TranslationCache() {
    if (m_db.isOpen()) {
        QSqlDatabase::removeDatabase(m_db.connectionName());
        m_db.close();
    }
}

bool TranslationCache::open() {
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbPath);
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) return false;
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    createSchema();
    return true;
}

void TranslationCache::createSchema() {
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS translation ("
           "  page_hash TEXT PRIMARY KEY,"
           "  zh_text TEXT NOT NULL,"
           "  source TEXT NOT NULL,"
           "  model TEXT,"
           "  translated_at INTEGER NOT NULL,"
           "  quality TEXT DEFAULT 'draft')");
}

QString TranslationCache::get(const QString& pageHash, QString* outSource) const {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT zh_text, source FROM translation WHERE page_hash = ?");
    q.addBindValue(pageHash);
    if (q.exec() && q.next()) {
        if (outSource) *outSource = q.value(1).toString();
        return q.value(0).toString();
    }
    return QString();
}

void TranslationCache::put(const QString& pageHash, const QString& zhText,
                            const QString& source, const QString& model) {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("INSERT OR REPLACE INTO translation (page_hash, zh_text, source, model, translated_at, quality) "
              "VALUES (?, ?, ?, ?, ?, 'draft')");
    q.addBindValue(pageHash);
    q.addBindValue(zhText);
    q.addBindValue(source);
    q.addBindValue(model);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();
}

bool TranslationCache::exists(const QString& pageHash) const {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT 1 FROM translation WHERE page_hash = ?");
    q.addBindValue(pageHash);
    return q.exec() && q.next();
}

void TranslationCache::remove(const QString& pageHash) {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("DELETE FROM translation WHERE page_hash = ?");
    q.addBindValue(pageHash);
    q.exec();
}

void TranslationCache::clearAll() {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.exec("DELETE FROM translation");
}

QString TranslationCache::computeHash(const QString& name, int section, qint64 mtime, const QString& targetLang) {
    QString raw = name + ":" + QString::number(section) + ":" + QString::number(mtime) + ":" + targetLang;
    return QString(QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha256).toHex());
}
