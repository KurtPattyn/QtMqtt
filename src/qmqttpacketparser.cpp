#include "qmqttprotocol.h"
#include "qmqttcontrolpacket_p.h"
#include "qmqttpacketparser_p.h"

#include <QBuffer>
#include <QtEndian>
#include <QByteArray>
#include <QDebug>
#include <QMetaEnum>

class MQTTPacket
{
public:
    MQTTPacket()
    {
        clear();
    }

    QMqttProtocol::Error error() const { return m_error; }
    QString errorString() const { return m_errorString; }
    bool isValid() const { return m_isValid; }
    QMqttControlPacket::PacketType packetType() const { return m_packetType; }
    bool retain() const { return m_retain; }
    bool dup() const { return m_dup; }
    QMqttProtocol::QoS qos() const { return m_qos; }
    uint8_t flags() const { return m_flags; }
    int32_t remainingLength() const { return m_remainingLength; }
    QByteArray payload() const { return m_payload; }

    static MQTTPacket readPacket(const QByteArray &data);

private:
    QMqttProtocol::Error m_error;
    QString m_errorString;
    bool m_isValid;
    QMqttControlPacket::PacketType m_packetType;
    bool m_retain;
    bool m_dup;
    QMqttProtocol::QoS m_qos;
    uint8_t m_flags;
    int32_t m_remainingLength;
    QByteArray m_payload;

    void clear() {
        m_error = QMqttProtocol::Error::OK;
        m_errorString.clear();
        m_isValid = false;
        m_packetType = QMqttControlPacket::PacketType::RESERVED_0;
        m_retain = false;
        m_dup = false;
        m_qos = QMqttProtocol::QoS::AT_MOST_ONCE;
        m_flags = 0;
        m_remainingLength = 0;
        m_payload.clear();
    }

    void setError(QMqttProtocol::Error error, const QString &errorString) {
        clear();
        m_error = error;
        m_errorString = errorString;
        m_isValid = false;
    }

    static bool parseHeader(QBuffer &buffer, MQTTPacket &packet);
    static bool parseRemainingLength(QBuffer &buffer, MQTTPacket &packet);
};
MQTTPacket MQTTPacket::readPacket(const QByteArray &data)
{
    MQTTPacket packet;

    QByteArray frameData(data);
    QBuffer buffer(&frameData);
    //TODO: check on open error
    buffer.open(QIODevice::ReadOnly);

    if (parseHeader(buffer, packet) && parseRemainingLength(buffer, packet)) {
        if (buffer.bytesAvailable() >= packet.remainingLength()) {
            packet.m_payload = buffer.read(packet.remainingLength());
            //TODO: check if packet.m_payload.size() == packet.remainingLength()
            packet.m_isValid = true;
        } else {
            packet.setError(QMqttProtocol::Error::INVALID_PACKET,
                            QStringLiteral("Payload of packet is too small"));
        }
    }

    buffer.close();

    return packet;
}

bool MQTTPacket::parseHeader(QBuffer &buffer, MQTTPacket &packet)
{
    if (buffer.bytesAvailable() < 1) {
        packet.setError(QMqttProtocol::Error::INVALID_PACKET,
                        QStringLiteral("Packet is empty"));
        return false;
    }

    uint8_t header = 0;
    //TODO: check on read error
    buffer.read((char *)&header, sizeof(header));

    packet.m_packetType = QMqttControlPacket::PacketType(header >> 4);
    if ((packet.m_packetType == QMqttControlPacket::PacketType::RESERVED_0)
            || (packet.m_packetType >= QMqttControlPacket::PacketType::RESERVED_15)) {
        packet.setError(QMqttProtocol::Error::INVALID_PACKET,
                        QStringLiteral("Invalid command detected %1").arg(uint8_t(packet.m_packetType)));
        return false;
    }
    packet.m_flags = header & 0x0F;
    packet.m_retain = bool(packet.m_flags & 0x01);
    const uint8_t qos = (packet.m_flags & 0x06) >> 1;
    packet.m_dup = bool((packet.m_flags & 0x08) >> 3);

    if (qos > 2) { //possible values are 0, 1, 2
        packet.setError(QMqttProtocol::Error::INVALID_PACKET,
                        QStringLiteral("Invalid qos value detected %1").arg(qos));
        return false;
    }
    packet.m_qos = QMqttProtocol::QoS(qos);

    return true;
}

bool MQTTPacket::parseRemainingLength(QBuffer &buffer, MQTTPacket &packet)
{
    uint8_t current = 0;
    int count = 0;
    int32_t length = 0;
    int32_t multiplier = 1;

    while (count < 5) {
        if (buffer.bytesAvailable() < 1) {
            packet.setError(QMqttProtocol::Error::INVALID_PACKET,
                            QStringLiteral("Packet does not contain complete length field"));
            return false;
        }
        //TODO: check for read errors
        buffer.read((char *)&current, sizeof(uint8_t));
        length += multiplier * (current & 0x7F);
        multiplier *= 0x80;

        if ((current & 0x80) == 0) break;
    }

    packet.m_remainingLength = length;

    return true;
}

