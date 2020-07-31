# ---------------------------------------------------------------------------
# Author:      Steven Adler based on (jongough aka. Jon Gough) Copyright: License:     wxWidgets License
# ---------------------------------------------------------------------------

## ----- When changing this file do NOT change the order in which sections occur        ----- ##
## ----- Changes should only be made between the section blocks that identify where     ----- ##
## ----- these changes should be. The whole configuration process relies on this        ----- ##
## ----- sequence to be successful                                                      ----- ##
##

##
## ----- Modify section below to include all the details for your plugin ----- ##
##

set(CMLOC "CMakeLists: ")

# define minimum cmake version
cmake_minimum_required(VERSION 3.1.1)
if(COMMAND cmake_policy)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif(POLICY CMP0043)
    cmake_policy(SET CMP0048 NEW)
    if(POLICY CMP0077)
        cmake_policy(SET CMP0077 NEW)
    endif(POLICY CMP0077)
endif(COMMAND cmake_policy)


# define plugin name, owner and versions
set(VERBOSE_NAME "Engine_Dashboard")
set(COMMON_NAME "Engine Dashboard")
set(TITLE_NAME "ENGINEDASHBOARD")
set(PACKAGE_CONTACT "twocanplugin@hotmail.com")
set(PACKAGE "engine_dashboard")
set(SHORT_DESCRIPTION "Engine Dashboard, Displays Engine parameters.")
set(LONG_DESCRIPTION "Engine Dashboard, Displays Engine RPM, Oil Pressure, Water Temperature, Alternator Output and tank levels.") 
set(VERSION_MAJOR "1")
set(VERSION_MINOR "2")
set(VERSION_PATCH "0")
set(VERSION_TWEAK "0")
set(VERSION_DATE "14/07/2020")
set(OCPN_MIN_VERSION "ov50")
set(OCPN_API_VERSION_MAJOR "1")
set(OCPN_API_VERSION_MINOR "16")
set(TP_COMMENT "Release for O5 using CI")

set(PARENT "opencpn")

# The next line allows for setting up your own local webserver with git on it for testing purposes, the default is github.com
set(GIT_REPOSITORY_SERVER "github.com")

# Specifies cloudsmith user and repository names (note suffixes for repositories production, beta & alpha)
set(PROD "prod") # Standard Repos
set(BETA "beta") # Standard Repos
set(ALPHA "alpha") # Standard Repos

set(CLOUDSMITH_USER "steven-adler")
# set this if your cloudsmith repository name does not match your git repository name 
set(CLOUDSMITH_BASE_REPOSITORY "engineplugin")

set(XML_INFO_URL "https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:developer_manual:plugins:beta_plugins:engine-dash")
set(XML_SUMMARY ${SHORT_DESCRIPTION})
set(XML_DESCRIPTION ${LONG_DESCRIPTION})

##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##

## Engine Dashboard (I think) uses SVG graphics for the toolbar icons
option(PLUGIN_USE_SVG "Use SVG graphics" ON)

set(CMAKE_CXX_STANDARD 11)

# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")
# Don't use local version of GLU library
set(USE_LOCAL_GLU FALSE)
option(USE_GL "Enable OpenGL support" OFF)
message(STATUS "${CMLOC}USE_GL: ${USE_GL}")

## Define the build type
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Choose the type of build" FORCE)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "" "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
message(STATUS "${CMLOC}Build type: ${CMAKE_BUILD_TYPE}")


project(${PACKAGE})

include("cmake/PluginSetup.cmake")

##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##

set(CMAKE_VERBOSE_MAKEFILE ON)

option(Plugin_CXX11 "Use c++11" OFF)


##
## ----- do not change next section - needed to configure build process ----- ##
##

include("cmake/PluginConfigure.cmake")

##
## ----- Change below to match project requirements for source, headers, etc. ----- ##
##

## Engine Dashboard Plugin does not understand/use this definition
##add_definitions(-DUSE_S57)

##Engine Dashboard plugin uses SVG for toolbar icons
add_definitions(-DPLUGIN_USE_SVG)

## We don't use JSON
##if(APPLE)
##    option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" OFF)
##else(APPLE)
##    option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" ON)
##endif(APPLE)

## Need api-16 for ocpn_plugin.h
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-16)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/nmea0183/inc ${CMAKE_SOURCE_DIR}/inc)

SET(SRC_DASHBOARD
    #inc/dashboard_pi.h
    src/dashboard_pi.cpp
    src/instrument.cpp
    #inc/instrument.h
    src/dial.cpp
    #inc/dial.h
    src/rudder_angle.cpp
    #inc/rudder_angle.h
    src/speedometer.cpp
    #inc/speedometer.h
    src/icons.cpp
    #inc/icons.h
)


SET(SRC_NMEA0183
    nmea0183/src/nmea0183.cpp
    #nmea0183/inc/nmea0183.hpp
    nmea0183/src/response.cpp
    #nmea0183/inc/response.hpp
    nmea0183/src/sentence.cpp
    #nmea0183/inc/sentence.hpp
    nmea0183/src/rsa.cpp
    #nmea0183/inc/rpm.hpp
    nmea0183/src/rpm.cpp
    #nmea0183/inc/rsa.hpp
    nmea0183/src/xdr.cpp
    #nmea0183/inc/xdr.hpp
    nmea0183/src/talkerid.cpp
    nmea0183/src/hexvalue.cpp
    nmea0183/src/expid.cpp
)


ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_DASHBOARD} ${SRC_NMEA0183} )

##
## ----- do not change next section - needed to configure build process ----- ##
##

include("cmake/PluginInstall.cmake")
include("cmake/PluginLocalization.cmake")
include("cmake/PluginPackage.cmake")

