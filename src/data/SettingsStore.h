// src/data/SettingsStore.h
#pragma once
#include <QString>

class SettingsStore {
public:
    explicit SettingsStore(const QString& filePath = "");
    ~SettingsStore();

    // 窗口布局
    int sidebarWidth() const;
    void setSidebarWidth(int width);
    int aiPanelWidth() const;
    void setAiPanelWidth(int width);
    bool terminalCollapsed() const;
    void setTerminalCollapsed(bool collapsed);

    // 隐私确认 (Phase 2 AI 用)
    bool aiContentConsent() const;
    void setAiContentConsent(bool consent);

private:
    QString m_filePath;
};
