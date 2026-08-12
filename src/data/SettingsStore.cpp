// src/data/SettingsStore.cpp
#include "SettingsStore.h"
#include <QSettings>

SettingsStore::SettingsStore(const QString& filePath)
    : m_filePath(filePath.isEmpty() ? "deepin-iman" : filePath)
{
}

SettingsStore::~SettingsStore() = default;

int SettingsStore::sidebarWidth() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("layout/sidebarWidth", 240).toInt();
}

void SettingsStore::setSidebarWidth(int width) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("layout/sidebarWidth", width);
}

int SettingsStore::aiPanelWidth() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("layout/aiPanelWidth", 360).toInt();
}

void SettingsStore::setAiPanelWidth(int width) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("layout/aiPanelWidth", width);
}

bool SettingsStore::terminalCollapsed() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("layout/terminalCollapsed", true).toBool();
}

void SettingsStore::setTerminalCollapsed(bool collapsed) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("layout/terminalCollapsed", collapsed);
}

bool SettingsStore::aiContentConsent() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("ai/contentConsent", false).toBool();
}

void SettingsStore::setAiContentConsent(bool consent) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("ai/contentConsent", consent);
}
