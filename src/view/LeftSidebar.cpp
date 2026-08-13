// src/view/LeftSidebar.cpp
#include "LeftSidebar.h"
#include "data/ManIndex.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>

LeftSidebar::LeftSidebar(QWidget* parent) : DWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_searchEdit = new DSearchEdit(this);
    m_searchEdit->setPlaceholderText("搜索 man 手册...");
    layout->addWidget(m_searchEdit);

    auto* btnLayout = new QHBoxLayout;
    m_modeBtn = new DPushButton(modeLabel(), this);
    m_modeBtn->setFixedHeight(28);
    m_modeBtn->setToolTip("点击切换搜索模式");
    btnLayout->addStretch();
    btnLayout->addWidget(m_modeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_navTree = new DTreeView(this);
    m_navModel = new QStandardItemModel(this);
    m_navModel->setHorizontalHeaderLabels({"命令", "章节"});
    m_navTree->setModel(m_navModel);
    m_navTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_navTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layout->addWidget(m_navTree, 1);

    connect(m_searchEdit, &DSearchEdit::textChanged, this, [this](const QString& t) {
        if (t.length() >= 2) emit searchRequested(t, m_mode);
    });
    connect(m_modeBtn, &DPushButton::clicked, this, [this]() {
        m_mode = (m_mode == SearchService::SearchMode::Fuzzy)
                     ? SearchService::SearchMode::Exact
                     : SearchService::SearchMode::Fuzzy;
        m_modeBtn->setText(modeLabel());
    });
    connect(m_navTree, &DTreeView::clicked, this, [this](const QModelIndex& idx) {
        auto* nameItem = m_navModel->item(idx.row(), 0);
        auto* sectionItem = m_navModel->item(idx.row(), 1);
        if (nameItem && sectionItem) {
            emit pageSelected(nameItem->text(), sectionItem->text().toInt());
        }
    });
}

QString LeftSidebar::modeLabel() const {
    return m_mode == SearchService::SearchMode::Fuzzy ? "模糊搜索" : "精确搜索";
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
