/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - Shared global variables
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _DASHBOARD_GLOBALS_H_
#define _DASHBOARD_GLOBALS_H_

#include <wx/fontdata.h>
#include <wx/colour.h>
#include <wx/listctrl.h>
#include "wx/wx.h"
#include <ocpn_plugin.h>

// Fonts used by the plugin for various controls
extern wxFontData *g_pFontTitle;
extern wxFontData *g_pFontData;
extern wxFontData *g_pFontLabel;
extern wxFontData *g_pFontSmall;

extern wxFontData g_FontTitle;
extern wxFontData g_FontData;
extern wxFontData g_FontLabel;
extern wxFontData g_FontSmall;

// User-set fonts (US = "user set")
extern wxFontData *g_pUSFontTitle;
extern wxFontData *g_pUSFontData;
extern wxFontData *g_pUSFontLabel;
extern wxFontData *g_pUSFontSmall;

extern wxFontData g_USFontTitle;
extern wxFontData g_USFontData;
extern wxFontData g_USFontLabel;
extern wxFontData g_USFontSmall;

extern int g_dashPrefWidth;
extern int g_dashPrefHeight;

// Engine Dashboard gauge settings
// Maximum RPM range
extern int g_tachometerMax;

// Imperial or Metric units
extern int g_temperatureUnit;
extern int g_pressureUnit;
// If a single or dual engine vessel
extern bool g_dualEngine;

// If the voltmeter display range is for 12 or 24 volt systems.
extern bool g_twentyFourVolts;

// Use high contrast colours instead of Day/Dusk/Night colours
extern bool g_highContrast;

// Appearance 
extern wxColor     g_BackgroundColor;
extern bool        g_ForceBackgroundColor;
extern wxAlignment g_TitleAlignment;
extern double      g_TitleVerticalOffset;
extern int         g_iTitleMargin;
extern bool        g_bShowUnit;
extern wxAlignment g_DataAlignment;
extern int         g_iDataMargin;
extern int         g_iInstrumentSpacing;
extern PI_ColorScheme actualColourScheme;

// Plugin bitmaps 
extern wxString g_pluginFolder;
extern wxBitmap	g_pluginBitmap;

extern wxBitmap g_dashboardBitmap;
extern wxBitmap g_dialBitmap;
extern wxBitmap g_instrumentBitmap;
extern wxBitmap g_minusBitmap;
extern wxBitmap g_plusBitmap;

// !!! WARNING !!!
// do not change the order, add new instruments at the end, before ID_DBP_LAST_ENTRY!
// otherwise, for users with an existing opencpn.ini file, their instruments are changing !
enum {
	ID_DBP_MAIN_ENGINE_RPM, ID_DBP_PORT_ENGINE_RPM, ID_DBP_STBD_ENGINE_RPM,
	ID_DBP_MAIN_ENGINE_OIL, ID_DBP_PORT_ENGINE_OIL, ID_DBP_STBD_ENGINE_OIL,
	ID_DBP_MAIN_ENGINE_WATER, ID_DBP_PORT_ENGINE_WATER, ID_DBP_STBD_ENGINE_WATER,
	ID_DBP_MAIN_ENGINE_VOLTS, ID_DBP_PORT_ENGINE_VOLTS, ID_DBP_STBD_ENGINE_VOLTS,
	ID_DBP_MAIN_ENGINE_EXHAUST, ID_DBP_PORT_ENGINE_EXHAUST, ID_DBP_STBD_ENGINE_EXHAUST,
	ID_DBP_FUEL_TANK_01, ID_DBP_WATER_TANK_01, ID_DBP_OIL_TANK, ID_DBP_LIVEWELL_TANK,
	ID_DBP_GREY_TANK, ID_DBP_BLACK_TANK, ID_DBP_RSA, ID_DBP_START_BATTERY_VOLTS,
	ID_DBP_START_BATTERY_AMPS, ID_DBP_HOUSE_BATTERY_VOLTS, ID_DBP_HOUSE_BATTERY_AMPS,
	ID_DBP_FUEL_TANK_02, ID_DBP_WATER_TANK_02, ID_DBP_WATER_TANK_03,
	ID_DBP_FUEL_TANK_GAUGE_01, ID_DBP_FUEL_TANK_GAUGE_02, ID_DBP_WATER_TANK_GAUGE_01,
	ID_DBP_WATER_TANK_GAUGE_02, ID_DBP_WATER_TANK_GAUGE_03, ID_DBP_START_BATTERY_SOC,
	ID_DBP_START_BATTERY_HOURS, ID_DBP_HOUSE_BATTERY_SOC, ID_DBP_HOUSE_BATTERY_HOURS,
	ID_DBP_LAST_ENTRY //this has a reference in one of the routines; defining a "LAST_ENTRY" and setting the reference to it, is one codeline less to change (and find) when adding new instruments :-)
};


// Helper functions 
wxString GetInstrumentCaption(unsigned int id);
void     GetListItemForInstrument(wxListItem &item, unsigned int id);
int      GetRandomNumber(int range_min, int range_max);
wxString GetUUID(void);
wxString MakeName(void);

#endif  // _DASHBOARD_GLOBALS_H_
