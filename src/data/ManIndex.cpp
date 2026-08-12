// src/data/ManIndex.cpp
#include "ManIndex.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QProcess>
#include <QDateTime>
#include <QDebug>

ManIndex::ManIndex(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath)
{
}

ManIndex::~ManIndex() {
    if (m_db.isOpen()) {
        QSqlDatabase::removeDatabase(m_db.connectionName());
        m_db.close();
    }
}

bool ManIndex::open() {
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbPath);
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) {
        qWarning() << "ManIndex: cannot open" << m_dbPath << m_db.lastError().text();
        return false;
    }
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    createSchema();
    return true;
}

void ManIndex::createSchema() {
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS man_page ("
           "  id INTEGER PRIMARY KEY,"
           "  name TEXT NOT NULL,"
           "  section INTEGER NOT NULL,"
           "  section_name TEXT,"
           "  source_path TEXT NOT NULL,"
           "  title TEXT,"
           "  source_mtime INTEGER,"
           "  indexed_at INTEGER)");
    q.exec("CREATE VIRTUAL TABLE IF NOT EXISTS man_fts USING fts5("
           "  name, title, body,"
           "  content='man_page', content_rowid='id',"
           "  tokenize='unicode61 remove_diacritics 2')");
}

bool ManIndex::tableExists(const QString& name) {
    QSqlQuery q(m_db);
    q.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    q.addBindValue(name);
    q.exec();
    return q.next();
}

QStringList ManIndex::findManFiles(const QString& manRoot) const {
    QStringList files;
    QDir root(manRoot);
    if (!root.exists()) return files;
    QRegularExpression manDirRe("^man([1-8])$");
    for (const auto& entry : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        auto m = manDirRe.match(entry.fileName());
        if (!m.hasMatch()) continue;
        for (const auto& f : QDir(entry.absoluteFilePath()).entryInfoList(QStringList() << "*.gz", QDir::Files)) {
            files << f.absoluteFilePath();
        }
    }
    return files;
}

ManPage ManIndex::parseManPath(const QString& path) const {
    ManPage page;
    QFileInfo fi(path);
    QRegularExpression re("^(.+)\\.([1-8])\\.gz$");
    auto m = re.match(fi.fileName());
    if (!m.hasMatch()) return page;
    page.name = m.captured(1);
    page.section = m.captured(2).toInt();
    page.sectionName = sectionName(page.section);
    page.sourcePath = path;
    page.sourceMtime = fi.lastModified().toSecsSinceEpoch();
    return page;
}

QString ManIndex::sectionName(int section) const {
    static const QMap<int, QString> names = {
        {1, "用户命令"}, {2, "系统调用"}, {3, "库函数"},
        {4, "特殊文件"}, {5, "文件格式"}, {6, "游戏"},
        {7, "杂项"}, {8, "管理命令"}
    };
    return names.value(section, QString::number(section));
}

QString ManIndex::extractTitle(const QString& gzPath) const {
    QString plain = extractPlainText(gzPath);
    if (plain.isEmpty()) return QString();
    QStringList lines = plain.split('\n', Qt::SkipEmptyParts);
    return lines.isEmpty() ? QString() : lines.first().trimmed();
}

QString ManIndex::extractPlainText(const QString& gzPath) const {
    QProcess p;
    p.start("man", {"-P", "cat", "-l", gzPath});
    if (!p.waitForFinished(3000)) {
        qWarning() << "ManIndex: man timeout" << gzPath;
        return QString();
    }
    return QString::fromUtf8(p.readAllStandardOutput());
}

