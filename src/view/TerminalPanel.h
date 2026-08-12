// src/view/TerminalPanel.h
#pragma once
#include <DWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QStringList>
#include <QEvent>
#include <QKeyEvent>

DWIDGET_USE_NAMESPACE

class TerminalPanel : public DWidget {
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget* parent = nullptr);

    void runCommand(const QString& cmd);
    void toggle();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QTextEdit* m_outputView;
    QLineEdit* m_inputEdit;
    QString m_workDir;
    QStringList m_history;
    int m_historyIdx = -1;

    QString currentPrompt() const;
    void updatePromptPlaceholder();
    void runViaQProcess(const QString& cmd);
};
