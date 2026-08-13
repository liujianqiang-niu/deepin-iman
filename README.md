# deepin-iman

AI-powered man page viewer for deepin v25.

## Features

- **Full-text search** across all man pages (SQLite FTS5) with case-sensitive and whole-word match toggles
- **Cross-reference navigation** (SEE ALSO links)
- **mandoc HTML rendering** with structure-preserving output
- **AI-powered translation** (user-configurable OpenAI-compatible providers, 7 target languages)
- **Inline split view** — translation and examples appear beside the original text with synchronized scrolling
- **AI-generated usage examples** with annotations and expected output
- **AI Q&A** — ask questions about the current man page and get context-aware answers
- **Command parsing** — paste a command line and jump to the corresponding man page
- **Favorites** — bookmark man pages and access them from the menu
- **Browse history** — auto-tracks visited pages (keeps latest 100)
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
| `glibc-doc` | GNU C library documentation (recommended) |

## Install

```bash
sudo apt install deepin-iman
```

## License

LGPL-3.0+

## Author

liujianqiang <liujianqiang@uniontech.com>