int ManIndex::scanManPages(const QString& manRoot) {
    QStringList files = findManFiles(manRoot);
    int count = 0;
    QSqlQuery q(m_db);
    q.exec("DELETE FROM man_page");
    q.exec("DELETE FROM man_fts");

    for (int i = 0; i < files.size(); ++i) {
        ManPage page = parseManPath(files[i]);
        if (page.name.isEmpty()) continue;

        // Scan uses name as title (no external process for speed)
        page.title = page.name;

        q.prepare("INSERT INTO man_page (name, section, section_name, source_path, "
                   "title, source_mtime, indexed_at) VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(page.name);
        q.addBindValue(page.section);
        q.addBindValue(page.sectionName);
        q.addBindValue(page.sourcePath);
        q.addBindValue(page.title);
        q.addBindValue(page.sourceMtime);
        q.addBindValue(QDateTime::currentSecsSinceEpoch());
        if (q.exec()) {
            int id = q.lastInsertId().toInt();
            // FTS body = name + name (no full text extraction during scan for speed)
            q.prepare("INSERT INTO man_fts (rowid, name, title, body) VALUES (?, ?, ?, ?)");
            q.addBindValue(id);
            q.addBindValue(page.name);
            q.addBindValue(page.title);
            q.addBindValue(page.name);
            q.exec();
            ++count;
        }
        if (i % 100 == 0) emit scanProgress(i + 1, files.size());
    }
    emit scanFinished(count);
    return count;
}

bool ManIndex::needsUpdate(const QString& manRoot) const {
    QStringList fsFiles = findManFiles(manRoot);
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.exec("SELECT COUNT(*) FROM man_page");
    int dbCount = q.next() ? q.value(0).toInt() : 0;
    return fsFiles.size() != dbCount;
}

int ManIndex::refreshManPages(const QString& manRoot) {
    QStringList fsFiles = findManFiles(manRoot);
    QSet<QString> fsFileSet(fsFiles.begin(), fsFiles.end());

    QSqlQuery q(m_db);

    QSet<int> staleIds;
    q.prepare("SELECT id, source_path, source_mtime FROM man_page");
    q.exec();
    QMap<QString, int> existingPaths;
    while (q.next()) {
        int id = q.value(0).toInt();
        QString path = q.value(1).toString();
        qint64 mtime = q.value(2).toLongLong();
        existingPaths[path] = id;
        if (!fsFileSet.contains(path)) {
            staleIds.insert(id);
        }
    }

    if (!staleIds.isEmpty()) {
        q.prepare("DELETE FROM man_page WHERE id = ?");
        for (int id : staleIds) {
            q.addBindValue(id);
            q.exec();
        }
        q.prepare("DELETE FROM man_fts WHERE rowid = ?");
        for (int id : staleIds) {
            q.addBindValue(id);
            q.exec();
        }
    }

    int added = 0;
    int updated = 0;
    for (int i = 0; i < fsFiles.size(); ++i) {
        const QString& path = fsFiles[i];
        ManPage page = parseManPath(path);
        if (page.name.isEmpty()) continue;

        QFileInfo fi(path);
        qint64 currentMtime = fi.lastModified().toSecsSinceEpoch();

        if (existingPaths.contains(path)) {
            QSqlQuery check(m_db);
            check.prepare("SELECT source_mtime FROM man_page WHERE id = ?");
            check.addBindValue(existingPaths[path]);
            check.exec();
            if (check.next() && check.value(0).toLongLong() == currentMtime) {
                continue;
            }
            check.prepare("DELETE FROM man_page WHERE id = ?");
            check.addBindValue(existingPaths[path]);
            check.exec();
            check.prepare("DELETE FROM man_fts WHERE rowid = ?");
            check.addBindValue(existingPaths[path]);
            check.exec();
            ++updated;
        } else {
            ++added;
        }

        page.title = page.name;
        q.prepare("INSERT INTO man_page (name, section, section_name, source_path, "
                  "title, source_mtime, indexed_at) VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(page.name);
        q.addBindValue(page.section);
        q.addBindValue(page.sectionName);
        q.addBindValue(page.sourcePath);
        q.addBindValue(page.title);
        q.addBindValue(page.sourceMtime);
        q.addBindValue(QDateTime::currentSecsSinceEpoch());
        if (q.exec()) {
            int id = q.lastInsertId().toInt();
            q.prepare("INSERT INTO man_fts (rowid, name, title, body) VALUES (?, ?, ?, ?)");
            q.addBindValue(id);
            q.addBindValue(page.name);
            q.addBindValue(page.title);
            q.addBindValue(page.name);
            q.exec();
        }
        if (i % 100 == 0) emit scanProgress(i + 1, fsFiles.size());
    }
    emit scanProgress(fsFiles.size(), fsFiles.size());
    emit scanFinished(added + updated);
    return added + updated;
}

int ManIndex::pageCount() const {
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.exec("SELECT COUNT(*) FROM man_page");
    return q.next() ? q.value(0).toInt() : 0;
}

QList<ManPage> ManIndex::findByName(const QString& name) const {
    QList<ManPage> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT id, name, section, section_name, source_path, title, "
              "source_mtime, indexed_at FROM man_page WHERE name = ? ORDER BY section LIMIT 20");
    q.addBindValue(name);
    if (!q.exec()) return results;
    while (q.next()) {
        ManPage p;
        p.id = q.value(0).toInt();
        p.name = q.value(1).toString();
        p.section = q.value(2).toInt();
        p.sectionName = q.value(3).toString();
        p.sourcePath = q.value(4).toString();
        p.title = q.value(5).toString();
        p.sourceMtime = q.value(6).toLongLong();
        p.indexedAt = q.value(7).toLongLong();
        results << p;
    }
    return results;
}

QList<ManPage> ManIndex::findByNameLike(const QString& pattern) const {
    QList<ManPage> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT id, name, section, section_name, source_path, title, "
              "source_mtime, indexed_at FROM man_page WHERE name LIKE ? "
              "ORDER BY name, section LIMIT 50");
    q.addBindValue("%" + pattern + "%");
    if (!q.exec()) return results;
    while (q.next()) {
        ManPage p;
        p.id = q.value(0).toInt();
        p.name = q.value(1).toString();
        p.section = q.value(2).toInt();
        p.sectionName = q.value(3).toString();
        p.sourcePath = q.value(4).toString();
        p.title = q.value(5).toString();
        p.sourceMtime = q.value(6).toLongLong();
        p.indexedAt = q.value(7).toLongLong();
        results << p;
    }
    return results;
}

