// src/view/PageListDialog.cpp
#include "PageListDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QIcon>
#include <DStyledItemDelegate>

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
    m_listView->setItemDelegate(new DStyledItemDelegate(m_listView));
    layout->addWidget(m_listView);

    auto* btnLayout = new QHBoxLayout;
    m_btnSelectAll = new DPushButton("全选", widget);
    m_btnDelete = new DPushButton("删除选中", widget);
    btnLayout->addWidget(m_btnSelectAll);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnDelete);
    layout->addLayout(btnLayout);

    addContent(widget);
    addButton("关闭", true, DDialog::ButtonRecommend);

    connect(m_listView, &DListView::doubleClicked, this, [this](const QModelIndex& idx) {
        auto* item = m_model->item(idx.row());
        if (item) {
            emit pageSelected(item->text(), item->data(Qt::UserRole + 1).toInt());
            accept();
        }
    });
    connect(m_btnSelectAll, &DPushButton::clicked, this, &PageListDialog::toggleSelectAll);
    connect(m_btnDelete, &DPushButton::clicked, this, &PageListDialog::deleteSelected);
}

void PageListDialog::setFavorites(const QList<FavoriteItem>& items) {
    m_isFavoritesMode = true;
    m_model->clear();
    for (const auto& item : items) {
        if (item.pageName.isEmpty()) continue;
        auto* row = new DStandardItem(QIcon::fromTheme("bookmark-new"),
            QString("%1(%2)").arg(item.pageName).arg(item.pageSection));
        row->setText(item.pageName);
        row->setData(item.pageSection, Qt::UserRole + 1);
        row->setData(item.id, Qt::UserRole + 2);
        row->setCheckable(true);
        row->setCheckState(Qt::Unchecked);
        m_model->appendRow(row);
    }
}

void PageListDialog::setHistory(const QList<HistoryItem>& items) {
    m_isFavoritesMode = false;
    m_model->clear();
    for (const auto& item : items) {
        if (item.pageName.isEmpty()) continue;
        auto* row = new DStandardItem(QIcon::fromTheme("view-history"),
            QString("%1(%2) - %3").arg(item.pageName).arg(item.pageSection)
                .arg(QDateTime::fromSecsSinceEpoch(item.visitedAt).toString("MM-dd HH:mm")));
        row->setText(item.pageName);
        row->setData(item.pageSection, Qt::UserRole + 1);
        row->setData(item.id, Qt::UserRole + 2);
        row->setCheckable(true);
        row->setCheckState(Qt::Unchecked);
        m_model->appendRow(row);
    }
}

void PageListDialog::toggleSelectAll() {
    bool anyUnchecked = false;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        auto* item = m_model->item(i);
        if (item && item->checkState() == Qt::Unchecked) { anyUnchecked = true; break; }
    }
    Qt::CheckState state = anyUnchecked ? Qt::Checked : Qt::Unchecked;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        auto* item = m_model->item(i);
        if (item) item->setCheckState(state);
    }
    m_btnSelectAll->setText(anyUnchecked ? "取消全选" : "全选");
}

QList<int> PageListDialog::selectedIds() const {
    QList<int> ids;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        auto* item = m_model->item(i);
        if (item && item->checkState() == Qt::Checked) {
            ids << item->data(Qt::UserRole + 2).toInt();
        }
    }
    return ids;
}

void PageListDialog::deleteSelected() {
    QList<int> ids = selectedIds();
    if (ids.isEmpty()) return;

    if (m_isFavoritesMode) {
        emit favoritesDeleted(ids);
    } else {
        emit historyDeleted(ids);
    }

    QList<int> rows;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        auto* item = m_model->item(i);
        if (item && item->checkState() == Qt::Checked) rows << i;
    }
    for (int i = rows.size() - 1; i >= 0; --i) {
        m_model->removeRow(rows[i]);
    }
    m_btnSelectAll->setText("全选");
}
