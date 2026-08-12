// src/view/SettingsDialog.cpp
#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardItem>
#include <QUuid>
#include <QDebug>

SettingsDialog::SettingsDialog(const QList<ProviderConfig>& configs,
                               const QString& activeProvider,
                               QWidget* parent)
    : DDialog(parent), m_configs(configs), m_activeProvider(activeProvider)
{
    setTitle("AI 设置");
    setFixedWidth(520);

    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setSpacing(8);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* listLabel = new DLabel("模型厂商列表：");
    layout->addWidget(listLabel);

    m_model = new QStandardItemModel(this);
    m_listView = new DListView(this);
    m_listView->setModel(m_model);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setMinimumHeight(140);
    m_listView->setMaximumHeight(180);
    layout->addWidget(m_listView);

    auto* btnLayout = new QHBoxLayout;
    m_btnAdd = new DPushButton("添加厂商", this);
    m_btnDelete = new DPushButton("删除厂商", this);
    m_btnSetActive = new DPushButton("设为当前", this);
    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addWidget(m_btnSetActive);
    layout->addLayout(btnLayout);

    auto* sep = new DHorizontalLine(this);
    layout->addWidget(sep);

    auto* editLabel = new DLabel("编辑厂商信息：");
    layout->addWidget(editLabel);

    auto* nameLabel = new DLabel("厂商名称：");
    layout->addWidget(nameLabel);
    m_nameEdit = new DLineEdit(this);
    m_nameEdit->setPlaceholderText("如：OpenAI、智谱 GLM");
    layout->addWidget(m_nameEdit);

    auto* apiBaseLabel = new DLabel("API 地址：");
    layout->addWidget(apiBaseLabel);
    m_apiBaseEdit = new DLineEdit(this);
    m_apiBaseEdit->setPlaceholderText("如：https://api.openai.com/v1/chat/completions");
    layout->addWidget(m_apiBaseEdit);

    auto* apiKeyLabel = new DLabel("API Key：");
    layout->addWidget(apiKeyLabel);
    m_apiKeyEdit = new DLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("在此输入 API Key...");
    layout->addWidget(m_apiKeyEdit);

    auto* modelLabel = new DLabel("模型名称：");
    layout->addWidget(modelLabel);
    m_modelEdit = new DLineEdit(this);
    m_modelEdit->setPlaceholderText("如：gpt-4o、glm-4-plus、qwen-max");
    layout->addWidget(m_modelEdit);

    addContent(widget);

    addButton("取消", false, DDialog::ButtonNormal);
    addButton("保存", true, DDialog::ButtonRecommend);

    updateListDisplay();

    connect(m_listView, &DListView::clicked, this, &SettingsDialog::onSelectionChanged);
    connect(m_nameEdit, &DLineEdit::textChanged, this, &SettingsDialog::onNameChanged);
    connect(m_apiBaseEdit, &DLineEdit::textChanged, this, &SettingsDialog::onApiBaseChanged);
    connect(m_apiKeyEdit, &DLineEdit::textChanged, this, &SettingsDialog::onApiKeyChanged);
    connect(m_modelEdit, &DLineEdit::textChanged, this, &SettingsDialog::onModelChanged);
    connect(m_btnAdd, &DPushButton::clicked, this, &SettingsDialog::onAddClicked);
    connect(m_btnDelete, &DPushButton::clicked, this, &SettingsDialog::onDeleteClicked);
    connect(m_btnSetActive, &DPushButton::clicked, this, &SettingsDialog::onSetActiveClicked);

    if (!m_configs.isEmpty()) {
        m_listView->setCurrentIndex(m_model->index(0, 0));
        populateFields(0);
    } else {
        clearFields();
    }
}

void SettingsDialog::updateListDisplay() {
    m_model->clear();
    for (const auto& cfg : m_configs) {
        QString label = QString("%1  (%2)").arg(cfg.displayName, cfg.model);
        if (cfg.id == m_activeProvider) {
            label = "● " + label;
        }
        auto* item = new DStandardItem(label);
        item->setData(cfg.id, Qt::UserRole);
        m_model->appendRow(item);
    }
}

