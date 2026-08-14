// src/view/AiChatWidget.h
#pragma once
#include <DWidget>
#include <DTextEdit>
#include <DPushButton>
#include <DIconButton>
#include <DComboBox>
#include <DLabel>
#include <DProgressBar>
#include <QTextBrowser>
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
    void clearChat();
    void setProviderList(const QStringList& ids, const QStringList& displayNames);
    void setActiveProvider(const QString& id);
    void setProviderModelInfo(const QString& displayName, const QString& model);

signals:
    void providerChanged(const QString& id);
    void translateRequested(const ManPage& page);
    void examplesRequested(const ManPage& page);
    void questionAsked(const ManPage& page, const QString& question);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onTranslateClicked();
    void onExamplesClicked();
    void onAskClicked();

private:
    DLabel* m_titleLabel;
    DLabel* m_modelLabel;
    DIconButton* m_btnNewChat;
    DComboBox* m_providerCombo;
    QTextBrowser* m_chatDisplay;
    QTextEdit* m_inputEdit;
    DPushButton* m_btnTranslate;
    DPushButton* m_btnExamples;
    DPushButton* m_btnAsk;
    DProgressBar* m_progressBar;

    QString m_currentModel;
    QString m_currentProviderName;
    ManPage m_currentPage;
    bool m_hasPage = false;
};
