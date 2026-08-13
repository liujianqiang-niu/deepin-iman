// src/service/ai/AiService.cpp
#include "AiService.h"
#include "OpenAiCompatibleProvider.h"
#include "data/ManIndex.h"
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <QDebug>
#include <QUuid>
#include <QProcess>

AiService::AiService(QObject* parent) : QObject(parent) {
}

QList<ProviderConfig> AiService::defaultProviderConfigs() {
    return {
        {"openai", "OpenAI",
         "https://api.openai.com/v1", "", "gpt-4o"},
        {"glm", "智谱 GLM",
         "https://open.bigmodel.cn/api/paas/v4", "", "glm-4-plus"},
        {"qwen", "通义千问",
         "https://dashscope.aliyuncs.com/compatible-mode/v1", "", "qwen-max"},
        {"deepseek", "DeepSeek",
         "https://api.deepseek.com/v1", "", "deepseek-chat"},
    };
}

void AiService::initializeProviders() {
    loadFromSettings();
    rebuildProviders();
}

void AiService::loadFromSettings() {
    QSettings settings("deepin", "deepin-iman");

    m_configs.clear();
    int size = settings.beginReadArray("ai/providers");
    if (size > 0) {
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            ProviderConfig cfg;
            cfg.id = settings.value("id").toString();
            cfg.displayName = settings.value("name").toString();
            cfg.apiBase = settings.value("apiBase").toString();
            cfg.apiKey = settings.value("apiKey").toString();
            cfg.model = settings.value("model").toString();
            if (!cfg.id.isEmpty()) m_configs.append(cfg);
        }
    } else {
        settings.endArray();
        bool hasOld = settings.contains("ai/openai_key") || settings.contains("ai/glm_key") ||
                      settings.contains("ai/qwen_key") || settings.contains("ai/claude_key");
        if (hasOld) {
            auto defaults = defaultProviderConfigs();
            for (auto& cfg : defaults) {
                cfg.apiKey = settings.value("ai/" + cfg.id + "_key").toString();
            }
            m_configs = defaults;
        } else {
            m_configs = defaultProviderConfigs();
        }
    }
    if (!size) settings.endArray();

    m_activeProviderId = settings.value("ai/active_provider",
                                         m_configs.isEmpty() ? "" : m_configs.first().id).toString();
    if (!m_configs.isEmpty()) {
        bool found = false;
        for (const auto& cfg : m_configs) {
            if (cfg.id == m_activeProviderId) { found = true; break; }
        }
        if (!found) m_activeProviderId = m_configs.first().id;
    }
}

void AiService::saveToSettings() {
    QSettings settings("deepin", "deepin-iman");
    settings.beginWriteArray("ai/providers");
    for (int i = 0; i < m_configs.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", m_configs[i].id);
        settings.setValue("name", m_configs[i].displayName);
        settings.setValue("apiBase", m_configs[i].apiBase);
        settings.setValue("apiKey", m_configs[i].apiKey);
        settings.setValue("model", m_configs[i].model);
    }
    settings.endArray();
    settings.setValue("ai/active_provider", m_activeProviderId);
}

void AiService::rebuildProviders() {
    qDeleteAll(m_providers);
    m_providers.clear();

    for (const auto& cfg : m_configs) {
        auto* p = new OpenAiCompatibleProvider(this);
        p->setId(cfg.id);
        p->setDisplayName(cfg.displayName);
        p->setApiBase(cfg.apiBase);
        p->setApiKey(cfg.apiKey);
        p->setModel(cfg.model);
        m_providers[cfg.id] = p;
    }

    if (!m_providers.contains(m_activeProviderId) && !m_providers.isEmpty()) {
        m_activeProviderId = m_providers.keys().first();
    }
    emit providerListChanged();
}

QList<ProviderConfig> AiService::providerConfigs() const {
    return m_configs;
}

ProviderConfig AiService::providerConfig(const QString& id) const {
    for (const auto& cfg : m_configs) {
        if (cfg.id == id) return cfg;
    }
    return {};
}

void AiService::setProviderConfigs(const QList<ProviderConfig>& configs) {
    m_configs = configs;
    if (!m_configs.isEmpty()) {
        bool found = false;
        for (const auto& cfg : m_configs) {
            if (cfg.id == m_activeProviderId) { found = true; break; }
        }
        if (!found) m_activeProviderId = m_configs.first().id;
    } else {
        m_activeProviderId.clear();
    }
    rebuildProviders();
    saveToSettings();
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
        onError("AI 供应商未配置，请在设置中填写 API 地址和 API Key");
        return;
    }
    AiRequest req;
    req.systemPrompt = systemPrompt;
    req.prompt = userPrompt;
    p->chat(req, onChunk, onDone, onError);
}

