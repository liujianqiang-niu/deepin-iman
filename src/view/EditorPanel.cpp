// src/view/EditorPanel.cpp
#include "EditorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>

EditorPanel::EditorPanel(QWidget* parent) : DWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_titleBar = new DWidget(this);
    m_titleBar->setFixedHeight(32);
    m_titleBar->installEventFilter(this);
    auto* barLayout = new QHBoxLayout(m_titleBar);
    barLayout->setContentsMargins(10, 0, 4, 0);
    barLayout->setSpacing(0);

    m_titleLabel = new DLabel(m_titleBar);
    m_detachBtn = new DIconButton(m_titleBar);
    m_detachBtn->setIcon(QIcon::fromTheme("window-new"));
    m_detachBtn->setFixedSize(24, 24);
    m_detachBtn->setToolTip("弹出为独立窗口");
    m_closeBtn = new DIconButton(m_titleBar);
    m_closeBtn->setIcon(QIcon::fromTheme("window-close"));
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setToolTip("关闭");

    barLayout->addWidget(m_titleLabel);
    barLayout->addStretch();
    barLayout->addWidget(m_detachBtn);
    barLayout->addWidget(m_closeBtn);
    mainLayout->addWidget(m_titleBar);

    m_contentHost = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(m_contentHost);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    mainLayout->addWidget(m_contentHost, 1);

    connect(m_closeBtn, &DIconButton::clicked, this, &EditorPanel::closed);
    connect(m_detachBtn, &DIconButton::clicked, this, &EditorPanel::detachRequested);
}

bool EditorPanel::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_titleBar && event->type() == QEvent::MouseButtonDblClick) {
        emit detachRequested();
        return true;
    }
    return DWidget::eventFilter(obj, event);
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
