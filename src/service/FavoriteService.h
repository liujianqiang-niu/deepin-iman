// src/service/FavoriteService.h
#pragma once
#include <QObject>
#include "data/FavoriteDb.h"

class FavoriteService : public QObject {
    Q_OBJECT
public:
    explicit FavoriteService(FavoriteDb* db, QObject* parent = nullptr);
    bool add(int pageId, const QString& pageName, int pageSection, const QString& note, const QString& tags);
    bool remove(int pageId);
    bool isFavorite(int pageId) const;
    QList<FavoriteItem> list() const;
    void clearAll();

private:
    FavoriteDb* m_db;
};
