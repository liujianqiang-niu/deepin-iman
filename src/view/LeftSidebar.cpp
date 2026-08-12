// src/view/LeftSidebar.cpp
#include "LeftSidebar.h"
#include "data/ManIndex.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QIcon>

LeftSidebar::LeftSidebar(QWidget* parent) : DWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new DSearchEdit(this);
    m_searchEdit->setPlaceholderText("搜索 man 手册...");
    connect(m_searchEdit, &DSearchEdit::textChanged, this, [this](const QString& t) {
        if (t.length() >= 2) emit searchRequested(t);
    });
    layout->addWidget(m_searchEdit);

    m_navTree = new DTreeView(this);
    m_navModel = new QStandardItemModel(this);
    m_navModel->setHorizontalHeaderLabels({"命令", "章节"});
    auto* hdr0 = m_navModel->horizontalHeaderItem(0);
    auto* hdr1 = m_navModel->horizontalHeaderItem(1);
    if (hdr0) hdr0->setIcon(QIcon::fromTheme("text-x-script"));
    if (hdr1) hdr1->setIcon(QIcon::fromTheme("dialog-information"));
    m_navTree->setModel(m_navModel);
    m_navTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_navTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(m_navTree, &DTreeView::clicked, this, [this](const QModelIndex& idx) {
        auto* nameItem = m_navModel->item(idx.row(), 0);
        auto* sectionItem = m_navModel->item(idx.row(), 1);
        if (nameItem && sectionItem) {
            emit pageSelected(nameItem->text(), sectionItem->text().toInt());
        }
    });
    layout->addWidget(m_navTree, 1);
}

void LeftSidebar::setManPages(const QList<ManPage>& pages) {
    m_navModel->removeRows(0, m_navModel->rowCount());
    for (const auto& p : pages) {
        QList<QStandardItem*> row;
        auto* nameItem = new QStandardItem(QIcon::fromTheme("text-x-script"), p.name);
        auto* sectionItem = new QStandardItem(QString::number(p.section));
        row << nameItem << sectionItem;
        m_navModel->appendRow(row);
    }
}
