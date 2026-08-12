# deepin-iman

AI-powered man page viewer for deepin v25.

## Features

- **Full-text search** across all man pages (SQLite FTS5)
- **Cross-reference navigation** (SEE ALSO links)
- **mandoc HTML rendering** with structure-preserving output
- **AI-powered translation** (multi-provider: OpenAI/Claude/Qwen/GLM)
- **Chinese + English side-by-side view**
- **AI-generated usage examples**
- **Embedded terminal** (QTermWidget)
- **Cheat sheet generation** (PDF/Markdown export)
- **Learning history** with AI weekly reports

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

## Install

```bash
sudo apt install deepin-iman
```

## License

LGPL-3.0+

## Author

liujianqiang <liujianqiang@uniontech.com>
