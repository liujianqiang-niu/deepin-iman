// src/service/ai/QwenProvider.h
#pragma once
#include "IAiProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QwenProvider : public IAiProvider {
    Q_OBJECT
public:
    explicit QwenProvider(QObject* parent = nullptr);

    QString id() const override { return "qwen"; }
    QString displayName() const override { return "通义千问 Qwen-Max"; }
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
