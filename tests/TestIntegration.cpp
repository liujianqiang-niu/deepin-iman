// tests/TestIntegration.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFile>
#include "data/ManIndex.h"
#include "service/ManService.h"
#include "service/SearchService.h"

class TestIntegration : public QObject {
    Q_OBJECT
private slots:
    void testFullPipeline() {
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_iman_integration.db";
        QFile::remove(dbPath);
        ManIndex idx(dbPath);
        QVERIFY(idx.open());

        int n = idx.scanManPages("/usr/share/man");
        if (n == 0) QSKIP("No man pages");

        SearchService search(&idx);
        auto results = search.search("ls", 10);
        QVERIFY(!results.isEmpty());

        ManService manSvc;
        QString html = manSvc.renderPage(results.first().sourcePath);
        QVERIFY(!html.isEmpty());

        auto refs = manSvc.parseCrossReferences(html);
        QVERIFY(!refs.isEmpty() || !html.contains("SEE ALSO"));

        if (!refs.isEmpty()) {
            auto refPages = idx.findByName(refs.first().name);
            QVERIFY(!refPages.isEmpty());
            QString html2 = manSvc.renderPage(refPages.first().sourcePath);
            QVERIFY(!html2.isEmpty());
        }
        QFile::remove(dbPath);
    }
};

QTEST_MAIN(TestIntegration)
#include "TestIntegration.moc"
