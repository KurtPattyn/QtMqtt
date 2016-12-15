#include <QtTest/QtTest>
#include <QtTest/qtestcase.h>
#include <QUrl>

#include "qmqttcontrolpacket_p.h"

class tst_QMqttControlPacket: public QObject
{
    Q_OBJECT

public:
    tst_QMqttControlPacket();

private Q_SLOTS:
//    void initTestCase();
//    void cleanupTestCase();
//    void init();
//    void cleanup();
    void packetTypes();
};

tst_QMqttControlPacket::tst_QMqttControlPacket() :
    QObject()
{}

void tst_QMqttControlPacket::packetTypes()
{
    QCOMPARE(int(QMqttControlPacket::PacketType::RESERVED_0), 0);
    QCOMPARE(int(QMqttControlPacket::PacketType::CONNECT), 1);
    QCOMPARE(int(QMqttControlPacket::PacketType::CONNACK), 2);
    QCOMPARE(int(QMqttControlPacket::PacketType::PUBLISH), 3);
    QCOMPARE(int(QMqttControlPacket::PacketType::PUBACK), 4);
    QCOMPARE(int(QMqttControlPacket::PacketType::PUBREC), 5);
    QCOMPARE(int(QMqttControlPacket::PacketType::PUBREL), 6);
    QCOMPARE(int(QMqttControlPacket::PacketType::PUBCOMP), 7);
    QCOMPARE(int(QMqttControlPacket::PacketType::SUBSCRIBE), 8);
    QCOMPARE(int(QMqttControlPacket::PacketType::SUBACK), 9);
    QCOMPARE(int(QMqttControlPacket::PacketType::UNSUBSCRIBE), 10);
    QCOMPARE(int(QMqttControlPacket::PacketType::UNSUBACK), 11);
    QCOMPARE(int(QMqttControlPacket::PacketType::PINGREQ), 12);
    QCOMPARE(int(QMqttControlPacket::PacketType::PINGRESP), 13);
    QCOMPARE(int(QMqttControlPacket::PacketType::DISCONNECT), 14);
    QCOMPARE(int(QMqttControlPacket::PacketType::RESERVED_15), 15);
}

QTEST_GUILESS_MAIN(tst_QMqttControlPacket)

#include "tst_qmqttcontrolpacket.moc"

