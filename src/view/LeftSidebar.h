// src/view/LeftSidebar.h
#pragma once
#include <DWidget>
#include <DSearchEdit>
#include <DTreeView>
#include <DTabWidget>
#include <QStandardItemModel>
#include <QList>

#include "data/FavoriteDb.h"
#include "data/HistoryDb.h"

DWIDGET_USE_NAMESPACE

struct ManPage;

class LeftSidebar : public DWidget {
    Q_OBJECT
public:
    explicit LeftSidebar(QWidget* parent = nullptr);

    void setManPages(const QList<ManPage>& pages);
    void setFavorites(const QList<FavoriteItem>& items);
    void setHistory(const QList<HistoryItem>& items);

signals:
    void searchRequested(const QString& query);
    void pageSelected(const QString& name, int section);
    void favoriteRemoved(int pageId);

private:
    DTabWidget* m_tabWidget;

    DSearchEdit* m_searchEdit;
    DTreeView* m_searchTree;
    QStandardItemModel* m_searchModel;

    DTreeView* m_favTree;
    QStandardItemModel* m_favModel;

    DTreeView* m_histTree;
    QStandardItemModel* m_histModel;

    void buildSearchTab();
    void buildFavoritesTab();
    void buildHistoryTab();
};
