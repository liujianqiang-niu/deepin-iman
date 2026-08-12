// src/service/ai/ClaudeProvider.cpp
#include "ClaudeProvider.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QTimer>
#include <QDebug>

static constexpr const char* API_URL = "https://api.anthropic.com/v1/messages";
static constexpr const char* MODEL = "claude-3-5-sonnet-20241022";

ClaudeProvider::ClaudeProvider(QObject* parent) : IAiProvider(parent) {
}

void ClaudeProvider::chat(const AiRequest& req,
                           std::function<void(const AiChunk&)> onChunk,
                           std::function<void(const AiResult&)> onDone,
                           std::function<void(const QString&)> onError) {
    if (!isConfigured()) {
        onError("Claude API key not configured");
        return;
    }

    QNetworkRequest request{QUrl(API_URL)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");

    QJsonObject body;
    body["model"] = MODEL;
    body["stream"] = true;
    body["max_tokens"] = req.maxTokens;
    if (!req.systemPrompt.isEmpty()) {
        body["system"] = req.systemPrompt;
    }
    QJsonArray messages;
    messages.append(QJsonObject{{"role", "user"}, {"content", req.prompt}});
    body["messages"] = messages;

    m_accumulated.clear();
    m_reply = m_nam.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_reply->setReadBufferSize(0);

    QTimer::singleShot(30000, m_reply, [this]() {
        if (m_reply && m_reply->isRunning()) m_reply->abort();
    });

    connect(m_reply, &QIODevice::readyRead, this, [this, onChunk]() {
        QByteArray data = m_reply->readAll();
        for (const auto& line : data.split('\n')) {
            QString s = QString::fromUtf8(line).trimmed();
            if (!s.startsWith("data: ")) continue;
            s = s.mid(6);
            auto doc = QJsonDocument::fromJson(s.toUtf8());
            auto type = doc.object().value("type").toString();
            if (type == "content_block_delta") {
                auto delta = doc.object().value("delta").toObject().value("text").toString();
                if (!delta.isEmpty()) {
                    m_accumulated += delta;
                    onChunk({delta});
                }
            }
        }
    });

    connect(m_reply, &QNetworkReply::finished, this, [this, onDone, onError]() {
        if (m_reply->error() != QNetworkReply::NoError && m_reply->error() != QNetworkReply::OperationCanceledError) {
            onError(m_reply->errorString());
        } else {
            AiResult result;
            result.text = m_accumulated;
            result.model = MODEL;
            onDone(result);
        }
        m_reply->deleteLater();
        m_reply = nullptr;
    });
}

void ClaudeProvider::cancel() {
    if (m_reply) m_reply->abort();
}
