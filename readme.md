# QtMqtt
[![License][license-image]][license-url]
[![Linux/OS X Build Status][travis-image]][travis-url]
[![Windows Build Status][appveyor-image]][appveyor-url]

##### Author: [Kurt Pattyn](https://github.com/kurtpattyn).

QtMqtt is an implementation of the [MQTT](http://mqtt.org) protocol for [Qt](https://www.qt.io)-based projects.
It implements version 3.1.1 of the protocol.
QtMqtt only depends on Qt libraries.

## Building and installing

### Requirements
QtMqtt is created using Qt version 5.  
QtMqtt uses CMake to build the sources. The minimum version of CMake is 3.5

### Building
```bash
#Create a directory to hold the sources.
mkdir qtmqtt
cd qtmqtt

#Check out the sources.
git clone...

#Create build directory  
mkdir build

#Go into the build directory  
cd build

#Create the build files  
cmake -DCMAKE_BUILD_TYPE=debug -DBUILD_SHARED_LIBS=OFF ..

#To make a release build, change `-DCMAKE_BUILD_TYPE=debug` to `-DCMAKE_BUILD_TYPE=release`  
#To make a dynamic library, change `-DBUILD_SHARED_LIBS=OFF` to `-DBUILD_SHARED_LIBS=ON`

#Build the library  
make
```
### Run unit tests
`make test`

> To enable testing of internal code, add `-DPRIVATE_TESTS_ENABLED` (default: OFF) to the `cmake` command line.

### Installing
`make install`

> This will install the library and the headers to `CMAKE_INSTALL_PREFIX`.  
> `CMAKE_INSTALL_PREFIX` defaults to `/usr/local` on UNIX/macOS and `c:/Program Files` on Windows.  
> To install in another location, add `-DCMAKE_INSTALL_PREFIX="<custom location>"` to the `cmake` command line.

## Usage
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

To enable debugging information of QtMqtt, the following environment variable can be defined.    
`QT_LOGGING_RULES="QtMqtt.*.debug=true"`

## License

  [MIT](LICENSE)


[license-image]: http://img.shields.io/badge/license-MIT-blue.svg?style=flat
[license-url]: LICENSE
[travis-image]: https://travis-ci.org/KurtPattyn/QtMqtt.svg?branch=develop
[travis-url]: https://travis-ci.org/KurtPattyn/QtMqtt
[appveyor-image]: https://ci.appveyor.com/api/projects/status/4tmm94uvuwscadsv?svg=true
[appveyor-url]: https://ci.appveyor.com/project/KurtPattyn/qtmqtt
