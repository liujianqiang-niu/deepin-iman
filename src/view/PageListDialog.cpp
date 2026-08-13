// src/view/PageListDialog.cpp
#include "PageListDialog.h"
#include <QVBoxLayout>
#include <QDateTime>
#include <QIcon>

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
}

void PageListDialog::setFavorites(const QList<FavoriteItem>& items) {
    m_model->clear();
    for (const auto& item : items) {
        if (item.pageName.isEmpty()) continue;
        auto* row = new DStandardItem(QIcon::fromTheme("bookmark-new"),
            QString("%1(%2)").arg(item.pageName).arg(item.pageSection));
        row->setText(item.pageName);
        row->setData(item.pageSection, Qt::UserRole + 1);
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
