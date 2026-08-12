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
    html = convertToQtHtml(html);
    html = injectCrossRefLinks(html);
    return html;
}

QString ManService::convertToQtHtml(const QString& mandocHtml) const {
    QString html = mandocHtml;

    // Remove permalink anchors BEFORE stripping class attributes
    // mandoc pattern: <a class="permalink" href="#NAME">NAME</a> → NAME
    html.replace(QRegularExpression("<a[^>]*permalink[^>]*>(.*?)</a>"), "\\1");

    // Strip all class="..." attributes (QTextBrowser ignores CSS classes)
    html.remove(QRegularExpression("\\s+class=\"[^\"]*\""));

    // Remove <table>...</table> header/footer blocks (title/date duplicate)
    html.remove(QRegularExpression("<table>\\s*<tr>\\s*<td.*?head-ltitle.*?</table>",
        QRegularExpression::DotMatchesEverythingOption));
    html.remove(QRegularExpression("<table>\\s*<tr>\\s*<td.*?foot-date.*?</table>",
        QRegularExpression::DotMatchesEverythingOption));

    // HTML5 → HTML4 tag conversions
    html.replace("<section", "<div");
    html.replace("</section>", "</div>");

    // <h1 class="Sh"> → <h2> (h1 is too large in QTextBrowser)
    html.replace(QRegularExpression("<h1[^>]*>"), "<h2>");
    html.replace("</h1>", "</h2>");
    html.replace(QRegularExpression("<h2[^>]*>"), "<h3>");
    html.replace("</h2>", "</h3>");

    // <code> → <tt> for better QTextBrowser compat
    html.replace("<code", "<tt");
    html.replace("</code>", "</tt>");

    html.replace("<br/>", "<br>");

    // Clean up id="..." attributes and leftover whitespace before >
    html.remove(QRegularExpression("\\s+id=\"[^\"]*\""));
    html.replace(QRegularExpression("\\s+>"), ">");

    // Remove empty <div></div> wrappers
    html.replace(QRegularExpression("<div>\\s*</div>"), "");

    return html.trimmed();
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
