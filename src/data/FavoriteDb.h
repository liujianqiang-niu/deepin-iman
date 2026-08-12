// src/data/FavoriteDb.h
#pragma once
#include <QObject>
#include <QString>
#include <QSqlDatabase>

struct FavoriteItem {
    int id = -1;
    int pageId = -1;
    QString note;
    QString tags;
    QString pageName;
    int pageSection = 0;
    qint64 createdAt = 0;
};

class FavoriteDb : public QObject {
    Q_OBJECT
public:
    explicit FavoriteDb(const QString& dbPath, QObject* parent = nullptr);
    ~FavoriteDb();

    bool open();
    bool add(int pageId, const QString& note, const QString& tags);
    bool remove(int pageId);
    bool isFavorite(int pageId) const;
    QList<FavoriteItem> list() const;

private:
    QString m_dbPath;
    QSqlDatabase m_db;
    void createSchema();
};
