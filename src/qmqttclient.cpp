#include "qmqttclient.h"
#include "qmqttclient_p.h"
#include "qmqttnetworkrequest.h"
#include "qmqttcontrolpacket_p.h"
#include "qmqttwill.h"
#include "logging_p.h"

LoggingModule("QMqttClient");

/*!
   \class QMqttClient

   \inmodule QtMqtt

    \brief Implements a client for the MQTT protocol over WebSockets.

    MQTT stands for Message Queue Telemetry Transport, and is a web technology providing full-duplex
    communications channels over a single TCP connection.
    The version 3.1.1 of the MQTT protocol was standardized by the OASIS in 2014:
    \l {MQTT Standard}{MQTT Version 3.1.1. Edited by Andrew Banks and Rahul Gupta. 29 October 2014. OASIS Standard}.

   Limitations:
   ============
   \list 1
   \li The QMqttClient class currently does not support sessions and hence does not redeliver
   packets.

   From the MQTT specification (4.4 Message delivery retry):
   "Historically retransmission of Control Packets was required to overcome data loss on some
   older TCP networks. This might remain a concern where MQTT 3.1.1 implementations
   are to be deployed in such environments."
   We assume that the MQTT client is deployed on state-of-the-art TCP networks.

   \li The QMqttClient currently does not support publishing messages with QoS 2.
   \endlist
 */

/*!
    \fn void QMqttClient::stateChanged(QMqttProtocol::State state)

    Emitted when the connectivity status of the connection changes.
    The \a state parameter is the new state.

    \note QMqttProtocol::State::CONNECTED is emitted after the client received a connection
    acknowledgement from the server.

    \sa connected()
*/

/*!
    \fn void QMqttClient::connected()

    This signal is emitted when a connection is successfully established.
    A connection is successfully established if the connection is acknowledged by the server.

    \sa connect(), disconnected()
*/

/*!
    \fn void QMqttClient::disconnected()

    This signal is emitted when the connection is completely closed.

    \sa disconnect(), connected()
*/

/*!
    \fn void QMqttClient::messageReceived(const QString &topicName, const QByteArray &message);

    This signal is emitted when a \a message was received on the topic with the given \a topicName;
*/

/*!
    \fn void QMqttClient::error(MQTTProtocol::Error err, const QString &errorMessage);

    This signal is emitted when an error occurs. The \a err parameter indicates the type of error
    that occurred and \a errorMessage contains a textual description of the error.
*/

/*!
    \brief Calls the given function \a f after \a ms milliseconds.

    setTimeout does not block, but uses the Qt event loop to schedule the invocation of the function.

    \code
    setTimeout([]() { qDebug() << "Called me after 1 second." }, 1000);
    qDebug() << "This will be shown first.";
    \endcode

    It is also possible to supply parameters to the callback by binding the function to its
    arguments.

    \code
    void printSomething(const QString &message) {
    qDebug() << message;
    }
    setTimeout(std::bind(printSomething, message), 1000);
    \endcode

    \sa setImmediate()

    \internal
 */
void setTimeout(std::function<void()> f, int ms)
{
    QTimer::singleShot(ms, f);
}

/*!
    \brief Puts the call to the given function \a f at the end of Qt's event loop.
    setImmediate does not block.

    \e setTimeout(f, 0) is the same as calling \e setImmediate(f).

    \sa setTimeout()

    \internal
 */
void setImmediate(std::function<void()> f)
{
    setTimeout(f, 0);
}

/*!
   \internal
 */
QMqttClientPrivate::QMqttClientPrivate(const QString &clientId, const QSet<QSslError> &allowedSslErrors, QMqttClient * const q) :
    QObject(),
    q_ptr(q),
    m_clientId(clientId),
    m_pongReceived(false),
    m_pingTimer(),
    m_pingIntervalMs(30000), // 30 seconds
    m_webSocket(new QWebSocket),
    m_state(QMqttProtocol::State::OFFLINE),
    m_packetParser(new QMqttPacketParser),
    m_packetIdentifier(0),
    m_subscribeCallbacks(),
    m_will(),
    m_signalSlotConnected(false),
    m_allowedSslErrors(allowedSslErrors),
    m_userName(),
    m_password()
{
    Q_ASSERT(q);
    Q_ASSERT(!clientId.isEmpty());
}

