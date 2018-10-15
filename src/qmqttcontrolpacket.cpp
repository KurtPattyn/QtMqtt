#include "qmqttcontrolpacket_p.h"
#include <QtEndian>
#include <QByteArray>
#include <QVector>
#include <QDebug>
#include <limits>
#include "logging_p.h"

LoggingModule("QMqttControlPacket");

/*!
    \enum ControlPacket::PacketType

    \inmodule QtMqtt

    The control packet types supported by MQTT v3.1.1

    \value RESERVED_0                       Reserved; should not be used
    \value CONNECT                          Client request to connect to server (client --> server)
    \value CONNACK                          Connect acknowledgement (server --> client)
    \value PUBLISH                          Publish message (client <--> server)
    \value PUBACK                           Publish acknowledgement (client <--> server)
    \value PUBREC                           Publish received (assured delivery part 1) (client <--> server)
    \value PUBREL                           Publish received (assured delivery part 2) (client <--> server)
    \value PUBCOMP                          Publish received (assured delivery part 2) (client <--> server)
    \value SUBSCRIBE                        Client subscribe request (client --> server)
    \value SUBACK                           Subscribe acknowledgement (server --> client)
    \value UNSUBSCRIBE                      Unsubscribe request (client --> server)
    \value UNSUBACK                         Unsubscribe acknlowedgement (server --> client)
    \value PINGREQ                          PING request (client --> server)
    \value PINGRESP                         PING response (server --> client)
    \value DISCONNECT                       Client is disconnecting (client --> server)
    \value RESERVED_15                      Reserved; should not be used

    \internal
*/

//helper methods
template<class T>
QByteArray encodeNumber(T t) Q_DECL_NOEXCEPT {
    QByteArray array;
    const T tmp = qToBigEndian(t);
    return array.append(static_cast<const char *>(static_cast<const void *>(&tmp)),
                        sizeof(T));
}

QByteArray encodeData(const QByteArray &data) Q_DECL_NOEXCEPT
{
    const int size = data.size();
    if (size < std::numeric_limits<uint16_t>::max()) {
        QByteArray encodedData;
        encodedData.append(encodeNumber(uint16_t(size)));
        if (size > 0) {
            encodedData.append(data);
        }
        return encodedData;
    } else {
        qCWarning(module) << "Data is too big: size =" << size
                          << "maximum size=" << std::numeric_limits<uint16_t>::max();
        return QByteArray();
    }
}

inline QByteArray encodeString(const QString &string) Q_DECL_NOEXCEPT {
    return encodeData(string.toUtf8());
}

inline QByteArray encodeLength(int32_t length) Q_DECL_NOEXCEPT {
    QByteArray encodedLength;
    do {
        uint8_t digit = length % 128;
        length = length / 128;
        if (length > 0) {
            digit = digit | 0x80;
        }
        encodedLength.append(digit);
    } while (length > 0);
    return encodedLength;
}

QMqttControlPacket::QMqttControlPacket(const PacketType &controlPacketType) :
    QObject(),
    m_type(controlPacketType)
{}

QMqttControlPacket::PacketType QMqttControlPacket::type() const
{
    return m_type;
}

QByteArray QMqttControlPacket::fixedHeader() const {
    QByteArray header;
    const uint8_t byte1 = (uint8_t(type()) << 4) | flags();
    return header.append(byte1);
}

QByteArray QMqttControlPacket::encode() const
{
    QByteArray packet;

    const QByteArray fixedHdr = fixedHeader();
    const QByteArray variableHdr = variableHeader();
    const QByteArray payloadData = payload();
    const int remainingLength = variableHdr.size() + payloadData.size();
    if (remainingLength > QMqttControlPacket::MAXIMUM_CONTROL_PACKET_SIZE) {
        qCWarning(module) << "Packet size too big:" << remainingLength
                          << "maximum:" << QMqttControlPacket::MAXIMUM_CONTROL_PACKET_SIZE;
        return packet;
    }
    return packet
            .append(fixedHdr)
            .append(encodeLength(int32_t(remainingLength)))
            .append(variableHdr)
            .append(payloadData);
}

