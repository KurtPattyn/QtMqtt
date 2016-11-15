#pragma once

#include <QObject>
#include "qmqtt_global.h"

//a class is used instead of a namespace so that the enumerations can be marked with
//Q_ENUM.
class QTMQTT_EXPORT QMqttProtocol: public QObject
{
    Q_OBJECT

public:
    //see 4.3 Quality of Service levels and protocol flows in MQTT v3.1.1 specification
    enum class QoS : uint8_t
    {
        AT_MOST_ONCE  = 0,
        AT_LEAST_ONCE = 1,
        EXACTLY_ONCE  = 2,
        INVALID       = 3
    };
    Q_ENUM(QoS)

    enum class Error {
        //MQTT specified errors
        CONNECTION_ACCEPTED = 0,
        CONNECTION_REFUSED_UNACCEPTABLE_PROTOCOL = 1,
        CONNECTION_REFUSED_IDENTIFIER_REJECTED = 2,
        CONNECTION_REFUSED_SERVER_UNAVAILABLE = 3,
        CONNECTION_REFUSED_BAD_USERNAME_OR_PASSWORD = 4,
        CONNECTION_REFUSED_NOT_AUTHORIZED = 5,

        //implementation defined errors
        OK,
        INVALID_PACKET,
        PROTOCOL_VIOLATION,
        PARSE_ERROR,
        TIME_OUT,
        CONNECTION_FAILED
    };
    Q_ENUM(Error)

    enum State {
        OFFLINE,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    };
    Q_ENUM(State)
};
