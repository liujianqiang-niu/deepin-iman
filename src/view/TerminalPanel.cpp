// src/view/TerminalPanel.cpp
#include "TerminalPanel.h"
#include <QVBoxLayout>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

TerminalPanel::TerminalPanel(QWidget* parent) : DWidget(parent) {
    m_workDir = QDir::homePath();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_outputView = new QTextEdit(this);
    m_outputView->setReadOnly(true);
    m_outputView->setFont(QFont("Monospace", 10));
    m_outputView->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; border: none;");
    m_outputView->append("<span style='color:#6a9955;'>deepin-iman 终端 — 输入命令按 Enter 执行，↑↓ 浏览历史</span>");
    layout->addWidget(m_outputView, 1);

    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setFont(QFont("Monospace", 10));
    m_inputEdit->setStyleSheet("background-color: #2d2d2d; color: #d4d4d4; border: none; padding: 4px;");
    updatePromptPlaceholder();
    layout->addWidget(m_inputEdit);

    connect(m_inputEdit, &QLineEdit::returnPressed, this, [this]() {
        QString cmd = m_inputEdit->text().trimmed();
        if (cmd.isEmpty()) return;
        if (!m_history.isEmpty() && m_history.last() != cmd) {
            m_history << cmd;
        } else if (m_history.isEmpty()) {
            m_history << cmd;
        }
        m_historyIdx = m_history.size();
        runCommand(cmd);
        m_inputEdit->clear();
    });

    m_inputEdit->installEventFilter(this);
}

bool TerminalPanel::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_inputEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (m_historyIdx > 0) {
                --m_historyIdx;
                m_inputEdit->setText(m_history.value(m_historyIdx));
            }
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down) {
            if (m_historyIdx < m_history.size() - 1) {
                ++m_historyIdx;
                m_inputEdit->setText(m_history.value(m_historyIdx));
            } else {
                m_historyIdx = m_history.size();
                m_inputEdit->clear();
            }
            return true;
        }
    }
    return DWidget::eventFilter(obj, event);
}

QString TerminalPanel::currentPrompt() const {
    QString shortDir = m_workDir;
    QString home = QDir::homePath();
    if (shortDir == home) shortDir = "~";
    else if (shortDir.startsWith(home + "/")) shortDir = "~" + shortDir.mid(home.length());
    return QString("<span style='color:#4ec9b0;'>uos@deepin</span><span style='color:#d4d4d4;'>:</span><span style='color:#569cd6;'>%1</span><span style='color:#d4d4d4;'>$</span>").arg(shortDir);
}

void TerminalPanel::updatePromptPlaceholder() {
    QString shortDir = m_workDir;
    QString home = QDir::homePath();
    if (shortDir == home) shortDir = "~";
    else if (shortDir.startsWith(home + "/")) shortDir = "~" + shortDir.mid(home.length());
    m_inputEdit->setPlaceholderText(QString("uos@deepin:%1$ 输入命令...").arg(shortDir));
}

void TerminalPanel::runCommand(const QString& cmd) {
    if (cmd.startsWith("cd ") || cmd == "cd") {
        QString target = cmd.mid(3).trimmed();
        if (target.isEmpty()) target = QDir::homePath();
        QDir dir(target);
        if (!dir.isAbsolute()) dir.setPath(m_workDir + "/" + target);
        QString resolved = QDir::cleanPath(dir.absolutePath());
        if (QFileInfo::exists(resolved) && QFileInfo(resolved).isDir()) {
            m_workDir = resolved;
            m_outputView->append(currentPrompt() + " " + cmd.toHtmlEscaped());
            updatePromptPlaceholder();
        } else {
            m_outputView->append(currentPrompt() + " " + cmd.toHtmlEscaped());
            m_outputView->append(QString("<span style='color:#f44747;'>cd: %1: 没有那个文件或目录</span>").arg(target.toHtmlEscaped()));
        }
        return;
    }

    m_outputView->append(currentPrompt() + " " + cmd.toHtmlEscaped());
    runViaQProcess(cmd);
}

void TerminalPanel::runViaQProcess(const QString& cmd) {
    auto* proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->setWorkingDirectory(m_workDir);

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
    if (isVisible()) m_inputEdit->setFocus();
}
