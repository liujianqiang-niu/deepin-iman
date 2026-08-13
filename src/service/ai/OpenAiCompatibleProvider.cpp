// src/service/ai/OpenAiCompatibleProvider.cpp
#include "OpenAiCompatibleProvider.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QTimer>
#include <QDebug>

OpenAiCompatibleProvider::OpenAiCompatibleProvider(QObject* parent) : IAiProvider(parent) {
}

void OpenAiCompatibleProvider::chat(const AiRequest& req,
                                     std::function<void(const AiChunk&)> onChunk,
                                     std::function<void(const AiResult&)> onDone,
                                     std::function<void(const QString&)> onError) {
    if (!isConfigured()) {
        onError(QString("%1 未配置，请检查 Base URL 和 API Key").arg(m_displayName));
        return;
    }

    QString url = m_apiBase;
    if (!url.endsWith("/chat/completions")) {
        if (url.endsWith("/")) url.chop(1);
        url += "/chat/completions";
    }

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    request.setTransferTimeout(120000);

    QJsonObject body;
    body["model"] = m_model;
    body["stream"] = false;
    body["max_tokens"] = req.maxTokens;
    body["temperature"] = req.temperature;

    QJsonArray messages;
    if (!req.systemPrompt.isEmpty()) {
        messages.append(QJsonObject{{"role", "system"}, {"content", req.systemPrompt}});
    }
    messages.append(QJsonObject{{"role", "user"}, {"content", req.prompt}});
    body["messages"] = messages;

    QNetworkReply* reply = m_nam.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

    QTimer::singleShot(120000, reply, [reply]() {
        if (reply && reply->isRunning()) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, reply, [reply, onDone, onError, model = m_model]() {
        if (reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::OperationCanceledError) {
            onError(reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        auto doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            onError(QString("响应解析失败，非 JSON 格式。前 200 字节: %1")
                        .arg(QString::fromUtf8(data.left(200))));
            return;
        }

        auto obj = doc.object();

        if (obj.contains("error")) {
            QString errMsg = obj.value("error").toObject().value("message").toString();
            if (errMsg.isEmpty()) errMsg = QString::fromUtf8(data.left(500));
            onError(QString("API 错误: %1").arg(errMsg));
            return;
        }

        auto choices = obj.value("choices").toArray();
        if (choices.isEmpty()) {
            onError(QString("API 返回无 choices，响应: %1").arg(QString::fromUtf8(data.left(500))));
            return;
        }

        auto message = choices.at(0).toObject().value("message").toObject();
        QString content = message.value("content").toString();

        if (content.isEmpty()) {
            onError("API 返回空内容，请检查模型名称是否正确");
            return;
        }

        AiResult result;
        result.text = content;
        result.model = model;
        auto usage = obj.value("usage").toObject();
        result.inputTokens = usage.value("prompt_tokens").toInt();
        result.outputTokens = usage.value("completion_tokens").toInt();
        onDone(result);
    });

    connect(reply, &QNetworkReply::errorOccurred, reply, [onError](QNetworkReply::NetworkError) {
        // handled in finished
    });
}

void OpenAiCompatibleProvider::cancel() {
    m_nam.deleteLater();
}
