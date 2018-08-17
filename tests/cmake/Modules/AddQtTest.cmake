macro(add_qt_test TEST_NAME SRCS)
    find_package(Qt5Test REQUIRED)

    add_executable(${TEST_NAME} ${SRCS})

    add_test(NAME tst_${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)

    target_link_libraries(${TEST_NAME} PUBLIC Qt5::Test)
endmacro()

macro(add_private_qt_test TEST_NAME SRCS)
    if(DEFINED PRIVATE_TESTS_ENABLED)
        if(${PRIVATE_TESTS_ENABLED})
            add_qt_test(${TEST_NAME} ${SRCS})
        endif(${PRIVATE_TESTS_ENABLED})
    endif(DEFINED PRIVATE_TESTS_ENABLED)
endmacro()

