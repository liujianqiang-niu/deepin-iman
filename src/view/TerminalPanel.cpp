// src/view/TerminalPanel.cpp
#include "TerminalPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

TerminalPanel::TerminalPanel(QWidget* parent) : DWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_outputView = new QTextEdit(this);
    m_outputView->setReadOnly(true);
    m_outputView->setFont(QFont("Monospace", 10));
    m_outputView->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(m_outputView, 1);
}

void TerminalPanel::runCommand(const QString& cmd) {
    m_outputView->append(QString("<b style='color:#569cd6;'>$ %1</b>").arg(cmd.toHtmlEscaped()));
    runViaQProcess(cmd);
}

void TerminalPanel::runViaQProcess(const QString& cmd) {
    auto* proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    QString shell = "/bin/bash";
    QStringList args = {"-c", cmd};

    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc]() {
        QString output = QString::fromUtf8(proc->readAll());
        m_outputView->append(output.toHtmlEscaped());
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int code, QProcess::ExitStatus) {
        if (code != 0) {
            m_outputView->append(QString("<span style='color:#f44747;'>[退出码: %1]</span>").arg(code));
        }
        proc->deleteLater();
    });

    proc->start(shell, args);
}

void TerminalPanel::toggle() {
    setVisible(!isVisible());
}
