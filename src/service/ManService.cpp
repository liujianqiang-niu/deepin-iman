// src/service/ManService.cpp
#include "ManService.h"
#include <QProcess>
#include <QRegularExpression>
#include <QFileInfo>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

ManService::ManService(QObject* parent) : QObject(parent) {
}

QString ManService::renderPage(const QString& gzPath) {
    if (!QFileInfo::exists(gzPath)) {
        qWarning() << "ManService: file not found" << gzPath;
        return QString();
    }

    QProcess p;
    p.start("man", {"-P", "cat", "-l", gzPath});
    if (!p.waitForFinished(5000)) {
        qWarning() << "ManService: man timeout" << gzPath;
        return QString();
    }
    if (p.exitCode() != 0) {
        qWarning() << "ManService: man error" << p.readAllStandardError();
        return QString();
    }

    QString plainText = QString::fromUtf8(p.readAllStandardOutput());
    QString html = ansiToHtml(plainText);

    // Parse SEE ALSO section and inject cross-reference links
    QRegularExpression seeAlsoRe(
        "SEE ALSO.*?\\n(.*?)(?:\\n\\n|\\n[A-Z]})", 
        QRegularExpression::DotMatchesEverythingOption);
    auto match = seeAlsoRe.match(html);
    if (match.hasMatch()) {
        QString seeAlsoContent = match.captured(1);
        QRegularExpression cmdRe("\\b([a-zA-Z_][a-zA-Z0-9_-]+)\\((\\d+)\\)");
        auto it = cmdRe.globalMatch(seeAlsoContent);
        QString updated = seeAlsoContent;
        while (it.hasNext()) {
            auto m = it.next();
            QString cmd = m.captured(0);
            QString name = m.captured(1);
            QString section = m.captured(2);
            QString link = QString("<a href=\"man:%1(%2)\">%3</a>").arg(name, section, cmd);
            updated.replace(cmd, link);
        }
        html.replace(seeAlsoContent, updated);
    }

    return html;
}

QString ManService::ansiToHtml(const QString& ansiText) const {
    QString text = ansiText;
    text.replace("&", "&amp;");
    text.replace("<", "&lt;");
    text.replace(">", "&gt;");

    // Bold: \033[1m ... \033[0m or \033[22m
    text.replace("\033[1m", "<b>");
    text.replace("\033[22m", "</b>");
    text.replace("\033[0m", "</b></u>");

    // Underline: \033[4m ... \033[24m
    text.replace("\033[4m", "<u>");
    text.replace("\033[24m", "</u>");

    // Remove other ANSI sequences
    text.remove(QRegularExpression("\033\\[[0-9;]*m"));

    // Convert line breaks
    text.replace("\n", "<br>\n");

    return text;
}

QList<CrossReference> ManService::parseCrossReferences(const QString& html) const {
    QList<CrossReference> refs;
    QRegularExpression re("href=\"man:([^(]+)\\((\\d+)\\)\"");
    auto it = re.globalMatch(html);
    while (it.hasNext()) {
        auto m = it.next();
        CrossReference ref;
        ref.name = m.captured(1);
        ref.section = m.captured(2).toInt();
        refs << ref;
    }
    return refs;
}

void ManService::renderPageAsync(const QString& gzPath) {
    QtConcurrent::run([this, gzPath]() {
        QString html = renderPage(gzPath);
        if (html.isEmpty()) {
            emit renderFailed(tr("Failed to render: %1").arg(gzPath));
        } else {
            emit pageRendered(html);
        }
    });
}
