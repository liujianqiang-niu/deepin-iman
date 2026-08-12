# deepin-iman 设计规格

**版本**: 1.0
**日期**: 2026-08-12
**状态**: 待审查
**目标平台**: deepin v25 (Linux)
**技术栈**: Qt6 + DTK6 + CMake
**应用类型**: 桌面单进程应用

---

## 1. 项目概述

### 1.1 目的

deepin-iman 是一款面向 deepin v25 桌面的 man 手册浏览器，核心价值是将传统 man 体验与 AI 能力深度融合：用户不仅能查阅、搜索、跳转所有 man 手册，还能借助 AI 获得中文翻译、使用样例、命令问答和命令解析能力。

### 1.2 核心需求

| 编号 | 需求 | 优先级 |
|------|------|--------|
| R1 | 查阅所有 man 手册，支持全文搜索与交叉引用跳转 | P0 |
| R2 | 接入 AI（云端 API 多供应商） | P0 |
| R3 | 显示 man 手册中文及中英对照 | P0 |
| R4 | 显示使用样例（AI 生成） | P0 |
| R5 | 扩展功能：收藏夹+AI 推荐 / 内嵌终端 / AI 速查卡 / 学习历史+周报 | P1 |
| R6 | 充分利用 AI（翻译 / 样例 / 问答 / 命令解析四类职责） | P0 |

### 1.3 非目标 (YAGNI)

- 不做多进程 daemon 架构（当前规模不需要）
- 不做插件系统（4 个 AI 供应商用 if/else 路由即可）
- 不做本地模型（Ollama）支持（用户选择云端方案）
- 不做自建全文搜索引擎（SQLite FTS5 足够）
- 不做 QSS 样式表（遵循 AGENTS.md，使用 DTK 原生主题）

---

## 2. 架构总览

### 2.1 单体三层架构

单进程，三层职责清晰，遵循 DTK6 桌面应用惯例。

```
┌─ UI 层 (src/view/) ──────────────────────────────────┐
│  DMainWindow 主壳                                    │
│  ├─ LeftSidebar: 命令导航树 + 收藏夹 + 历史           │
│  ├─ 主区: ManView(HTML) / TranslationView(对照)      │
│  ├─ AiChatWidget: 问答 / 样例 / 速查卡 / 命令解析    │
│  └─ TerminalPanel: 可折叠内嵌终端                    │
├─ Service 层 (src/service/) ─────────────────────────┤
│  ManService          man 源→HTML 管线 + 交叉引用解析  │
│  SearchService       FTS5 查询 + 命令名直达           │
│  AiService           多供应商路由 + prompt 模板 + 限流│
│  TranslationService  预置包查询 + AI 翻译 + 缓存     │
│  ExampleService      AI 生成样例 + 缓存              │
│  FavoriteService     收藏 CRUD + AI 推荐             │
│  HistoryService      浏览记录 + 周报生成             │
│  SettingsService    API key / 供应商 / 主题配置      │
├─ Data 层 (src/data/) ────────────────────────────────┤
│  ManIndex            SQLite FTS5 (man 全文索引)      │
│  TranslationCache    SQLite (hash→译文+来源+时间+模型)│
│  HistoryDb           SQLite (浏览历史)               │
│  FavoriteDb          SQLite (收藏)                  │
│  SettingsStore       QSettings (配置)               │
└──────────────────────────────────────────────────────┘
```

### 2.2 三条铁律

1. **异步纪律** — AI 调用、首次索引、翻译均走 `QtConcurrent::run` + 信号回主线程，禁止阻塞 UI 线程
2. **接口隔离** — Service 之间经抽象接口（`IAiProvider`、`ITranslationSource`）通信，不直接依赖具体实现，便于未来拆分
3. **500 行上限** — 每个 Service 单 .h/.cpp，超过即拆分（AGENTS.md 硬要求）

### 2.3 数据流总览

```
用户操作 → UI 层信号 → Service 层调用
    │                       ├─ 同步快操作 → 直接返回
    │                       └─ 异步慢操作 → QtConcurrent::run
    │                                          │
    ▼                                          ▼
UI 层更新 ← Service 层信号 ← Data 层 / AI Provider
```

