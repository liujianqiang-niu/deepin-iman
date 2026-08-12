// src/main.cpp
#include <DApplication>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QtConcurrent/QtConcurrent>

DWIDGET_USE_NAMESPACE

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

    // Check man command availability
    QProcess whichProc;
    whichProc.start("which", {"man"});
    whichProc.waitForFinished();
    if (whichProc.exitCode() != 0) {
        qCritical() << "man command not found. Please install man-db.";
        return 1;
    }

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    ManIndex index(dataDir + "/manindex.db");
    if (!index.open()) {
        qCritical() << "Cannot open index database";
        return 1;
    }

    // First-time scan in background thread
    if (index.pageCount() == 0) {
        QtConcurrent::run([&index]() {
            index.scanManPages("/usr/share/man");
        });
    }

    ManService manService;
    SearchService searchService(&index);
    MainWindow w(&index, &searchService, &manService);
    w.show();

    return app.exec();
}
