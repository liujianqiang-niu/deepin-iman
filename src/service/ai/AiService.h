// src/service/ai/AiService.h
#pragma once
#include <QObject>
#include <QMap>
#include <functional>
#include "IAiProvider.h"

struct ManPage;

class AiService : public QObject {
    Q_OBJECT
public:
    explicit AiService(QObject* parent = nullptr);

    void initializeProviders();
    QStringList providerIds() const;
    QString providerDisplayName(const QString& id) const;
    void setActiveProvider(const QString& id);
    QString activeProvider() const;
    IAiProvider* provider(const QString& id) const;
    IAiProvider* activeProviderPtr() const;

    void translatePage(const ManPage& page,
                       std::function<void(const AiChunk&)> onChunk,
                       std::function<void(const AiResult&)> onDone,
                       std::function<void(const QString&)> onError);
    void generateExamples(const ManPage& page,
                          std::function<void(const AiChunk&)> onChunk,
                          std::function<void(const AiResult&)> onDone,
                          std::function<void(const QString&)> onError);
    void askQuestion(const ManPage& page, const QString& question,
                     std::function<void(const AiChunk&)> onChunk,
                     std::function<void(const AiResult&)> onDone,
                     std::function<void(const QString&)> onError);
    QString parseCommandQuick(const QString& cmdline);

    void cancelCurrentTask();

    static QString loadPromptTemplate(const QString& name);

signals:
    void providerChanged(const QString& id);

private:
    QMap<QString, IAiProvider*> m_providers;
    QString m_activeProviderId;

    void callAi(const QString& systemPrompt, const QString& userPrompt,
                std::function<void(const AiChunk&)> onChunk,
                std::function<void(const AiResult&)> onDone,
                std::function<void(const QString&)> onError);
};
