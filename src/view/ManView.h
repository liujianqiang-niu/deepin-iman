// src/view/ManView.h
#pragma once
#include <QTextBrowser>
#include <QString>

class ManView : public QTextBrowser {
    Q_OBJECT
public:
    explicit ManView(QWidget* parent = nullptr);

    void loadHtml(const QString& html);

signals:
    void crossReferenceClicked(const QString& name, int section);

private slots:
    void onAnchorClicked(const QUrl& url);
};
