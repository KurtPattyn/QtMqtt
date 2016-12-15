#pragma once

#include "qmqttprotocol.h"
#include "qmqttwill.h"
#include "qmqtt_global.h"
#include <QByteArray>
#include <QVector>

//MQTT v3.1.1 specification: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html

//For Control Packets, see 3. MQTT Control Packets in the MQTT v3.1.1 specification

class QTMQTT_AUTOTEST_EXPORT QMqttControlPacket:public QObject
{
    Q_OBJECT

public:
    //see 2.1.1 MQTT Control Packet type in MQTT v3.1.1 specification
    enum class PacketType : uint8_t
    {
        RESERVED_0   = 0,   //Reserved
        CONNECT      = 1,   //Client request to connect to server (client --> server)
        CONNACK      = 2,   //Connect acknowledgement (server --> client)
        PUBLISH      = 3,   //Publish message (client <--> server)
        PUBACK       = 4,   //Publish acknowledgement (client <--> server)
        PUBREC       = 5,   //Publish received (assured delivery part 1) (client <--> server)
        PUBREL       = 6,   //Publish release  (assured delivery part 2) (client <--> server)
        PUBCOMP      = 7,   //Publish complete (assured delivery part 3) (client <--> server)
        SUBSCRIBE    = 8,   //Client subscribe request (client --> server)
        SUBACK       = 9,   //Subscribe acknowledgement (server --> client)
        UNSUBSCRIBE  = 10,  //Unsubscribe request (client --> server)
        UNSUBACK     = 11,  //Unsubscribe acknlowedgement (server --> client)
        PINGREQ      = 12,  //PING request (client --> server)
        PINGRESP     = 13,  //PING response (server --> client)
        DISCONNECT   = 14,  //Client is disconnecting (client --> server)
        RESERVED_15  = 15   //Reserved
    };
    Q_ENUM(PacketType)
    static const int32_t MAXIMUM_CONTROL_PACKET_SIZE = 256 * 1024 * 1024; //256MiB

    QMqttControlPacket(const PacketType &controlPacketType);

    PacketType type() const;

    //fixed header without remaining length field
    QByteArray fixedHeader() const;
    virtual QByteArray variableHeader() const = 0;
    virtual QByteArray payload() const = 0;

    QByteArray encode() const;

protected:
    virtual uint8_t flags() const = 0;

private:
    PacketType m_type;
};

class QTMQTT_AUTOTEST_EXPORT QMqttConnectControlPacket: public QMqttControlPacket
{
public:
    /**
     * @brief ConnectControlPacket
     * @param clientIdentifier A string identifying the client. This identifier should be unique
     * amongst all clients that connect, as this id is used, a.o.,
     */
    QMqttConnectControlPacket(const QString &clientIdentifier);

    void setCredentials(const QString &userName, const QByteArray &password = QByteArray());
    void setWill(const QMqttWill &will);
    void setCleanSession(bool isClean);
    /**
     * keepAliveSecs is the maximum time interval in seconds that is permitted to elapse between the
     * point at which a client finishes transmitting one control packet and the point it starts
     * sending the next.
     * A keepAliveSecs value of zero has the effect of turning off the keep alive mechanism.
     * If the keepAliveSecs value is non-zero and the server does not receive a control packet from
     * the client within one and a half times the keepAliveSecs period, the server will disconnect
     * the network connection as if the network has failed.
     * The default keepAliveSecs is zero.
     */
    void setKeepAlive(uint16_t keepAliveSecs); //if 0, no keep alive; max = 18 hours, 12 minutes and 15 seconds
    void setClientIdentifier(const QString &identifier); //length must be > 0 and < 24

    bool hasUserName() const;
    bool hasPassword() const;
    bool hasWill() const;
    bool isCleanSession() const;

protected:
    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;

private:
    QString m_userName;
    QByteArray m_password;
    QMqttWill m_will;
    bool m_clean;
    uint16_t m_keepAlive;
    QString m_clientIdentifier;
};

