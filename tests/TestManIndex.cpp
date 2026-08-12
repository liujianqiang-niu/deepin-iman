// tests/TestManIndex.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "data/ManIndex.h"

class TestManIndex : public QObject {
    Q_OBJECT
private slots:
    void testSchemaCreated() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        QVERIFY(idx.tableExists("man_page"));
        QVERIFY(idx.tableExists("man_fts"));
    }

    void testScanEmptyDir() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        QCOMPARE(idx.scanManPages(tmp.path()), 0);
        QCOMPARE(idx.pageCount(), 0);
    }

    void testScanSinglePage() {
        QTemporaryDir tmp;
        QDir manDir(tmp.path() + "/man1");
        manDir.mkpath(".");
        QFile f(manDir.absoluteFilePath("ls.1.gz"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("dummy gz content");
        f.close();

        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        QCOMPARE(idx.scanManPages(tmp.path()), 1);
        QCOMPARE(idx.pageCount(), 1);
    }

    void testFindByName() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        int n = idx.scanManPages("/usr/share/man");
        if (n == 0) QSKIP("No man pages on this system");
        auto results = idx.findByName("ls");
        QVERIFY(!results.isEmpty());
        QCOMPARE(results.first().name, QString("ls"));
    }

    void testFullTextSearch() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        QVERIFY(idx.open());
        int n = idx.scanManPages("/usr/share/man");
        if (n == 0) QSKIP("No man pages on this system");
        // FTS body currently contains only command names (no full text during scan)
        // Searching for "ls" should find "ls" command
        auto results = idx.fullTextSearch("ls", 10);
        QVERIFY(!results.isEmpty());
        bool foundLs = false;
        for (const auto& p : results) {
            if (p.name == "ls") { foundLs = true; break; }
        }
        QVERIFY(foundLs);
    }
};

QTEST_MAIN(TestManIndex)
#include "TestManIndex.moc"
