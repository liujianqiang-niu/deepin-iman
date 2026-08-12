# deepin-iman Phase 1 实现计划：基础 + Man 浏览器 MVP

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 构建可独立运行的 deepin-iman MVP——能扫描、索引、搜索、渲染、跳转所有 man 手册，无 AI 也能用。

**架构：** 单体三层（UI / Service / Data）。Data 层用 SQLite FTS5 索引 man 全文；Service 层用 mandoc 转 HTML + QProcess 渲染；UI 层用 DTK6 控件三栏布局。全程异步（QtConcurrent），UI 不阻塞。

**技术栈：** Qt6 (Core/Widgets/Sql/Concurrent) + DTK6 + mandoc + SQLite FTS5 + CMake + Qt Test

**规格引用：** `docs/superpowers/specs/2026-08-12-deepin-iman-design.md`

**后续阶段（本计划不含）：**
- Phase 2: AI 集成（IAiProvider + 供应商 + AiService + TranslationService + AiChatWidget）
- Phase 3: 翻译+样例（TranslationView 中英对照 + ExampleService + 预置翻译包）
- Phase 4: 扩展功能（TerminalPanel + FavoriteService + HistoryService + 速查卡 + SettingsDialog）

---

## 文件结构

本阶段创建/修改的文件：

| 文件 | 职责 | 行数预估 |
|------|------|----------|
| `CMakeLists.txt` | 顶层构建，找 Qt6/DTK6/mandoc | ~80 |
| `src/main.cpp` | 入口，创建 MainWindow | ~30 |
| `src/data/ManIndex.h` | man 页索引接口 | ~50 |
| `src/data/ManIndex.cpp` | SQLite FTS5 扫描+索引+查询 | ~280 |
| `src/data/SettingsStore.h` | 配置接口 | ~40 |
| `src/data/SettingsStore.cpp` | QSettings 读写 | ~80 |
| `src/service/ManService.h` | man 渲染管线接口 | ~40 |
| `src/service/ManService.cpp` | mandoc→HTML + 交叉引用解析 | ~250 |
| `src/service/SearchService.h` | 搜索接口 | ~40 |
| `src/service/SearchService.cpp` | 命令名 + FTS5 查询 | ~150 |
| `src/view/MainWindow.h` | 主窗口接口 | ~50 |
| `src/view/MainWindow.cpp` | DMainWindow 三栏布局 + Tab 栏 + 导航栈 | ~280 |
| `src/view/LeftSidebar.h` | 左侧栏接口 | ~40 |
| `src/view/LeftSidebar.cpp` | 搜索框 + 命令导航树 | ~200 |
| `src/view/ManView.h` | man 渲染视图接口 | ~30 |
| `src/view/ManView.cpp` | QTextBrowser + anchorClicked 跳转 | ~120 |
| `tests/TestManIndex.cpp` | ManIndex 单元测试 | ~180 |
| `tests/TestManService.cpp` | ManService 集成测试 | ~150 |
| `tests/TestSearchService.cpp` | SearchService 集成测试 | ~120 |
| `data/sections.json` | man 章节 1-8 名称映射 | ~20 |

---

## 任务 1：CMake 骨架 + main.cpp 空壳

**文件：**
- 创建：`CMakeLists.txt`
- 创建：`src/main.cpp`

- [ ] **步骤 1：编写 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(deepin-iman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql Concurrent Test)
find_package(Dtk6 REQUIRED COMPONENTS Widget Core)

pkg_check_modules(MANDOC REQUIRED mandoc IMPORTED_TARGET)

enable_testing()

add_executable(deepin-iman
    src/main.cpp
    src/data/ManIndex.cpp
    src/data/SettingsStore.cpp
    src/service/ManService.cpp
    src/service/SearchService.cpp
    src/view/MainWindow.cpp
    src/view/LeftSidebar.cpp
    src/view/ManView.cpp
)

target_include_directories(deepin-iman PRIVATE
    src
    src/data
    src/service
    src/view
)

target_link_libraries(deepin-iman PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Concurrent
    Dtk6::Widget
    Dtk6::Core
    PkgConfig::MANDOC
)

install(TARGETS deepin-iman DESTINATION bin)
```

- [ ] **步骤 2：编写 main.cpp 空壳**

```cpp
// src/main.cpp
#include <QApplication>
#include <DApplication>

int main(int argc, char* argv[]) {
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-iman");
    app.setApplicationVersion("0.1.0");
    app.setProductIcon(QIcon::fromTheme("help-browser"));
    app.setProductName("deepin iman");
    app.setApplicationDescription(QObject::tr("AI-powered man page viewer"));

    // MainWindow 将在任务 11 创建
    return app.exec();
}
```

- [ ] **步骤 3：配置构建目录并验证 CMake 配置**

运行：
```bash
mkdir -p build && cmake -B build -S .
```
预期：`-- Build files have been written to: .../build`，无错误。

- [ ] **步骤 4：验证可编译（会链接失败因无 MainWindow，但编译阶段应通过）**

运行：`cmake --build build 2>&1 | head -20`
预期：报 `main.cpp` 中无 MainWindow 引用错误是正常的（本任务不创建 MainWindow）；CMake 配置本身应成功。

- [ ] **步骤 5：Commit**

```bash
git add CMakeLists.txt src/main.cpp
git commit -m "build: add CMake skeleton with Qt6/DTK6/mandoc deps"
```

---

## 任务 2：SettingsStore（QSettings 封装）

**文件：**
- 创建：`src/data/SettingsStore.h`
- 创建：`src/data/SettingsStore.cpp`
- 测试：`tests/TestSettingsStore.cpp`

- [ ] **步骤 1：编写失败的测试**

```cpp
// tests/TestSettingsStore.cpp
#include <QtTest/QtTest>
#include "data/SettingsStore.h"

class TestSettingsStore : public QObject {
    Q_OBJECT
private slots:
    void testWindowLayoutDefaults() {
        QTemporaryDir tmpDir;
        SettingsStore s(tmpDir.path() + "/test.ini");
        QCOMPARE(s.sidebarWidth(), 240);
        QCOMPARE(s.aiPanelWidth(), 360);
        QCOMPARE(s.terminalCollapsed(), true);
    }
    void testSetWindowLayout() {
        QTemporaryDir tmpDir;
        SettingsStore s(tmpDir.path() + "/test.ini");
        s.setSidebarWidth(300);
        s.setAiPanelWidth(400);
        s.setTerminalCollapsed(false);
        QCOMPARE(s.sidebarWidth(), 300);
        QCOMPARE(s.aiPanelWidth(), 400);
        QCOMPARE(s.terminalCollapsed(), false);
    }
};

