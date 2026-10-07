# ~~~
# Summary:      Local, generic plugin setup
# Copyright (c) 2020-2021 Mike Rossiter
# License:      GPLv3+
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


# -------- Options ----------

set(OCPN_TEST_REPO
    "twocanplugin/engineplugin-alpha"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "twocanplugin/engineplugin-beta"
    CACHE STRING
    "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "twocanplugin/engineplugin-prod"
    CACHE STRING
    "Default repository for tagged builds not matching 'beta'"
)

#
#
# -------  Plugin setup --------
#
set(PKG_NAME engine_dashboard_pi) #or _pi
set(PKG_VERSION  2.0)
set(PKG_PRERELEASE "")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME Engine Dashboard)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME Engine_Dashboard) # As of GetCommonName() in plugin API
set(PKG_SUMMARY "Engine Dashboard")
set(PKG_DESCRIPTION "Displays Engine RPM, Oil pressure, Water temperature, Alternator output and Tank levels")
set(PKG_AUTHOR "twocanplugin@hotmail.com")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE https://https://github.com/twocanplugin/enginedashboard)
set(PKG_INFO_URL https://opencpn-manuals.github.io/main/engine-dash/0.1/index.html)

set(PKG_API_LIB api-20)   # Specify the plugin API level

add_definitions(-DocpnUSE_GL) # Specifiy the use of OpenGL

include_directories(${CMAKE_SOURCE_DIR}/inc)

set(SRC
${CMAKE_SOURCE_DIR}/src/add_instrument_dlg.cpp
${CMAKE_SOURCE_DIR}/src/dashboard_globals.cpp
${CMAKE_SOURCE_DIR}/src/dashboard_pi.cpp
${CMAKE_SOURCE_DIR}/src/dashboard_preferences_dialog.cpp
${CMAKE_SOURCE_DIR}/src/dashboard_window.cpp
${CMAKE_SOURCE_DIR}/src/dial.cpp
${CMAKE_SOURCE_DIR}/src/edit_dialog.cpp
${CMAKE_SOURCE_DIR}/src/instrument.cpp
${CMAKE_SOURCE_DIR}/src/ocpn_font_button.cpp
${CMAKE_SOURCE_DIR}/src/rudder_angle.cpp
${CMAKE_SOURCE_DIR}/src/speedometer.cpp

)
set (INC
${CMAKE_SOURCE_DIR}/inc/add_instrument_dlg.h
${CMAKE_SOURCE_DIR}/inc/dashboard_globals.h
${CMAKE_SOURCE_DIR}/inc/dashboard_pi.h
${CMAKE_SOURCE_DIR}/inc/dashboard_preferences_dialog.h
${CMAKE_SOURCE_DIR}/inc/dashboard_window.h
${CMAKE_SOURCE_DIR}/inc/dashboard_window_container.h
${CMAKE_SOURCE_DIR}/inc/dial.h
${CMAKE_SOURCE_DIR}/inc/edit_dialog.h
${CMAKE_SOURCE_DIR}/inc/instrument.h
${CMAKE_SOURCE_DIR}/inc/ocpn_font_button.h
${CMAKE_SOURCE_DIR}/inc/rudder_angle.h
${CMAKE_SOURCE_DIR}/inc/speedometer.h
)

set (SOURCE_FILES ${SRC} ${INC})

macro(late_init)
  # Perform initialization after the PACKAGE_NAME library, compilers
  # and ocpn::api is available.
  if (APPLE)
    target_compile_definitions(${PACKAGE_NAME} PUBLIC OCPN_GHC_FILESYSTEM)
  endif ()
endmacro ()

macro(add_plugin_libraries)

    add_subdirectory(opencpn-libs/nmea0183)
    target_link_libraries(${PACKAGE_NAME} ocpn::nmea0183)

    add_subdirectory(opencpn-libs/n2kparser)
    target_link_libraries(${PACKAGE_NAME} ocpn::N2KParser)

    add_subdirectory(opencpn-libs/wxJSON)
    target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

    add_subdirectory(opencpn-libs/plugin_dc)
    target_link_libraries(${PACKAGE_NAME} ocpn::plugin-dc)


endmacro ()
