// src/view/AiChatWidget.cpp
#include "AiChatWidget.h"
#include "data/ManIndex.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

AiChatWidget::AiChatWidget(QWidget* parent) : DWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    m_titleLabel = new DLabel("AI 助手", this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(m_titleLabel);

    m_providerCombo = new DComboBox(this);
    connect(m_providerCombo, &DComboBox::currentIndexChanged, this, [this](int idx) {
        if (idx >= 0) emit providerChanged(m_providerCombo->itemData(idx).toString());
    });
    mainLayout->addWidget(m_providerCombo);

    m_chatDisplay = new DTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setPlaceholderText("AI 回答将显示在这里...");
    mainLayout->addWidget(m_chatDisplay, 1);

    m_inputEdit = new DTextEdit(this);
    m_inputEdit->setMaximumHeight(80);
    m_inputEdit->setPlaceholderText("输入问题，点击「提问」...");
    mainLayout->addWidget(m_inputEdit);

    m_progressBar = new DProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    auto* btnLayout = new QHBoxLayout;
    m_btnTranslate = new DPushButton("翻译", this);
    m_btnExamples = new DPushButton("生成样例", this);
    m_btnAsk = new DPushButton("提问", this);
    m_btnParse = new DPushButton("解析命令", this);
    btnLayout->addWidget(m_btnTranslate);
    btnLayout->addWidget(m_btnExamples);
    btnLayout->addWidget(m_btnAsk);
    btnLayout->addWidget(m_btnParse);
    mainLayout->addLayout(btnLayout);

    connect(m_btnTranslate, &DPushButton::clicked, this, &AiChatWidget::onTranslateClicked);
    connect(m_btnExamples, &DPushButton::clicked, this, &AiChatWidget::onExamplesClicked);
    connect(m_btnAsk, &DPushButton::clicked, this, &AiChatWidget::onAskClicked);
    connect(m_btnParse, &DPushButton::clicked, this, &AiChatWidget::onParseClicked);

    m_btnTranslate->setEnabled(false);
    m_btnExamples->setEnabled(false);
    m_btnAsk->setEnabled(false);
}

void AiChatWidget::setCurrentPage(const ManPage& page) {
    m_currentPage = page;
    m_hasPage = true;
    m_btnTranslate->setEnabled(true);
    m_btnExamples->setEnabled(true);
    m_btnAsk->setEnabled(true);
    m_titleLabel->setText("AI 助手 - " + page.name + "(" + QString::number(page.section) + ")");
}

void AiChatWidget::appendMessage(const QString& role, const QString& content) {
    QString color = (role == "AI") ? "#0066cc" : "#333333";
    QString msg = QString("<p><b style='color:%1;'>%2:</b> %3</p>").arg(color, role, content);
    m_chatDisplay->append(msg);
}

void AiChatWidget::setProviderList(const QStringList& ids, const QStringList& displayNames) {
    m_providerCombo->clear();
    for (int i = 0; i < ids.size(); ++i) {
        m_providerCombo->addItem(displayNames.value(i, ids[i]), ids[i]);
    }
}

void AiChatWidget::setActiveProvider(const QString& id) {
    for (int i = 0; i < m_providerCombo->count(); ++i) {
        if (m_providerCombo->itemData(i).toString() == id) {
            m_providerCombo->setCurrentIndex(i);
            break;
        }
    }
}

void AiChatWidget::onTranslateClicked() {
    if (m_hasPage) emit translateRequested(m_currentPage);
}

void AiChatWidget::onExamplesClicked() {
    if (m_hasPage) emit examplesRequested(m_currentPage);
}

void AiChatWidget::onAskClicked() {
    QString q = m_inputEdit->toPlainText().trimmed();
    if (q.isEmpty() || !m_hasPage) return;
    emit questionAsked(m_currentPage, q);
    m_inputEdit->clear();
}

void AiChatWidget::onParseClicked() {
    QString cmd = m_inputEdit->toPlainText().trimmed();
    if (cmd.isEmpty()) return;
    emit parseCommandRequested(cmd);
    m_inputEdit->clear();
}
