# QtMqtt

##### Author: [Kurt Pattyn](https://github.com/kurtpattyn).

QtMqtt is an implementation of the [MQTT](http://mqtt.org) protocol for [Qt](https://www.qt.io)-based projects.
It implements version 3.1.1 of the protocol.
QtMqtt only depends on Qt libraries.

## Motivation


## Installation



Build Requirements
==================
QtMqtt is created using Qt version 5.
QtMqtt uses CMake to build the sources. The minimum version of CMake is 3.5

Building and installing
=======================
Check out the sources.
In a terminal go to the directory where the sources are installed.

Create build directory:
mkdir build

Go into the build directory:
cd build

Create the build files:
cmake -DCMAKE_BUILD_TYPE=debug -DBUILD_SHARED_LIBS=OFF ..

To make a release build, change _DCMAKE_BUILD_TYPE=debug to -DCMAKE_BUILD_TYPE=release
To make a dynamic library, change -DBUILD_SHARED_LIBS=OFF to -DBUILD_SHARED_LIBS=ON

Build the library:
make

Execute unit tests:
make test

-DPRIVATE_TESTS_ENABLED
(default: OFF)

Install the library:
make install

Usage
=====
find_package(Qt5Mqtt)

target_link_libraries(<target> Qt5::Mqtt)

config-file package: see: https://cmake.org/cmake/help/v3.5/manual/cmake-packages.7.html#config-file-packages

#include <QtMqtt>
