#pragma once

#include <QObject>
#include "qmqttprotocol.h"

class QByteArray;
class QString;
class MQTTPacket;
class QTMQTT_AUTOTEST_EXPORT QMqttPacketParser : public QObject
{
    Q_OBJECT

public:
    QMqttPacketParser();

    void parse(const QByteArray &packet);

Q_SIGNALS:
    void error(QMqttProtocol::Error error, const QString &errorMessage);
    void connack(QMqttProtocol::Error error, bool sessionPresent);
    void publish(QMqttProtocol::QoS qos, uint16_t packetIdentifier,
                 const QString &topicName, const QByteArray &message);
    void puback(uint16_t packetIdentifier);
    void pubrel(uint16_t packetIdentifier);
    void suback(uint16_t packetIdentifier, QVector<QMqttProtocol::QoS> qos);
    void unsuback(uint16_t packetIdentifier);
    void pong();

private:
    void parseCONNACK(const MQTTPacket &packet);
    void parseSUBACK(const MQTTPacket &packet);
    void parsePUBLISH(const MQTTPacket &packet);
    void parsePUBREL(const MQTTPacket &packet);
    void parsePUBACK(const MQTTPacket &packet);
    void parseUNSUBACK(const MQTTPacket &packet);
};
