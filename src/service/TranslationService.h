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

    void getTranslation(const ManPage& page, const QString& targetLang,
                        std::function<void(const QString&)> onReady,
                        std::function<void(const QString&)> onError);

    QString tryPresetPackage(const ManPage& page) const;
    bool presetPackageAvailable() const;

private:
    TranslationCache* m_cache;
    AiService* m_ai;
    QDir m_presetDir;
};
