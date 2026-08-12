// src/main.cpp
#include <QGuiApplication>
#include <DApplication>
#include <DDialog>
#include <DProgressBar>
#include <DLabel>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QVBoxLayout>
#include <QThread>
#include <QTranslator>
#include <QLocale>
#include <QtConcurrent/QtConcurrent>
#include <cstdlib>

DWIDGET_USE_NAMESPACE

#include "view/MainWindow.h"
#include "data/ManIndex.h"
#include "data/SettingsStore.h"
#include "data/TranslationCache.h"
#include "data/FavoriteDb.h"
#include "data/HistoryDb.h"
#include "service/ManService.h"
#include "service/SearchService.h"
#include "service/ai/AiService.h"
#include "service/TranslationService.h"
#include "service/FavoriteService.h"
#include "service/HistoryService.h"

int main(int argc, char* argv[]) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }

    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-iman");
    app.setApplicationVersion("0.1.0");
    app.setProductIcon(QIcon::fromTheme("deepin-iman", QIcon(":/assets/icons/deepin-iman.svg")));
    app.setProductName("deepin man 手册");
    app.setApplicationDescription("AI 驱动的 man 手册查看器");
    app.setWindowIcon(QIcon::fromTheme("deepin-iman", QIcon(":/assets/icons/deepin-iman.svg")));

    QTranslator dtkTranslator;
    if (dtkTranslator.load(QLocale(), "dtkwidget", "_", "/usr/share/dtk6/DWidget/translations")) {
        app.installTranslator(&dtkTranslator);
    }

    QTranslator translator;
    QString locale = QLocale::system().name();
    QStringList tsSearchPaths = {
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/translations",
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/translations",
    };
    QDir appDir(QCoreApplication::applicationDirPath());
    tsSearchPaths << appDir.absoluteFilePath("../share/deepin-iman/translations");
    tsSearchPaths << appDir.absoluteFilePath("translations");
    tsSearchPaths << ":/translations";
    for (const auto& dir : tsSearchPaths) {
        if (translator.load("deepin_iman_" + locale, dir)) {
            app.installTranslator(&translator);
            break;
        }
    }

    QProcess whichProc;
    whichProc.start("which", {"mandoc"});
    whichProc.waitForFinished();
    if (whichProc.exitCode() != 0) {
        DDialog dlg;
        dlg.setWindowTitle("mandoc 未安装");
        dlg.setMessage("deepin-iman 需要 mandoc 来渲染 man 手册。\n"
                        "请通过以下命令安装：\n\n  sudo apt install mandoc\n\n"
                        "然后重新启动 deepin-iman。");
        dlg.addButton("退出", false, DDialog::ButtonRecommend);
        dlg.exec();
        return 1;
    }

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    ManIndex index(dataDir + "/manindex.db");
    if (!index.open()) {
        qCritical() << "Cannot open index database";
        return 1;
    }

    if (index.pageCount() == 0) {
        DDialog progressDlg;
        progressDlg.setWindowTitle("正在索引 man 手册...");
        auto* progressBar = new DProgressBar;
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        auto* label = new DLabel("正在扫描 man 手册，请稍候...");
        auto* layout = new QVBoxLayout;
        layout->addWidget(label);
        layout->addWidget(progressBar);
        auto* content = new QWidget;
        content->setLayout(layout);
        progressDlg.addContent(content);
        progressDlg.setFixedWidth(400);
        progressDlg.show();
        app.processEvents();

        QObject::connect(&index, &ManIndex::scanProgress, [&](int cur, int total) {
            progressBar->setMaximum(total);
            progressBar->setValue(cur);
            app.processEvents();
        });
        QObject::connect(&index, &ManIndex::scanFinished, &progressDlg, &DDialog::accept);

        auto future = QtConcurrent::run([&index]() { index.scanManPages("/usr/share/man"); });
        Q_UNUSED(future)

        while (progressDlg.isVisible()) {
            app.processEvents();
            QThread::msleep(50);
        }
    }

    AiService aiService;
    aiService.initializeProviders();

    TranslationCache trCache(dataDir + "/translation.db");
    trCache.open();
    TranslationService trService(&trCache, &aiService);

    FavoriteDb favDb(dataDir + "/favorite.db");
    favDb.open();
    FavoriteService favService(&favDb);

    HistoryDb histDb(dataDir + "/history.db");
    histDb.open();
    HistoryService histService(&histDb);

    ManService manService;
    SearchService searchService(&index);
    MainWindow w(&index, &searchService, &manService, &aiService, &trService,
                 &favService, &histService);
    w.show();

    return app.exec();
}
