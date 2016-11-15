#include <QtTest/QtTest>
#include <QtTest/qtestcase.h>
#include <QUrl>

#include "qmqttprotocol.h"

class tst_QMqttProtocol: public QObject
{
    Q_OBJECT

public:
    tst_QMqttProtocol();

private Q_SLOTS:
//    void initTestCase();
//    void cleanupTestCase();
//    void init();
//    void cleanup();
    void qos();
    void standardErrors();
};

tst_QMqttProtocol::tst_QMqttProtocol() :
    QObject()
{}

void tst_QMqttProtocol::qos()
{
    QCOMPARE(int(QMqttProtocol::QoS::AT_MOST_ONCE), 0);
    QCOMPARE(int(QMqttProtocol::QoS::AT_LEAST_ONCE), 1);
    QCOMPARE(int(QMqttProtocol::QoS::EXACTLY_ONCE), 2);
    QCOMPARE(int(QMqttProtocol::QoS::INVALID), 3);
}

void tst_QMqttProtocol::standardErrors()
{
    QCOMPARE(int(QMqttProtocol::Error::CONNECTION_ACCEPTED), 0);
    QCOMPARE(int(QMqttProtocol::Error::CONNECTION_REFUSED_UNACCEPTABLE_PROTOCOL), 1);
    QCOMPARE(int(QMqttProtocol::Error::CONNECTION_REFUSED_IDENTIFIER_REJECTED), 2);
    QCOMPARE(int(QMqttProtocol::Error::CONNECTION_REFUSED_SERVER_UNAVAILABLE), 3);
    QCOMPARE(int(QMqttProtocol::Error::CONNECTION_REFUSED_BAD_USERNAME_OR_PASSWORD), 4);
    QCOMPARE(int(QMqttProtocol::Error::CONNECTION_REFUSED_NOT_AUTHORIZED), 5);
}

QTEST_GUILESS_MAIN(tst_QMqttProtocol)

#include "tst_qmqttprotocol.moc"
