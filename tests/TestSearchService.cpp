// tests/TestSearchService.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include "service/SearchService.h"
#include "data/ManIndex.h"

class TestSearchService : public QObject {
    Q_OBJECT
private slots:
    void testEmptyQuery() {
        QTemporaryDir tmp;
        ManIndex idx(tmp.path() + "/test.db");
        idx.open();
        SearchService svc(&idx);
        auto results = svc.search("", 20);
        QVERIFY(results.isEmpty());
    }

    void testRealSystemSearch() {
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_iman_search.db";
        QFile::remove(dbPath);
        ManIndex idx(dbPath);
        idx.open();
        int n = idx.scanManPages("/usr/share/man");
        if (n == 0) QSKIP("No man pages");

        SearchService svc(&idx);
        auto results = svc.search("ls", 20);
        QVERIFY(!results.isEmpty());
        bool foundByName = false;
        for (const auto& p : results) {
            if (p.name == "ls") { foundByName = true; break; }
        }
        QVERIFY(foundByName);
        QFile::remove(dbPath);
    }
};

QTEST_MAIN(TestSearchService)
#include "TestSearchService.moc"
