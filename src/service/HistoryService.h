// src/service/HistoryService.h
#pragma once
#include <QObject>
#include "data/HistoryDb.h"

class HistoryService : public QObject {
    Q_OBJECT
public:
    explicit HistoryService(HistoryDb* db, QObject* parent = nullptr);
    void recordVisit(int pageId, const QString& pageName, int pageSection);
    QList<HistoryItem> recent(int limit = 50) const;

private:
    HistoryDb* m_db;
};
