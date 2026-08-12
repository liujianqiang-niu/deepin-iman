// src/view/LeftSidebar.h
#pragma once
#include <DWidget>
#include <DSearchEdit>
#include <DTreeView>
#include <QStandardItemModel>

DWIDGET_USE_NAMESPACE

struct ManPage;

class LeftSidebar : public DWidget {
    Q_OBJECT
public:
    explicit LeftSidebar(QWidget* parent = nullptr);

    void setManPages(const QList<ManPage>& pages);

signals:
    void searchRequested(const QString& query);
    void pageSelected(const QString& name, int section);

private:
    DSearchEdit* m_searchEdit;
    DTreeView* m_navTree;
    QStandardItemModel* m_navModel;
};