/*!
   \internal
 */
QMqttClientPrivate::~QMqttClientPrivate()
{}


/*!
   \internal
 */
void QMqttClientPrivate::connect(const QMqttNetworkRequest &request, const QMqttWill &will, const QString &userName, const QByteArray &password)
{
    if (m_state != QMqttProtocol::State::OFFLINE) {
        qCWarning(module) << "Already connected.";
        return;
    }
    m_will = will;
    m_userName = userName;
    m_password = password;
    qCDebug(module) << "Connecting to Mqtt backend @ endpoint" << request.url();
    setState(QMqttProtocol::State::CONNECTING);

    makeSignalSlotConnections();

    m_webSocket->open(request);
}

/*!
   \internal
 */
void QMqttClientPrivate::disconnect()
{
    if (m_state != QMqttProtocol::State::OFFLINE) {
        m_pingTimer.stop();
        setState(QMqttProtocol::State::DISCONNECTING);
        m_webSocket->sendBinaryMessage(QMqttDisconnectControlPacket().encode());
        m_webSocket->close();
    }
}

/*!
  Checks validity of a topic name.
  A topic name is valid if it follows the following rules:
  - Rule #1: If any part of the topic is not `+` or `#`, then it must not contain `+` and `#`
  - Rule #2: Part `#` must be located at the end of the name

  \internal
*/
bool isTopicNameValid(const QString &topicName)
{
    const QStringList parts = topicName.split(QStringLiteral("/"));
    for (int i = 0; i < parts.length(); ++i) {
      const QString part = parts[i];
      if (part == QStringLiteral("+")) {
        continue;
      }

      if (part == QStringLiteral("#")) {
        // for Rule #2
        return i == (parts.length() - 1);
      }

      if (part.contains('+') || part.contains('#')) {
        return false;
      }
    }
    return true;
}

/*!
   \internal
 */
void QMqttClientPrivate::subscribe(const QString &topic, QMqttProtocol::QoS qos,
                                  std::function<void(bool)> cb)
{
    if (!isTopicNameValid(topic)) {
        qCWarning(module) << "Invalid topic name detected:" << topic;
        setImmediate(std::bind(cb, false));
        return;
    }
    qCDebug(module) << "Subscribing to topic" << topic;
    QVector<QPair<QString, QMqttProtocol::QoS>> topicFilters
            = { { topic, qos } };
    QMqttSubscribeControlPacket subscribePacket(++m_packetIdentifier, topicFilters);
    m_subscribeCallbacks.insert(m_packetIdentifier, cb);
    sendData(subscribePacket.encode());
}

/*!
   \internal
 */
void QMqttClientPrivate::unsubscribe(const QString &topic, std::function<void (bool)> cb)
{
    if (!isTopicNameValid(topic)) {
        qCWarning(module) << "Invalid topic name detected:" << topic;
        setImmediate(std::bind(cb, false));
        return;
    }
    QMqttUnsubscribeControlPacket unsubscribePacket(++m_packetIdentifier, {topic});
    m_subscribeCallbacks.insert(m_packetIdentifier, cb);
    sendData(unsubscribePacket.encode());
}

/*!
   \internal
 */
void QMqttClientPrivate::publish(const QString &topic, const QByteArray &message)
{
    qCDebug(module) << "Publishing" << message << "to topic" << topic;
    QMqttPublishControlPacket packet(topic, message, QMqttProtocol::QoS::AT_MOST_ONCE, false);
    sendData(packet.encode());
}

/*!
   \internal
 */
void QMqttClientPrivate::publish(const QString &topic, const QByteArray &message,
                                std::function<void (bool)> cb)
{
    qCDebug(module) << "Publishing" << message << "to topic" << topic;
    QMqttPublishControlPacket packet(topic, message, QMqttProtocol::QoS::AT_LEAST_ONCE,
                                     false, ++m_packetIdentifier);
    m_subscribeCallbacks.insert(m_packetIdentifier, cb);
    sendData(packet.encode());
}

/*!
   \internal
 */
void QMqttClientPrivate::setState(QMqttProtocol::State newState)
{
    if (m_state != newState) {
        Q_Q(QMqttClient);

        m_state = newState;
        Q_EMIT q->stateChanged(m_state);
    }
}

/*!
   \internal
 */
