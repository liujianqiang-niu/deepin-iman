// src/view/SettingsDialog.cpp
#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QLineEdit>

SettingsDialog::SettingsDialog(QWidget* parent) : DDialog(parent) {
    setTitle("AI 设置");
    setFixedWidth(480);

    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setSpacing(12);

    auto* providerLabel = new DLabel("AI 供应商：");
    layout->addWidget(providerLabel);

    m_providerCombo = new DComboBox;
    m_providerCombo->addItem("OpenAI GPT-4o", "openai");
    m_providerCombo->addItem("Claude 3.5 Sonnet", "claude");
    m_providerCombo->addItem("通义千问 Qwen-Max", "qwen");
    m_providerCombo->addItem("智谱 GLM-4-Plus", "glm");
    layout->addWidget(m_providerCombo);

    auto* keyLabel = new DLabel("API Key：");
    layout->addWidget(keyLabel);

    m_keyEdit = new DLineEdit;
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setPlaceholderText("在此输入 API Key...");
    layout->addWidget(m_keyEdit);

    m_hintLabel = new DLabel("");
    m_hintLabel->setStyleSheet("color: #888; font-size: 12px;");
    layout->addWidget(m_hintLabel);

    addContent(widget);

    addButton("取消", false, DDialog::ButtonNormal);
    addButton("保存", true, DDialog::ButtonRecommend);

    QSettings settings("deepin", "deepin-iman");
    m_activeProvider = settings.value("ai/active_provider", "glm").toString();
    for (const auto& id : {"openai", "claude", "qwen", "glm"}) {
        m_keys[id] = settings.value("ai/" + QString(id) + "_key").toString();
    }
    for (int i = 0; i < m_providerCombo->count(); ++i) {
        if (m_providerCombo->itemData(i).toString() == m_activeProvider) {
            m_providerCombo->setCurrentIndex(i);
            break;
        }
    }

    connect(m_providerCombo, &DComboBox::currentIndexChanged, this, &SettingsDialog::onProviderChanged);
    connect(this, &SettingsDialog::buttonClicked, this, [this](int idx) {
        if (idx == 1) onSaveClicked();
    });

    onProviderChanged(m_providerCombo->currentIndex());
}

void SettingsDialog::onProviderChanged(int index) {
    QString id = m_providerCombo->itemData(index).toString();
    m_keyEdit->setText(m_keys.value(id));
    if (id == "openai") m_hintLabel->setText("https://platform.openai.com/api-keys");
    else if (id == "claude") m_hintLabel->setText("https://console.anthropic.com/");
    else if (id == "qwen") m_hintLabel->setText("https://dashscope.console.aliyun.com/");
    else if (id == "glm") m_hintLabel->setText("https://open.bigmodel.cn/");
}

void SettingsDialog::onSaveClicked() {
    QString id = m_providerCombo->currentData().toString();
    m_keys[id] = m_keyEdit->text();
    m_activeProvider = id;

    QSettings settings("deepin", "deepin-iman");
    settings.setValue("ai/active_provider", id);
    for (auto it = m_keys.begin(); it != m_keys.end(); ++it) {
        settings.setValue("ai/" + it.key() + "_key", it.value());
    }
}

QString SettingsDialog::activeProvider() const {
    return m_activeProvider;
}

QString SettingsDialog::apiKey(const QString& providerId) const {
    return m_keys.value(providerId);
}
