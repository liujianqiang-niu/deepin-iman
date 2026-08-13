// src/service/ExampleService.h
#pragma once
#include <QObject>
#include <QString>
#include <functional>

class AiService;
struct ManPage;

class ExampleService : public QObject {
    Q_OBJECT
public:
    explicit ExampleService(AiService* ai, QObject* parent = nullptr);

    void generateExamples(const ManPage& page,
                          std::function<void(const QString&)> onReady,
                          std::function<void(const QString&)> onError);

private:
    AiService* m_ai;
    static QString cacheFilePath(const ManPage& page);
    static QString loadFromCache(const ManPage& page);
    static void saveToCache(const ManPage& page, const QString& text);
};