---

## 3. UI 组件设计

### 3.1 主窗口布局

遵循 DTK6 控件优先、禁用 QSS、QPainter 自绘原则（AGENTS.md）。

```
┌─────────────────────────────────────────────────────┐
│ DMainWindow (DTitlebar + 中央 DWidget)              │
├──────────┬──────────────────────────┬───────────────┤
│ 左侧栏    │  主内容区                 │  右侧 AI 栏   │
│ 240px    │  (QSplitter 可拖动)       │  360px 可折叠 │
│          │                          │               │
│ 搜索框    │  ┌─ Tab 栏 ──────────┐   │  AI 对话      │
│ DLineEdit│  │ [ls(1)*] [grep(1)] │   │  DTextEdit   │
│          │  └───────────────────┘   │  消息气泡     │
│ 命令导航  │                          │               │
│ DTreeView│  ┌─ ManView ─────────┐   │  ───────────  │
│          │  │ HTML 渲染区        │   │  快捷操作:    │
│ 收藏夹    │  │ (QTextBrowser)    │   │  [翻译]      │
│ DListView│  │                    │   │  [生成样例]  │
│          │  │  支持 SEE ALSO    │   │  [速查卡]    │
│ 历史      │  │  超链接点击跳转    │   │  [解析命令]  │
│ DListView│  └──────────────────┘   │               │
│          │  ┌─ 对照视图 ────────┐   │  供应商切换   │
│          │  │ 英文│中文│对照     │   │  DComboBox   │
│          │  └──────────────────┘   │               │
├──────────┴──────────────────────────┴───────────────┤
│ TerminalPanel (QTermWidget/QProcess + DTextEdit)    │
└─────────────────────────────────────────────────────┘
```

### 3.2 控件映射

| 区域 | DTK6 控件 | 说明 |
|------|-----------|------|
| 主壳 | `DMainWindow` + `DWidget` | 中央用 DWidget 容器 |
| 搜索 | `DLineEdit` / `DSearchEdit` | 带 placeholder + 清除按钮 |
| 导航/收藏/历史 | `DTreeView` / `DListView` + `DStandardItemModel` | 树形/列表 |
| man 内容 | `QTextBrowser` (首选) | 零额外依赖，支持 `setHtml` + `anchorClicked` |
| AI 对话 | `DTextEdit` + 自绘气泡 | QPainter 绘制消息 |
| 终端 | `QTermWidget` (若可用) 或 QProcess + DTextEdit | 降级方案见第 8.2 节 |
| 设置 | `DDialog` + `DLineEdit` / `DComboBox` | API key 等配置 |
| 颜色/字体 | `DApplicationHelper` + `DPalette` | 跟随系统主题 |

### 3.3 信号槽

统一使用新式语法（AGENTS.md）：

```cpp
connect(btn, &QPushButton::clicked, this, &MyClass::onClick);
```

---

## 4. 数据层设计

三套 SQLite 数据库 + QSettings，全部经 Data 层访问，UI 不直接碰 SQL。

### 4.1 ManIndex (SQLite FTS5) — 全文索引

```sql
-- man_page: 元数据表
CREATE TABLE man_page (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,          -- 命令名 "ls"
    section INTEGER NOT NULL,    -- man 章节 1..8
    section_name TEXT,            -- "用户命令"
    source_path TEXT NOT NULL,   -- /usr/share/man/man1/ls.1.gz
    title TEXT,                  -- man 页标题
    source_mtime INTEGER,        -- 源文件 mtime (缓存失效判断)
    indexed_at INTEGER            -- 索引时间戳
);

-- man_fts: FTS5 全文索引 (name + title + 正文)
CREATE VIRTUAL TABLE man_fts USING fts5(
    name, title, body,
    content='man_page', content_rowid='id',
    tokenize='unicode61 remove_diacritics 2'
);
```

