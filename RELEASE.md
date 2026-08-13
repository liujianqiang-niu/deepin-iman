# deepin-iman 发布说明

## 应用简介

deepin-iman 是一款面向 deepin/UOS 系统的 AI 驱动 man 手册查看器，将传统的 man 手册浏览体验与 AI 能力深度融合，帮助用户更高效地学习和使用 Linux 命令。

项目地址：https://github.com/liujianqiang-niu/deepin-iman

## 核心功能

### man 手册浏览

- 基于 mandoc 的 HTML 渲染，保留手册原始结构
- 启动时自动扫描并索引系统中所有 man 手册，支持增量更新
- 支持 SEE ALSO 交叉引用链接跳转
- 前进/后退导航，快捷键 `Alt+Left` / `Alt+Right`

### 全文搜索

- 基于 SQLite FTS5 的全文搜索，200ms 防抖
- 支持大小写敏感和全字匹配两种切换开关
- 搜索结果实时显示在左侧栏

### AI 翻译

- 将英文 man 手册翻译为中文，支持本地缓存复用
- 翻译优先级：本地缓存 > AI 翻译 > 系统预设中文包降级
- 长手册自动按章节分段翻译，保证翻译完整性
- 翻译结果在独立面板中与原文并排显示

### AI 使用样例生成

- 根据手册所属 section 自动判断类型：
  - 用户命令/管理命令（section 1/8）：生成命令行样例
  - 系统调用/库函数（section 2/3）：生成 C 语言代码样例，含头文件和编译说明
- 基于手册真实正文生成，不臆造
- 样例结果本地缓存，重复生成直接读取

### AI 问答

- 针对当前查看的 man 手册提问
- AI 回答以 Markdown 格式渲染，代码块、列表、标题等格式清晰可读
- 深色/浅色主题自适应

### 编辑器风格分栏面板

- 原文面板和翻译/样例面板各自带标题栏和关闭按钮
- 面板可弹出为独立窗口（点击弹出按钮或双击标题栏），关闭后自动回嵌
- 解决分栏宽度有限时长行内容阅读不便的问题

### 收藏与历史

- 收藏 man 手册，支持批量勾选删除
- 自动记录浏览历史，支持批量勾选删除
- 一键全选/取消全选

### 数据管理

- 统一的数据管理对话框，可清空：
  - 翻译缓存
  - 浏览历史
  - 收藏列表
  - 索引数据库（下次启动自动重建）
- 清理过程带进度条提示

### 其他特性

- 单实例模式：重复启动自动激活已有窗口
- AI 供应商管理：支持 OpenAI 兼容的任意厂商（OpenAI、智谱 GLM、通义千问、DeepSeek 等）
- 主题适配：深色/浅色主题全适配

## 系统要求

- deepin v25 / UOS
- Qt6 (Core/Widgets/Network/Sql/Concurrent)
- DTK6 (Widget/Core/GUI)
- mandoc
- SQLite3 with FTS5

## 安装

```bash
sudo apt install deepin-iman
```

## 构建

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## 依赖包

| 包 | 说明 |
|------|------|
| `manpages` | 标准系统命令 man 手册（必须） |
| `manpages-dev` | C 库函数/开发 man 手册（必须） |
| `manpages-posix` | POSIX 标准 man 手册（推荐） |
| `manpages-posix-dev` | POSIX 开发 man 手册（推荐） |
| `manpages-zh` 或 `manpages-cn` | 中文 man 手册（推荐） |

## 技术栈

- **语言**：C++17
- **UI 框架**：Qt6 + DTK6
- **渲染引擎**：mandoc
- **数据库**：SQLite (FTS5)
- **AI 接口**：OpenAI-compatible API
- **构建系统**：CMake

## 许可证

LGPL-3.0+

## 作者

liujianqiang <liujianqiang@uniontech.com>

## 仓库地址

https://github.com/liujianqiang-niu/deepin-iman
