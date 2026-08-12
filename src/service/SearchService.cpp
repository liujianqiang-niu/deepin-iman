// src/service/SearchService.cpp
#include "SearchService.h"
#include "data/ManIndex.h"
#include <QSet>

SearchService::SearchService(ManIndex* index, QObject* parent)
    : QObject(parent), m_index(index)
{
}

QList<ManPage> SearchService::search(const QString& query, int limit) const {
    QList<ManPage> results;
    QString q = query.trimmed();
    if (q.isEmpty()) return results;

    QSet<int> seenIds;

    // 1. 精确匹配
    auto byName = m_index->findByName(q);
    for (const auto& p : byName) {
        results << p;
        seenIds << p.id;
        if (results.size() >= limit) return results;
    }

    // 2. 部分匹配（LIKE）
    if (results.size() < limit) {
        auto byLike = m_index->findByNameLike(q);
        for (const auto& p : byLike) {
            if (!seenIds.contains(p.id)) {
                results << p;
                seenIds << p.id;
            }
            if (results.size() >= limit) return results;
        }
    }

    // 3. 全文搜索（FTS5）
    if (results.size() < limit) {
        auto fts = m_index->fullTextSearch(q, limit - results.size());
        for (const auto& p : fts) {
            if (!seenIds.contains(p.id)) {
                results << p;
                seenIds << p.id;
            }
            if (results.size() >= limit) break;
        }
    }
    return results;
}

void SearchService::openPage(const QString& name, int section) {
    emit openPageRequested(name, section);
}
