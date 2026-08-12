// src/main.cpp
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

    QProcess whichProc;
    whichProc.start("which", {"man"});
    whichProc.waitForFinished();
    if (whichProc.exitCode() != 0) {
        DDialog dlg;
        dlg.setWindowTitle(QObject::tr("man not installed"));
        dlg.setMessage(QObject::tr(
            "deepin-iman requires man-db to render man pages.\n"
            "Please install it with:\n\n  sudo apt install man-db\n\n"
            "Then restart deepin-iman."));
        dlg.addButton(QObject::tr("Exit"), false, DDialog::ButtonRecommend);
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

    MainWindow* w = nullptr;
    if (index.pageCount() == 0) {
        DDialog progressDlg;
        progressDlg.setWindowTitle(QObject::tr("Indexing man pages..."));
        auto* progressBar = new DProgressBar;
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        auto* label = new DLabel(QObject::tr("Scanning man pages, please wait..."));
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

    ManService manService;
    SearchService searchService(&index);
    w = new MainWindow(&index, &searchService, &manService);
    w->show();

    return app.exec();
}
