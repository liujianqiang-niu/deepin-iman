// src/view/LeftSidebar.h
#pragma once
#include <DWidget>
#include <DSearchEdit>
#include <DTreeView>
#include <DPushButton>
#include <QStandardItemModel>
#include <QList>

#include "service/SearchService.h"

DWIDGET_USE_NAMESPACE

struct ManPage;

class LeftSidebar : public DWidget {
    Q_OBJECT
public:
    explicit LeftSidebar(QWidget* parent = nullptr);

    void setManPages(const QList<ManPage>& pages);

signals:
    void searchRequested(const QString& query, SearchService::SearchMode mode);
    void pageSelected(const QString& name, int section);

private:
    DSearchEdit* m_searchEdit;
    DPushButton* m_modeBtn;
    DTreeView* m_navTree;
    QStandardItemModel* m_navModel;
    SearchService::SearchMode m_mode = SearchService::SearchMode::Fuzzy;

    QString modeLabel() const;
};
