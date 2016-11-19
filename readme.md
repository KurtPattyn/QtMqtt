# QtMqtt

##### Author: [Kurt Pattyn](https://github.com/kurtpattyn).

QtMqtt is an implementation of the [MQTT](http://mqtt.org) protocol for [Qt](https://www.qt.io)-based projects.
It implements version 3.1.1 of the protocol.
QtMqtt only depends on Qt libraries.

## Building and installing

### Requirements
QtMqtt is created using Qt version 5.
QtMqtt uses CMake to build the sources. The minimum version of CMake is 3.5

### Building
Check out the sources.
In a terminal go to the directory where the sources are installed.

Create build directory

`mkdir build`

Go into the build directory

`cd build`

Create the build files

`cmake -DCMAKE_BUILD_TYPE=debug -DBUILD_SHARED_LIBS=OFF ..`

To make a release build, change `-DCMAKE_BUILD_TYPE=debug` to `-DCMAKE_BUILD_TYPE=release`

To make a dynamic library, change `-DBUILD_SHARED_LIBS=OFF` to `-DBUILD_SHARED_LIBS=ON`

Build the library

`make`

Execute unit tests

`make test`

To enable testing of internal code, add `-DPRIVATE_TESTS_ENABLED` (default: OFF) to the `cmake` command line.

Install the library

`make install`

Usage
=====
Include the following in your `CMakeLists.txt` file
```CMake
find_package(Qt5Mqtt)

target_link_libraries(<target> Qt5::Mqtt)
```

In your C++ source file include the QtMqtt module
```C++
#include <QtMqtt>
```

### Enabling debug output

If you want
`QT_LOGGING_RULES="QtMqtt.*.debug=true"`
