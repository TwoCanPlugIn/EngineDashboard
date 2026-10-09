/***************************************************************************
 * $Id: dashboard_pi.cpp, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 * expanded: Bernd Cirotzki 2023 (special colour design)
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

#include "dashboard_globals.h"

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif  // precompiled headers

#include <assert.h>
#include <cmath>

// wx 2.8
#include <wx/filename.h>
#include <wx/fontdlg.h>

#include <typeinfo>

#include "dashboard_pi.h"

//#include "manual.h"

wxFontData *g_pFontTitle;
wxFontData *g_pFontData;
wxFontData *g_pFontLabel;
wxFontData *g_pFontSmall;

wxFontData g_FontTitle;
wxFontData g_FontData;
wxFontData g_FontLabel;
wxFontData g_FontSmall;

wxFontData *g_pUSFontTitle;
wxFontData *g_pUSFontData;
wxFontData *g_pUSFontLabel;
wxFontData *g_pUSFontSmall;

wxFontData g_USFontTitle;
wxFontData g_USFontData;
wxFontData g_USFontLabel;
wxFontData g_USFontSmall;

int g_dashPrefWidth;
int g_dashPrefHeight;

// The plugin's bitmaps
wxString g_pluginFolder;
wxBitmap g_pluginBitmap;

// Bitmaps used in Edit Dashboard dialog
wxBitmap g_dashboardBitmap;
wxBitmap g_dialBitmap;
wxBitmap g_instrumentBitmap;
wxBitmap g_minusBitmap;
wxBitmap g_plusBitmap;

// Dashboard settings 
int g_tachometerMax;
int g_temperatureUnit;
int g_pressureUnit;
int g_volumeUnit;

// Global values because used by instances of both the plugin & preference classes
bool g_dualEngine;

// If the voltmeter display range is for 12 or 24 volt systems.
bool g_twentyFourVolts;

// Use high contrast colours instead of Day/Dusk/Night colours
bool g_highContrast;

wxColor g_BackgroundColor;
bool g_ForceBackgroundColor;
wxAlignment g_TitleAlignment;
double g_TitleVerticalOffset;
int g_iTitleMargin;
bool g_bShowUnit;
wxAlignment g_DataAlignment;
int g_iDataMargin;
int g_iInstrumentSpacing;

PI_ColorScheme actualColourScheme;
#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double *)&lNaN)
#endif

#ifdef __OCPN__ANDROID__
#include "qdebug.h"
#endif

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin *create_pi(void *ppimgr) {
  return (opencpn_plugin *)new Dashboard(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin *p) { delete p; }

#ifdef __OCPN__ANDROID__

QString qtStyleSheet =
    "QScrollBar:horizontal {\
border: 0px solid grey;\
background-color: rgb(240, 240, 240);\
height: 35px;\
margin: 0px 1px 0 1px;\
}\
QScrollBar::handle:horizontal {\
background-color: rgb(200, 200, 200);\
min-width: 20px;\
border-radius: 10px;\
}\
QScrollBar::add-line:horizontal {\
border: 0px solid grey;\
background: #32CC99;\
width: 0px;\
subcontrol-position: right;\
subcontrol-origin: margin;\
}\
QScrollBar::sub-line:horizontal {\
border: 0px solid grey;\
background: #32CC99;\
width: 0px;\
subcontrol-position: left;\
subcontrol-origin: margin;\
}\
QScrollBar:vertical {\
border: 0px solid grey;\
background-color: rgb(240, 240, 240);\
width: 35px;\
margin: 1px 0px 1px 0px;\
}\
QScrollBar::handle:vertical {\
background-color: rgb(200, 200, 200);\
min-height: 20px;\
border-radius: 10px;\
}\
QScrollBar::add-line:vertical {\
border: 0px solid grey;\
background: #32CC99;\
height: 0px;\
subcontrol-position: top;\
subcontrol-origin: margin;\
}\
QScrollBar::sub-line:vertical {\
border: 0px solid grey;\
background: #32CC99;\
height: 0px;\
subcontrol-position: bottom;\
subcontrol-origin: margin;\
}\
QCheckBox {\
spacing: 25px;\
}\
QCheckBox::indicator {\
width: 30px;\
height: 30px;\
}\
";

#endif

#ifdef __OCPN__ANDROID__
#include <QtWidgets/QScroller>
#endif

//    Dashboard PlugIn Global vbls and functions

// Retrieve a caption for each instrument
wxString GetInstrumentCaption(unsigned int id) {
	switch (id) {
	case ID_DBP_MAIN_ENGINE_RPM:
		return _("Main RPM");
	case ID_DBP_PORT_ENGINE_RPM:
		return _("Port RPM");
	case ID_DBP_STBD_ENGINE_RPM:
		return _("Stbd RPM");
	case ID_DBP_MAIN_ENGINE_OIL:
		return _("Main Oil Pressure");
	case ID_DBP_PORT_ENGINE_OIL:
		return _("Port Oil Pressure");
	case ID_DBP_STBD_ENGINE_OIL:
		return _("Stbd Oil Pressure");
	case ID_DBP_MAIN_ENGINE_WATER:
		return _("Main Water Temperature");
	case ID_DBP_PORT_ENGINE_WATER:
		return _("Port Water Temperature");
	case ID_DBP_STBD_ENGINE_WATER:
		return _("Stbd Water Temperature");
	case ID_DBP_MAIN_ENGINE_EXHAUST:
		return _("Main Exhaust Temperature");
	case ID_DBP_PORT_ENGINE_EXHAUST:
		return _("Port Exhaust Temperature");
	case ID_DBP_STBD_ENGINE_EXHAUST:
		return _("Stbd Exhaust Temperature");
	case ID_DBP_MAIN_ENGINE_VOLTS:
		return _("Main Alternator Voltage");
	case ID_DBP_PORT_ENGINE_VOLTS:
		return _("Port Alternator Voltage");
	case ID_DBP_STBD_ENGINE_VOLTS:
		return _("Stbd Alternator Voltage");
	case ID_DBP_FUEL_TANK_01:
	case ID_DBP_FUEL_TANK_GAUGE_01:
		return _("Fuel 1");
	case ID_DBP_FUEL_TANK_02:
	case ID_DBP_FUEL_TANK_GAUGE_02:
		return _("Fuel 2");
	case ID_DBP_WATER_TANK_01:
	case ID_DBP_WATER_TANK_GAUGE_01:
		return _("Water 1");
	case ID_DBP_WATER_TANK_02:
	case ID_DBP_WATER_TANK_GAUGE_02:
		return _("Water 2");
	case ID_DBP_WATER_TANK_03:
	case ID_DBP_WATER_TANK_GAUGE_03:
		return _("Water 3");
	case ID_DBP_OIL_TANK:
		return _("Oil");
	case ID_DBP_LIVEWELL_TANK:
		return _("Live Well");
	case ID_DBP_GREY_TANK:
		return _("Grey Waste");
	case ID_DBP_BLACK_TANK:
		return _("Black Waste");
	case ID_DBP_RSA:
		return _("Rudder Angle");
	case ID_DBP_START_BATTERY_VOLTS:
		return _("Start Battery Voltage");
	case ID_DBP_HOUSE_BATTERY_VOLTS:
		return _("House Battery Voltage");
	case ID_DBP_START_BATTERY_AMPS:
		return _("Start Battery Current");
	case ID_DBP_HOUSE_BATTERY_AMPS:
		return _("House Battery Current");
	case ID_DBP_START_BATTERY_SOC:
		return _("Start Battery SOC");
	case ID_DBP_START_BATTERY_HOURS:
		return _("Start Battery Hours");
	case ID_DBP_HOUSE_BATTERY_SOC:
		return _("House Battery SOC");
	case ID_DBP_HOUSE_BATTERY_HOURS:
		return _("House Battery Hours");
	case ID_DBP_MAIN_ENGINE_FUEL_RATE:
		return "Main Engine Fuel Rate";
	case ID_DBP_PORT_ENGINE_FUEL_RATE:
		return "Port Engine Fuel Rate";
	case ID_DBP_STBD_ENGINE_FUEL_RATE:
		return "Stbd Engine Fuel Rate";
	default:
		return _("");
	}
}

// Populate an index, caption and image for each instrument for use in a list control
void GetListItemForInstrument(wxListItem& item, unsigned int id) {
	item.SetData(id);
	item.SetText(GetInstrumentCaption(id));
	// The engine dashboard instruments use the speedometer control (derived from the dial control)
	// the rudder control (both of which are gauges) or a single text line display or 
	// single block line display both of which are text controls
	switch (id) {
	case ID_DBP_MAIN_ENGINE_RPM:
	case ID_DBP_PORT_ENGINE_RPM:
	case ID_DBP_STBD_ENGINE_RPM:
	case ID_DBP_MAIN_ENGINE_OIL:
	case ID_DBP_PORT_ENGINE_OIL:
	case ID_DBP_STBD_ENGINE_OIL:
	case ID_DBP_MAIN_ENGINE_EXHAUST:
	case ID_DBP_PORT_ENGINE_EXHAUST:
	case ID_DBP_STBD_ENGINE_EXHAUST:
	case ID_DBP_MAIN_ENGINE_WATER:
	case ID_DBP_PORT_ENGINE_WATER:
	case ID_DBP_STBD_ENGINE_WATER:
	case ID_DBP_MAIN_ENGINE_VOLTS:
	case ID_DBP_PORT_ENGINE_VOLTS:
	case ID_DBP_STBD_ENGINE_VOLTS:
	case ID_DBP_FUEL_TANK_01:
	case ID_DBP_FUEL_TANK_02:
	case ID_DBP_WATER_TANK_01:
	case ID_DBP_WATER_TANK_02:
	case ID_DBP_WATER_TANK_03:
	case ID_DBP_OIL_TANK:
	case ID_DBP_LIVEWELL_TANK:
	case ID_DBP_GREY_TANK:
	case ID_DBP_BLACK_TANK:
	case ID_DBP_RSA:
	case ID_DBP_HOUSE_BATTERY_VOLTS:
	case ID_DBP_START_BATTERY_VOLTS:
	case ID_DBP_HOUSE_BATTERY_AMPS:
	case ID_DBP_START_BATTERY_AMPS:
	case ID_DBP_MAIN_ENGINE_FUEL_RATE:
	case ID_DBP_PORT_ENGINE_FUEL_RATE:
	case ID_DBP_STBD_ENGINE_FUEL_RATE:
		item.SetImage(1);
		break;
	case ID_DBP_FUEL_TANK_GAUGE_01:
	case ID_DBP_FUEL_TANK_GAUGE_02:
	case ID_DBP_WATER_TANK_GAUGE_01:
	case ID_DBP_WATER_TANK_GAUGE_02:
	case ID_DBP_WATER_TANK_GAUGE_03:
	case ID_DBP_START_BATTERY_SOC:
	case ID_DBP_START_BATTERY_HOURS:
	case ID_DBP_HOUSE_BATTERY_SOC:
	case ID_DBP_HOUSE_BATTERY_HOURS:
		item.SetImage(0);
		break;
	default:
		item.SetImage(0);
		break;
	}
}

//  These two function were taken from gpxdocument.cpp
int GetRandomNumber(int range_min, int range_max) {
  long u = (long)wxRound(
      ((double)rand() / ((double)(RAND_MAX) + 1) * (range_max - range_min)) +
      range_min);
  return (int)u;
}

// RFC4122 version 4 compliant random UUIDs generator.
wxString GetUUID(void) {
  wxString str;
  struct {
    int time_low;
    int time_mid;
    int time_hi_and_version;
    int clock_seq_hi_and_rsv;
    int clock_seq_low;
    int node_hi;
    int node_low;
  } uuid;

  uuid.time_low = GetRandomNumber(
      0, 2147483647);  // FIXME: the max should be set to something like
                       // MAXINT32, but it doesn't compile un gcc...
  uuid.time_mid = GetRandomNumber(0, 65535);
  uuid.time_hi_and_version = GetRandomNumber(0, 65535);
  uuid.clock_seq_hi_and_rsv = GetRandomNumber(0, 255);
  uuid.clock_seq_low = GetRandomNumber(0, 255);
  uuid.node_hi = GetRandomNumber(0, 65535);
  uuid.node_low = GetRandomNumber(0, 2147483647);

  /* Set the two most significant bits (bits 6 and 7) of the
   * clock_seq_hi_and_rsv to zero and one, respectively. */
  uuid.clock_seq_hi_and_rsv = (uuid.clock_seq_hi_and_rsv & 0x3F) | 0x80;

  /* Set the four most significant bits (bits 12 through 15) of the
   * time_hi_and_version field to 4 */
  uuid.time_hi_and_version = (uuid.time_hi_and_version & 0x0fff) | 0x4000;

  str.Printf("%08x-%04x-%04x-%02x%02x-%04x%08x", uuid.time_low, uuid.time_mid,
             uuid.time_hi_and_version, uuid.clock_seq_hi_and_rsv,
             uuid.clock_seq_low, uuid.node_hi, uuid.node_low);

  return str;
}

wxString MakeName() { 
	return "ENGINE_DASHBOARD_" + GetUUID(); 
}

