// src/service/TranslationService.h
#pragma once
#include <QObject>
#include <QString>
#include <QDir>
#include <functional>

class TranslationCache;
class AiService;
struct ManPage;

class TranslationService : public QObject {
    Q_OBJECT
public:
    explicit TranslationService(TranslationCache* cache, AiService* ai, QObject* parent = nullptr);

    // onReady 第二参数 isHtml: true=预设包 HTML(用 setHtml 显示)，false=AI 翻译 markdown(用 setMarkdown 显示)
    void getTranslation(const ManPage& page, const QString& targetLang,
                        std::function<void(const QString&, bool)> onReady,
                        std::function<void(const QString&)> onError);

    QString tryPresetPackage(const ManPage& page) const;
    bool presetPackageAvailable() const;
    void clearCache();

private:
    TranslationCache* m_cache;
    AiService* m_ai;
    QDir m_presetDir;
};