template<typename T>
inline typename QtPrivate::QEnableIf<QtPrivate::IsQEnumHelper<T>::Value, QString>::Type toString(T e)
{
    QMetaEnum me = QMetaEnum::fromType<T>();
    return QString::fromLatin1(me.valueToKey(int(e))); // int cast is necessary to support enum classes
}

template <typename T> // Fallback
inline typename QtPrivate::QEnableIf<!QtPrivate::IsQEnumHelper<T>::Value, QString>::Type toString(const T &)
{
    return Q_NULLPTR;
}

QMqttPacketParser::QMqttPacketParser()
{
}

void QMqttPacketParser::parse(const QByteArray &packet)
{
    const MQTTPacket mqttPacket = MQTTPacket::readPacket(packet);
    if (Q_UNLIKELY(!mqttPacket.isValid())) {
        const QString errorMessage = QStringLiteral("Error reading packet: %1 (%2).")
                .arg(toString(mqttPacket.error()))
                .arg(mqttPacket.errorString());
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
//    qDebug() << "Command:" << mqttPacket.packetType()
//             << "retain:" << mqttPacket.retain()
//             << "qos:" << mqttPacket.qos()
//             << "dup:" << mqttPacket.dup()
//             << "payload:" << mqttPacket.payload();

    switch (mqttPacket.packetType()) {
        case QMqttControlPacket::PacketType::CONNACK: {
            parseCONNACK(mqttPacket);
            break;
        }

        case QMqttControlPacket::PacketType::SUBACK: {
            parseSUBACK(mqttPacket);
            break;
        }

        case QMqttControlPacket::PacketType::PUBLISH: {
            parsePUBLISH(mqttPacket);
            break;
        }

        case QMqttControlPacket::PacketType::PUBACK: {
            parsePUBACK(mqttPacket);
            break;
        }

        case QMqttControlPacket::PacketType::PUBREL: {
            parsePUBREL(mqttPacket);
            break;
        }
        case QMqttControlPacket::PacketType::UNSUBACK: {
            parseUNSUBACK(mqttPacket);
            break;
        }

        case QMqttControlPacket::PacketType::PINGRESP: {
            Q_EMIT pong();
            break;
        }

        case QMqttControlPacket::PacketType::PUBREC:
        case QMqttControlPacket::PacketType::PUBCOMP: {
            qWarning() << "PUBREC and PUBCOMP is not supported currently.";
            break;
        }

        case QMqttControlPacket::PacketType::RESERVED_0:
        case QMqttControlPacket::PacketType::RESERVED_15:
        case QMqttControlPacket::PacketType::CONNECT:
        case QMqttControlPacket::PacketType::DISCONNECT:
        case QMqttControlPacket::PacketType::SUBSCRIBE:
        case QMqttControlPacket::PacketType::UNSUBSCRIBE:
        case QMqttControlPacket::PacketType::PINGREQ: {
                break;
        }

        default: {
            qWarning() << "Unhandled MQTT Packet type:" << mqttPacket.packetType();
            break;
        }
    }
}

void QMqttPacketParser::parseCONNACK(const MQTTPacket &packet)
{
    if (packet.remainingLength() != 2) {
        const QString errorMessage = QStringLiteral("Invalid CONNACK packet received");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
    const QByteArray payload = packet.payload();
    const uint8_t connectAcknowledgeFlags = payload[0];
    const uint8_t connectReturnCode = payload[1];

    if ((connectAcknowledgeFlags & 0xFE) != 0) {
        const QString errorMessage =
                QStringLiteral("Invalid acknowledge flags detected: %1. Upper 7 bits must be zero.")
                .arg(connectAcknowledgeFlags);
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
    const bool sessionPresent = bool(connectAcknowledgeFlags & 0x01);

    if (connectReturnCode > 5) {
        const QString errorMessage =
                QStringLiteral("Invalid return code detected: %1.").arg(connectReturnCode);
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }

    Q_EMIT connack(QMqttProtocol::Error(connectReturnCode), sessionPresent);
}

void QMqttPacketParser::parseSUBACK(const MQTTPacket &packet)
{
    if (packet.remainingLength() < 2) {
        const QString errorMessage = QStringLiteral("Invalid SUBACK packet received");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
    const QByteArray payload = packet.payload();
    const uint16_t packetIdentifier = (uint16_t(payload[0]) * 256) + uint8_t(payload[1]); //big endian
    QVector<QMqttProtocol::QoS> qos;

    int remainingSize = payload.size() - 2;
    while (remainingSize > 0) {
        const uint8_t returnCode = payload[--remainingSize + 2];
        if (returnCode == 0x80) {
            qos.prepend(QMqttProtocol::QoS::INVALID);
        } else {
            if (returnCode > 2) {
                const QString errorMessage =
                        QStringLiteral("Invalid return code detected in SUBACK packet: %1")
                        .arg(returnCode);
                qWarning() << errorMessage;
                Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
                return;
            }
            qos.prepend(QMqttProtocol::QoS(returnCode));
        }
    }
    Q_EMIT suback(packetIdentifier, qos);
}

void QMqttPacketParser::parsePUBLISH(const MQTTPacket &packet)
{
    if (packet.remainingLength() <  2) {
        const QString errorMessage = QStringLiteral("Invalid PUBLISH packet received");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
    QByteArray payload = packet.payload();
    QBuffer buffer(&payload);

    if (!buffer.open(QIODevice::ReadOnly)) {
        const QString errorMessage
                = QStringLiteral("Error opening read buffer: %1")
                .arg(buffer.errorString());
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::PARSE_ERROR, errorMessage);
        return;
    }

    int variableHeaderLength = 0;

    uint16_t topicNameLength;
    const qint64 bytesRead =
            buffer.read(static_cast<char *>(static_cast<void *>(&topicNameLength)),
                        sizeof(uint16_t));
    if (bytesRead != sizeof(uint16_t)) {
        const QString errorMessage
                = QStringLiteral("Error reading from packet buffer. Bytes read = %1 <> 2")
                .arg(bytesRead);
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::PARSE_ERROR, errorMessage);
        return;
    }
    topicNameLength = qFromBigEndian(topicNameLength);
    variableHeaderLength += 2;

    if (buffer.bytesAvailable() < topicNameLength) {
        const QString errorMessage
                = QStringLiteral("Invalid PUBLISH packet received. Invalid topic name.");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
    variableHeaderLength += topicNameLength;
    const QString topicName = QString::fromUtf8(buffer.read(topicNameLength));
    if (topicName.length() != topicNameLength) {
        const QString errorMessage
                = QStringLiteral("Error reading from packet buffer. Bytes read = %1 <> %2")
                .arg(bytesRead).arg(topicNameLength);
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::PARSE_ERROR, errorMessage);
        return;
    }

    uint16_t packetIdentifier = 0;

    if (packet.qos() != QMqttProtocol::QoS::AT_MOST_ONCE) {
        if (packet.remainingLength() <  2) {
            const QString errorMessage
                    = QStringLiteral("Invalid PUBLISH packet received. No packet identifier.");
            qWarning() << errorMessage;
            Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
            return;
        }

        const qint64 bytesRead =
                buffer.read(static_cast<char *>(static_cast<void *>(&packetIdentifier)),
                            sizeof(uint16_t));
        if (bytesRead != sizeof(uint16_t)) {
            const QString errorMessage
                    = QStringLiteral("Error reading from packet buffer. Bytes read = %1 <> 2")
                    .arg(bytesRead);
            qWarning() << errorMessage;
            Q_EMIT error(QMqttProtocol::Error::PARSE_ERROR, errorMessage);
            return;
        }
        packetIdentifier = qFromBigEndian(packetIdentifier);

        variableHeaderLength += 2;
    }

    const int32_t messageLength = packet.remainingLength() - variableHeaderLength;
    if (buffer.bytesAvailable() != messageLength) {
        const QString errorMessage
                = QStringLiteral("Invalid PUBLISH packet received. Payload too small.");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
    const QByteArray message = buffer.read(messageLength);
    if (message.size() != messageLength) {
        const QString errorMessage = QStringLiteral("Error reading message: %1.").arg(buffer.errorString());
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::PARSE_ERROR, errorMessage);
        return;
    }

    Q_EMIT publish(packet.qos(), packetIdentifier, topicName, message);
}

void QMqttPacketParser::parsePUBREL(const MQTTPacket &packet)
{
    if (packet.remainingLength() < 2) {
        const QString errorMessage = QStringLiteral("Invalid PUBREL packet received");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }
    if (packet.flags() != 0x02) {
        const QString errorMessage = QStringLiteral("Invalid flags in PUBREL packet.");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::PROTOCOL_VIOLATION, errorMessage);
        return;
    }

    const QByteArray payload = packet.payload();
    const uint16_t packetIdentifier = uint16_t(uint8_t(payload[0])) * 256 + uint8_t(payload[1]);

    Q_EMIT pubrel(packetIdentifier);
}

void QMqttPacketParser::parsePUBACK(const MQTTPacket &packet)
{
    if (packet.remainingLength() < 2) {
        const QString errorMessage = QStringLiteral("Invalid PUBACK packet received");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }

    const QByteArray payload = packet.payload();
    const uint16_t packetIdentifier = uint16_t(uint8_t(payload[0])) * 256 + uint8_t(payload[1]);

    Q_EMIT puback(packetIdentifier);
}

void QMqttPacketParser::parseUNSUBACK(const MQTTPacket &packet)
{
    if (packet.remainingLength() < 2) {
        const QString errorMessage = QStringLiteral("Invalid UNSUBACK packet received");
        qWarning() << errorMessage;
        Q_EMIT error(QMqttProtocol::Error::INVALID_PACKET, errorMessage);
        return;
    }

    const QByteArray payload = packet.payload();
    const uint16_t packetIdentifier = uint16_t(uint8_t(payload[0])) * 256 + uint8_t(payload[1]);

    Q_EMIT unsuback(packetIdentifier);
}