QMqttConnectControlPacket::QMqttConnectControlPacket(const QString &clientIdentifier) :
    QMqttControlPacket(PacketType::CONNECT),
    m_userName(),
    m_password(),
    m_will(),
    m_clean(true),
    m_keepAlive(30),
    m_clientIdentifier(clientIdentifier)
{
    Q_ASSERT(clientIdentifier.length() < 24);
}

void QMqttConnectControlPacket::setCredentials(const QString &userName, const QByteArray &password)
{
    Q_ASSERT(!userName.isEmpty());
    m_userName = userName;
    m_password = password;
}

void QMqttConnectControlPacket::setWill(const QMqttWill &will)
{
    m_will = will;
}

bool QMqttConnectControlPacket::hasUserName() const
{
    return !m_userName.isEmpty();
}

bool QMqttConnectControlPacket::hasPassword() const
{
    //use isNull iso isEmpty, so that empty password can be supplied
    return !m_password.isNull();
}

bool QMqttConnectControlPacket::hasWill() const
{
    return m_will.isValid();
}

bool QMqttConnectControlPacket::isCleanSession() const
{
    return m_clean;
}

uint8_t QMqttConnectControlPacket::flags() const
{
    return 0x00;
}

QByteArray QMqttConnectControlPacket::variableHeader() const
{
    QByteArray header;
    //protocol name
    header.append(encodeString(QStringLiteral("MQTT")));
    //protocol level
    header.append(uint8_t(4));
    //connect flags
    const uint8_t connectFlags =
            ((hasUserName() << 7) |
            (hasPassword() << 6) |
            (m_will.retain() << 5) |
            (uint8_t(m_will.qos()) << 3) |
            (hasWill() << 2) |
            (isCleanSession() << 1)) &
            0xF7;  //set lowest bit to 0
    header.append(connectFlags);
    //keep alive
    header.append(encodeNumber(m_keepAlive));
    return header;
}

QByteArray QMqttConnectControlPacket::payload() const
{
    QByteArray buffer;
    buffer.append(encodeString(m_clientIdentifier));
    if (hasWill()) {
        buffer.append(encodeString(m_will.topic()));
        buffer.append(encodeNumber(uint16_t(m_will.message().size())));
        buffer.append(m_will.message());
    }
    if (hasUserName()) {
        buffer.append(encodeString(m_userName));
    }
    if (hasPassword()) {
        buffer.append(encodeData(m_password));
    }

    return buffer;
}

QMqttPublishControlPacket::QMqttPublishControlPacket(const QString &topicName, const QByteArray &message,
                                           QMqttProtocol::QoS qos, bool retain,
                                           uint16_t packetIdentifier) :
    QMqttControlPacket(PacketType::PUBLISH),
    m_topicName(topicName),
    m_message(message),
    m_dup(false),
    m_qos(qos),
    m_retain(retain),
    m_packetIdentifier(packetIdentifier)
{
    Q_ASSERT(!topicName.isEmpty());
}

uint8_t QMqttPublishControlPacket::flags() const
{
    return (uint8_t(m_dup) << 3) | (uint8_t(m_qos) << 1) | uint8_t(m_retain);
}

QByteArray QMqttPublishControlPacket::variableHeader() const
{
    QByteArray header = encodeString(m_topicName);
    if ((m_qos == QMqttProtocol::QoS::AT_LEAST_ONCE) || (m_qos == QMqttProtocol::QoS::EXACTLY_ONCE))
    {
        header.append(encodeNumber(m_packetIdentifier));
    }
    return header;
}

QByteArray QMqttPublishControlPacket::payload() const
{
    return m_message;
}

QMqttPubAckControlPacket::QMqttPubAckControlPacket(uint16_t packetIdentifier) :
    QMqttControlPacket(PacketType::PUBACK),
    m_packetIdentifier(packetIdentifier)
{}

