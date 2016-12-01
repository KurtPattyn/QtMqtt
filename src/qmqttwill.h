#pragma once

#include <QString>
#include <QByteArray>
#include <QScopedPointer>
#include "qmqttprotocol.h"
#include "qmqtt_global.h"

class QMqttWillPrivate;
class QTMQTT_EXPORT QMqttWill
{
    Q_DECLARE_PRIVATE(QMqttWill)

public:
    QMqttWill();
    QMqttWill(const QString &topic, const QByteArray &message, bool retain, QMqttProtocol::QoS qos);

    QMqttWill(const QMqttWill &other);
    QMqttWill(QMqttWill &&other);

    ~QMqttWill();

    QMqttWill &operator =(const QMqttWill &other);
    QMqttWill &operator =(QMqttWill &&other);

    void swap(QMqttWill &other);

    bool isValid() const;
    bool retain() const;
    QMqttProtocol::QoS qos() const;
    QString topic() const;
    QByteArray message() const;

private:
    QScopedPointer<QMqttWillPrivate> d_ptr;
};
