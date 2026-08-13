// src/view/SplitViewDialog.cpp
#include "SplitViewDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>

SplitViewDialog::SplitViewDialog(const QString& leftTitle, const QString& leftContent,
                                 const QString& rightTitle, const QString& rightContent,
                                 QWidget* parent)
    : DDialog(parent)
{
    setTitle(QString("%1 / %2").arg(leftTitle, rightTitle));
    setFixedSize(1100, 600);

    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* splitter = new QSplitter(Qt::Horizontal, widget);

    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    auto* leftLabel = new DLabel(leftTitle);
    leftLabel->setStyleSheet("font-weight: bold; padding: 4px; background: #f0f0f0;");
    leftLayout->addWidget(leftLabel);
    m_leftView = new DTextEdit(leftPanel);
    m_leftView->setReadOnly(true);
    m_leftView->setPlainText(leftContent);
    leftLayout->addWidget(m_leftView, 1);
    splitter->addWidget(leftPanel);

    auto* rightPanel = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    auto* rightLabel = new DLabel(rightTitle);
    rightLabel->setStyleSheet("font-weight: bold; padding: 4px; background: #f0f0f0;");
    rightLayout->addWidget(rightLabel);
    m_rightView = new DTextEdit(rightPanel);
    m_rightView->setReadOnly(true);
    m_rightView->setPlainText(rightContent);
    rightLayout->addWidget(m_rightView, 1);
    splitter->addWidget(rightPanel);

    splitter->setSizes({500, 550});
    layout->addWidget(splitter);

    addContent(widget);

    addButton("关闭", true, DDialog::ButtonRecommend);
}
