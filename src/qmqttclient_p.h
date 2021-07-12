#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QWebSocket>
#include <QMap>
#include <QVector>
#include <QScopedPointer>
#include <QTimer>
#include "qmqttprotocol.h"
#include "qmqttpacketparser_p.h"
#include "qmqttwill.h"

class QMqttClient;
class QMqttNetworkRequest;
class QMqttClientPrivate : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QMqttClientPrivate)
    Q_DECLARE_PUBLIC(QMqttClient)

public:
    QMqttClientPrivate(const QString &clientId, const QSet<QSslError> &allowedSslErrors, QMqttClient * const q);
    virtual ~QMqttClientPrivate();

    void connect(const QMqttNetworkRequest &request, const QMqttWill &will);
    void disconnect();
    void subscribe(const QString &topic, QMqttProtocol::QoS qos, std::function<void(bool)> cb);
    void unsubscribe(const QString &topic, std::function<void (bool)> cb);
    void publish(const QString &topic, const QByteArray &message);
    void publish(const QString &topic, const QByteArray &message, std::function<void(bool)> cb);

    void sendPing();

    void setState(QMqttProtocol::State newState);

    QHostAddress localAddress() const;
    quint16 localPort() const;

private:
    QMqttClient * const q_ptr;
    const QString m_clientId;
    bool m_pongReceived;
    QTimer m_pingTimer;
    int m_pingIntervalMs;
    QScopedPointer<QWebSocket> m_webSocket;
    QMqttProtocol::State m_state;
    QScopedPointer<QMqttPacketParser> m_packetParser;
    uint16_t m_packetIdentifier;
    QMap<uint16_t, std::function<void(bool)>> m_subscribeCallbacks;
    QMqttWill m_will;
    bool m_signalSlotConnected;
    const QSet<QSslError> m_allowedSslErrors;

private Q_SLOTS:
    void onSocketConnected();
    void onConnackReceived(QMqttProtocol::Error error, bool sessionPresent);
    void onSubackReceived(uint16_t packetIdentifier, QVector<QMqttProtocol::QoS> qos);
    void onPublishReceived(QMqttProtocol::QoS qos, uint16_t packetIdentifier,
                           const QString &topicName, const QByteArray &message);
    void onPubRelReceived(uint16_t packetIdentifier);
    void onPubAckReceived(uint16_t packetIdentifier);
    void onUnsubackReceived(uint16_t packetIdentifier);
    void onPongReceived();

private: //helpers
    bool sslErrorsAllowed(const QList<QSslError> &sslErrors) const;
    void makeSignalSlotConnections();

    void sendData(const QByteArray &data);
};

