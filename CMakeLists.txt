# ---------------------------------------------------------------------------
# Frontend2 Author:   jongough aka. Jon Gough Copyright: License:  wxWidgets
# License
# ---------------------------------------------------------------------------
# - Copyright (C)  by Jon Gough                            *
# - License:     GPLv3+
# - This program is free software; you can redistribute it and/or modify  *
# - it under the terms of the GNU General Public License as published by  *
# - the Free Software Foundation; either version 2 of the License, or     *
# - (at your option) any later version.                                   *
# - *
# - This program is distributed in the hope that it will be useful,       *
# - but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# - GNU General Public License for more details.                          *
# - *
# - You should have received a copy of the GNU General Public License     *
# - along with this program; if not, write to the                         *
# - xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,                                       *
# - xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx                                       *
# -----------------------------------------------------------------------------
# 
# ---------------------------------------------------------------------------
# -- When changing this file do NOT change the order of the sections.    -- ##
# -- Make changes only between the section blocks identified             -- ##
# -- The whole configuration process relies on this sequence for success.-- ##
#
# -- Modify section below to include all the details for your plugin.    -- ##

set(CMLOC "CMakeLists: ")

# define minimum cmake version
cmake_minimum_required(VERSION 3.5.1)
if (COMMAND cmake_policy)
  if (POLICY CMP0043)
    cmake_policy(SET CMP0043 NEW)
  endif (POLICY CMP0043)
  cmake_policy(SET CMP0048 NEW)
  if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
  endif (POLICY CMP0076)
  if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
  endif (POLICY CMP0077)
endif (COMMAND cmake_policy)

# set for vscode/vscodium
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# define plugin name, owner and versions
set(VERBOSE_NAME "Engine_Dashboard")
set(COMMON_NAME "Engine Dashboard")
set(TITLE_NAME "ENGINEDASHBOARD")
set(PACKAGE_CONTACT "twocanplugin@hotmail.com")
set(PACKAGE "engine_dashboard")
set(SHORT_DESCRIPTION "Engine Dashboard, Displays Engine parameters.")
set(LONG_DESCRIPTION "Engine Dashboard, Displays Engine RPM, Oil pressure, Water temperature, Alternator output and Tank levels.") 
set(VERSION_MAJOR "1")
set(VERSION_MINOR "9")
set(VERSION_PATCH "0")
set(VERSION_TWEAK "0")
set(VERSION_DATE "01/12/2024")
set(OCPN_MIN_VERSION "ov50")
set(OCPN_API_VERSION_MAJOR "1")
set(OCPN_API_VERSION_MINOR "18")
set(TP_COMMENT "  * Release using CI")

set(PARENT "opencpn")

# Use to set up a local webserver with git installed. Default is github.com. Now
# set in pluginconfigure.cmake set(GIT_REPOSITORY_SERVER "github.com")

# Specifies Cloudsmith upload repository suffix for each catalog
set(PROD "prod") # Standard Repos
set(BETA "beta") # Standard Repos
set(ALPHA "alpha") # Standard Repos

# Set if  Cloudsmith Base Repository name does not match your Git Repository
# name. set(CLOUDSMITH_BASE_REPOSITORY "${GIT_REPOSITORY_NAME}")
set(CLOUDSMITH_USER "steven-adler")

# set this if your cloudsmith repository name does not match your git repository name 
set(CLOUDSMITH_BASE_REPOSITORY "engineplugin")

set(XML_INFO_URL "https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:developer_manual:plugins:beta_plugins:engine-dash")
set(XML_SUMMARY ${SHORT_DESCRIPTION})
set(XML_DESCRIPTION ${LONG_DESCRIPTION})

# ----- Modify section above to include all the details for your plugin
#
# ----- Modify section below if there are special requirements for the plugin
#

option(PLUGIN_USE_SVG "Use SVG graphics" ON)

set(CMAKE_CXX_STANDARD 11)

# Use local version of GLU library requires libs/glu directory
set(USE_LOCAL_GLU FALSE)
# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY") # not operational

option(USE_GL "Enable OpenGL support" ON)
message(STATUS "${CMLOC}USE_GL: ${USE_GL}")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build" FORCE
  )
endif ("${CMAKE_BUILD_TYPE}" STREQUAL "")
set_property(
  CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "" "Debug" "Release" "RelWithDebInfo"
                                  "MinSizeRel"
)
message(STATUS "${CMLOC}Build type: ${CMAKE_BUILD_TYPE}")

# ----- Statements below used to setup standard variables required by the CMAKE
# ----- process -- Do not remove --

project(${PACKAGE})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ========================================================

include("PluginSetup")

# ========================================================
# ----- Above sets up standard variables required by CMAKE - Do not remove

# ----- Modify section below if there are special requirements for the plugin

set(CMAKE_VERBOSE_MAKEFILE "Activate verbose mode for make files" ON)

option(Plugin_CXX11 "Use c++11" OFF)

# ----- Modify section above if there are special requirements for the plugin

# ----- Do not change next section - needed to configure build process
# ===========================================================

include("PluginConfigure")

# ===========================================================
# ----- Do not change section above - needed to configure build process

# ----- Change below to match project requirements for source, headers, etc.

add_definitions(-DUSE_S57)

# ---- Section Below - Add your project SET(SRCS and SET(HDRS  etc..

SET(SRC_DASHBOARD
    src/dashboard_pi.cpp
    src/instrument.cpp
    src/dial.cpp
    src/rudder_angle.cpp
    src/speedometer.cpp
    src/icons.cpp)

