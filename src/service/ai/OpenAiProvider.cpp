// src/service/ai/OpenAiProvider.cpp
#include "OpenAiProvider.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QTimer>
#include <QDebug>

static constexpr const char* API_URL = "https://api.openai.com/v1/chat/completions";
static constexpr const char* MODEL = "gpt-4o";

OpenAiProvider::OpenAiProvider(QObject* parent) : IAiProvider(parent) {
}

void OpenAiProvider::chat(const AiRequest& req,
                           std::function<void(const AiChunk&)> onChunk,
                           std::function<void(const AiResult&)> onDone,
                           std::function<void(const QString&)> onError) {
    if (!isConfigured()) {
        onError("OpenAI API key not configured");
        return;
    }

    QNetworkRequest request{QUrl(API_URL)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    QJsonObject body;
    body["model"] = MODEL;
    body["stream"] = true;
    body["max_tokens"] = req.maxTokens;
    body["temperature"] = req.temperature;

    QJsonArray messages;
    if (!req.systemPrompt.isEmpty()) {
        messages.append(QJsonObject{{"role", "system"}, {"content", req.systemPrompt}});
    }
    messages.append(QJsonObject{{"role", "user"}, {"content", req.prompt}});
    body["messages"] = messages;

    m_accumulated.clear();
    m_reply = m_nam.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_reply->setReadBufferSize(0);

    QTimer::singleShot(30000, m_reply, [this]() {
        if (m_reply && m_reply->isRunning()) {
            m_reply->abort();
        }
    });

    connect(m_reply, &QIODevice::readyRead, this, [this, onChunk, onError]() {
        QByteArray data = m_reply->readAll();
        for (const auto& line : data.split('\n')) {
            QString s = QString::fromUtf8(line).trimmed();
            if (!s.startsWith("data: ")) continue;
            s = s.mid(6);
            if (s == "[DONE]") continue;
            auto doc = QJsonDocument::fromJson(s.toUtf8());
            auto delta = doc.object()
                            .value("choices").toArray().at(0).toObject()
                            .value("delta").toObject()
                            .value("content").toString();
            if (!delta.isEmpty()) {
                m_accumulated += delta;
                onChunk({delta});
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

    connect(m_reply, &QNetworkReply::errorOccurred, this, [onError](QNetworkReply::NetworkError) {
        // handled in finished
    });
}

void OpenAiProvider::cancel() {
    if (m_reply) m_reply->abort();
}
