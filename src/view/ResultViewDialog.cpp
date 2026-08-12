// src/view/ResultViewDialog.cpp
#include "ResultViewDialog.h"
#include <QVBoxLayout>

ResultViewDialog::ResultViewDialog(const QString& title, const QString& content, QWidget* parent)
    : DDialog(parent)
{
    setTitle(title);
    setFixedSize(640, 520);

    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 4, 4, 4);

    m_textView = new DTextEdit(widget);
    m_textView->setReadOnly(true);
    m_textView->setPlainText(content);
    layout->addWidget(m_textView);

    addContent(widget);

    addButton("关闭", true, DDialog::ButtonRecommend);
}
