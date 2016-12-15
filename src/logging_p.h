#pragma once

#include <QLoggingCategory>
#include <QDebug>

#define LoggingModule(moduleName) \
    static const QLoggingCategory module("QtMqtt." moduleName, QtWarningMsg)
