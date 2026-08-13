// src/service/SearchService.cpp
#include "SearchService.h"
#include "data/ManIndex.h"
#include <QSet>
#include <QRegularExpression>

SearchService::SearchService(ManIndex* index, QObject* parent)
    : QObject(parent), m_index(index)
{
}

QList<ManPage> SearchService::search(const QString& query, int limit, bool caseSensitive, bool wholeWord) const {
    QList<ManPage> results;
    QString q = query.trimmed();
    if (q.isEmpty()) return results;

    QSet<int> seenIds;

    if (wholeWord) {
        auto byName = m_index->findByName(q);
        for (const auto& p : byName) {
            if (caseSensitive) {
                if (p.name != q) continue;
            } else {
                if (p.name.toLower() != q.toLower()) continue;
            }
            results << p;
            seenIds << p.id;
            if (results.size() >= limit) return results;
        }
        return results;
    }

    auto byName = m_index->findByName(q);
    for (const auto& p : byName) {
        results << p;
        seenIds << p.id;
        if (results.size() >= limit) return results;
    }

    if (results.size() < limit) {
        auto byLike = m_index->findByNameLike(q);
        for (const auto& p : byLike) {
            if (!seenIds.contains(p.id)) {
                if (caseSensitive && !p.name.contains(q, Qt::CaseSensitive)) continue;
                results << p;
                seenIds << p.id;
            }
            if (results.size() >= limit) return results;
        }
    }

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
