#include "qmqttnetworkrequest.h"
#include <QUrl>
#include <QByteArray>
#include <QNetworkRequest>

/*!
   \class QMqttNetworkRequest

   \inmodule QtMqtt

    \brief Implements a QNetworkRequest to be used for the MQTT protocol over WebSockets.

    QMqttNetworkRequest inherits from QNetworkRequest and adds the mqtt http header to the
    request.
    Other headers can be added by calling setRawHeader() or by deriving from this class.

    \code
    QMqttNetworkRequest request(QUrl("https://mymqttserver"));
    request.setRawHeader("Authorization", "ABCDEFGHIJKL");
    \endcode

    The example above sets an extra "Authorization" header on the request.

    \note QMqttNetworkRequest does not support querystrings. This is due to a limitation in
    the implementation of QWebSocket.
 */

/*!
  Constructs a new QMqttNetworkRequest with the given \a url. The WebSocket sub protocol will be
  set to mqttv3.1 as required by the standard.
 */
QMqttNetworkRequest::QMqttNetworkRequest(const QUrl &url) :
    QNetworkRequest(url)
{
    setRawHeader(QByteArrayLiteral("Sec-WebSocket-Protocol"), QByteArrayLiteral("mqttv3.1"));
}

/*!
  Constructs a new default QMqttNetworkRequest. The WebSocket sub protocol will be
  set to mqttv3.1 as required by the standard.
  The default constructed request has no url and hence cannot be used open a connection,
  unless a url has been set through the setUrl() method.
 */
QMqttNetworkRequest::QMqttNetworkRequest() :
    QNetworkRequest()
{
    setRawHeader(QByteArrayLiteral("Sec-WebSocket-Protocol"), QByteArrayLiteral("mqttv3.1"));
}
