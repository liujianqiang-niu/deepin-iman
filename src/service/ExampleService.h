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
};