ManPage ManIndex::findById(int id) const {
    ManPage p;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT id, name, section, section_name, source_path, title, "
              "source_mtime, indexed_at FROM man_page WHERE id = ?");
    q.addBindValue(id);
    if (!q.exec() || !q.next()) return p;
    p.id = q.value(0).toInt();
    p.name = q.value(1).toString();
    p.section = q.value(2).toInt();
    p.sectionName = q.value(3).toString();
    p.sourcePath = q.value(4).toString();
    p.title = q.value(5).toString();
    p.sourceMtime = q.value(6).toLongLong();
    p.indexedAt = q.value(7).toLongLong();
    return p;
}

QList<ManPage> ManIndex::fullTextSearch(const QString& query, int limit) const {
    QList<ManPage> results;
    QSqlQuery q(QSqlDatabase::database(m_db.connectionName()));
    q.prepare("SELECT man_page.id, man_page.name, man_page.section, "
              "man_page.section_name, man_page.source_path, "
              "man_page.title, man_page.source_mtime, man_page.indexed_at, "
              "bm25(man_fts) AS rank "
              "FROM man_fts JOIN man_page ON man_fts.rowid = man_page.id "
              "WHERE man_fts MATCH ? "
              "ORDER BY rank LIMIT ?");
    q.addBindValue(query);
    q.addBindValue(limit);
    if (!q.exec()) return results;
    while (q.next()) {
        ManPage p;
        p.id = q.value(0).toInt();
        p.name = q.value(1).toString();
        p.section = q.value(2).toInt();
        p.sectionName = q.value(3).toString();
        p.sourcePath = q.value(4).toString();
        p.title = q.value(5).toString();
        p.sourceMtime = q.value(6).toLongLong();
        p.indexedAt = q.value(7).toLongLong();
        results << p;
    }
    return results;
}
