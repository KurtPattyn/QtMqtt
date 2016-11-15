#pragma once

#include <QNetworkRequest>
#include "qmqtt_global.h"

class QUrl;
class QTMQTT_EXPORT QMqttNetworkRequest: public QNetworkRequest
{
public:
    QMqttNetworkRequest(const QUrl &url);
    QMqttNetworkRequest();
};
