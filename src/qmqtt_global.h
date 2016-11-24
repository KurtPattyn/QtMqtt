#pragma once

#include <QtCore/QtGlobal>

#ifndef QTMQTT_STATIC_BUILD_LIB
#  if defined(QTMQTT_LIBRARY)
#    define QTMQTT_EXPORT Q_DECL_EXPORT
#    ifdef PRIVATE_TESTS_ENABLED
#      define QTMQTT_AUTOTEST_EXPORT Q_DECL_EXPORT
#    else
#      define QTMQTT_AUTOTEST_EXPORT
#    endif
#  else
#    define QTMQTT_EXPORT Q_DECL_IMPORT
#    ifdef PRIVATE_TESTS_ENABLED
#      define QTMQTT_AUTOTEST_EXPORT Q_DECL_IMPORT
#    else
#      define QTMQTT_AUTOTEST_EXPORT
#    endif
#  endif
#else
#  define QTMQTT_EXPORT
#  define QTMQTT_AUTOTEST_EXPORT
#endif
