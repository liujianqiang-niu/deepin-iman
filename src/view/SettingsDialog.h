// src/view/SettingsDialog.h
#pragma once
#include <DDialog>
#include <DLineEdit>
#include <DComboBox>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class SettingsDialog : public DDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    QString activeProvider() const;
    QString apiKey(const QString& providerId) const;

private slots:
    void onProviderChanged(int index);
    void onSaveClicked();

private:
    DComboBox* m_providerCombo;
    DLineEdit* m_keyEdit;
    DLabel* m_hintLabel;
    QMap<QString, QString> m_keys;
    QString m_activeProvider;
};
