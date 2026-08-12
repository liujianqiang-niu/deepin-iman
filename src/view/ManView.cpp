// src/view/ManView.cpp
#include "ManView.h"
#include <QUrl>
#include <QRegularExpression>

ManView::ManView(QWidget* parent) : QTextBrowser(parent) {
    setOpenExternalLinks(false);
    connect(this, &QTextBrowser::anchorClicked, this, &ManView::onAnchorClicked);
}

void ManView::loadHtml(const QString& html) {
    setHtml(html);
}

void ManView::onAnchorClicked(const QUrl& url) {
    if (url.scheme() != "man") {
        QTextBrowser::setSource(url);
        return;
    }
    QString path = url.path().isEmpty() ? url.toString().mid(4) : url.path();
    QRegularExpression re("^(.+)\\((\\d+)\\)$");
    auto m = re.match(path);
    if (m.hasMatch()) {
        emit crossReferenceClicked(m.captured(1), m.captured(2).toInt());
    }
}
