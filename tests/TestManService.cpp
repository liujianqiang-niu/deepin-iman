// tests/TestManService.cpp
#include <QtTest/QtTest>
#include <QFile>
#include <QFileInfo>
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

    void testAnsiToHtml() {
        ManService svc;
        QString input = "\033[1mBOLD\033[0m and \033[4mUNDERLINE\033[24m text";
        QString html = svc.ansiToHtml(input);
        QVERIFY(html.contains("<b>BOLD</b>"));
        QVERIFY(html.contains("<u>UNDERLINE</u>"));
    }

    void testRenderRealSystemPage() {
        ManService svc;
        QFile f("/usr/share/man/man1/ls.1.gz");
        if (!f.exists()) QSKIP("No ls.1.gz on system");
        QString html = svc.renderPage("/usr/share/man/man1/ls.1.gz");
        QVERIFY(!html.isEmpty());
        QVERIFY(html.contains("ls") || html.contains("LS"));
    }
};

QTEST_MAIN(TestManService)
#include "TestManService.moc"