**索引构建**：启动时检查 `indexed_at` 与 `source_mtime`，若 man 页更新则 `QtConcurrent::run` 重建。首次构建显示 `DProgressBar` 进度对话框。

**中文分词**：`unicode61` tokenizer 基础支持 CJK 字符的逐字分词。若需更精准中文分词，编译时启用 SQLite ICU 扩展并切换 `tokenize='icu zh_CN'`。

### 4.2 TranslationCache (SQLite) — 译文缓存

```sql
CREATE TABLE translation (
    page_hash TEXT PRIMARY KEY,   -- SHA256(name+section+content_version)
    zh_text TEXT NOT NULL,         -- 完整中文译文
    source TEXT NOT NULL,          -- 'preset' | 'ai-openai' | 'ai-claude'...
    model TEXT,                    -- AI 来源时记录模型名
    translated_at INTEGER NOT NULL,
    quality TEXT DEFAULT 'draft'   -- 'draft' | 'reviewed'
);
```

**缓存策略**：
- 预置包覆盖的页 `source='preset'`，永不失效
- AI 翻译的页当 man 源 mtime 变化时标记过期
- `quality='draft'` 自动重译；`quality='reviewed'` 提示用户"原文已更新，是否重新翻译"

### 4.3 HistoryDb (SQLite) — 浏览历史

```sql
CREATE TABLE history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    page_id INTEGER NOT NULL REFERENCES man_page(id),
    visited_at INTEGER NOT NULL,
    duration_sec INTEGER,          -- 停留时长 (AI 周报用)
    ai_interactions INTEGER DEFAULT 0
);
CREATE INDEX idx_history_visited ON history(visited_at DESC);
```

### 4.4 FavoriteDb (SQLite) — 收藏夹

```sql
CREATE TABLE favorite (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    page_id INTEGER NOT NULL REFERENCES man_page(id),
    note TEXT,                     -- 用户备注
    tags TEXT,                     -- 逗号分隔标签
    created_at INTEGER NOT NULL
);
```

### 4.5 SettingsStore — QSettings

存储项：
- AI 供应商选择 (`active_provider`: "openai" | "claude" | "qwen" | "glm")
- 各供应商 API key (优先用 deepin secrets API，fallback QSettings)
- 翻译来源偏好 (`translation_mode`: "auto" | "preset_only" | "ai_only")
- 窗口布局 (侧栏宽度、AI 栏宽度、终端折叠状态)
- 隐私确认标记 (`ai_content_consent`: bool)
- 周报开关 (`weekly_report_enabled`: bool)

### 4.6 SQLite 配置

所有 SQLite 数据库启用 WAL 模式以提升并发读：
```sql
PRAGMA journal_mode=WAL;
PRAGMA integrity_check;  -- 启动时检查，损坏则重建
```

数据库位置：
- `~/.local/share/deepin-iman/manindex.db`
- `~/.local/share/deepin-iman/translation.db`
- `~/.local/share/deepin-iman/history.db`
- `~/.local/share/deepin-iman/favorite.db`

---

## 5. AI 服务层设计

### 5.1 供应商抽象接口

```cpp
// src/service/ai/IAiProvider.h
#pragma once
#include <QObject>
#include <functional>

struct AiRequest {
    QString prompt;
    QString systemPrompt;
    int maxTokens = 2000;
    double temperature = 0.3;
};

struct AiChunk {
    QString delta;   // 流式增量
};

struct AiResult {
    QString text;    // 完整结果
    int inputTokens;
    int outputTokens;
    QString model;
};

class IAiProvider {
public:
    virtual ~IAiProvider() = default;
    virtual QString id() const = 0;           // "openai" / "claude" / "qwen" / "glm"
    virtual QString displayName() const = 0;  // "OpenAI GPT-4o"
    virtual void chat(const AiRequest& req,
                      std::function<void(const AiChunk&)> onChunk,
                      std::function<void(const AiResult&)> onDone,
                      std::function<void(const QString&)> onError) = 0;
    virtual bool isConfigured() const = 0;     // API key 是否已设置
};
```

### 5.2 供应商实现

