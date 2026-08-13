// src/service/HistoryService.cpp
#include "HistoryService.h"

HistoryService::HistoryService(HistoryDb* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

void HistoryService::recordVisit(int pageId, const QString& pageName, int pageSection) {
    m_db->recordVisit(pageId, pageName, pageSection);
}

QList<HistoryItem> HistoryService::recent(int limit) const {
    return m_db->recent(limit);
}
