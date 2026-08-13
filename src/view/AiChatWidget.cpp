// src/view/AiChatWidget.cpp
#include "AiChatWidget.h"
#include "data/ManIndex.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextDocument>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyle>

AiChatWidget::AiChatWidget(QWidget* parent) : DWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    m_titleLabel = new DLabel("AI 助手", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPixelSize(14);
    m_titleLabel->setFont(titleFont);
    mainLayout->addWidget(m_titleLabel);

    m_modelLabel = new DLabel("当前模型：未配置", this);
    mainLayout->addWidget(m_modelLabel);

    m_providerCombo = new DComboBox(this);
    connect(m_providerCombo, &DComboBox::currentIndexChanged, this, [this](int idx) {
        if (idx >= 0) {
            m_currentProviderName = m_providerCombo->itemText(idx);
            emit providerChanged(m_providerCombo->itemData(idx).toString());
        }
    });
    mainLayout->addWidget(m_providerCombo);

    m_chatDisplay = new QTextBrowser(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setOpenExternalLinks(true);
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
    btnLayout->addWidget(m_btnTranslate);
    btnLayout->addWidget(m_btnExamples);
    btnLayout->addWidget(m_btnAsk);
    mainLayout->addLayout(btnLayout);

    connect(m_btnTranslate, &DPushButton::clicked, this, &AiChatWidget::onTranslateClicked);
    connect(m_btnExamples, &DPushButton::clicked, this, &AiChatWidget::onExamplesClicked);
    connect(m_btnAsk, &DPushButton::clicked, this, &AiChatWidget::onAskClicked);

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
    QString msg = QString("<p><b style='color:%1;'>%2:</b> %3</p>")
                      .arg(color, role, content.toHtmlEscaped());
    m_chatDisplay->append(msg);
}

void AiChatWidget::appendAiResult(const QString& role, const QString& content, const QString& model) {
    QString color = (role == "AI") ? "#0066cc" : "#333333";
    QString modelTag = model.isEmpty() ? "" :
        QString(" <span style='color:#999; font-size:11px;'>[%1]</span>").arg(model.toHtmlEscaped());

    QTextDocument doc;
    doc.setMarkdown(content);
    QString fullHtml = doc.toHtml();

    int bodyStart = fullHtml.indexOf("<body>");
    int bodyEnd = fullHtml.indexOf("</body>");
    QString bodyHtml = (bodyStart >= 0 && bodyEnd > bodyStart)
        ? fullHtml.mid(bodyStart + 6, bodyEnd - bodyStart - 6)
        : fullHtml;

    bool isDark = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;
    QString codeBg = isDark ? "#2a2a2a" : "#f0f0f0";
    QString codeColor = isDark ? "#e0e0e0" : "#333333";
    QString preBg = isDark ? "#1e1e1e" : "#f5f5f5";
    QString borderColor = isDark ? "#444444" : "#dddddd";

    bodyHtml.prepend(QString(
        "<style>"
        "code{background:%1;color:%2;padding:1px 4px;border-radius:3px;font-family:monospace;}"
        "pre{background:%3;padding:8px;border-radius:4px;overflow-x:auto;color:%4;}"
        "pre code{background:transparent;padding:0;color:%4;}"
        "h1,h2,h3{margin-top:8px;margin-bottom:4px;}"
        "ul,ol{margin:4px 0;padding-left:20px;}"
        "p{margin:4px 0;}"
        "table{border-collapse:collapse;}"
        "th,td{border:1px solid %5;padding:4px 8px;}"
        "</style>")
        .arg(codeBg, codeColor, preBg, codeColor, borderColor));

    QString header = QString("<p><b style='color:%1;'>%2:</b>%3</p>").arg(color, role, modelTag);
    m_chatDisplay->append(header);
    m_chatDisplay->insertHtml(bodyHtml);
    m_chatDisplay->append("");
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
            m_currentProviderName = m_providerCombo->itemText(i);
            break;
        }
    }
}

void AiChatWidget::setProviderModelInfo(const QString& displayName, const QString& model) {
    m_currentProviderName = displayName;
    m_currentModel = model;
    if (!model.isEmpty()) {
        m_modelLabel->setText(QString("当前模型：%1 / %2").arg(displayName, model));
    } else {
        m_modelLabel->setText(QString("当前模型：%1").arg(displayName));
    }
}

void AiChatWidget::onTranslateClicked() {
    if (m_hasPage) {
        emit translateRequested(m_currentPage);
    }
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