QHostAddress QMqttClientPrivate::localAddress() const
{
    qInfo() << "socket info:" << m_webSocket->localAddress() << m_webSocket->peerAddress() << m_webSocket->state();
    return m_webSocket->localAddress();
}

/*!
   \internal
 */
quint16 QMqttClientPrivate::localPort() const
{
    return m_webSocket->localPort();
}

/*!
   \internal
 */
void QMqttClientPrivate::sendPing()
{
    qCDebug(module) << "Sending ping.";
    if (m_pongReceived) {
        m_pongReceived = false;
        QMqttPingReqControlPacket packet;
        m_webSocket->sendBinaryMessage(packet.encode());
    } else {
        Q_Q(QMqttClient);

        const QString errorMessage = QStringLiteral("Pong not received within expected time.");

        Q_EMIT q->error(QMqttProtocol::Error::TIME_OUT, errorMessage);

        disconnect();
    }
}

/*!
   \internal
 */
void QMqttClientPrivate::onPongReceived()
{
    qCDebug(module) << "Received pong.";
    m_pongReceived = true;
}

/*!
   \internal
 */
void QMqttClientPrivate::onSocketConnected()
{
    qCDebug(module) << "WebSockets successfully connected.";

    QMqttConnectControlPacket packet(m_clientId);
    packet.setWill(m_will);
    packet.setCredentials(m_userName, m_password);
    m_webSocket->sendBinaryMessage(packet.encode());

    //TODO: initialize connection timeout
}

/*!
   \internal
 */
void QMqttClientPrivate::onConnackReceived(QMqttProtocol::Error err, bool sessionPresent)
{
    Q_Q(QMqttClient);

    qCDebug(module) << "Received connack with returncode:" << err << "and session present:" << sessionPresent;
    if (m_state != QMqttProtocol::State::CONNECTING) {
        const QString errorString =
                QStringLiteral("Received a CONNACK packet while the MQTT connection is already connected.");
        Q_EMIT q->error(QMqttProtocol::Error::PROTOCOL_VIOLATION, errorString);
        m_webSocket->abort();
        return;
    }
    if (err != QMqttProtocol::Error::CONNECTION_ACCEPTED) {
        const QString errorString = QStringLiteral("Connection refused");
        Q_EMIT q->error(err, errorString);
        m_webSocket->abort();
        return;
    }

    if (m_pingIntervalMs > 0) {
        m_pongReceived = true;
        m_pingTimer.setInterval(m_pingIntervalMs);
        m_pingTimer.setSingleShot(false);
        m_pingTimer.start();
    }

    Q_EMIT q->connected();
}

/*!
   \internal
 */
void QMqttClientPrivate::onSubackReceived(uint16_t packetIdentifier,
                                         QVector<QMqttProtocol::QoS> qos)
{
    qCDebug(module) << "Received suback for packet with id" << packetIdentifier;
    if (m_subscribeCallbacks.contains(packetIdentifier)) {
        const std::vector<QMqttProtocol::QoS> qosVector = qos.toStdVector();
        const bool result = std::none_of(qosVector.begin(), qosVector.end(),
                                         [](QMqttProtocol::QoS qos) { return qos == QMqttProtocol::QoS::INVALID; });
        auto cb = m_subscribeCallbacks.value(packetIdentifier);
        m_subscribeCallbacks.remove(packetIdentifier);
        setImmediate(std::bind(cb, result));
    }
}

/*!
   \internal
 */
void QMqttClientPrivate::onPublishReceived(QMqttProtocol::QoS qos, uint16_t packetIdentifier,
                                          const QString &topicName, const QByteArray &message)
{
    Q_Q(QMqttClient);

    qCDebug(module) << "Received publish packet with qos" << qos << "and id" << packetIdentifier;

    Q_EMIT q->messageReceived(topicName, message);

    if (qos == QMqttProtocol::QoS::EXACTLY_ONCE) {
        const QMqttPubRecControlPacket packet(packetIdentifier);
        sendData(packet.encode());
    } else if (qos == QMqttProtocol::QoS::AT_LEAST_ONCE) {
        const QMqttPubAckControlPacket packet(packetIdentifier);
        sendData(packet.encode());
    }
}

/*!
   \internal
 */
