// src/service/ai/AiService.cpp
#include "AiService.h"
#include "OpenAiProvider.h"
#include "ClaudeProvider.h"
#include "QwenProvider.h"
#include "GlmProvider.h"
#include "data/ManIndex.h"
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <QDebug>

AiService::AiService(QObject* parent) : QObject(parent) {
}

void AiService::initializeProviders() {
    m_providers["openai"] = new OpenAiProvider(this);
    m_providers["claude"] = new ClaudeProvider(this);
    m_providers["qwen"] = new QwenProvider(this);
    m_providers["glm"] = new GlmProvider(this);

    QSettings settings("deepin", "deepin-iman");
    for (auto it = m_providers.begin(); it != m_providers.end(); ++it) {
        QString key = settings.value("ai/" + it.key() + "_key").toString();
        it.value()->setApiKey(key);
    }
    m_activeProviderId = settings.value("ai/active_provider", "glm").toString();
    if (!m_providers.contains(m_activeProviderId)) {
        m_activeProviderId = m_providers.keys().first();
    }
}

QStringList AiService::providerIds() const {
    return m_providers.keys();
}

QString AiService::providerDisplayName(const QString& id) const {
    auto it = m_providers.find(id);
    return it != m_providers.end() ? it.value()->displayName() : id;
}

void AiService::setActiveProvider(const QString& id) {
    if (m_providers.contains(id)) {
        m_activeProviderId = id;
        QSettings settings("deepin", "deepin-iman");
        settings.setValue("ai/active_provider", id);
        emit providerChanged(id);
    }
}

QString AiService::activeProvider() const {
    return m_activeProviderId;
}

IAiProvider* AiService::provider(const QString& id) const {
    return m_providers.value(id, nullptr);
}

IAiProvider* AiService::activeProviderPtr() const {
    return m_providers.value(m_activeProviderId, nullptr);
}

void AiService::callAi(const QString& systemPrompt, const QString& userPrompt,
                        std::function<void(const AiChunk&)> onChunk,
                        std::function<void(const AiResult&)> onDone,
                        std::function<void(const QString&)> onError) {
    auto* p = activeProviderPtr();
    if (!p || !p->isConfigured()) {
        onError("AI 供应商未配置，请在设置中填写 API key");
        return;
    }
    AiRequest req;
    req.systemPrompt = systemPrompt;
    req.prompt = userPrompt;
    p->chat(req, onChunk, onDone, onError);
}

void AiService::translatePage(const ManPage& page,
                               std::function<void(const AiChunk&)> onChunk,
                               std::function<void(const AiResult&)> onDone,
                               std::function<void(const QString&)> onError) {
    QString sys = loadPromptTemplate("translate");
    QString user = QString("请将以下 man 手册页翻译为中文，保留所有 man 结构标记（.SH/.SS/.TP 等）：\n\n"
                           "命令：%1(%2)\n\n%3")
                       .arg(page.name).arg(page.section).arg(page.title);
    callAi(sys, user, onChunk, onDone, onError);
}

void AiService::generateExamples(const ManPage& page,
                                   std::function<void(const AiChunk&)> onChunk,
                                   std::function<void(const AiResult&)> onDone,
                                   std::function<void(const QString&)> onError) {
    QString sys = loadPromptTemplate("examples");
    QString user = QString("为以下命令生成 3-5 个使用样例，每个样例包含：命令、注释说明、预期输出：\n\n"
                           "命令：%1(%2)\n\n%3")
                       .arg(page.name).arg(page.section).arg(page.title);
    callAi(sys, user, onChunk, onDone, onError);
}

void AiService::askQuestion(const ManPage& page, const QString& question,
                             std::function<void(const AiChunk&)> onChunk,
                             std::function<void(const AiResult&)> onDone,
                             std::function<void(const QString&)> onError) {
    QString sys = loadPromptTemplate("qa");
    QString user = QString("用户正在查看 %1(%2) 的 man 手册。\n\n"
                           "手册标题：%3\n\n"
                           "用户问题：%4\n\n"
                           "请基于该 man 手册的内容回答问题。")
                       .arg(page.name).arg(page.section).arg(page.title).arg(question);
    callAi(sys, user, onChunk, onDone, onError);
}

QString AiService::parseCommandQuick(const QString& cmdline) {
    QString cmd = cmdline.trimmed();
    cmd.remove(QRegularExpression("^(sudo\\s+|nohup\\s+|time\\s+)+"));
    cmd.remove(QRegularExpression("^\\w+\\s*=\\S+\\s+"));
    QRegularExpression re("^([a-zA-Z][a-zA-Z0-9_.+-]*)");
    auto m = re.match(cmd);
    return m.hasMatch() ? m.captured(1) : QString();
}

void AiService::cancelCurrentTask() {
    auto* p = activeProviderPtr();
    if (p) p->cancel();
}

QString AiService::loadPromptTemplate(const QString& name) {
    QStringList paths = {
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/prompts/" + name + ".txt",
        QDir::currentPath() + "/data/prompts/" + name + ".txt",
        QDir::currentPath() + "/../share/deepin-iman/prompts/" + name + ".txt",
        ":/prompts/" + name + ".txt",
    };
    for (const auto& path : paths) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(f.readAll());
        }
    }
    if (name == "translate")
        return "你是一位专业的 Linux man 手册翻译专家。请将英文 man 手册翻译为准确的中文，保留所有格式标记。";
    if (name == "examples")
        return "你是一位 Linux 命令行专家。请为给定命令生成实用的使用样例，包含命令、注释和预期输出。";
    if (name == "qa")
        return "你是一位 Linux man 手册专家。请基于 man 手册内容准确回答用户的问题，用中文回答。";
    if (name == "parse")
        return "你是一位命令行解析专家。请解析给定的命令行，提取其中的命令名和参数结构。";
    return "You are a helpful assistant.";
}
