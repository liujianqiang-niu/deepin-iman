// src/service/TranslationService.cpp
#include "TranslationService.h"
#include "data/TranslationCache.h"
#include "data/ManIndex.h"
#include "ai/AiService.h"
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

TranslationService::TranslationService(TranslationCache* cache, AiService* ai, QObject* parent)
    : QObject(parent), m_cache(cache), m_ai(ai)
{
    m_presetDir = QDir("/usr/share/man/zh_CN");
}

bool TranslationService::presetPackageAvailable() const {
    return m_presetDir.exists() && !m_presetDir.entryList(QStringList() << "man*", QDir::Dirs).isEmpty();
}

void TranslationService::clearCache() {
    if (m_cache) m_cache->clearAll();
}

QString TranslationService::tryPresetPackage(const ManPage& page) const {
    QString sectionDir = QString("man%1").arg(page.section);
    QString path = m_presetDir.absoluteFilePath(sectionDir + "/" + page.name + "." + QString::number(page.section) + ".gz");
    if (!QFileInfo::exists(path)) return QString();

    QProcess p;
    p.start("mandoc", {"-Thtml", "-O", "fragment", path});
    if (!p.waitForFinished(3000)) return QString();
    if (p.exitCode() != 0) return QString();
    return QString::fromUtf8(p.readAllStandardOutput());
}

void TranslationService::getTranslation(const ManPage& page, const QString& targetLang,
                                          std::function<void(const QString&, bool)> onReady,
                                          std::function<void(const QString&)> onError) {
    QString pageHash = TranslationCache::computeHash(page.name, page.section, page.sourceMtime, targetLang);

    QString cachedSource;
    QString cached = m_cache->get(pageHash, &cachedSource);
    if (!cached.isEmpty()) {
        onReady(cached, cachedSource.startsWith("preset"));
        return;
    }

    // AI 翻译优先；AI 失败/未配置时才降级用本地预设中文 man 包
    m_ai->translatePage(page, targetLang,
        [this, pageHash](const AiChunk& chunk) { Q_UNUSED(chunk); },
        [this, pageHash, onReady](const AiResult& result) {
            m_cache->put(pageHash, result.text, "ai-" + result.model, result.model);
            onReady(result.text, false);
        },
        [this, page, pageHash, targetLang, onReady, onError](const QString& err) {
            QString preset = tryPresetPackage(page);
            if (!preset.isEmpty()) {
                m_cache->put(pageHash, preset, "preset", QString());
                onReady(preset, true);
            } else {
                onError(err);
            }
        });
}
