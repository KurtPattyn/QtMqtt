#pragma once

#include <QString>
#include <QByteArray>
#include <QScopedPointer>
#include "qmqttprotocol.h"
#include "qmqtt_global.h"

class QMqttWillPrivate
{
public:
    QMqttWillPrivate();
    QMqttWillPrivate(const QString &topic, const QByteArray &message, bool retain, QMqttProtocol::QoS qos);

public:
    QString m_topic;
    QByteArray m_message;
    bool m_valid;
    bool m_retain;
    QMqttProtocol::QoS m_qos;
};
