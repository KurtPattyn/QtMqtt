# qmqttnetworkrequest
add_qt_test(qmqttnetworkrequest tst_qmqttnetworkrequest.cpp)
target_link_libraries(qmqttnetworkrequest PUBLIC Mqtt)

# qmqttprotocol
add_qt_test(qmqttprotocol tst_qmqttprotocol.cpp)
target_link_libraries(qmqttprotocol PUBLIC Mqtt)

# qmqttcontrolpacket
if(DEFINED PRIVATE_TESTS_ENABLED)
    if(${PRIVATE_TESTS_ENABLED})
        add_qt_test(qmqttcontrolpacket tst_qmqttcontrolpacket.cpp)
        target_link_libraries(qmqttcontrolpacket PUBLIC Mqtt)
    endif(${PRIVATE_TESTS_ENABLED})
endif(DEFINED PRIVATE_TESTS_ENABLED)
