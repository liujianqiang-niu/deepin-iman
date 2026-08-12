// src/service/ai/IAiProvider.h
#pragma once
#include <QObject>
#include <QString>
#include <functional>

struct AiRequest {
    QString prompt;
    QString systemPrompt;
    int maxTokens = 2000;
    double temperature = 0.3;
};

struct AiChunk {
    QString delta;
};

struct AiResult {
    QString text;
    int inputTokens = 0;
    int outputTokens = 0;
    QString model;
};

class IAiProvider : public QObject {
    Q_OBJECT
public:
    explicit IAiProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAiProvider() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual void setApiKey(const QString& key) = 0;
    virtual QString apiKey() const = 0;
    virtual bool isConfigured() const = 0;

    virtual void setApiBase(const QString& base) { Q_UNUSED(base) }
    virtual QString apiBase() const { return {}; }
    virtual void setModel(const QString& model) { Q_UNUSED(model) }
    virtual QString model() const { return {}; }

    virtual void chat(const AiRequest& req,
                      std::function<void(const AiChunk&)> onChunk,
                      std::function<void(const AiResult&)> onDone,
                      std::function<void(const QString&)> onError) = 0;
    virtual void cancel() = 0;
};