void QMqttClientPrivate::onPubRelReceived(uint16_t packetIdentifier)
{
    qCDebug(module) << "Received PubRel packet with id" << packetIdentifier;
    QMqttPubCompControlPacket packet(packetIdentifier);
    sendData(packet.encode());
}

/*!
   \internal
 */
void QMqttClientPrivate::onPubAckReceived(uint16_t packetIdentifier)
{
    qCDebug(module) << "Received PubAck packet with id" << packetIdentifier;
    if (m_subscribeCallbacks.contains(packetIdentifier)) {
        auto cb = m_subscribeCallbacks.value(packetIdentifier);
        m_subscribeCallbacks.remove(packetIdentifier);
        setImmediate(std::bind(cb, true));
    }
}

/*!
   \internal
 */
void QMqttClientPrivate::onUnsubackReceived(uint16_t packetIdentifier)
{
    qCDebug(module) << "Received unsuback for packet with id" << packetIdentifier;
    if (m_subscribeCallbacks.contains(packetIdentifier)) {
        auto cb = m_subscribeCallbacks.value(packetIdentifier);
        m_subscribeCallbacks.remove(packetIdentifier);
        setImmediate(std::bind(cb, true));
    }
}

/*!
   \internal
 */
void QMqttClientPrivate::sendData(const QByteArray &data)
{
    m_webSocket->sendBinaryMessage(data);
    if (m_pingIntervalMs > 0) {
        m_pingTimer.start();  //restart the timer
    }
}

/*!
  Converts the given list of QSSlErrors to a list of strings.
   \internal
 */
QString toString(const QList<QSslError> &sslErrors) {
    QString sslErrorString;
    for (const QSslError &sslError : sslErrors) {
        sslErrorString.append(QStringLiteral("%1 (%2)\n")
                              .arg(sslError.errorString()).arg(sslError.error()));
    }
    return sslErrorString;
}


/*!
   \internal
 */
bool QMqttClientPrivate::sslErrorsAllowed(const QList<QSslError> &errors) const
{
    if (!m_allowedSslErrors.isEmpty())
    {
        // The QSslErrors received with the QWebSocket::sslErrors signal are probably defined with the QSslError constructor
        // with a certificate QSslError::QSslError(QSslError::SslError error, const QSslCertificate &certificate)
        // If m_allowedSslErrors is defined using the QSslError constructor without a certificate QSslError::QSslError(QSslError::SslError error),
        // e.g. QSet({QSslError::HostNameMismatch, QSslError::SelfSignedCertificate, QSslError::SelfSignedCertificateInChain}),
        // the subtraction is never empty.
        // This is because the QSslErrors comparison operator compares both the error() and the certificate().
        // If we construct a QSet<QSslError> with only the error() of the errors received with the QWebSocket::sslErrors signal,
        // the subtraction is empty when needed.
        QSet<QSslError> errorsSet;
        for (const auto &error : errors)
        {
            errorsSet << QSslError(error.error());
        }
        const QSet<QSslError> subtraction = errorsSet.subtract(m_allowedSslErrors);
        if (subtraction.isEmpty())
        {
            return true;
        }
    }

    return false;
}

/*!
   \internal
 */
