// src/service/SearchService.h
#pragma once
#include <QObject>
#include <QList>

class ManIndex;
struct ManPage;

class SearchService : public QObject {
    Q_OBJECT
public:
    explicit SearchService(ManIndex* index, QObject* parent = nullptr);

    QList<ManPage> search(const QString& query, int limit = 50) const;
    void openPage(const QString& name, int section);

signals:
    void searchCompleted(const QList<ManPage>& results);
    void openPageRequested(const QString& name, int section);

private:
    ManIndex* m_index;
};