| 供应商 | 类名 | API 端点 | 流式协议 |
|--------|------|----------|----------|
| OpenAI | `OpenAiProvider` | `https://api.openai.com/v1/chat/completions` | SSE |
| Claude | `ClaudeProvider` | `https://api.anthropic.com/v1/messages` | SSE |
| 通义千问 | `QwenProvider` | `https://dashscope.aliyuncs.com/...` | SSE |
| 智谱 GLM | `GlmProvider` | `https://open.bigmodel.cn/api/paas/v4/chat/completions` | SSE |

每个供应商封装 `QNetworkAccessManager` 的 HTTP POST + SSE 流式解析。每个 .cpp <300 行。

### 5.3 AiService — 任务路由器

```cpp
class AiService : public QObject {
    Q_OBJECT
public:
    // 四类任务，每类有独立 prompt 模板
    void translatePage(const ManPage& page,
                       std::function<void(const AiChunk&)> onChunk,
                       std::function<void(const AiResult&)> onDone,
                       std::function<void(const QString&)> onError);
    void generateExamples(const ManPage& page, ...);
    void askQuestion(const ManPage& ctx, const QString& question, ...);
    QString parseCommand(const QString& cmdline);  // 同步快速解析

    void setActiveProvider(const QString& id);
    void cancelCurrentTask();
};
```

### 5.4 Prompt 模板

模板文件存于 `data/prompts/*.txt`，不硬编码在 C++ 中，便于调优。

| 任务 | 模板文件 | 输入 Token 预算 | 输出 Token 预算 | 策略 |
|------|----------|-----------------|-----------------|------|
| 翻译 | `translate.txt` | ≤8K | ≈输入长度 | 整页翻译，保留 man 结构标记 (`.SH`/`.SS`/`.TP`) |
| 样例生成 | `examples.txt` | ≤2K (man 摘要) | ≤1.5K | 3-5 个样例，带注释+预期输出 |
| 问答 | `qa.txt` | ≤6K (上下文) | ≤1K | RAG：注入当前 man 页相关段落 |
| 命令解析 | `parse.txt` | ≤500 | ≤200 | 仅提取命令名+管道/重定向结构 |

### 5.5 限流与错误处理

- **并发限流**：`QSemaphore` 并发上限 2（避免 API 限速）
- **任务队列**：`QQueue<AiTask>` 管理待执行任务
- **重试策略**：
  - 网络错误：重试 1 次
  - API 限速 (429)：指数退避 1s/3s/9s，最多 3 次
  - 内容错误：不重试，直接报错
- **取消**：每个任务持 `QFutureWatcher`，用户可取消进行中的 AI 调用
- **隐私提示**：首次发送 man 内容到云端时弹 `DMessageBox` 确认，记录选择到 `SettingsStore.ai_content_consent`

---

## 6. Man 页渲染管线

### 6.1 获取与转换流程

```
/usr/share/man/man1/ls.1.gz
    │ gunzip (QProcess 或 zlib)
    ▼
ls.1 (roff 源)
    │ mandoc -Thtml -O fragment (QProcess)
    ▼
ls.html (片段，无 <html><body> 包裹)
    │ ManService::parseCrossReferences() 解析 <a href="man:grep(1)">
    ▼
ManView 渲染 (QTextBrowser::setHtml)
```

### 6.2 渲染控件选择

**选择 QTextBrowser**：
- 优点：轻量、零额外依赖、支持 `setHtml` + `anchorClicked` 信号
- 缺点：CSS 支持有限，复杂表格渲染弱
- man 页 HTML 结构简单（标题、段落、列表、少量表格），QTextBrowser 足够
- 若后续发现渲染缺陷再评估 QWebEngineView（但会引入 Chromium ~100MB 依赖）

### 6.3 交叉引用解析

mandoc 生成的 HTML 中，交叉引用格式为：
```html
<a href="man:grep(1)">grep(1)</a>
```

`ManService::parseCrossReferences()` 解析所有 `href` 以 `man:` 开头的 `<a>` 标签，提取命令名和章节号。点击时 `QTextBrowser::anchorClicked` 信号触发 `SearchService::openPage(name, section)`。

