// src/view/PageListDialog.cpp
#include "PageListDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QIcon>
#include <DPushButton>

PageListDialog::PageListDialog(const QString& title, QWidget* parent)
    : DDialog(parent)
{
    setTitle(title);
    setFixedSize(400, 480);

    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 4, 4, 4);

    m_model = new QStandardItemModel(this);
    m_listView = new DListView(widget);
    m_listView->setModel(m_model);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_listView);

    auto* btnLayout = new QHBoxLayout;
    auto* btnDelete = new DPushButton("删除选中收藏", widget);
    btnLayout->addStretch();
    btnLayout->addWidget(btnDelete);
    layout->addLayout(btnLayout);

    addContent(widget);
    addButton("关闭", true, DDialog::ButtonRecommend);

    connect(m_listView, &DListView::clicked, this, [this](const QModelIndex& idx) {
        auto* item = m_model->item(idx.row());
        if (item) {
            emit pageSelected(item->text(), item->data(Qt::UserRole + 1).toInt());
        }
    });
    connect(m_listView, &DListView::doubleClicked, this, [this](const QModelIndex& idx) {
        auto* item = m_model->item(idx.row());
        if (item) {
            emit pageSelected(item->text(), item->data(Qt::UserRole + 1).toInt());
            accept();
        }
    });
    connect(btnDelete, &DPushButton::clicked, this, &PageListDialog::deleteSelectedFavorite);
}

void PageListDialog::setFavorites(const QList<FavoriteItem>& items) {
    m_model->clear();
    for (const auto& item : items) {
        if (item.pageName.isEmpty()) continue;
        auto* row = new DStandardItem(QIcon::fromTheme("bookmark-new"),
            QString("%1(%2)").arg(item.pageName).arg(item.pageSection));
        row->setText(item.pageName);
        row->setData(item.pageSection, Qt::UserRole + 1);
        row->setData(item.pageId, Qt::UserRole + 2);
        m_model->appendRow(row);
    }
}

void PageListDialog::setHistory(const QList<HistoryItem>& items) {
    m_model->clear();
    for (const auto& item : items) {
        if (item.pageName.isEmpty()) continue;
        auto* row = new DStandardItem(QIcon::fromTheme("view-history"),
            QString("%1(%2) - %3").arg(item.pageName).arg(item.pageSection)
                .arg(QDateTime::fromSecsSinceEpoch(item.visitedAt).toString("MM-dd HH:mm")));
        row->setText(item.pageName);
        row->setData(item.pageSection, Qt::UserRole + 1);
        m_model->appendRow(row);
    }
}

void PageListDialog::deleteSelectedFavorite() {
    auto idx = m_listView->currentIndex();
    if (!idx.isValid()) return;
    auto* item = m_model->item(idx.row());
    if (!item) return;
    int pageId = item->data(Qt::UserRole + 2).toInt();
    if (pageId <= 0) return;
    emit favoriteDeleted(pageId);
    m_model->removeRow(idx.row());
}
