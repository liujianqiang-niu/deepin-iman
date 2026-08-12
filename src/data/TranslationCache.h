// src/data/TranslationCache.h
#pragma once
#include <QObject>
#include <QString>
#include <QSqlDatabase>

class TranslationCache : public QObject {
    Q_OBJECT
public:
    explicit TranslationCache(const QString& dbPath, QObject* parent = nullptr);
    ~TranslationCache();

    bool open();
    QString get(const QString& pageHash) const;
    void put(const QString& pageHash, const QString& zhText, const QString& source, const QString& model);
    bool exists(const QString& pageHash) const;
    void remove(const QString& pageHash);

    static QString computeHash(const QString& name, int section, qint64 mtime);

private:
    QString m_dbPath;
    QSqlDatabase m_db;
    void createSchema();
};