### 6.4 中英对照显示

TranslationView 提供 3 种模式（用户切换）：
- **仅英文**：直接显示 mandoc HTML
- **仅中文**：预置包/AI 译文经结构对齐后渲染
- **对照**：按段落分块，左英右中，用 `QSplitter` 左右滚动同步

**段落对齐**：翻译时要求 AI 保持 man 结构标记（`.SH`/`.SS`/`.TP`），解析 HTML DOM 按节标题对齐中英文段落。

---

## 7. 搜索与跳转

### 7.1 搜索流程

```
用户输入 "递归列出"
    │
    ▼
SearchService::search("递归列出")
    │
    ├─ 命令名匹配 (快速):
    │   SELECT * FROM man_page WHERE name LIKE '%递归列出%' LIMIT 10
    │
    └─ FTS5 全文 (相关度排序):
        SELECT bm25(man_fts) AS rank, man_page.*
        FROM man_fts JOIN man_page ON man_fts.rowid = man_page.id
        WHERE man_fts MATCH '递归列出'
        ORDER BY rank LIMIT 20
    │
    ▼
结果列表 (DListView): [ls(1) - 列出目录内容 ★★★] [find(1) - 搜索文件 ★★]
    │
    ▼
点击 → openPage(name, section) → ManService 加载 → ManView 渲染
```

### 7.2 跳转类型

| 跳转源 | 触发方式 | 实现 |
|--------|----------|------|
| 搜索结果 → man 页 | 点击列表项 | `SearchService::openPage()` |
| man 内交叉引用 | 点击 `SEE ALSO` 中的 `grep(1)` | 解析 `<a href="man:grep(1)">` → `openPage()` |
| AI 解析命令 → man 页 | 粘贴终端命令，AI 提取命令名 | `AiService::parseCommand()` → `openPage()` |
| 历史/收藏 → man 页 | 点击侧栏项 | `openPage()` |
| 返回/前进 | DTitlebar 按钮 / 快捷键 Alt+Left/Right | 维护 `QStack<pageId>` 导航栈 |

---

## 8. 翻译系统

### 8.1 混合策略数据流

```
用户打开 ls(1) 英文页
    │
    ▼
TranslationService::getTranslation("ls", 1, contentHash)
    │
    ├─ 1. 查 TranslationCache (source='preset')
    │     └─ 命中 → 直接返回译文
    │
    ├─ 2. 查预置翻译包 (manpages-zh, /usr/share/man/zh_CN/)
    │     ├─ 命中 → 存入 Cache(source='preset') → 返回
    │     └─ 未命中 ↓
    │
    ├─ 3. 查 TranslationCache (source='ai-*')
    │     ├─ 命中且未过期 (比对 man 源 mtime) → 返回缓存译文
    │     └─ 未命中或过期 ↓
    │
    └─ 4. AiService::translatePage() 实时翻译
          ├─ 流式返回 (onChunk 更新 UI 增量显示)
          └─ 完成后存入 Cache(source='ai-openai', model='gpt-4o')
```

### 8.2 预置翻译包

- **apt 依赖**：`manpages-zh`（Debian/UOS 仓库有）
- **安装位置**：`/usr/share/man/zh_CN/`，应用启动时扫描并入索引
- **降级**：若未安装，`TranslationService` 跳过预置层，直接走 AI（设置页提示安装翻译包以节省 token）

### 8.3 译文质量控制

- AI 翻译的页标记 `quality='draft'`
- 用户可点击"标记为已审核" → `quality='reviewed'`，后续不再自动重译
- man 源更新（mtime 变化）：
  - `draft` 译文自动标记过期，下次打开重译
  - `reviewed` 译文提示用户"原文已更新，是否重新翻译"

---

## 9. 扩展功能

### 9.1 收藏夹 + AI 推荐

