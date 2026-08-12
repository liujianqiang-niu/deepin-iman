// src/view/SettingsDialog.h
#pragma once
#include <DDialog>
#include <DLineEdit>
#include <DListView>
#include <DLabel>
#include <DPushButton>
#include <DHorizontalLine>
#include <QStandardItemModel>
#include <QList>
#include "service/ai/AiService.h"

DWIDGET_USE_NAMESPACE

class SettingsDialog : public DDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const QList<ProviderConfig>& configs,
                           const QString& activeProvider,
                           QWidget* parent = nullptr);

    QList<ProviderConfig> configs() const;
    QString activeProvider() const;

private slots:
    void onSelectionChanged(const QModelIndex& current);
    void onNameChanged(const QString& text);
    void onApiBaseChanged(const QString& text);
    void onApiKeyChanged(const QString& text);
    void onModelChanged(const QString& text);
    void onAddClicked();
    void onDeleteClicked();
    void onSetActiveClicked();
    void onTestClicked();

private:
    void updateListDisplay();
    void populateFields(int row);
    void clearFields();
    int currentRow() const;
    ProviderConfig currentConfig() const;

    DListView* m_listView;
    QStandardItemModel* m_model;
    DLineEdit* m_nameEdit;
    DLineEdit* m_apiBaseEdit;
    DLineEdit* m_apiKeyEdit;
    DLineEdit* m_modelEdit;
    DPushButton* m_btnAdd;
    DPushButton* m_btnDelete;
    DPushButton* m_btnSetActive;
    DPushButton* m_btnTest;
    DLabel* m_statusLabel;

    QList<ProviderConfig> m_configs;
    QString m_activeProvider;
    bool m_updatingFields = false;
};
