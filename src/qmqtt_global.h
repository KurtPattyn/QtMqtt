#pragma once

#include <QtCore/QtGlobal>

#ifndef QT_STATIC
#  if defined(QT_BUILD_QTMQTT_LIB)
#    define QTMQTT_EXPORT Q_DECL_EXPORT
#    if defined(PRIVATE_TESTS_ENABLED)
#      define QTMQTT_AUTOTEST_EXPORT Q_DECL_EXPORT
#    endif
#  else
#    define QTMQTT_EXPORT Q_DECL_IMPORT
#    define QTMQTT_AUTOTEST_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define QTMQTT_EXPORT
#  define QTMQTT_AUTOTEST_EXPORT
#endif
