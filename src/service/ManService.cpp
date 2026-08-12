// src/service/ManService.cpp
#include "ManService.h"
#include <QProcess>
#include <QRegularExpression>
#include <QFileInfo>
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
    p.start("mandoc", {"-Thtml", "-O", "fragment", gzPath});
    if (!p.waitForFinished(5000)) {
        qWarning() << "ManService: mandoc timeout" << gzPath;
        return QString();
    }
    if (p.exitCode() != 0) {
        qWarning() << "ManService: mandoc error" << p.readAllStandardError();
        return QString();
    }

    QString html = QString::fromUtf8(p.readAllStandardOutput());
    return injectCrossRefLinks(html);
}

QString ManService::injectCrossRefLinks(const QString& html) const {
    QString result = html;
    // mandoc renders cross-refs as <b>name</b>(section) — wrap in <a href="man:...">
    QRegularExpression re("<b>([a-zA-Z_][a-zA-Z0-9_.+-]+)</b>\\((\\d+)\\)");
    auto it = re.globalMatch(result);
    while (it.hasNext()) {
        auto m = it.next();
        QString fullMatch = m.captured(0);
        QString name = m.captured(1);
        QString section = m.captured(2);
        QString link = QString("<a href=\"man:%1(%2)\">%3</a>")
                          .arg(name, section, fullMatch);
        result.replace(fullMatch, link);
    }
    return result;
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
    auto future = QtConcurrent::run([this, gzPath]() {
        QString html = renderPage(gzPath);
        if (html.isEmpty()) {
            emit renderFailed(tr("Failed to render: %1").arg(gzPath));
        } else {
            emit pageRendered(html);
        }
    });
    Q_UNUSED(future)
}
