// src/view/TerminalPanel.h
#pragma once
#include <DWidget>
#include <QTextEdit>

DWIDGET_USE_NAMESPACE

class TerminalPanel : public DWidget {
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget* parent = nullptr);

    void runCommand(const QString& cmd);
    void toggle();

private:
    QTextEdit* m_outputView;
    void runViaQProcess(const QString& cmd);
};
