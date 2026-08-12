// src/service/ai/OpenAiCompatibleProvider.h
#pragma once
#include "IAiProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class OpenAiCompatibleProvider : public IAiProvider {
    Q_OBJECT
public:
    explicit OpenAiCompatibleProvider(QObject* parent = nullptr);

    QString id() const override { return m_id; }
    QString displayName() const override { return m_displayName; }
    void setApiKey(const QString& key) override { m_apiKey = key; }
    QString apiKey() const override { return m_apiKey; }
    bool isConfigured() const override { return !m_apiKey.isEmpty() && !m_apiBase.isEmpty(); }

    void setApiBase(const QString& base) override { m_apiBase = base; }
    QString apiBase() const override { return m_apiBase; }
    void setModel(const QString& model) override { m_model = model; }
    QString model() const override { return m_model; }

    void setId(const QString& id) { m_id = id; }
    void setDisplayName(const QString& name) { m_displayName = name; }

    void chat(const AiRequest& req,
              std::function<void(const AiChunk&)> onChunk,
              std::function<void(const AiResult&)> onDone,
              std::function<void(const QString&)> onError) override;
    void cancel() override;

private:
    QString m_id;
    QString m_displayName;
    QString m_apiBase;
    QString m_apiKey;
    QString m_model;
    QNetworkAccessManager m_nam;
    QNetworkReply* m_reply = nullptr;
    QString m_accumulated;
};
