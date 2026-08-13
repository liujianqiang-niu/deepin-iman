// src/view/PageListDialog.h
#pragma once
#include <DDialog>
#include <DListView>
#include <DPushButton>
#include <QStandardItemModel>
#include <QList>
#include "data/FavoriteDb.h"
#include "data/HistoryDb.h"

DWIDGET_USE_NAMESPACE

class PageListDialog : public DDialog {
    Q_OBJECT
public:
    explicit PageListDialog(const QString& title, QWidget* parent = nullptr);

    void setFavorites(const QList<FavoriteItem>& items);
    void setHistory(const QList<HistoryItem>& items);

signals:
    void pageSelected(const QString& name, int section);
    void favoritesDeleted(const QList<int>& ids);
    void historyDeleted(const QList<int>& ids);

private:
    DListView* m_listView;
    QStandardItemModel* m_model;
    DPushButton* m_btnSelectAll;
    DPushButton* m_btnDelete;
    bool m_isFavoritesMode = false;

    void toggleSelectAll();
    void deleteSelected();
    QList<int> selectedIds() const;
};