- **收藏**：man 页右键 → 添加到收藏 → 填备注/标签
- **AI 推荐**：基于收藏夹 + 历史的命令名，AI 生成"你可能感兴趣的命令"列表
  - 频率：每周最多 1 次调用，结果缓存到 `~/.cache/deepin-iman/recommendations.json`
  - 触发：用户在收藏夹面板点击"推荐"按钮

### 9.2 内嵌终端

- **首选**：`QTermWidget`（若 deepin 仓库有 `libqtermwidget6-dev`）
- **降级**：`QProcess` + `DTextEdit` 自建简易终端（仅支持命令执行+输出显示，无完整 TTY）
- **安全**：终端默认非 root，执行前 AI 不拦截但输出异常时 AI 可解释
- **集成**：TerminalPanel 在主窗口底部可折叠，点击 man 页示例命令的"运行"按钮自动填充到终端

### 9.3 AI 速查卡

- **触发**：AI 栏"速查卡"按钮
- **AI 生成**：最常用 5 参数 + 3 个典型用法 + 2 个常见错误 → Markdown 格式
- **导出**：`DDialog` 选择 PDF（`QPrinter`）或 Markdown（`QFile`）
- **缓存**：速查卡存 `~/.cache/deepin-iman/cheatsheets/<name>-<section>.md`，man 源更新时失效

### 9.4 学习历史 + AI 周报

- **记录**：HistoryDb 记录每次访问的 page_id + 时长 + AI 交互次数
- **周报生成**：每周日（或下次启动时若超过 7 天未生成）AI 生成本周学习总结
- **存储**：`~/.local/share/deepin-iman/reports/<year>-<week>.md`
- **展示**：设置页"学习报告"入口，DListView 列出历史周报
- **开关**：`SettingsStore.weekly_report_enabled` 控制，默认开启

---

## 10. 错误处理

| 错误场景 | 处理 | 用户感知 |
|----------|------|----------|
| man 页不存在 | `ManService` 返回空，UI 显示 DLabel "无此 man 页" + AI 问答入口 | 友好提示，不崩溃 |
| mandoc 未安装 | 启动检测 `which mandoc`，缺失则弹 DDialog 引导 `apt install mandoc` | 启动拦截 |
| AI 网络/超时 | `onError` 回调 → AI 栏显示红色错误气泡 + 重试按钮 | 可重试 |
| API key 无效 | `IAiProvider::isConfigured()=false` → AI 栏提示"请配置 API key" + 设置入口 | 引导配置 |
| API 限速 (429) | 指数退避重试，超过 3 次则提示"请求过于频繁，稍后再试" | 限频提示 |
| 翻译缓存损坏 | SQLite WAL 模式 + 启动时 `PRAGMA integrity_check`，损坏则重建 | 重建进度条 |
| 终端执行失败 | QProcess 错误信号 → 终端面板显示 stderr | 内联显示 |
| man 页源更新导致缓存过期 | TranslationCache 比对 mtime → 自动重译 draft，提示 reviewed | 无感/提示 |

**通用原则**：错误不吞没（无空 catch），用 `qWarning()` / `qCritical()` 记录，UI 层用 DTK 控件友好展示。

---

## 11. 测试策略

| 层级 | 框架 | 覆盖范围 |
|------|------|----------|
| 单元测试 | Qt Test (`QTEST_MAIN`) | Data 层 (ManIndex/TranslationCache/HistoryDb/FavoriteDb 的 CRUD) |
| 集成测试 | Qt Test + 临时 SQLite | ManService 管线 (mandoc→HTML→交叉引用解析)、SearchService FTS5 查询 |
| AI Mock | `FakeAiProvider` 实现 IAiProvider | 返回固定译文/样例，不依赖网络，测试 TranslationService/ExampleService 逻辑 |
| UI 冒烟 | 手动 + `QTest::keyClicks` | 搜索→打开→翻译→问答→收藏 主流程 |
| 构建验证 | `cmake --build build` | 每次改动后必须通过 (AGENTS.md 硬要求) |

测试目录：`tests/` 与 `src/` 平级，CMakeLists.txt 中 `enable_testing()` + `add_test()`。

---

## 12. 文件结构

