// src/service/FavoriteService.cpp
#include "FavoriteService.h"

FavoriteService::FavoriteService(FavoriteDb* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

bool FavoriteService::add(int pageId, const QString& pageName, int pageSection, const QString& note, const QString& tags) {
    return m_db->add(pageId, pageName, pageSection, note, tags);
}

bool FavoriteService::remove(int pageId) {
    return m_db->remove(pageId);
}

bool FavoriteService::isFavorite(int pageId) const {
    return m_db->isFavorite(pageId);
}

QList<FavoriteItem> FavoriteService::list() const {
    return m_db->list();
}
