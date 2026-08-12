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
    void setProviderList(const QStringList& ids, const QStringList& displayNames);
    void setActiveProvider(const QString& id);

signals:
    void providerChanged(const QString& id);
    void translateRequested(const ManPage& page);
    void examplesRequested(const ManPage& page);
    void questionAsked(const ManPage& page, const QString& question);
    void parseCommandRequested(const QString& cmdline);

private slots:
    void onTranslateClicked();
    void onExamplesClicked();
    void onAskClicked();
    void onParseClicked();

private:
    DLabel* m_titleLabel;
    DComboBox* m_providerCombo;
    DTextEdit* m_chatDisplay;
    DTextEdit* m_inputEdit;
    DPushButton* m_btnTranslate;
    DPushButton* m_btnExamples;
    DPushButton* m_btnAsk;
    DPushButton* m_btnParse;
    DProgressBar* m_progressBar;

    ManPage m_currentPage;
    bool m_hasPage = false;
};