```
deepin-iman/
├─ CMakeLists.txt
├─ src/
│  ├─ main.cpp
│  ├─ view/                    # UI 层 (<300 行/文件)
│  │  ├─ MainWindow.h/cpp
│  │  ├─ LeftSidebar.h/cpp     # 导航+收藏+历史
│  │  ├─ ManView.h/cpp         # man HTML 渲染
│  │  ├─ TranslationView.h/cpp # 中英对照
│  │  ├─ AiChatWidget.h/cpp    # AI 对话面板
│  │  ├─ TerminalPanel.h/cpp   # 内嵌终端
│  │  └─ SettingsDialog.h/cpp
│  ├─ service/                 # Service 层
│  │  ├─ ManService.h/cpp
│  │  ├─ SearchService.h/cpp
│  │  ├─ TranslationService.h/cpp
│  │  ├─ ExampleService.h/cpp
│  │  ├─ FavoriteService.h/cpp
│  │  ├─ HistoryService.h/cpp
│  │  ├─ SettingsService.h/cpp
│  │  └─ ai/
│  │     ├─ IAiProvider.h
│  │     ├─ AiService.h/cpp
│  │     ├─ OpenAiProvider.h/cpp
│  │     ├─ ClaudeProvider.h/cpp
│  │     ├─ QwenProvider.h/cpp
│  │     └─ GlmProvider.h/cpp
│  └─ data/                    # Data 层
│     ├─ ManIndex.h/cpp
│     ├─ TranslationCache.h/cpp
│     ├─ HistoryDb.h/cpp
│     ├─ FavoriteDb.h/cpp
│     └─ SettingsStore.h/cpp
├─ data/                       # 非代码资源
│  └─ prompts/                 # AI prompt 模板
│     ├─ translate.txt
│     ├─ examples.txt
│     ├─ qa.txt
│     └─ parse.txt
├─ tests/
│  ├─ TestManIndex.cpp
│  ├─ TestTranslationCache.cpp
│  ├─ TestManService.cpp
│  ├─ TestSearchService.cpp
│  └─ FakeAiProvider.h
├─ translations/               # 应用 UI i18n
│  └─ deepin_iman_zh_CN.ts
└─ docs/superpowers/specs/    # 设计文档
   └─ 2026-08-12-deepin-iman-design.md
```

---

## 13. 依赖

| 依赖 | 用途 | 获取方式 | 必需性 |
|------|------|----------|--------|
| Qt6 Core/Widgets/Network/Sql/Concurrent | 基础框架 | `pkg-config Qt6Core Qt6Widgets...` | 必需 |
| DTK6 Widget/Core | DTK 控件 | `pkg-config DtkWidget6 DtkCore6` | 必需 |
| mandoc | man→HTML 转换 | `apt install mandoc` | 必需 (运行时) |
| SQLite3 (含 FTS5) | 索引与缓存 | Qt6 Sql 自带驱动 | 必需 |
| manpages-zh | 预置中文翻译 | `apt install manpages-zh` | 可选 (推荐) |
| QTermWidget6 | 内嵌终端 | `apt install libqtermwidget6-dev` | 可选 (有降级方案) |

**CMake 骨架**：
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Concurrent)
find_package(Dtk6 REQUIRED)
pkg_check_modules(MANDOC REQUIRED mandoc)
```

---

## 14. 编码规范

遵循 `AGENTS.md`：

- **C++ 标准**：C++17
- **头文件保护**：`#pragma once`
- **成员变量**：`m_` 前缀
- **指针/引用符号**：紧贴类型名 (`int* ptr`, `const QString& str`)
- **const 限定**：不修改的参数声明为 const 引用
- **资源管理**：智能指针或 Qt 父子对象机制，避免裸指针
- **日志**：`qDebug()` / `qWarning()` / `qCritical()`
- **国际化**：UI 字符串用 `tr()` 包裹
- **信号槽**：新式语法 (函数指针)
- **UI**：DTK 控件优先，禁用 QSS，自定义绘制用 QPainter
- **文件长度**：每个 .cpp 不超过 500 行