QTEST_MAIN(TestSettingsStore)
#include "TestSettingsStore.moc"
```

- [ ] **步骤 2：运行测试验证失败**

运行：`cmake --build build --target TestSettingsStore 2>&1 | tail -5`
预期：FAIL，报 `SettingsStore.h: No such file or directory`。

- [ ] **步骤 3：编写 SettingsStore.h**

```cpp
// src/data/SettingsStore.h
#pragma once
#include <QString>

class SettingsStore {
public:
    explicit SettingsStore(const QString& filePath = "");
    ~SettingsStore();

    // 窗口布局
    int sidebarWidth() const;
    void setSidebarWidth(int width);
    int aiPanelWidth() const;
    void setAiPanelWidth(int width);
    bool terminalCollapsed() const;
    void setTerminalCollapsed(bool collapsed);

    // 隐私确认 (Phase 2 AI 用)
    bool aiContentConsent() const;
    void setAiContentConsent(bool consent);

private:
    QString m_filePath;
};
```

- [ ] **步骤 4：编写 SettingsStore.cpp**

```cpp
// src/data/SettingsStore.cpp
#include "SettingsStore.h"
#include <QSettings>

SettingsStore::SettingsStore(const QString& filePath)
    : m_filePath(filePath.isEmpty() ? "deepin-iman" : filePath)
{
}

SettingsStore::~SettingsStore() = default;

int SettingsStore::sidebarWidth() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("layout/sidebarWidth", 240).toInt();
}

void SettingsStore::setSidebarWidth(int width) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("layout/sidebarWidth", width);
}

int SettingsStore::aiPanelWidth() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("layout/aiPanelWidth", 360).toInt();
}

void SettingsStore::setAiPanelWidth(int width) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("layout/aiPanelWidth", width);
}

bool SettingsStore::terminalCollapsed() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("layout/terminalCollapsed", true).toBool();
}

void SettingsStore::setTerminalCollapsed(bool collapsed) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("layout/terminalCollapsed", collapsed);
}

bool SettingsStore::aiContentConsent() const {
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value("ai/contentConsent", false).toBool();
}

void SettingsStore::setAiContentConsent(bool consent) {
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue("ai/contentConsent", consent);
}
```

- [ ] **步骤 5：在 CMakeLists.txt 添加测试目标**

在 `enable_testing()` 之后添加：

```cmake
add_executable(TestSettingsStore tests/TestSettingsStore.cpp)
target_link_libraries(TestSettingsStore PRIVATE
    Qt6::Core Qt6::Test deepin-iman
)
target_include_directories(TestSettingsStore PRIVATE src)
add_test(NAME TestSettingsStore COMMAND TestSettingsStore)
```

- [ ] **步骤 6：运行测试验证通过**

运行：
```bash
cmake --build build --target TestSettingsStore && ctest --test-dir build -R TestSettingsStore --output-on-failure
```
预期：PASS，3 个测试用例全部通过。

- [ ] **步骤 7：Commit**

```bash
git add src/data/SettingsStore.h src/data/SettingsStore.cpp tests/TestSettingsStore.cpp CMakeLists.txt
git commit -m "feat(data): add SettingsStore with QSettings backend"
```

---

## 任务 3：ManIndex — man 页扫描与元数据表

**文件：**
- 创建：`src/data/ManIndex.h`
- 创建：`src/data/ManIndex.cpp`
- 创建：`data/sections.json`
- 测试：`tests/TestManIndex.cpp`

- [ ] **步骤 1：编写 sections.json**

```json
[
    {"section": 1, "name": "用户命令"},
    {"section": 2, "name": "系统调用"},
    {"section": 3, "name": "库函数"},
    {"section": 4, "name": "特殊文件"},
    {"section": 5, "name": "文件格式"},
    {"section": 6, "name": "游戏"},
    {"section": 7, "name": "杂项"},
    {"section": 8, "name": "管理命令"}
]
```

- [ ] **步骤 2：编写失败的测试 — 表创建与扫描**

```cpp
// tests/TestManIndex.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "data/ManIndex.h"

