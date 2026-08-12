// src/service/ai/GlmProvider.h
#pragma once
#include "IAiProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class GlmProvider : public IAiProvider {
    Q_OBJECT
public:
    explicit GlmProvider(QObject* parent = nullptr);

    QString id() const override { return "glm"; }
    QString displayName() const override { return "智谱 GLM-4-Plus"; }
    void setApiKey(const QString& key) override { m_apiKey = key; }
    QString apiKey() const override { return m_apiKey; }
    bool isConfigured() const override { return !m_apiKey.isEmpty(); }

    void chat(const AiRequest& req,
              std::function<void(const AiChunk&)> onChunk,
              std::function<void(const AiResult&)> onDone,
              std::function<void(const QString&)> onError) override;
    void cancel() override;

private:
    QString m_apiKey;
    QNetworkAccessManager m_nam;
    QNetworkReply* m_reply = nullptr;
    QString m_accumulated;
};
