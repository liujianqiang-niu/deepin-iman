// src/service/ExampleService.cpp
#include "ExampleService.h"
#include "ai/AiService.h"
#include "data/ManIndex.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>

ExampleService::ExampleService(AiService* ai, QObject* parent)
    : QObject(parent), m_ai(ai)
{
}

QString ExampleService::cacheFilePath(const ManPage& page) {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return cacheDir + QString("/deepin-iman/examples/%1-%2.md").arg(page.name).arg(page.section);
}

QString ExampleService::loadFromCache(const ManPage& page) {
    QFile f(cacheFilePath(page));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(f.readAll());
    }
    return QString();
}

void ExampleService::saveToCache(const ManPage& page, const QString& text) {
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/deepin-iman/examples");
    QFile f(cacheFilePath(page));
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(text.toUtf8());
        f.close();
    }
}

void ExampleService::generateExamples(const ManPage& page,
                                         std::function<void(const QString&)> onReady,
                                         std::function<void(const QString&)> onError) {
    QString cached = loadFromCache(page);
    if (!cached.isEmpty()) {
        onReady(cached);
        return;
    }

    m_ai->generateExamples(page,
        [](const AiChunk&) {},
        [page, onReady](const AiResult& result) {
            saveToCache(page, result.text);
            onReady(result.text);
        },
        onError);
}