void QMqttClientPrivate::makeSignalSlotConnections()
{
    if (m_signalSlotConnected)
    {
        return;
    }

    Q_Q(QMqttClient);

    QObject::connect(m_webSocket.data(), &QWebSocket::connected,
                     this, &QMqttClientPrivate::onSocketConnected, Qt::QueuedConnection);
    QObject::connect(m_webSocket.data(), &QWebSocket::disconnected,
                     [this, q]() {
        qCDebug(module) << "Received QWebSocket::disconnected, close code" << m_webSocket->closeCode() << "close reason" << m_webSocket->closeReason();
        setState(QMqttProtocol::State::OFFLINE);
        Q_EMIT q->disconnected();
    });

    typedef void (QWebSocket::* sslErrorsSignal)(const QList<QSslError> &);
    QObject::connect(m_webSocket.data(), static_cast<sslErrorsSignal>(&QWebSocket::sslErrors),
                     [this, q](const QList<QSslError> &errors) {
        if (sslErrorsAllowed(errors))
        {
            qCDebug(module) << "Ignoring SSL errors" << errors;
            m_webSocket->ignoreSslErrors();
        }
        else
        {
            const QString errorMessage = QStringLiteral("SSL errors encountered: %1.").arg(toString(errors));
            Q_EMIT q->error(QMqttProtocol::Error::CONNECTION_FAILED, errorMessage);
            setState(QMqttProtocol::State::OFFLINE);
        }
    });

    typedef void (QWebSocket::* errorSignal)(QAbstractSocket::SocketError);
    QObject::connect(m_webSocket.data(), static_cast<errorSignal>(&QWebSocket::error),
                     [this, q](QAbstractSocket::SocketError error) {
        const QString errorMessage = QStringLiteral("Error connecting to MQTT server: %1 (%2).")
                .arg(error).arg(m_webSocket->errorString());
        Q_EMIT q->error(QMqttProtocol::Error::CONNECTION_FAILED, errorMessage);
        setState(QMqttProtocol::State::OFFLINE);
    });
    QObject::connect(m_webSocket.data(), &QWebSocket::textMessageReceived, [this, q](const QString &msg) {
        const QString errorMessage
                = QStringLiteral("Received a text message on the MQTT connection (%1). This should not happen. Connection will be closed.")
                .arg(msg);
        Q_EMIT q->error(QMqttProtocol::Error::PROTOCOL_VIOLATION, errorMessage);
    });
    QObject::connect(m_webSocket.data(), &QWebSocket::binaryMessageReceived,
                     m_packetParser.data(), &QMqttPacketParser::parse, Qt::QueuedConnection);

    QObject::connect(m_packetParser.data(), &QMqttPacketParser::connack,
                     this, &QMqttClientPrivate::onConnackReceived, Qt::QueuedConnection);
    QObject::connect(m_packetParser.data(), &QMqttPacketParser::suback,
                     this, &QMqttClientPrivate::onSubackReceived, Qt::QueuedConnection);
    QObject::connect(m_packetParser.data(), &QMqttPacketParser::publish,
                     this, &QMqttClientPrivate::onPublishReceived, Qt::QueuedConnection);
    QObject::connect(m_packetParser.data(), &QMqttPacketParser::pubrel,
                     this, &QMqttClientPrivate::onPubRelReceived, Qt::QueuedConnection);
    QObject::connect(m_packetParser.data(), &QMqttPacketParser::unsuback,
                     this, &QMqttClientPrivate::onUnsubackReceived, Qt::QueuedConnection);
    QObject::connect(m_packetParser.data(), &QMqttPacketParser::puback,
                     this, &QMqttClientPrivate::onPubAckReceived, Qt::QueuedConnection);

    QObject::connect(&m_pingTimer, &QTimer::timeout,
                     this, &QMqttClientPrivate::sendPing, Qt::QueuedConnection);
    QObject::connect(m_packetParser.data(), &QMqttPacketParser::pong,
                     this, &QMqttClientPrivate::onPongReceived, Qt::QueuedConnection);

    //forward parser errors to user of QMqttClient
    QObject::connect(m_packetParser.data(), &QMqttPacketParser::error,
                     q, &QMqttClient::error, Qt::QueuedConnection);

    m_signalSlotConnected = true;
}

/*!
  Creates a new QMqttClient with the given \a clientId and \a parent.
  \a clientId should be a unique id representing the connection.
  The length of the \a clientId should be larger than smaller than 24 characters.
  If an empty \a clientId is provided, the server will generate a random one.
  \a allowedSslErrors: specify any SSL errors you want to allow
 */
QMqttClient::QMqttClient(const QString &clientId, const QSet<QSslError> &allowedSslErrors, QObject *parent) :
    QObject(parent),
    d_ptr(new QMqttClientPrivate(clientId, allowedSslErrors, this))
{
    qRegisterMetaType<QMqttProtocol::State>("QMqttProtocol::State");
}

/*!
  Destroys the QMqttClient. If a connection was open, the connection is aborted and the last will
  will be executed by the server.
  To have an orderly disconnection, disconnect() should be called prior to destroying the
  QMqttlient.

  \sa disconnect()
 */
QMqttClient::~QMqttClient()
{}

