// src/view/LeftSidebar.cpp
#include "LeftSidebar.h"
#include "data/ManIndex.h"
#include <QVBoxLayout>

LeftSidebar::LeftSidebar(QWidget* parent) : DWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new DLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search man pages..."));
    connect(m_searchEdit, &DLineEdit::textChanged, this, [this](const QString& t) {
        if (t.length() >= 2) emit searchRequested(t);
    });
    layout->addWidget(m_searchEdit);

    m_navTree = new DTreeView(this);
    m_navModel = new QStandardItemModel(this);
    m_navModel->setHorizontalHeaderLabels({tr("Command"), tr("Section")});
    m_navTree->setModel(m_navModel);
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
        row << new QStandardItem(p.name);
        row << new QStandardItem(QString::number(p.section));
        m_navModel->appendRow(row);
    }
}
