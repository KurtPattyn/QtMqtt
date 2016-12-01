#include "qmqttwill.h"
#include "qmqttwill_p.h"

QMqttWillPrivate::QMqttWillPrivate() :
    m_topic(), m_message(), m_valid(false), m_retain(false),
    m_qos(QMqttProtocol::QoS::AT_MOST_ONCE)
{}

QMqttWillPrivate::QMqttWillPrivate(const QString &topic, const QByteArray &message,
                                   bool retain, QMqttProtocol::QoS qos) :
    m_topic(topic), m_message(message), m_valid(true), m_retain(retain), m_qos(qos)
{}

QMqttWill::QMqttWill() :
    d_ptr(new QMqttWillPrivate())
{}

QMqttWill::QMqttWill(const QString &topic, const QByteArray &message,
                     bool retain, QMqttProtocol::QoS qos) :
    d_ptr(new QMqttWillPrivate(topic, message, retain, qos))
{}

QMqttWill::QMqttWill(const QMqttWill &other) :
    d_ptr(new QMqttWillPrivate(other.topic(), other.message(), other.retain(), other.qos()))
{
}

QMqttWill::QMqttWill(QMqttWill &&other) :
    d_ptr(other.d_ptr.take())
{
}

QMqttWill::~QMqttWill()
{
}

QMqttWill &QMqttWill::operator =(const QMqttWill &other)
{
    Q_D(QMqttWill);
    d->m_message = other.message();
    d->m_qos = other.qos();
    d->m_retain = other.retain();
    d->m_topic = other.topic();
    d->m_valid = other.isValid();

    return *this;
}

QMqttWill &QMqttWill::operator =(QMqttWill &&other)
{
    swap(other);
    return *this;
}

void QMqttWill::swap(QMqttWill &other)
{
    qSwap(d_ptr, other.d_ptr);
}

bool QMqttWill::isValid() const
{
    Q_D(const QMqttWill);
    return d->m_valid;
}

bool QMqttWill::retain() const
{
    Q_D(const QMqttWill);
    return d->m_retain;
}

QMqttProtocol::QoS QMqttWill::qos() const
{
    Q_D(const QMqttWill);
    return d->m_qos;
}

QString QMqttWill::topic() const
{
    Q_D(const QMqttWill);
    return d->m_topic;
}

QByteArray QMqttWill::message() const
{
    Q_D(const QMqttWill);
    return d->m_message;
}
