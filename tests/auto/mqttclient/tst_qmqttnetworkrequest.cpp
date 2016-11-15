#include <QtTest/QtTest>
#include <QtTest/qtestcase.h>
#include <QUrl>

#include "qmqttnetworkrequest.h"

class tst_QMqttNetworkRequest: public QObject
{
    Q_OBJECT

public:
    tst_QMqttNetworkRequest();

private Q_SLOTS:
//    void initTestCase();
//    void cleanupTestCase();
//    void init();
//    void cleanup();

    void defaultConstructor();
    void constructor();
    void customHeaders();
};

tst_QMqttNetworkRequest::tst_QMqttNetworkRequest() :
    QObject()
{}

void tst_QMqttNetworkRequest::defaultConstructor()
{
    QMqttNetworkRequest request;

    QVERIFY(request.url().isEmpty());
}

void tst_QMqttNetworkRequest::constructor()
{
    QUrl url(QStringLiteral("http://test.mqtt.org"));
    QMqttNetworkRequest request(url);

    QCOMPARE(request.url(), url);
}

void tst_QMqttNetworkRequest::customHeaders()
{
    QMqttNetworkRequest request;

    request.setRawHeader(QByteArrayLiteral("X-My-Custom-Header"), QByteArrayLiteral("SomeValue"));

    QCOMPARE(request.rawHeader("X-My-Custom-Header"), QByteArrayLiteral("SomeValue"));
}

QTEST_GUILESS_MAIN(tst_QMqttNetworkRequest)

#include "tst_qmqttnetworkrequest.moc"
