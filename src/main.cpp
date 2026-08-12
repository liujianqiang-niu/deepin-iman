// src/main.cpp
#include <DApplication>
#include <QIcon>

DWIDGET_USE_NAMESPACE

int main(int argc, char* argv[]) {
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-iman");
    app.setApplicationVersion("0.1.0");
    app.setProductIcon(QIcon::fromTheme("help-browser"));
    app.setProductName("deepin iman");
    app.setApplicationDescription(QObject::tr("AI-powered man page viewer"));

    // MainWindow will be added in Task 9
    return app.exec();
}
