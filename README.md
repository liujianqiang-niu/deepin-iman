# deepin-iman

AI-powered man page viewer for deepin v25.

## Features

- **Full-text search** across all man pages (SQLite FTS5) with case-sensitive and whole-word match toggles
- **Cross-reference navigation** (SEE ALSO links)
- **mandoc HTML rendering** with structure-preserving output
- **AI-powered translation** — translate English man pages to Chinese, with cache and preset fallback
- **Inline split view** — translation and examples appear beside the original text, each panel can detach to standalone window
- **AI-generated usage examples** — section-aware: C code for library functions (section 2/3), shell commands for user commands (section 1/8)
- **AI Q&A** — ask questions about the current man page, responses rendered as Markdown
- **Favorites** — bookmark man pages, batch select and delete
- **Browse history** — auto-tracks visited pages, batch select and delete
- **Data management** — clear translation cache, history, favorites, or index database
- **Single instance** — repeat launch activates existing window
- **Auto index update** — detects new/removed man pages on startup, plus manual refresh

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Dependencies

- Qt6 (Core/Widgets/Network/Sql/Concurrent)
- DTK6 (Widget/Core/GUI)
- mandoc (man page → HTML)
- SQLite3 with FTS5

### Man page packages (auto-installed)

| Package | Scope |
|---------|-------|
| `manpages` | Standard system command man pages (required) |
| `manpages-dev` | C library / development man pages (required) |
| `manpages-posix` | POSIX standard man pages (recommended) |
| `manpages-posix-dev` | POSIX development man pages (recommended) |
| `manpages-zh` or `manpages-cn` | Chinese man pages (recommended) |

## Install

```bash
sudo apt install deepin-iman
```

## License

LGPL-3.0+

## Author

liujianqiang <liujianqiang@uniontech.com>

## Repository

https://github.com/liujianqiang-niu/deepin-iman
