##---------------------------------------------------------------------------
## Author:      Steven Adler (based on standard OpenCPN Plug-In CMAKE commands)
## Copyright:   2018
## License:     GPL v3+
##---------------------------------------------------------------------------

# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(engine_dashboard_pi)

SET(PACKAGE_NAME engine_dashboard_pi)
SET(VERBOSE_NAME ENGINEDASHBOARD)
SET(TITLE_NAME ENGINEDASHBOARD)
SET(CPACK_PACKAGE_CONTACT "TwoCanPlugin@hotmail.com")
# SET(PLUGIN_VERSION_MAJOR "1")
# SET(PLUGIN_VERSION_MINOR "1")
# SET(PLUGIN_VERSION_PATCH "0")
# SET(NAME_SUFFIX "ov50")
# SET(PLUGIN_VERSION_DATE "2019-11-23")
SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_STANDARD 11)

INCLUDE("cmake/PluginConfigure.cmake")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/nmea0183 ${CMAKE_SOURCE_DIR}/src)

SET(SRC_DASHBOARD
    src/dashboard_pi.h
    src/dashboard_pi.cpp
    src/instrument.cpp
    src/instrument.h
    src/dial.cpp
    src/dial.h
    src/rudder_angle.cpp
    src/rudder_angle.h
    src/speedometer.cpp
    src/speedometer.h
    src/icons.cpp
    src/icons.h
)

SET(SRC_NMEA0183
    src/nmea0183/nmea0183.cpp
    src/nmea0183/nmea0183.hpp
    src/nmea0183/response.hpp
    src/nmea0183/response.cpp
    src/nmea0183/sentence.hpp
    src/nmea0183/sentence.cpp
    src/nmea0183/talkerid.cpp
    src/nmea0183/hexvalue.cpp
    src/nmea0183/expid.cpp
    src/nmea0183/rpm.hpp
    src/nmea0183/rsa.cpp
    src/nmea0183/rsa.hpp
    src/nmea0183/rpm.cpp
    src/nmea0183/xdr.cpp
    src/nmea0183/xdr.hpp
)

ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_DASHBOARD} ${SRC_NMEA0183} )


INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")

