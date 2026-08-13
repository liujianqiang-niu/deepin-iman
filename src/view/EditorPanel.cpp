// src/view/EditorPanel.cpp
#include "EditorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

EditorPanel::EditorPanel(QWidget* parent) : DWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* titleBar = new DWidget(this);
    titleBar->setFixedHeight(32);
    auto* barLayout = new QHBoxLayout(titleBar);
    barLayout->setContentsMargins(10, 0, 4, 0);
    barLayout->setSpacing(0);

    m_titleLabel = new DLabel(titleBar);
    m_closeBtn = new DIconButton(titleBar);
    m_closeBtn->setIcon(QIcon::fromTheme("window-close"));
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setToolTip("关闭");

    barLayout->addWidget(m_titleLabel);
    barLayout->addStretch();
    barLayout->addWidget(m_closeBtn);
    mainLayout->addWidget(titleBar);

    m_contentHost = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(m_contentHost);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    mainLayout->addWidget(m_contentHost, 1);

    connect(m_closeBtn, &DIconButton::clicked, this, &EditorPanel::closed);
}

void EditorPanel::setTitle(const QString& title) { m_titleLabel->setText(title); }
QString EditorPanel::title() const { return m_titleLabel->text(); }

void EditorPanel::setContent(QWidget* w) {
    if (!w) return;
    auto* l = qobject_cast<QVBoxLayout*>(m_contentHost->layout());
    if (!l) return;
    while (l->count() > 0) {
        auto* item = l->takeAt(0);
        if (item && item->widget()) item->widget()->setParent(nullptr);
    }
    w->setParent(m_contentHost);
    l->addWidget(w);
}

QWidget* EditorPanel::contentWidget() const {
    auto* l = qobject_cast<QVBoxLayout*>(m_contentHost->layout());
    if (l && l->count() > 0) return l->itemAt(0)->widget();
    return nullptr;
}