int SettingsDialog::currentRow() const {
    auto idx = m_listView->currentIndex();
    return idx.isValid() ? idx.row() : -1;
}

void SettingsDialog::populateFields(int row) {
    if (row < 0 || row >= m_configs.size()) {
        clearFields();
        return;
    }
    m_updatingFields = true;
    const auto& cfg = m_configs[row];
    m_nameEdit->setText(cfg.displayName);
    m_apiBaseEdit->setText(cfg.apiBase);
    m_apiKeyEdit->setText(cfg.apiKey);
    m_modelEdit->setText(cfg.model);
    m_updatingFields = false;

    m_btnDelete->setEnabled(m_configs.size() > 1);
    m_btnSetActive->setEnabled(cfg.id != m_activeProvider);
}

void SettingsDialog::clearFields() {
    m_updatingFields = true;
    m_nameEdit->clear();
    m_apiBaseEdit->clear();
    m_apiKeyEdit->clear();
    m_modelEdit->clear();
    m_updatingFields = false;
    m_btnDelete->setEnabled(false);
    m_btnSetActive->setEnabled(false);
}

void SettingsDialog::onSelectionChanged(const QModelIndex& current) {
    populateFields(current.row());
}

void SettingsDialog::onNameChanged(const QString& text) {
    if (m_updatingFields) return;
    int row = currentRow();
    if (row >= 0 && row < m_configs.size()) {
        m_configs[row].displayName = text;
        updateListDisplay();
        m_listView->setCurrentIndex(m_model->index(row, 0));
    }
}

void SettingsDialog::onApiBaseChanged(const QString& text) {
    if (m_updatingFields) return;
    int row = currentRow();
    if (row >= 0 && row < m_configs.size()) {
        m_configs[row].apiBase = text;
    }
}

void SettingsDialog::onApiKeyChanged(const QString& text) {
    if (m_updatingFields) return;
    int row = currentRow();
    if (row >= 0 && row < m_configs.size()) {
        m_configs[row].apiKey = text;
    }
}

void SettingsDialog::onModelChanged(const QString& text) {
    if (m_updatingFields) return;
    int row = currentRow();
    if (row >= 0 && row < m_configs.size()) {
        m_configs[row].model = text;
        updateListDisplay();
        m_listView->setCurrentIndex(m_model->index(row, 0));
    }
}

void SettingsDialog::onAddClicked() {
    ProviderConfig cfg;
    cfg.id = "custom_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    cfg.displayName = "新厂商";
    cfg.apiBase = "https://";
    cfg.apiKey = "";
    cfg.model = "gpt-4o";
    m_configs.append(cfg);
    updateListDisplay();
    int row = m_configs.size() - 1;
    m_listView->setCurrentIndex(m_model->index(row, 0));
    populateFields(row);
    m_nameEdit->setFocus();
}

void SettingsDialog::onDeleteClicked() {
    int row = currentRow();
    if (row < 0 || row >= m_configs.size()) return;
    QString removedId = m_configs[row].id;
    m_configs.removeAt(row);
    if (m_activeProvider == removedId) {
        m_activeProvider = m_configs.isEmpty() ? "" : m_configs.first().id;
    }
    updateListDisplay();
    if (!m_configs.isEmpty()) {
        int newRow = qMin(row, m_configs.size() - 1);
        m_listView->setCurrentIndex(m_model->index(newRow, 0));
        populateFields(newRow);
    } else {
        clearFields();
    }
}

void SettingsDialog::onSetActiveClicked() {
    int row = currentRow();
    if (row < 0 || row >= m_configs.size()) return;
    m_activeProvider = m_configs[row].id;
    updateListDisplay();
    m_listView->setCurrentIndex(m_model->index(row, 0));
    populateFields(row);
}

QList<ProviderConfig> SettingsDialog::configs() const {
    return m_configs;
}

QString SettingsDialog::activeProvider() const {
    return m_activeProvider;
}
