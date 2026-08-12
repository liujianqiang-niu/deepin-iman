// src/view/LeftSidebar.cpp
#include "LeftSidebar.h"
#include "data/ManIndex.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QDateTime>

LeftSidebar::LeftSidebar(QWidget* parent) : DWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new DTabWidget(this);

    buildSearchTab();
    buildFavoritesTab();
    buildHistoryTab();

    layout->addWidget(m_tabWidget);
}

void LeftSidebar::buildSearchTab() {
    auto* page = new DWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new DSearchEdit(page);
    m_searchEdit->setPlaceholderText("搜索 man 手册...");
    connect(m_searchEdit, &DSearchEdit::textChanged, this, [this](const QString& t) {
        if (t.length() >= 2) emit searchRequested(t);
    });
    layout->addWidget(m_searchEdit);

    m_searchTree = new DTreeView(page);
    m_searchModel = new QStandardItemModel(this);
    m_searchModel->setHorizontalHeaderLabels({"命令", "章节"});
    m_searchTree->setModel(m_searchModel);
    m_searchTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_searchTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(m_searchTree, &DTreeView::clicked, this, [this](const QModelIndex& idx) {
        auto* nameItem = m_searchModel->item(idx.row(), 0);
        auto* sectionItem = m_searchModel->item(idx.row(), 1);
        if (nameItem && sectionItem) {
            emit pageSelected(nameItem->text(), sectionItem->text().toInt());
        }
    });
    layout->addWidget(m_searchTree, 1);

    m_tabWidget->addTab(page, "搜索");
}

void LeftSidebar::buildFavoritesTab() {
    auto* page = new DWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_favTree = new DTreeView(page);
    m_favModel = new QStandardItemModel(this);
    m_favModel->setHorizontalHeaderLabels({"命令", "章节"});
    m_favTree->setModel(m_favModel);
    m_favTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_favTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(m_favTree, &DTreeView::clicked, this, [this](const QModelIndex& idx) {
        auto* nameItem = m_favModel->item(idx.row(), 0);
        auto* sectionItem = m_favModel->item(idx.row(), 1);
        if (nameItem && sectionItem) {
            emit pageSelected(nameItem->text(), sectionItem->text().toInt());
        }
    });
    layout->addWidget(m_favTree, 1);

    m_tabWidget->addTab(page, "收藏");
}

void LeftSidebar::buildHistoryTab() {
    auto* page = new DWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_histTree = new DTreeView(page);
    m_histModel = new QStandardItemModel(this);
    m_histModel->setHorizontalHeaderLabels({"命令", "章节"});
    m_histTree->setModel(m_histModel);
    m_histTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_histTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(m_histTree, &DTreeView::clicked, this, [this](const QModelIndex& idx) {
        auto* nameItem = m_histModel->item(idx.row(), 0);
        auto* sectionItem = m_histModel->item(idx.row(), 1);
        if (nameItem && sectionItem) {
            emit pageSelected(nameItem->text(), sectionItem->text().toInt());
        }
    });
    layout->addWidget(m_histTree, 1);

    m_tabWidget->addTab(page, "历史");
}

void LeftSidebar::setManPages(const QList<ManPage>& pages) {
    m_searchModel->removeRows(0, m_searchModel->rowCount());
    for (const auto& p : pages) {
        QList<QStandardItem*> row;
        auto* nameItem = new QStandardItem(QIcon::fromTheme("text-x-script"), p.name);
        auto* sectionItem = new QStandardItem(QString::number(p.section));
        row << nameItem << sectionItem;
        m_searchModel->appendRow(row);
    }
}

void LeftSidebar::setFavorites(const QList<FavoriteItem>& items) {
    m_favModel->removeRows(0, m_favModel->rowCount());
    for (const auto& item : items) {
        if (item.pageName.isEmpty()) continue;
        QList<QStandardItem*> row;
        auto* nameItem = new QStandardItem(QIcon::fromTheme("bookmark-new"), item.pageName);
        auto* sectionItem = new QStandardItem(QString::number(item.pageSection));
        row << nameItem << sectionItem;
        m_favModel->appendRow(row);
    }
}

void LeftSidebar::setHistory(const QList<HistoryItem>& items) {
    m_histModel->removeRows(0, m_histModel->rowCount());
    for (const auto& item : items) {
        if (item.pageName.isEmpty()) continue;
        QList<QStandardItem*> row;
        auto* nameItem = new QStandardItem(QIcon::fromTheme("view-history"), item.pageName);
        auto* sectionItem = new QStandardItem(QString::number(item.pageSection));
        row << nameItem << sectionItem;
        m_histModel->appendRow(row);
    }
}
