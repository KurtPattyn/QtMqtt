cmake_minimum_required(VERSION 3.5.2 FATAL_ERROR)

###############################
# Project specific parameters #
# Change as you see fit       #
###############################
set(PROJECT_NAME_PREFIX Qt)
set(PROJECT_BASE_NAME Mqtt)
set(FULL_VERSION 1.0.0)
set(SO_VERSION 1)
set(QT_MAJOR_VERSION_REQUIRED 5)

#################
# Build options #
#################
# When set to ON, the library will be built as a static library
option(QTMQTT_STATIC_BUILD "Build static library" OFF)

#####################################################
# Start of configuration                            #
# Lines below should not be touched in normal cases #
#####################################################

#=======================================
# Set some variables to be used later on
#=======================================
# Point CMake search path to Qt intallation directory
# Either supply QTDIR as -DQTDIR=<path> to cmake or set an environment variable QTDIR pointing to the Qt installation
if ((NOT DEFINED QTDIR) AND DEFINED ENV{QTDIR})
  set(QTDIR $ENV{QTDIR})
endif ((NOT DEFINED QTDIR) AND DEFINED ENV{QTDIR})

set(PROJECT_NAME ${PROJECT_NAME_PREFIX}${PROJECT_BASE_NAME})
set(TARGET_NAME ${PROJECT_BASE_NAME})
set(PROJECT_NAMESPACE ${PROJECT_NAME_PREFIX}${QT_MAJOR_VERSION_REQUIRED})
set(INSTALL_DIRECTORY_NAME ${PROJECT_NAME}/Qt${QT_MAJOR_VERSION_REQUIRED})
set(CMAKE_DIRECTORY_NAME ${PROJECT_NAME_PREFIX}${QT_MAJOR_VERSION_REQUIRED}${PROJECT_BASE_NAME})
set(PACKAGE_NAME ${PROJECT_NAME_PREFIX}${QT_MAJOR_VERSION_REQUIRED}${PROJECT_BASE_NAME})
set(MODULE_NAME ${PROJECT_NAME})
#base name used for cmake config files:
#<CMAKE_CONFIG_FILE_BASE_NAME>Config.cmake
#<CMAKE_CONFIG_FILE_BASE_NAME>ConfigVersion.cmake
#<CMAKE_CONFIG_FILE_BASE_NAME>Targets.cmake
#<CMAKE_CONFIG_FILE_BASE_NAME>Targets_noconfig.cmake
set(CMAKE_CONFIG_FILE_BASE_NAME ${PROJECT_NAME_PREFIX}${QT_MAJOR_VERSION_REQUIRED}${PROJECT_BASE_NAME})

set(LIB_INSTALL_DIR lib/${INSTALL_DIRECTORY_NAME})
set(INCLUDE_INSTALL_DIR include/${INSTALL_DIRECTORY_NAME})
set(BIN_INSTALL_DIR bin)
set(CMAKE_INSTALL_DIR lib/cmake/${CMAKE_DIRECTORY_NAME})

project(${PROJECT_NAME})

#================================
# Check build pre-requisites
# Fail the build if not fulfilled
#================================
if (NOT DEFINED QTDIR)
  message(FATAL_ERROR "QTDIR has not been set nor supplied as a define parameter to cmake.")
endif (NOT DEFINED QTDIR)

if (QTDIR)
  list (APPEND CMAKE_PREFIX_PATH ${QTDIR})
endif (QTDIR)

#include required CMake modules
include(CMakePackageConfigHelpers)

#include required Qt libraries and include directories
find_package(Qt${QT_MAJOR_VERSION_REQUIRED}Core REQUIRED)
find_package(Qt${QT_MAJOR_VERSION_REQUIRED}Network REQUIRED)
find_package(Qt${QT_MAJOR_VERSION_REQUIRED}WebSockets REQUIRED)

#==============================
# Add compiler and linker flags
#==============================

if (NOT ${BUILD_SHARED_LIBS})
    set(QTMQTT_STATIC_BUILD_LIB ON)
    add_definitions(-DQTMQTT_STATIC_BUILD_LIB)
endif()

export(PACKAGE ${PACKAGE_NAME})

add_definitions(-Wall -fvisibility=hidden)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Compile with @rpath option on Apple
if (APPLE)
  set(CMAKE_MACOSX_RPATH 1)
endif (APPLE)

enable_testing()

message(STATUS "Libraries will be installed to: ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}")
message(STATUS "Header files will be installed to: ${CMAKE_INSTALL_PREFIX}/${INCLUDE_INSTALL_DIR}")
message(STATUS "Executables will be installed in: ${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}")
message(STATUS "CMake config-files will be written to: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DIR}")
message(STATUS "${PROJECT_NAME} import target: ${PROJECT_NAMESPACE}::${TARGET_NAME}")
message(STATUS "Building ${PROJECT_NAME} ${FULL_VERSION} in ${CMAKE_BUILD_TYPE} mode")
if (PRIVATE_TESTS_ENABLED)
  message(STATUS "Private tests are enabled")
endif()
add_subdirectory(src)
add_subdirectory(tests)
