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

    QString renderPage(const QString& gzPath);
    QList<CrossReference> parseCrossReferences(const QString& html) const;
    QString ansiToHtml(const QString& ansiText) const;

    void renderPageAsync(const QString& gzPath);

signals:
    void pageRendered(const QString& html);
    void renderFailed(const QString& reason);
};
