// src/service/ManService.h
#pragma once
#include <QObject>
#include <QString>
#include <QList>

struct CrossReference {
    QString name;
    int section;
};

class ManService : public QObject {
    Q_OBJECT
public:
    explicit ManService(QObject* parent = nullptr);

    // mandoc -Thtml -O fragment → HTML, then inject cross-ref links
    QString renderPage(const QString& gzPath);

    // Parse <a href="man:name(section)"> from rendered HTML
    QList<CrossReference> parseCrossReferences(const QString& html) const;

    // Inject man: hyperlinks into SEE ALSO <b>name</b>(section) patterns
    QString injectCrossRefLinks(const QString& html) const;

    void renderPageAsync(const QString& gzPath);

signals:
    void pageRendered(const QString& html);
    void renderFailed(const QString& reason);
};
