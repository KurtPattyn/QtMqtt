#pragma once

#include <QObject>
#include <QHostAddress>
#include <QSet>
#include <QSslError>
#include <functional>
#include "qmqttwill.h"
#include "qmqttprotocol.h"
#include "qmqtt_global.h"

//TODO: add SSL connectivity
//Currently only secure websockets are supported

class QMqttNetworkRequest;
class QString;
class QByteArray;
class QMqttClientPrivate;
class QTMQTT_EXPORT QMqttClient : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QMqttClient)
    Q_DISABLE_COPY(QMqttClient)

public:
    QMqttClient(const QString &clientId, const QSet<QSslError> &allowedSslErrors = QSet<QSslError>(), QObject *parent = nullptr);
    virtual ~QMqttClient();

    using QObject::connect;
    void connect(const QMqttNetworkRequest &request, const QMqttWill &will = QMqttWill(), const QString &userName = QString(), const QByteArray &password = QByteArray());
    using QObject::disconnect;
    void disconnect();

    void subscribe(const QString &topic, QMqttProtocol::QoS qos, std::function<void(bool)> cb);
    void unsubscribe(const QString &topic, std::function<void(bool)> cb);
    void publish(const QString &topic, const QByteArray &message);
    void publish(const QString &topic, const QByteArray &message, std::function<void(bool)> cb);

    QHostAddress localAddress() const;
    quint16 localPort() const;

Q_SIGNALS:
    void stateChanged(QMqttProtocol::State);
    void connected();
    void disconnected();
    void messageReceived(const QString &topicName, const QByteArray &message);
    void error(QMqttProtocol::Error err, const QString &errorMessage);

private:
    QScopedPointer<QMqttClientPrivate> d_ptr;
};
