// src/view/TranslationView.cpp
#include "TranslationView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

TranslationView::TranslationView(QWidget* parent) : DWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* bar = new QHBoxLayout;
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("仅英文", 0);
    m_modeCombo->addItem("仅中文", 1);
    m_modeCombo->addItem("中英对照", 2);
    m_modeCombo->setCurrentIndex(2);
    bar->addWidget(m_modeCombo);
    bar->addStretch();
    layout->addLayout(bar);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_enView = new QTextBrowser(this);
    m_zhView = new QTextBrowser(this);
    m_splitter->addWidget(m_enView);
    m_splitter->addWidget(m_zhView);
    m_splitter->setSizes({500, 500});
    layout->addWidget(m_splitter, 1);

    connect(m_modeCombo, &QComboBox::currentIndexChanged, this, &TranslationView::onModeChanged);
}

void TranslationView::setEnglishHtml(const QString& html) {
    m_enHtml = html;
    m_enView->setHtml(html);
}

void TranslationView::setChineseHtml(const QString& html) {
    m_zhHtml = html;
    m_zhView->setHtml(html);
}

void TranslationView::onModeChanged(int idx) {
    int mode = m_modeCombo->itemData(idx).toInt();
    if (mode == 0) {
        m_splitter->widget(1)->setVisible(false);
    } else if (mode == 1) {
        m_splitter->widget(0)->setVisible(false);
    } else {
        m_splitter->widget(0)->setVisible(true);
        m_splitter->widget(1)->setVisible(true);
    }
}