uint8_t QMqttPubAckControlPacket::flags() const
{
    return 0x00;
}

QByteArray QMqttPubAckControlPacket::variableHeader() const
{
    return encodeNumber(m_packetIdentifier);
}

QByteArray QMqttPubAckControlPacket::payload() const
{
    return QByteArray();
}

QMqttPubRecControlPacket::QMqttPubRecControlPacket(uint16_t packetIdentifier) :
    QMqttControlPacket(PacketType::PUBREC),
    m_packetIdentifier(packetIdentifier)
{}

uint8_t QMqttPubRecControlPacket::flags() const
{
    return 0x00;
}

QByteArray QMqttPubRecControlPacket::variableHeader() const
{
    return encodeNumber(m_packetIdentifier);
}

QByteArray QMqttPubRecControlPacket::payload() const
{
    return QByteArray();
}

QMqttPubCompControlPacket::QMqttPubCompControlPacket(uint16_t packetIdentifier) :
    QMqttControlPacket(PacketType::PUBCOMP),
    m_packetIdentifier(packetIdentifier)
{}

uint8_t QMqttPubCompControlPacket::flags() const
{
    return 0x00;
}

QByteArray QMqttPubCompControlPacket::variableHeader() const
{
    return encodeNumber(m_packetIdentifier);
}

QByteArray QMqttPubCompControlPacket::payload() const
{
    return QByteArray();
}

QMqttSubscribeControlPacket::QMqttSubscribeControlPacket(uint16_t packetIdentifier,
                                               QVector<TopicFilter> topicFilters) :
    QMqttControlPacket(PacketType::SUBSCRIBE),
    m_packetIdentifier(packetIdentifier),
    m_topicFilters(topicFilters)
{
}

uint8_t QMqttSubscribeControlPacket::flags() const
{
    return 0x02;    //QoS = 1
}

QByteArray QMqttSubscribeControlPacket::variableHeader() const
{
    return encodeNumber(m_packetIdentifier);
}

QByteArray QMqttSubscribeControlPacket::payload() const
{
    QByteArray buffer;
    for (const TopicFilter &topicFilter : m_topicFilters) {
        buffer.append(encodeString(topicFilter.first));
        const uint8_t qos = uint8_t(topicFilter.second);
        buffer.append(qos);
    }
    return buffer;
}

QMqttUnsubscribeControlPacket::QMqttUnsubscribeControlPacket(uint16_t packetIdentifier,
                                                   QVector<QString> topics) :
    QMqttControlPacket(PacketType::UNSUBSCRIBE),
    m_packetIdentifier(packetIdentifier),
    m_topics(topics)
{}

uint8_t QMqttUnsubscribeControlPacket::flags() const
{
    return 0x02;
}

QByteArray QMqttUnsubscribeControlPacket::variableHeader() const
{
    return encodeNumber(m_packetIdentifier);
}

QByteArray QMqttUnsubscribeControlPacket::payload() const
{
    QByteArray buffer;
    for (const QString &topic : m_topics) {
        buffer.append(encodeString(topic));
    }
    return buffer;
}

QMqttPingReqControlPacket::QMqttPingReqControlPacket() :
    QMqttControlPacket(PacketType::PINGREQ)
{}

uint8_t QMqttPingReqControlPacket::flags() const
{
    return 0x00;
}

QByteArray QMqttPingReqControlPacket::variableHeader() const
{
    return QByteArray();
}

QByteArray QMqttPingReqControlPacket::payload() const
{
    return QByteArray();
}

QMqttDisconnectControlPacket::QMqttDisconnectControlPacket() :
    QMqttControlPacket(PacketType::DISCONNECT)
{}

uint8_t QMqttDisconnectControlPacket::flags() const
{
    return 0x00;
}

QByteArray QMqttDisconnectControlPacket::variableHeader() const
{
    return QByteArray();
}

QByteArray QMqttDisconnectControlPacket::payload() const
{
    return QByteArray();
}
