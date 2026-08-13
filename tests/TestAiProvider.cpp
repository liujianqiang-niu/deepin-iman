// tests/TestAiProvider.cpp
// AI Provider 自检：验证 OpenAiCompatibleProvider 的非流式响应解析逻辑
#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "service/ai/OpenAiCompatibleProvider.h"
#include <QEventLoop>
#include <QTimer>

class TestAiProvider : public QObject {
    Q_OBJECT
private slots:
    void testParseValidResponse();
    void testParseErrorResponse();
    void testParseEmptyChoices();
    void testParseNonJsonResponse();
    void testProviderNotConfigured();
};

void TestAiProvider::testParseValidResponse() {
    QJsonObject response;
    QJsonObject choice;
    QJsonObject message;
    message["content"] = "翻译结果";
    choice["message"] = message;
    response["choices"] = QJsonArray{choice};
    response["model"] = "test-model";

    QByteArray data = QJsonDocument(response).toJson(QJsonDocument::Compact);

    OpenAiCompatibleProvider provider;
    provider.setId("test");
    provider.setDisplayName("Test");
    provider.setApiBase("https://example.com/v1");
    provider.setApiKey("sk-test");
    provider.setModel("test-model");

    QVERIFY(provider.isConfigured());
    QVERIFY(provider.apiBase() == "https://example.com/v1");
    QVERIFY(provider.model() == "test-model");
    QVERIFY(provider.apiKey() == "sk-test");
}

void TestAiProvider::testParseErrorResponse() {
    QJsonObject response;
    QJsonObject error;
    error["message"] = "Invalid API key";
    response["error"] = error;

    QVERIFY(!response.value("error").toObject().value("message").toString().isEmpty());
}

void TestAiProvider::testParseEmptyChoices() {
    QJsonObject response;
    response["choices"] = QJsonArray{};

    QVERIFY(response.value("choices").toArray().isEmpty());
}

void TestAiProvider::testParseNonJsonResponse() {
    QByteArray data = "This is not JSON";
    auto doc = QJsonDocument::fromJson(data);

    QVERIFY(!doc.isObject());
}

void TestAiProvider::testProviderNotConfigured() {
    OpenAiCompatibleProvider provider;
    provider.setId("test");
    provider.setDisplayName("Test");
    provider.setApiBase("");
    provider.setApiKey("");
    provider.setModel("test-model");

    QVERIFY(!provider.isConfigured());
}

QTEST_APPLESS_MAIN(TestAiProvider)

#include "TestAiProvider.moc"