SET(INC_DASHBOARD
    inc/dashboard_pi.h
    inc/instrument.h
    inc/dial.h
    inc/rudder_angle.h
    inc/speedometer.h
    inc/icons.h)


SET(SRC_NMEA0183
    nmea0183/src/nmea0183.cpp 
    nmea0183/src/response.cpp
    nmea0183/src/sentence.cpp
    nmea0183/src/rsa.cpp    
    nmea0183/src/rpm.cpp
    nmea0183/src/xdr.cpp
    nmea0183/src/talkerid.cpp
    nmea0183/src/hexvalue.cpp
    nmea0183/src/expid.cpp
    )

SET(INC_NMEA0183
    nmea0183/inc/nmea0183.hpp
    nmea0183/inc/response.hpp
    nmea0183/inc/sentence.hpp
    nmea0183/inc/rpm.hpp
    nmea0183/inc/rsa.hpp
    nmea0183/inc/xdr.hpp)


add_definitions(-DPLUGIN_USE_SVG)

# ----- Section above - Add your project SET(SRCS and SET(HDRS  etc.

# ----- If using JSON validation in plugin section below is needed

if (APPLE)
  option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" OFF)
else (APPLE)
  option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" ON)
endif (APPLE)

set(PI_JSON_SCHEMA_VALIDATOR OFF)

if (PI_JSON_SCHEMA_VALIDATOR)
  message(
    STATUS
      "${CMLOC}Using JSON Schema validation - warning this changes the 'flavour' of the executable and it may not be compatible with OpenCPN"
  )
  add_definitions(-DPI_JSON_SCHEMA_VALIDATOR)
  message(STATUS "${CMLOC}Adding JSON Schema validation")

  add_subdirectory(opencpn-libs/json-schema-validator)
  target_link_libraries(${PACKAGE_NAME} ocpn::json-schema-validator)

else (PI_JSON_SCHEMA_VALIDATOR)
  message(
    STATUS
      "${CMLOC}Not using JSON Schema validation - warning invalid JSON 'may' cause unexpected crashes"
  )
endif (PI_JSON_SCHEMA_VALIDATOR)

# ----- If using JSON validation in plugin section above is needed

if (QT_ANDROID)

endif (QT_ANDROID)

# =============================================================
# Change as required to collect all the set( headers and SRCS This add_library
# can be used in conditionals below.
# =============================================================

set(SRC ${SRC_DASHBOARD} ${INC_DASHBOARD} ${SRC_NMEA0183} ${INC_NMEA0183})

# =============================================================
# Modify/configure 'Include' Directories below for the Plugin
# =============================================================

include_directories(BEFORE ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/nmea0183/inc)

# Change target_sources to match your project needs.
# ==============================================================

target_sources(${PACKAGE_NAME} PUBLIC ${SRC})

# ==============================================================

if (NOT OCPN_FLATPAK_CONFIG)
  # Build environment not available when flatpak is being configured
  # so following statements will not work
  message(STATUS "${CMLOC}Adding target link libraries to ${PACKAGE_NAME}")

  if (WIN32)
    add_subdirectory(opencpn-libs/WindowsHeaders)
    target_link_libraries(${PACKAGE_NAME} windows::headers)
  endif (WIN32)

  add_subdirectory(opencpn-libs/api-18)
  target_link_libraries(${PACKAGE_NAME} ocpn::api)

  add_subdirectory(opencpn-libs/wxJSON)
  target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

 endif (NOT OCPN_FLATPAK_CONFIG)

add_definitions(-DTIXML_USE_STL)

#
# ----- If using JSON validation section below is needed
#

if (UNIX
    AND NOT APPLE
    AND NOT QT_ANDRIOD
)
  if (PI_JSON_SCHEMA_VALIDATOR)
    # Change OS/ABI to be UNIX - System V to make it run with OCPN
    set(lib_name lib${PACKAGE_NAME}.so)
    message(STATUS "${CMLOC}lib-name: ${lib_name}")
    find_file(elfedit elfedit)
    if (NOT (${elfedit} STREQUAL "elfedit-NOTFOUND"))
      message(STATUS "${CMLOC}Will ensure OS/ABI is set to 0 to match OCPN")
      add_custom_command(
        TARGET ${PACKAGE_NAME}
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${PACKAGE_NAME}
        COMMENT " Running post build action on ${lib_name}."
        COMMAND
          sh -c
          'elfedit --input-osabi=Linux --output-osabi=none ${lib_name} || (echo 0) '
      )
    else (NOT (${elfedit} STREQUAL "elfedit-NOTFOUND"))
      message(
        STATUS
          "${CMLOC}Cannot correct OS/ABI to match OCPN if it is generated incorrectly. Package binutils needed"
      )
    endif (NOT (${elfedit} STREQUAL "elfedit-NOTFOUND"))
  endif (PI_JSON_SCHEMA_VALIDATOR)
endif (
  UNIX
  AND NOT APPLE
  AND NOT QT_ANDRIOD
)

# ------- Change below to match project requirements

# Needed for android builds
if (QT_ANDROID)
  include_directories(BEFORE ${qt_android_include})
endif (QT_ANDROID)

# Needed for all builds Do not change - needed to build app
# ===============================================================

include("PluginInstall")
include("PluginLocalization")
include("PluginPackage")

# ================================================================
# ----- Do not change section above - needed to configure build process

message(STATUS "${CMLOC}include directories: ")
get_property(
  dirs
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY INCLUDE_DIRECTORIES
)
foreach (dir ${dirs})
  message(STATUS "${CMLOC}dir='${dir}'")
endforeach ()
