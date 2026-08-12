// tests/TestSettingsStore.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include "data/SettingsStore.h"

class TestSettingsStore : public QObject {
    Q_OBJECT
private slots:
    void testWindowLayoutDefaults() {
        QTemporaryDir tmpDir;
        SettingsStore s(tmpDir.path() + "/test.ini");
        QCOMPARE(s.sidebarWidth(), 240);
        QCOMPARE(s.aiPanelWidth(), 360);
        QCOMPARE(s.terminalCollapsed(), true);
    }
    void testSetWindowLayout() {
        QTemporaryDir tmpDir;
        SettingsStore s(tmpDir.path() + "/test.ini");
        s.setSidebarWidth(300);
        s.setAiPanelWidth(400);
        s.setTerminalCollapsed(false);
        QCOMPARE(s.sidebarWidth(), 300);
        QCOMPARE(s.aiPanelWidth(), 400);
        QCOMPARE(s.terminalCollapsed(), false);
    }
    void testAiConsent() {
        QTemporaryDir tmpDir;
        SettingsStore s(tmpDir.path() + "/test.ini");
        QCOMPARE(s.aiContentConsent(), false);
        s.setAiContentConsent(true);
        QCOMPARE(s.aiContentConsent(), true);
    }
};

QTEST_MAIN(TestSettingsStore)
#include "TestSettingsStore.moc"