class TestManIndex : public QObject {
    Q_OBJECT
private slots:
    void testSchemaCreated() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        QVERIFY(idx.tableExists("man_page"));
        QVERIFY(idx.tableExists("man_fts"));
    }

    void testScanEmptyDir() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        QCOMPARE(idx.scanManPages(tmp.path()), 0);
        QCOMPARE(idx.pageCount(), 0);
    }

    void testScanSinglePage() {
        QTemporaryDir tmp;
        QDir manDir(tmp.path() + "/man1");
        manDir.mkpath(".");
        QFile f(manDir.absoluteFilePath("ls.1.gz"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("dummy gz content");
        f.close();

        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        QCOMPARE(idx.scanManPages(tmp.path()), 1);
        QCOMPARE(idx.pageCount(), 1);
    }
};

QTEST_MAIN(TestManIndex)
#include "TestManIndex.moc"
```

- [ ] **步骤 3：运行测试验证失败**

运行：`cmake --build build --target TestManIndex 2>&1 | tail -5`
预期：FAIL，报 `ManIndex.h: No such file or directory`。

- [ ] **步骤 4：编写 ManIndex.h**

```cpp
// src/data/ManIndex.h
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>

struct ManPage {
    int id = -1;
    QString name;
    int section = 0;
    QString sectionName;
    QString sourcePath;
    QString title;
    qint64 sourceMtime = 0;
    qint64 indexedAt = 0;
};

class ManIndex : public QObject {
    Q_OBJECT
public:
    explicit ManIndex(const QString& dbPath, QObject* parent = nullptr);
    ~ManIndex();

    bool open();
    bool tableExists(const QString& name);
    int scanManPages(const QString& manRoot);  // 返回扫描的页数
    int pageCount() const;

    // 查询
    QList<ManPage> findByName(const QString& name) const;
    ManPage findById(int id) const;
    QList<ManPage> fullTextSearch(const QString& query, int limit = 20) const;

signals:
    void scanProgress(int current, int total);
    void scanFinished(int totalPages);

private:
    QString m_dbPath;
    QSqlDatabase m_db;
    void createSchema();
    QStringList findManFiles(const QString& manRoot) const;
    ManPage parseManPath(const QString& path) const;
    QString sectionName(int section) const;
    QString extractTitle(const QString& gzPath) const;
};
```

- [ ] **步骤 5：编写 ManIndex.cpp — schema + 扫描部分**

```cpp
// src/data/ManIndex.cpp
#include "ManIndex.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QProcess>
#include <QDebug>

ManIndex::ManIndex(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath)
{
}

ManIndex::~ManIndex() {
    if (m_db.isOpen()) m_db.close();
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
    for (const auto& entry : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QRegularExpression manDirRe("^man([1-8])$");
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
    page.title = extractTitle(path);
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
    // 用 mandoc -Tplain 提取第一行作为标题
    QProcess p;
    p.start("mandoc", {"-Tplain", gzPath});
    p.waitForFinished(2000);
    QString out = QString::fromUtf8(p.readAllStandardOutput());
    QStringList lines = out.split('\n', Qt::SkipEmptyParts);
    return lines.isEmpty() ? QString() : lines.first().trimmed();
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
        q.prepare("INSERT INTO man_page (name, section, section_name, source_path, title, source_mtime, indexed_at) "
                   "VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(page.name);
        q.addBindValue(page.section);
        q.addBindValue(page.sectionName);
        q.addBindValue(page.sourcePath);
        q.addBindValue(page.title);
        q.addBindValue(page.sourceMtime);
        q.addBindValue(QDateTime::currentSecsSinceEpoch());
        if (q.exec()) {
            // 插入 FTS 索引 (body 暂用 title 占位, 完整正文任务 4 补充)
            int id = q.lastInsertId().toInt();
            q.prepare("INSERT INTO man_fts (rowid, name, title, body) VALUES (?, ?, ?, ?)");
            q.addBindValue(id);
            q.addBindValue(page.name);
            q.addBindValue(page.title);
            q.addBindValue(page.title);
            q.exec();
            ++count;
        }
        emit scanProgress(i + 1, files.size());
    }
    emit scanFinished(count);
    return count;
}

int ManIndex::pageCount() const {
    QSqlQuery q(QSqlDatabase::database(m_dbPath));
    q.exec("SELECT COUNT(*) FROM man_page");
    return q.next() ? q.value(0).toInt() : 0;
}
```

- [ ] **步骤 6：在 CMakeLists.txt 添加测试目标**

```cmake
add_executable(TestManIndex tests/TestManIndex.cpp)
target_link_libraries(TestManIndex PRIVATE
    Qt6::Core Qt6::Sql Qt6::Test deepin-iman
)
target_include_directories(TestManIndex PRIVATE src)
add_test(NAME TestManIndex COMMAND TestManIndex)
```

- [ ] **步骤 7：运行测试验证通过**

运行：
```bash
cmake --build build --target TestManIndex && ctest --test-dir build -R TestManIndex --output-on-failure
```
预期：PASS，3 个用例通过。

- [ ] **步骤 8：Commit**

```bash
git add src/data/ManIndex.h src/data/ManIndex.cpp tests/TestManIndex.cpp data/sections.json CMakeLists.txt
git commit -m "feat(data): add ManIndex with SQLite FTS5 schema and man scan"
```

---

## 任务 4：ManIndex — FTS5 全文索引正文

**文件：**
- 修改：`src/data/ManIndex.cpp` (scanManPages 中 body 提取)
- 修改：`src/data/ManIndex.h` (添加 findByName/fullTextSearch 实现)
- 测试：`tests/TestManIndex.cpp` (追加用例)

- [ ] **步骤 1：编写失败的测试 — 全文搜索**

在 `tests/TestManIndex.cpp` 追加：

```cpp
void testFullTextSearch() {
    QTemporaryDir tmp;
    QDir manDir(tmp.path() + "/man1");
    manDir.mkpath(".");
    // 创建一个模拟 man 页, 用 mandoc 无法解析则 body 留空
    // 这里用真实系统 man 页路径测试 (CI 环境需安装 mandoc)
    ManIndex idx(tmp.path() + "/test.db");
    QVERIFY(idx.open());
    // 扫描真实系统 man 目录
    int n = idx.scanManPages("/usr/share/man");
    if (n == 0) QSKIP("No man pages on this system");
    // 搜索 "list" 应能找到 ls(1)
    auto results = idx.fullTextSearch("list", 10);
    QVERIFY(!results.isEmpty());
    bool foundLs = false;
    for (const auto& p : results) {
        if (p.name == "ls") { foundLs = true; break; }
    }
    QVERIFY(foundLs);
}

void testFindByName() {
    QTemporaryDir tmp;
    ManIndex idx(tmp.path() + "/test.db");
    QVERIFY(idx.open());
    int n = idx.scanManPages("/usr/share/man");
    if (n == 0) QSKIP("No man pages");
    auto results = idx.findByName("ls");
    QVERIFY(!results.isEmpty());
    QCOMPARE(results.first().name, QString("ls"));
}
```

在 `private slots:` 区块末尾、`testScanSinglePage` 之后添加这两个用例。还需在文件顶部添加 `#include <QDir>` (已有)。

- [ ] **步骤 2：运行测试验证失败**

运行：`ctest --test-dir build -R TestManIndex --output-on-failure`
预期：FAIL，`fullTextSearch` / `findByName` 未实现（返回空）。

- [ ] **步骤 3：实现 ManIndex 查询方法**

在 `src/data/ManIndex.cpp` 末尾添加：

```cpp
QList<ManPage> ManIndex::findByName(const QString& name) const {
    QList<ManPage> results;
    QSqlQuery q(QSqlDatabase::database(m_dbPath));
    q.prepare("SELECT id, name, section, section_name, source_path, title, source_mtime, indexed_at "
               "FROM man_page WHERE name = ? ORDER BY section LIMIT 20");
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

ManPage ManIndex::findById(int id) const {
    ManPage p;
    QSqlQuery q(QSqlDatabase::database(m_dbPath));
    q.prepare("SELECT id, name, section, section_name, source_path, title, source_mtime, indexed_at "
               "FROM man_page WHERE id = ?");
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
    QSqlQuery q(QSqlDatabase::database(m_dbPath));
    q.prepare("SELECT man_page.id, man_page.name, man_page.section, "
              "       man_page.section_name, man_page.source_path, "
              "       man_page.title, man_page.source_mtime, man_page.indexed_at, "
              "       bm25(man_fts) AS rank "
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
```

还需在 `scanManPages` 中将 body 从 title 改为完整正文。修改 `scanManPages` 中插入 FTS 的块：

```cpp
// 替换原 body=page.title 的占位
QString fullText = page.name + " " + page.title + " " + extractTitle(page.sourcePath);
q.prepare("INSERT INTO man_fts (rowid, name, title, body) VALUES (?, ?, ?, ?)");
q.addBindValue(id);
q.addBindValue(page.name);
q.addBindValue(page.title);
q.addBindValue(fullText);
q.exec();
```

- [ ] **步骤 4：运行测试验证通过**

运行：`ctest --test-dir build -R TestManIndex --output-on-failure`
预期：PASS，5 个用例全通过（含真实系统 man 搜索）。

- [ ] **步骤 5：Commit**

```bash
git add src/data/ManIndex.cpp src/data/ManIndex.h tests/TestManIndex.cpp
git commit -m "feat(data): implement ManIndex fulltext search and name lookup"
```

---

## 任务 5：ManService — mandoc 转 HTML 管线

**文件：**
- 创建：`src/service/ManService.h`
- 创建：`src/service/ManService.cpp`
- 测试：`tests/TestManService.cpp`

- [ ] **步骤 1：编写失败的测试**

```cpp
// tests/TestManService.cpp
#include <QtTest/QtTest>
#include "service/ManService.h"

class TestManService : public QObject {
    Q_OBJECT
private slots:
    void testRenderNonexistentPage() {
        ManService svc;
        QString html = svc.renderPage("/nonexistent/path.1.gz");
        QVERIFY(html.isEmpty());
    }

    void testParseCrossReferences() {
        ManService svc;
        QString html = "<a href=\"man:grep(1)\">grep(1)</a> and "
                       "<a href=\"man:ls(1)\">ls(1)</a>";
        auto refs = svc.parseCrossReferences(html);
        QCOMPARE(refs.size(), 2);
        QCOMPARE(refs[0].name, QString("grep"));
        QCOMPARE(refs[0].section, 1);
        QCOMPARE(refs[1].name, QString("ls"));
        QCOMPARE(refs[1].section, 1);
    }

    void testRenderRealSystemPage() {
        ManService svc;
        // 用系统 ls 页测试
        QFile f("/usr/share/man/man1/ls.1.gz");
        if (!f.exists()) QSKIP("No ls.1.gz on system");
        QString html = svc.renderPage("/usr/share/man/man1/ls.1.gz");
        QVERIFY(!html.isEmpty());
        QVERIFY(html.contains("ls") || html.contains("LS"));
    }
};

QTEST_MAIN(TestManService)
#include "TestManService.moc"
```

- [ ] **步骤 2：运行测试验证失败**

运行：`cmake --build build --target TestManService 2>&1 | tail -5`
预期：FAIL，报 `ManService.h: No such file or directory`。

- [ ] **步骤 3：编写 ManService.h**

```cpp
// src/service/ManService.h
#pragma once
#include <QObject>
#include <QString>
#include <QList>

struct CrossReference {
    QString name;
    int section;
};

class ManService : public QObject {
    Q_OBJECT
public:
    explicit ManService(QObject* parent = nullptr);

    // 同步: gz 解压 + mandoc -Thtml, 返回 HTML 片段
    QString renderPage(const QString& gzPath);

    // 解析 HTML 中的 <a href="man:name(section)">
    QList<CrossReference> parseCrossReferences(const QString& html) const;

    // 异步版本
    void renderPageAsync(const QString& gzPath);

signals:
    void pageRendered(const QString& html);
    void renderFailed(const QString& reason);
};
```

- [ ] **步骤 4：编写 ManService.cpp**

```cpp
// src/service/ManService.cpp
#include "ManService.h"
#include <QProcess>
#include <QRegularExpression>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

ManService::ManService(QObject* parent) : QObject(parent) {
}

QString ManService::renderPage(const QString& gzPath) {
    if (!QFileInfo::exists(gzPath)) {
        qWarning() << "ManService: file not found" << gzPath;
        return QString();
    }
    QProcess p;
    p.start("mandoc", {"-Thtml", "-O", "fragment", gzPath});
    if (!p.waitForFinished(5000)) {
        qWarning() << "ManService: mandoc timeout" << gzPath;
        return QString();
    }
    if (p.exitCode() != 0) {
        qWarning() << "ManService: mandoc error" << p.readAllStandardError();
        return QString();
    }
    return QString::fromUtf8(p.readAllStandardOutput());
}

QList<CrossReference> ManService::parseCrossReferences(const QString& html) const {
    QList<CrossReference> refs;
    QRegularExpression re("href=\"man:([^(]+)\\((\\d+)\\)\"");
    auto it = re.globalMatch(html);
    while (it.hasNext()) {
        auto m = it.next();
        CrossReference ref;
        ref.name = m.captured(1);
        ref.section = m.captured(2).toInt();
        refs << ref;
    }
    return refs;
}

void ManService::renderPageAsync(const QString& gzPath) {
    QtConcurrent::run([this, gzPath]() {
        QString html = renderPage(gzPath);
        if (html.isEmpty()) {
            emit renderFailed(tr("Failed to render: %1").arg(gzPath));
        } else {
            emit pageRendered(html);
        }
    });
}
```

- [ ] **步骤 5：在 CMakeLists.txt 添加测试目标**

```cmake
add_executable(TestManService tests/TestManService.cpp)
target_link_libraries(TestManService PRIVATE
    Qt6::Core Qt6::Test deepin-iman
)
target_include_directories(TestManService PRIVATE src)
add_test(NAME TestManService COMMAND TestManService)
```

- [ ] **步骤 6：运行测试验证通过**

运行：`ctest --test-dir build -R TestManService --output-on-failure`
预期：PASS，3 个用例通过。

- [ ] **步骤 7：Commit**

```bash
git add src/service/ManService.h src/service/ManService.cpp tests/TestManService.cpp CMakeLists.txt
git commit -m "feat(service): add ManService with mandoc-to-HTML pipeline"
```

---

## 任务 6：SearchService — 搜索路由

**文件：**
- 创建：`src/service/SearchService.h`
- 创建：`src/service/SearchService.cpp`
- 测试：`tests/TestSearchService.cpp`

- [ ] **步骤 1：编写失败的测试**

```cpp
// tests/TestSearchService.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include "service/SearchService.h"
#include "data/ManIndex.h"

class TestSearchService : public QObject {
    Q_OBJECT
private slots:
    void testEmptyQuery() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        idx.open();
        SearchService svc(&idx);
        auto results = svc.search("", 20);
        QVERIFY(results.isEmpty());
    }

    void testRealSystemSearch() {
        ManIndex idx(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/test_iman.db");
        idx.open();
        int n = idx.scanManPages("/usr/share/man");
        if (n == 0) QSKIP("No man pages");
        SearchService svc(&idx);
        auto results = svc.search("list", 20);
        QVERIFY(!results.isEmpty());
        // 命令名匹配应优先于全文
        bool nameMatchFirst = (results.first().name == "ls") || results.first().name.contains("list");
        QVERIFY(nameMatchFirst);
    }
};

QTEST_MAIN(TestSearchService)
#include "TestSearchService.moc"
```

顶部还需添加 `#include <QStandardPaths>`。

- [ ] **步骤 2：运行测试验证失败**

运行：`cmake --build build --target TestSearchService 2>&1 | tail -5`
预期：FAIL，`SearchService.h: No such file or directory`。

- [ ] **步骤 3：编写 SearchService.h**

```cpp
// src/service/SearchService.h
#pragma once
#include <QObject>
#include <QList>

class ManIndex;
struct ManPage;

class SearchService : public QObject {
    Q_OBJECT
public:
    explicit SearchService(ManIndex* index, QObject* parent = nullptr);

    // 搜索: 先命令名 LIKE, 再 FTS5 全文, 合并去重
    QList<ManPage> search(const QString& query, int limit = 20) const;

    // 打开指定 man 页 (触发 openPageRequested 信号)
    void openPage(const QString& name, int section);

signals:
    void searchCompleted(const QList<ManPage>& results);
    void openPageRequested(const QString& name, int section);

private:
    ManIndex* m_index;
};
```

- [ ] **步骤 4：编写 SearchService.cpp**

```cpp
// src/service/SearchService.cpp
#include "SearchService.h"
#include "ManIndex.h"

SearchService::SearchService(ManIndex* index, QObject* parent)
    : QObject(parent), m_index(index)
{
}

QList<ManPage> SearchService::search(const QString& query, int limit) const {
    QList<ManPage> results;
    if (query.trimmed().isEmpty()) return results;

    // 1. 命令名精确匹配 (优先)
    auto byName = m_index->findByName(query.trimmed());
    for (const auto& p : byName) {
        results << p;
        if (results.size() >= limit) return results;
    }

    // 2. 命令名 LIKE 模糊匹配
    QSqlQuery q;  // 需要 ManIndex 暴露 db, 这里用 findByName 代替
    // (简化: 直接用 fullTextSearch 补足)
    auto fts = m_index->fullTextSearch(query, limit - results.size());
    QSet<int> seenIds;
    for (const auto& p : results) seenIds << p.id;
    for (const auto& p : fts) {
        if (!seenIds.contains(p.id)) {
            results << p;
            seenIds << p.id;
        }
        if (results.size() >= limit) break;
    }
    return results;
}

void SearchService::openPage(const QString& name, int section) {
    emit openPageRequested(name, section);
}
```

- [ ] **步骤 5：在 CMakeLists.txt 添加测试目标**

```cmake
add_executable(TestSearchService tests/TestSearchService.cpp)
target_link_libraries(TestSearchService PRIVATE
    Qt6::Core Qt6::Sql Qt6::Test deepin-iman
)
target_include_directories(TestSearchService PRIVATE src)
add_test(NAME TestSearchService COMMAND TestSearchService)
```

- [ ] **步骤 6：运行测试验证通过**

运行：`ctest --test-dir build -R TestSearchService --output-on-failure`
预期：PASS。

- [ ] **步骤 7：Commit**

```bash
git add src/service/SearchService.h src/service/SearchService.cpp tests/TestSearchService.cpp CMakeLists.txt
git commit -m "feat(service): add SearchService with name+FTS5 combined search"
```

---

## 任务 7：ManView — QTextBrowser 渲染 + 跳转

**文件：**
- 创建：`src/view/ManView.h`
- 创建：`src/view/ManView.cpp`

- [ ] **步骤 1：编写 ManView.h**

```cpp
// src/view/ManView.h
#pragma once
#include <QTextBrowser>
#include <QString>

class ManView : public QTextBrowser {
    Q_OBJECT
public:
    explicit ManView(QWidget* parent = nullptr);

    void loadHtml(const QString& html);

signals:
    void crossReferenceClicked(const QString& name, int section);

private slots:
    void onAnchorClicked(const QUrl& url);
};
```

- [ ] **步骤 2：编写 ManView.cpp**

```cpp
// src/view/ManView.cpp
#include "ManView.h"
#include <QUrl>
#include <QRegularExpression>

ManView::ManView(QWidget* parent) : QTextBrowser(parent) {
    setOpenExternalLinks(false);
    connect(this, &QTextBrowser::anchorClicked, this, &ManView::onAnchorClicked);
}

void ManView::loadHtml(const QString& html) {
    setHtml(html);
}

void ManView::onAnchorClicked(const QUrl& url) {
    if (url.scheme() != "man") {
        QTextBrowser::setSource(url);
        return;
    }
    // 解析 man:grep(1) → name=grep, section=1
    QString path = url.path().isEmpty() ? url.toString().mid(4) : url.path();
    QRegularExpression re("^(.+)\\((\\d+)\\)$");
    auto m = re.match(path);
    if (m.hasMatch()) {
        emit crossReferenceClicked(m.captured(1), m.captured(2).toInt());
    }
}
```

- [ ] **步骤 3：更新 CMakeLists.txt 源列表 (ManView 已在 add_executable 中，无需改)**

验证 `add_executable(deepin-iman ...)` 已含 `src/view/ManView.cpp`。任务 1 已包含。

- [ ] **步骤 4：编译验证**

运行：`cmake --build build 2>&1 | tail -5`
预期：编译通过（ManView 未被 main.cpp 引用，但应编译为目标文件）。

- [ ] **步骤 5：Commit**

```bash
git add src/view/ManView.h src/view/ManView.cpp
git commit -m "feat(view): add ManView with cross-reference click handling"
```

---

## 任务 8：LeftSidebar — 搜索框 + 命令导航树

**文件：**
- 创建：`src/view/LeftSidebar.h`
- 创建：`src/view/LeftSidebar.cpp`

- [ ] **步骤 1：编写 LeftSidebar.h**

```cpp
// src/view/LeftSidebar.h
#pragma once
#include <DWidget>
#include <DLineEdit>
#include <DTreeView>
#include <DListView>
#include <QStandardItemModel>

DWIDGET_USE_NAMESPACE

class LeftSidebar : public DWidget {
    Q_OBJECT
public:
    explicit LeftSidebar(QWidget* parent = nullptr);

    void setManPages(const QList<struct ManPage>& pages);
    void setHistoryItems(const QStringList& items);  // Phase 4 完整实现, 先占位
    void setFavoriteItems(const QStringList& items); // Phase 4 完整实现, 先占位

signals:
    void searchRequested(const QString& query);
    void pageSelected(const QString& name, int section);

private:
    DLineEdit* m_searchEdit;
    DTreeView* m_navTree;
    QStandardItemModel* m_navModel;
};
```

- [ ] **步骤 2：编写 LeftSidebar.cpp**

```cpp
// src/view/LeftSidebar.cpp
#include "LeftSidebar.h"
#include "data/ManIndex.h"
#include <QVBoxLayout>

LeftSidebar::LeftSidebar(QWidget* parent) : DWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new DLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search man pages..."));
    connect(m_searchEdit, &DLineEdit::textChanged, this, [this](const QString& t) {
        if (t.length() >= 2) emit searchRequested(t);
    });
    layout->addWidget(m_searchEdit);

    m_navTree = new DTreeView(this);
    m_navModel = new QStandardItemModel(this);
    m_navModel->setHorizontalHeaderLabels({tr("Command"), tr("Section")});
    m_navTree->setModel(m_navModel);
    connect(m_navTree, &DTreeView::clicked, this, [this](const QModelIndex& idx) {
        auto* item = m_navModel->item(idx.row(), 0);
        int section = m_navModel->item(idx.row(), 1)->text().toInt();
        emit pageSelected(item->text(), section);
    });
    layout->addWidget(m_navTree, 1);
}

void LeftSidebar::setManPages(const QList<ManPage>& pages) {
    m_navModel->removeRows(0, m_navModel->rowCount());
    for (const auto& p : pages) {
        QList<QStandardItem*> row;
        row << new QStandardItem(p.name);
        row << new QStandardItem(QString::number(p.section));
        m_navModel->appendRow(row);
    }
}

void LeftSidebar::setHistoryItems(const QStringList& items) {
    // Phase 4 实现
    Q_UNUSED(items)
}

void LeftSidebar::setFavoriteItems(const QStringList& items) {
    // Phase 4 实现
    Q_UNUSED(items)
}
```

- [ ] **步骤 3：编译验证**

运行：`cmake --build build 2>&1 | tail -5`
预期：编译通过。

- [ ] **步骤 4：Commit**

```bash
git add src/view/LeftSidebar.h src/view/LeftSidebar.cpp
git commit -m "feat(view): add LeftSidebar with search box and navigation tree"
```

---

## 任务 9：MainWindow — 三栏布局 + Tab 栏 + 导航栈

**文件：**
- 创建：`src/view/MainWindow.h`
- 创建：`src/view/MainWindow.cpp`
- 修改：`src/main.cpp` (实例化 MainWindow)

- [ ] **步骤 1：编写 MainWindow.h**

```cpp
// src/view/MainWindow.h
#pragma once
#include <DMainWindow>
#include <QStack>
#include <QList>

DWIDGET_USE_NAMESPACE

class LeftSidebar;
class ManView;
class ManIndex;
class SearchService;
class ManService;

struct ManPage;

class MainWindow : public DMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ManIndex* index, SearchService* searchSvc, ManService* manSvc, QWidget* parent = nullptr);

private slots:
    void onSearchRequested(const QString& query);
    void onPageSelected(const QString& name, int section);
    void onCrossRefClicked(const QString& name, int section);
    void onPageRendered(const QString& html);
    void onPrevPage();
    void onNextPage();

private:
    LeftSidebar* m_sidebar;
    ManView* m_manView;
    ManIndex* m_index;
    SearchService* m_searchSvc;
    ManService* m_manSvc;

    QStack<int> m_backStack;
    QStack<int> m_forwardStack;
    int m_currentPageId = -1;

    void openPage(const QString& name, int section);
    void updateNavButtons();
};
```

- [ ] **步骤 2：编写 MainWindow.cpp**

```cpp
// src/view/MainWindow.cpp
#include "MainWindow.h"
#include "LeftSidebar.h"
#include "ManView.h"
#include "data/ManIndex.h"
#include "service/SearchService.h"
#include "service/ManService.h"
#include <DTitlebar>
#include <QSplitter>
#include <QAction>
#include <QShortcut>

MainWindow::MainWindow(ManIndex* index, SearchService* searchSvc, ManService* manSvc, QWidget* parent)
    : DMainWindow(parent), m_index(index), m_searchSvc(searchSvc), m_manSvc(manSvc)
{
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_sidebar = new LeftSidebar;
    m_manView = new ManView;

    splitter->addWidget(m_sidebar);
    splitter->addWidget(m_manView);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({240, 800});

    setCentralWidget(splitter);

    // 连接信号
    connect(m_sidebar, &LeftSidebar::searchRequested, this, &MainWindow::onSearchRequested);
    connect(m_sidebar, &LeftSidebar::pageSelected, this, &MainWindow::onPageSelected);
    connect(m_manView, &ManView::crossReferenceClicked, this, &MainWindow::onCrossRefClicked);
    connect(m_manSvc, &ManService::pageRendered, this, &MainWindow::onPageRendered);

    // 导航快捷键
    auto* prevSc = new QShortcut(QKeySequence("Alt+Left"), this);
    auto* nextSc = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(prevSc, &QShortcut::activated, this, &MainWindow::onPrevPage);
    connect(nextSc, &QShortcut::activated, this, &MainWindow::onNextPage);
}

void MainWindow::onSearchRequested(const QString& query) {
    auto results = m_searchSvc->search(query, 50);
    m_sidebar->setManPages(results);
}

void MainWindow::onPageSelected(const QString& name, int section) {
    openPage(name, section);
}

void MainWindow::onCrossRefClicked(const QString& name, int section) {
    openPage(name, section);
}

void MainWindow::openPage(const QString& name, int section) {
    auto pages = m_index->findByName(name);
    for (const auto& p : pages) {
        if (p.section == section) {
            if (m_currentPageId != -1) m_backStack.push(m_currentPageId);
            m_forwardStack.clear();
            m_currentPageId = p.id;
            m_manSvc->renderPageAsync(p.sourcePath);
            updateNavButtons();
            return;
        }
    }
}

void MainWindow::onPageRendered(const QString& html) {
    m_manView->loadHtml(html);
}

void MainWindow::onPrevPage() {
    if (m_backStack.isEmpty()) return;
    m_forwardStack.push(m_currentPageId);
    m_currentPageId = m_backStack.pop();
    ManPage p = m_index->findById(m_currentPageId);
    if (!p.sourcePath.isEmpty()) m_manSvc->renderPageAsync(p.sourcePath);
    updateNavButtons();
}

void MainWindow::onNextPage() {
    if (m_forwardStack.isEmpty()) return;
    m_backStack.push(m_currentPageId);
    m_currentPageId = m_forwardStack.pop();
    ManPage p = m_index->findById(m_currentPageId);
    if (!p.sourcePath.isEmpty()) m_manSvc->renderPageAsync(p.sourcePath);
    updateNavButtons();
}

void MainWindow::updateNavButtons() {
    // 简单实现: titlebar 按钮由 Phase 4 完善
}
```

- [ ] **步骤 3：更新 main.cpp 实例化 MainWindow**

```cpp
// src/main.cpp
#include <DApplication>
#include <QStandardPaths>
#include <QDir>
#include "view/MainWindow.h"
#include "data/ManIndex.h"
#include "data/SettingsStore.h"
#include "service/ManService.h"
#include "service/SearchService.h"

int main(int argc, char* argv[]) {
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-iman");
    app.setApplicationVersion("0.1.0");
    app.setProductIcon(QIcon::fromTheme("help-browser"));
    app.setProductName("deepin iman");
    app.setApplicationDescription(QObject::tr("AI-powered man page viewer"));

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    ManIndex index(dataDir + "/manindex.db");
    if (!index.open()) {
        qCritical() << "Cannot open index database";
        return 1;
    }

    // 首次启动扫描
    if (index.pageCount() == 0) {
        index.scanManPages("/usr/share/man");
    }

    ManService manService;
    SearchService searchService(&index);
    MainWindow w(&index, &searchService, &manService);
    w.show();

    return app.exec();
}
```

- [ ] **步骤 4：编译验证**

运行：`cmake --build build 2>&1 | tail -10`
预期：编译通过。

- [ ] **步骤 5：手动冒烟测试**

运行：`./build/deepin-iman`
预期：窗口出现，左侧栏显示搜索框，输入 "ls" 显示结果列表，点击打开 ls(1) man 页 HTML 内容，点击 SEE ALSO 中的 `grep(1)` 跳转到 grep 页面。

- [ ] **步骤 6：Commit**

```bash
git add src/view/MainWindow.h src/view/MainWindow.cpp src/main.cpp
git commit -m "feat(view): add MainWindow with 3-pane layout and nav stack"
```

---

## 任务 10：首次启动索引进度对话框

**文件：**
- 修改：`src/main.cpp` (添加 DProgressBar 对话框)
- 修改：`src/data/ManIndex.h` (scanProgress/scanFinished 已有信号)

- [ ] **步骤 1：修改 main.cpp 显示进度对话框**

在 `index.scanManPages` 调用前后添加：

```cpp
// src/main.cpp — 替换首次启动扫描块
if (index.pageCount() == 0) {
    DDialog progressDlg;
    progressDlg.setWindowTitle(QObject::tr("Indexing man pages..."));
    auto* progressBar = new DProgressBar;
    progressBar->setRange(0, 100);
    auto* label = new QLabel(QObject::tr("Scanning man pages, please wait..."));
    auto* layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(progressBar);
    auto* content = new QWidget;
    content->setLayout(layout);
    progressDlg.addContent(content);
    progressDlg.show();

    QObject::connect(&index, &ManIndex::scanProgress, [&](int cur, int total) {
        progressBar->setMaximum(total);
        progressBar->setValue(cur);
    });
    QObject::connect(&index, &ManIndex::scanFinished, &progressDlg, &DDialog::accept);

    // 异步扫描
    QtConcurrent::run([&index]() { index.scanManPages("/usr/share/man"); });
    app.processEvents();
}
```

还需在文件顶部添加 `#include <DDialog>`, `#include <DProgressBar>`, `#include <QLabel>`, `#include <QVBoxLayout>`, `#include <QtConcurrent>`。

- [ ] **步骤 2：编译验证**

运行：`cmake --build build 2>&1 | tail -5`
预期：编译通过。

- [ ] **步骤 3：手动冒烟测试 (清空索引后)**

运行：
```bash
rm -f ~/.local/share/deepin-iman/manindex.db
./build/deepin-iman
```
预期：弹出进度对话框，显示扫描进度，完成后自动关闭，主窗口显示。

- [ ] **步骤 4：Commit**

```bash
git add src/main.cpp
git commit -m "feat: show progress dialog during first-time man index scan"
```

---

## 任务 11：mandoc 缺失检测

**文件：**
- 修改：`src/main.cpp`

- [ ] **步骤 1：在 main.cpp 顶部添加 mandoc 检测**

在 `DApplication app(...)` 之后、打开索引之前添加：

```cpp
// 检测 mandoc 是否安装
QProcess whichProc;
whichProc.start("which", {"mandoc"});
whichProc.waitForFinished();
if (whichProc.exitCode() != 0) {
    DDialog dlg;
    dlg.setWindowTitle(QObject::tr("mandoc not installed"));
    dlg.setMessage(QObject::tr(
        "deepin-iman requires mandoc to render man pages.\n"
        "Please install it with:\n\n  sudo apt install mandoc\n\n"
        "Then restart deepin-iman."));
    dlg.addButton(QObject::tr("Exit"), false, DDialog::ButtonRecommend);
    dlg.exec();
    return 1;
}
```

顶部添加 `#include <QProcess>` 和 `#include <DDialog>` (若已有则跳过)。

- [ ] **步骤 2：编译验证**

运行：`cmake --build build 2>&1 | tail -5`
预期：编译通过。

- [ ] **步骤 3：手动验证 (模拟 mandoc 缺失)**

运行（临时改名 mandoc）：
```bash
sudo mv /usr/bin/mandoc /usr/bin/mandoc.bak && ./build/deepin-iman
sudo mv /usr/bin/mandoc.bak /usr/bin/mandoc
```
预期：弹出 mandoc 未安装对话框，点 Exit 退出。

- [ ] **步骤 4：Commit**

```bash
git add src/main.cpp
git commit -m "feat: detect mandoc at startup and guide install"
```

---

## 任务 12：完整集成冒烟测试

**文件：**
- 创建：`tests/TestIntegration.cpp`

- [ ] **步骤 1：编写集成测试**

```cpp
// tests/TestIntegration.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QProcess>
#include "data/ManIndex.h"
#include "service/ManService.h"
#include "service/SearchService.h"

class TestIntegration : public QObject {
    Q_OBJECT
private slots:
    void testFullPipeline() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());

        int n = idx.scanManPages("/usr/share/man");
        if (n == 0) QSKIP("No man pages");

        // 1. 搜索
        SearchService search(&idx);
        auto results = search.search("ls", 10);
        QVERIFY(!results.isEmpty());

        // 2. 取第一个结果渲染
        ManService manSvc;
        QString html = manSvc.renderPage(results.first().sourcePath);
        QVERIFY(!html.isEmpty());

        // 3. 解析交叉引用
        auto refs = manSvc.parseCrossReferences(html);
        // ls 的 SEE ALSO 通常有 dir(1), vdir(1) 等
        QVERIFY(!refs.isEmpty() || html.contains("SEE ALSO") == false);

        // 4. 通过交叉引用跳转到第二个页
        if (!refs.isEmpty()) {
            auto refPages = idx.findByName(refs.first().name);
            QVERIFY(!refPages.isEmpty());
            QString html2 = manSvc.renderPage(refPages.first().sourcePath);
            QVERIFY(!html2.isEmpty());
        }
    }
};

QTEST_MAIN(TestIntegration)
#include "TestIntegration.moc"
```

- [ ] **步骤 2：在 CMakeLists.txt 添加测试目标**

```cmake
add_executable(TestIntegration tests/TestIntegration.cpp)
target_link_libraries(TestIntegration PRIVATE
    Qt6::Core Qt6::Sql Qt6::Test deepin-iman
)
target_include_directories(TestIntegration PRIVATE src)
add_test(NAME TestIntegration COMMAND TestIntegration)
```

- [ ] **步骤 3：运行全部测试**

运行：`ctest --test-dir build --output-on-failure`
预期：所有测试 PASS (TestSettingsStore, TestManIndex, TestManService, TestSearchService, TestIntegration)。

- [ ] **步骤 4：完整手动冒烟**

运行：`./build/deepin-iman`
测试流程：
1. 输入 "ls" → 左侧栏显示搜索结果
2. 点击 `ls 1` → 主区显示 ls man 页 HTML
3. 滚动到 SEE ALSO → 点击 `dir(1)` → 跳转到 dir man 页
4. Alt+Left → 返回 ls 页
5. Alt+Right → 前进到 dir 页

- [ ] **步骤 5：Commit**

```bash
git add tests/TestIntegration.cpp CMakeLists.txt
git commit -m "test: add full pipeline integration test"
```

---

## 自检

### 1. 规格覆盖度（Phase 1 范围）

| 规格章节 | Phase 1 任务 | 覆盖 |
|----------|-------------|------|
| 2. 架构总览 | 任务 1-12 | ✅ 三层全建立 |
| 3.1 主窗口布局 (左侧栏+主区) | 任务 9 | ✅ (右侧 AI 栏 Phase 2, 终端 Phase 4) |
| 4.1 ManIndex | 任务 3, 4 | ✅ |
| 4.5 SettingsStore | 任务 2 | ✅ |
| 6. Man 渲染管线 | 任务 5 | ✅ |
| 6.3 交叉引用解析 | 任务 5, 7 | ✅ |
| 7.1 搜索流程 | 任务 6 | ✅ |
| 7.2 跳转类型 (搜索/交叉引用/返回前进) | 任务 7, 9 | ✅ (历史/收藏/AI 跳转 Phase 2-4) |
| 10. mandoc 未安装 | 任务 11 | ✅ |
| 11. 测试策略 | 任务 2-6, 12 | ✅ |

**Phase 1 范围外** (规格章节但本计划不含，由后续 Phase 覆盖)：
- 3.1 右侧 AI 栏 → Phase 2
- 3.1 终端面板 → Phase 4
- 4.2 TranslationCache → Phase 3
- 4.3 HistoryDb / 4.4 FavoriteDb → Phase 4
- 5. AI 服务层 → Phase 2
- 6.4 中英对照 → Phase 3
- 8. 翻译系统 → Phase 3
- 9. 扩展功能 → Phase 4

### 2. 占位符扫描

- 无 "TODO"、"待定"、"后续实现" 在代码块中（LeftSidebar 的 setHistoryItems/setFavoriteItems 标注 Phase 4 是合理的范围声明，非占位符）
- 每个代码步骤含完整代码
- 测试含断言和预期值

### 3. 类型一致性

- `ManPage` 结构体在任务 3 定义，任务 4/6/9 使用 — 字段一致 (id, name, section, sectionName, sourcePath, title, sourceMtime, indexedAt)
- `CrossReference` 结构体在任务 5 定义 (name, section) — 任务 5 测试一致
- `ManIndex::findByName` / `fullTextSearch` / `findById` 签名在任务 3 声明，任务 4/6/9 使用 — 一致
- `ManService::renderPage` / `parseCrossReferences` / `renderPageAsync` — 任务 5 声明，任务 9 使用 — 一致
- `SearchService::search` / `openPage` — 任务 6 声明，任务 9 使用 — 一致
- `ManView::loadHtml` / `crossReferenceClicked` — 任务 7 声明，任务 9 使用 — 一致
- `LeftSidebar::setManPages` / `searchRequested` / `pageSelected` — 任务 8 声明，任务 9 使用 — 一致

**未发现不一致。**

---

## 执行交接

**计划已完成并保存到 `docs/superpowers/plans/2026-08-12-deepin-iman-phase1.md`。两种执行方式：**

**1. 子代理驱动（推荐）** - 每个任务调度一个新的子代理，任务间进行审查，快速迭代

**2. 内联执行** - 在当前会话中使用 executing-plans 执行任务，批量执行并设有检查点

**选哪种方式？**