/*!
  Connects the QMqttClient to the server specified in the \a request.
  All HTTP headers present in the \a request will be sent to the server during the WebSocket
  handshake request.
  When the connection succeeds, a connected() signal will be emitted.

  During setup of the connection, the state of the client will change from DISCONNECTED over
  CONNECTING to CONNECTED.
  \a userName: specify the userName to be used with connect; connect QMqttPublishControlPacket
  does not contain this userName if it is empty
  \a password: specify the password to be used with connect; connect QMqttPublishControlPacket
  does not contain this password if it is null (use QByteArray() as a null password)

  \sa disconnect(), stateChanged()
 */
void QMqttClient::connect(const QMqttNetworkRequest &request, const QMqttWill &will, const QString &userName, const QByteArray &password)
{
    Q_D(QMqttClient);

    d->connect(request, will, userName, password);
}

/*!
  Disconnects the client from the server in an orderly manner.
  The client will first send a DISCONNECT packet to indicate to the server that it will close
  the connection soon.

  During tear down of the connection, the QMqttClient will change state from CONNECTED over
  DISCONNECTING to OFFLINE.

  \sa connect(), stateChanged()
 */
void QMqttClient::disconnect()
{
    Q_D(QMqttClient);

    d->disconnect();
}

/*!
  Subscribes the client to \a topic with the given Quality of Service \a qos.
  When subscription has finished, the given callback \a cb is called indicating whether the
  subscription succeeded or not.

  The \a topic must follow the following rules:
  \list
    \li Rule #1: If any part of the topic is not `+` or `#`, then it must not contain `+` and `#`
    \li Rule #2: Part `#` must be located at the end of the name
    \li Rule #3: The length must be at least 1
  \endlist

  If the \a topic is invalid, the callback will be called with false. The connection will not
  be dropped, as the check is done before it is sent to the server.

  The \a topic can contain the MQTT supported wildcard characters `#` and `+`.

  Example topic names:
  \list
  \li \c /: a single forward slash
  \li \c +: any single topic name
  \li \c {resources/+/weight}: matches resources/table/weight, resources/car/weight,
                          but not resources/weight nor resources/car/door/weight
  \li \c {resources/#}: matches all topics starting with resources/
  \endlist

  \sa unsubscribe()
*/
void QMqttClient::subscribe(const QString &topic, QMqttProtocol::QoS qos,
                           std::function<void (bool)> cb)
{
    Q_D(QMqttClient);

    d->subscribe(topic, qos, cb);
}

/*!
  Unsubscribes the client from the given \a topic. When unsubscription has finished, the
  callback \a cb will be called with the result.

  The same rules hold for the \a topic as for the subscribe() call.
  When an invalid \a topic is detected, the callback will be called with false. The connection
  will not be dropped, as the check is done before the request is sent to the server.

  \sa subscribe()
 */
void QMqttClient::unsubscribe(const QString &topic, std::function<void (bool)> cb)
{
    Q_D(QMqttClient);

    d->unsubscribe(topic, cb);
}

/*!
  Published the given \a message to the given \a topic with a QoS equal to AT_MOST_ONCE (0).
  Publishing an empty \a message is allowed, however the topic name should not be empty and
  should not contain wildcard characters.
  If the topic name is invalid, the connection will be dropped by the server.

  When an error occurs during publising, an error() signal will be emitted and errorString() will
  contain a description of the last error.
 */
void QMqttClient::publish(const QString &topic, const QByteArray &message)
{
    Q_D(QMqttClient);

    d->publish(topic, message);
}

/*!
  Published the given \a message to the given \a topic with a QoS equal to AT_LEAST_ONCE (1).
  When publication finished the given callback \a cb is called indicating success or failure.
  Publishing an empty \a message is allowed, however the topic name should not be empty and
  should not contain wildcard characters.
  If the topic name is invalid, the connection will be dropped by the server.

  When an error occurs during publising, an error() signal will be emitted and errorString() will
  contain a description of the last error.

  \note Exactly once delivery (EXACTLY_ONCE) is currently not supported.

  \overload publish()
 */
void QMqttClient::publish(const QString &topic, const QByteArray &message,
                         std::function<void (bool)> cb)
{
    Q_D(QMqttClient);

    d->publish(topic, message, cb);
}

/*!
 * Returns the local address
 */
QHostAddress QMqttClient::localAddress() const
{
    Q_D(const QMqttClient);


    return d->localAddress();
}

/*!
 * Returns the local port
 */
quint16 QMqttClient::localPort() const
{
    Q_D(const QMqttClient);

    return d->localPort();
}
