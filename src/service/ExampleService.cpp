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

void ExampleService::generateExamples(const ManPage& page,
                                         std::function<void(const QString&)> onReady,
                                         std::function<void(const QString&)> onError) {
    m_ai->generateExamples(page,
        [](const AiChunk&) {},
        [page, onReady](const AiResult& result) {
            QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
            QDir().mkpath(cacheDir + "/deepin-iman/examples");
            QString cacheFile = cacheDir + QString("/deepin-iman/examples/%1-%2.md")
                                    .arg(page.name).arg(page.section);
            QFile f(cacheFile);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(result.text.toUtf8());
                f.close();
            }
            onReady(result.text);
        },
        onError);
}
