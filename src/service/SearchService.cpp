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
    if (query.trimmed().isEmpty()) return results;

    auto byName = m_index->findByName(query.trimmed());
    QSet<int> seenIds;
    for (const auto& p : byName) {
        results << p;
        seenIds << p.id;
        if (results.size() >= limit) return results;
    }

    auto fts = m_index->fullTextSearch(query, limit - results.size());
    for (const auto& p : fts) {
        if (!seenIds.contains(p.id)) {
            results << p;
            seenIds << p.id;
        }
        if (results.size() >= limit) break;
    }
    return results;
}

void SearchService::openPage(const QString& name, int section) {
    emit openPageRequested(name, section);
}