void AiService::translatePage(const ManPage& page, const QString& targetLang,
                               std::function<void(const AiChunk&)> onChunk,
                               std::function<void(const AiResult&)> onDone,
                               std::function<void(const QString&)> onError) {
    QString sys = loadPromptTemplate("translate");
    QString langName = targetLang.isEmpty() ? "中文" : targetLang;

    QString manText = extractManText(page.sourcePath);
    if (manText.isEmpty()) {
        onError(QString("无法提取 %1(%2) 的手册正文，请确认 mandoc 已安装且手册文件存在")
                    .arg(page.name).arg(page.section));
        return;
    }
    if (manText.length() > 6000) manText = manText.left(6000) + "\n\n[...内容过长，已截断...]";

    QString user = QString("请将以下 man 手册页的英文内容翻译为%1。\n"
                           "要求：\n"
                           "1. 保留原文的段落结构\n"
                           "2. 保留所有命令、参数、选项不翻译\n"
                           "3. 翻译要准确通顺，符合中文技术文档习惯\n\n"
                           "命令：%2(%3)\n\n%4")
                       .arg(langName).arg(page.name).arg(page.section).arg(manText);

    AiRequest req;
    req.systemPrompt = sys;
    req.prompt = user;
    req.maxTokens = 4096;
    req.temperature = 0.3;

    auto* p = activeProviderPtr();
    if (!p || !p->isConfigured()) {
        onError("AI 供应商未配置，请在设置中填写 Base URL 和 API Key");
        return;
    }
    p->chat(req, onChunk, onDone, onError);
}

void AiService::generateExamples(const ManPage& page,
                                   std::function<void(const AiChunk&)> onChunk,
                                   std::function<void(const AiResult&)> onDone,
                                   std::function<void(const QString&)> onError) {
    QString sys = loadPromptTemplate("examples");

    QString manText = extractManText(page.sourcePath);
    if (manText.isEmpty()) {
        onError(QString("无法提取 %1(%2) 的手册正文，请确认 mandoc 已安装且手册文件存在")
                    .arg(page.name).arg(page.section));
        return;
    }
    if (manText.length() > 6000) manText = manText.left(6000) + "\n\n[...内容过长，已截断...]";

    static const QMap<int, QString> typeMap = {
        {1, "用户命令（shell 命令）"}, {2, "系统调用（C 语言接口）"},
        {3, "库函数（C 语言接口）"}, {4, "特殊文件/设备"},
        {5, "文件格式/配置"}, {6, "游戏"}, {7, "杂项/约定"}, {8, "管理命令（shell 命令）"}
    };
    QString pageType = typeMap.value(page.section, "手册页");
    bool isCInterface = (page.section == 2 || page.section == 3);

    QString exampleKind = isCInterface
        ? "C 语言代码样例（包含头文件、main 函数、编译说明）"
        : "命令行样例（包含命令、注释和预期输出）";

    QString user = QString("这是 man 手册页 %1(%2)，属于：%3。\n"
                           "请为它生成 3-5 个%4。每个样例包含："
                           "%5。必须基于下方手册正文真实内容，不要臆造。\n\n"
                           "%6 手册正文：\n%7")
                       .arg(page.name).arg(page.section).arg(pageType)
                       .arg(exampleKind)
                       .arg(isCInterface ? "C 代码、注释说明、编译运行命令" : "命令、注释说明、预期输出")
                       .arg(page.name).arg(manText);

    AiRequest req;
    req.systemPrompt = sys;
    req.prompt = user;
    req.maxTokens = 4096;
    req.temperature = 0.5;

    auto* p = activeProviderPtr();
    if (!p || !p->isConfigured()) {
        onError("AI 供应商未配置，请在设置中填写 Base URL 和 API Key");
        return;
    }
    p->chat(req, onChunk, onDone, onError);
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

QString AiService::extractManText(const QString& sourcePath) {
    if (sourcePath.isEmpty()) return QString();
    QProcess p;
    p.start("man", {"-P", "cat", "-l", sourcePath});
    if (!p.waitForFinished(5000) || p.exitCode() != 0) return QString();
    return QString::fromUtf8(p.readAllStandardOutput());
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
