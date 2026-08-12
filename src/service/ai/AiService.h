// src/service/ai/AiService.h
#pragma once
#include <QObject>
#include <QMap>
#include <QList>
#include <functional>
#include "IAiProvider.h"
#include "OpenAiCompatibleProvider.h"

struct ManPage;

struct ProviderConfig {
    QString id;
    QString displayName;
    QString apiBase;
    QString apiKey;
    QString model;
};

class AiService : public QObject {
    Q_OBJECT
public:
    explicit AiService(QObject* parent = nullptr);

    void initializeProviders();

    QList<ProviderConfig> providerConfigs() const;
    ProviderConfig providerConfig(const QString& id) const;
    void setProviderConfigs(const QList<ProviderConfig>& configs);

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
    static QList<ProviderConfig> defaultProviderConfigs();

signals:
    void providerChanged(const QString& id);
    void providerListChanged();

private:
    QMap<QString, OpenAiCompatibleProvider*> m_providers;
    QList<ProviderConfig> m_configs;
    QString m_activeProviderId;

    void callAi(const QString& systemPrompt, const QString& userPrompt,
                std::function<void(const AiChunk&)> onChunk,
                std::function<void(const AiResult&)> onDone,
                std::function<void(const QString&)> onError);
    void loadFromSettings();
    void saveToSettings();
    void rebuildProviders();
};