/**
 * @brief The PublishControlPacket class
 * Limitations:
 * ============
 * The PublishControlPacket currently does not support the dup flag.
 * The `dup` flag is used to indicate a redelivered control packet when clean session is set
 * to false. The MQTT Client currently does not support sessions and hence does not redeliver
 * packets.
 * From the MQTT specification (4.4 Message delivery retry):
 * "Historically retransmission of Control Packets was required to overcome data loss on some
 * older TCP networks. This might remain a concern where MQTT 3.1.1 implementations
 * are to be deployed in such environments."
 * We assume that the MQTT client is deployed on state-of-the-art TCP networks.
 */
class QTMQTT_AUTOTEST_EXPORT QMqttPublishControlPacket : public QMqttControlPacket
{
public:
    /**
     * @brief Constructs a publish MQTT control packet with the given topicName and message.
     * The topic name must not be empty and must not contain wild cards. The message can be
     * empty. The message will be published with the given Quality of Service.
     * When retain is true, the message will be stored by the server. When a new client subscribes,
     * the last retained message, if any, will be sent to the client for each matching topic.
     * The packetIdentifier is a unique number assigned to the packet. It is only required
     * when qos != MQTTProtocol::QoS::AT_MOST_ONCE. When qos == MQTTProtocol::QoS::AT_MOST_ONCE,
     * the packetIdentifier will be ignored.
     */
    QMqttPublishControlPacket(const QString &topicName, const QByteArray &message,
                         QMqttProtocol::QoS qos, bool retain, uint16_t packetIdentifier = 0);

private:
    const QString m_topicName;
    const QByteArray m_message;
    const bool m_dup;
    QMqttProtocol::QoS m_qos;
    const bool m_retain;
    uint16_t m_packetIdentifier;

    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};

class QTMQTT_AUTOTEST_EXPORT QMqttPubAckControlPacket: public QMqttControlPacket
{
public:
    QMqttPubAckControlPacket(uint16_t packetIdentifier);

private:
    const uint16_t m_packetIdentifier;

    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};

class QTMQTT_AUTOTEST_EXPORT QMqttPubRecControlPacket: public QMqttControlPacket
{
public:
    QMqttPubRecControlPacket(uint16_t packetIdentifier);

private:
    const uint16_t m_packetIdentifier;

    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};

class QTMQTT_AUTOTEST_EXPORT QMqttPubCompControlPacket: public QMqttControlPacket
{
public:
    QMqttPubCompControlPacket(uint16_t packetIdentifier);

private:
    const uint16_t m_packetIdentifier;

    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};

typedef QPair<QString, QMqttProtocol::QoS> TopicFilter;

class QTMQTT_AUTOTEST_EXPORT QMqttSubscribeControlPacket: public QMqttControlPacket
{
public:
    QMqttSubscribeControlPacket(uint16_t packetIdentifier, QVector<TopicFilter> topicFilters);

private:
    const uint16_t m_packetIdentifier;
    const QVector<TopicFilter> m_topicFilters;

    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};

class QTMQTT_AUTOTEST_EXPORT QMqttUnsubscribeControlPacket: public QMqttControlPacket
{
public:
    QMqttUnsubscribeControlPacket(uint16_t packetIdentifier, QVector<QString> topics);

private:
    const uint16_t m_packetIdentifier;
    const QVector<QString> m_topics;

    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};

class QTMQTT_AUTOTEST_EXPORT QMqttPingReqControlPacket: public QMqttControlPacket
{
public:
    QMqttPingReqControlPacket();

private:
    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};

class QTMQTT_AUTOTEST_EXPORT QMqttDisconnectControlPacket: public QMqttControlPacket
{
public:
    QMqttDisconnectControlPacket();

private:
    uint8_t flags() const Q_DECL_OVERRIDE;
    QByteArray variableHeader() const Q_DECL_OVERRIDE;
    QByteArray payload() const Q_DECL_OVERRIDE;
};
