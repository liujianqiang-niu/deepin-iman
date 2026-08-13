// src/view/AiChatWidget.h
#pragma once
#include <DWidget>
#include <DTextEdit>
#include <DPushButton>
#include <DComboBox>
#include <DLabel>
#include <DProgressBar>
#include "data/ManIndex.h"

DWIDGET_USE_NAMESPACE

struct ManPage;

class AiChatWidget : public DWidget {
    Q_OBJECT
public:
    explicit AiChatWidget(QWidget* parent = nullptr);

    void setCurrentPage(const ManPage& page);
    void appendMessage(const QString& role, const QString& content);
    void appendAiResult(const QString& role, const QString& content, const QString& model);
    void setProviderList(const QStringList& ids, const QStringList& displayNames);
    void setActiveProvider(const QString& id);
    void setProviderModelInfo(const QString& displayName, const QString& model);

signals:
    void providerChanged(const QString& id);
    void translateRequested(const ManPage& page, const QString& targetLang);
    void examplesRequested(const ManPage& page);
    void questionAsked(const ManPage& page, const QString& question);

private slots:
    void onTranslateClicked();
    void onExamplesClicked();
    void onAskClicked();

private:
    DLabel* m_titleLabel;
    DLabel* m_modelLabel;
    DComboBox* m_providerCombo;
    DComboBox* m_langCombo;
    DTextEdit* m_chatDisplay;
    DTextEdit* m_inputEdit;
    DPushButton* m_btnTranslate;
    DPushButton* m_btnExamples;
    DPushButton* m_btnAsk;
    DProgressBar* m_progressBar;

    QString m_currentModel;
    QString m_currentProviderName;
    ManPage m_currentPage;
    bool m_hasPage = false;
};
