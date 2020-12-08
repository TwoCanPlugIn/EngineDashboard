//
// Author: Steven Adler
// 
// Modified the existing dashboard plugin to create an "Engine Dashboard"
// Parses NMEA 0183 RSA, RPM & XDR sentences and displays Engine RPM, Oil Pressure, Water Temperature, 
// Alternator Voltage, Engine Hours andFluid Levels in a dashboard
//
// Version History
// 1.0. 10-10-2019 - Original Release
// 1.1. 23-11-2019 - Fixed original dashboard resize bug (linux with cairo libs only), battery status, gauge background 
// 1.2. 01-08-2020 - Updated to OpenCPN 5.2 Plugin Manager and Continuous Integration (CI) build process
// 1.3. 07-12-2020 - Add support for additional transducer names
// 
// Please send bug reports to twocanplugin@hotmail.com or to the opencpn forum
//
/*
 * $Id: dashboard_pi.cpp, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 *
 */

 /**************************************************************************
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

// wxWidgets Precompiled Headers
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif 

#include "dashboard_pi.h"

#include <typeinfo>
#include "icons.h"

// Global variables for fonts
wxFont *g_pFontTitle;
wxFont *g_pFontData;
wxFont *g_pFontLabel;
wxFont *g_pFontSmall;

// Preferences, Units and Max Values
int g_iDashTachometerMax;
int g_iDashTemperatureUnit;
int g_iDashPressureUnit;

// Store the current engine hours for displaying in the Tachometer Dial
double mainEngineHours;
double portEngineHours;
double stbdEngineHours;

// If using NME 183 v4.11 or ShipModul/Maretron transducer names,
// Whether we are a dual engne vessel and if so if instance 0 refers to port or starboard engine
// if not a dual engine vessel, instance 0 refers to the main engine
bool dualEngine; 

// Watchdog timer, performs two functions, firstly refresh the dashboard every second,  
// and secondly, if no data is received, set instruments to zero (eg. Engine switched off)
// BUG BUG Zeroing instruments not yet implemented
wxDateTime watchDogTime;

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif

// a few conversion functions
double celsius2fahrenheit(double temperature) {
	return (temperature * 9 / 5) + 32;
}

double fahrenheit2celsius(double temperature) {
	return (temperature - 32) * 5 / 9;
}

double pascal2psi(double pressure) {
	return pressure * 0.000145f;
}

double psi2pascal(double pressure) {
	return pressure * 6894.745f;
}


// The class factories, used to create and destroy instances of the PlugIn
// BUG BUG Consider refactoring/renaming the classes

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
    return (opencpn_plugin *) new dashboard_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    Engine Dashboard PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

// !!! WARNING !!!
// do not change the order, add new instruments at the end, before ID_DBP_LAST_ENTRY!
// otherwise, for users with an existing opencpn.ini file, their instruments are changing !
enum {
    ID_DBP_MAIN_ENGINE_RPM, ID_DBP_PORT_ENGINE_RPM, ID_DBP_STBD_ENGINE_RPM,
	ID_DBP_MAIN_ENGINE_OIL, ID_DBP_PORT_ENGINE_OIL, ID_DBP_STBD_ENGINE_OIL,
	ID_DBP_MAIN_ENGINE_WATER, ID_DBP_PORT_ENGINE_WATER, ID_DBP_STBD_ENGINE_WATER,
	ID_DBP_MAIN_ENGINE_VOLTS, ID_DBP_PORT_ENGINE_VOLTS, ID_DBP_STBD_ENGINE_VOLTS,
	ID_DBP_FUEL_TANK, ID_DBP_WATER_TANK, ID_DBP_OIL_TANK, ID_DBP_LIVEWELL_TANK,ID_DBP_GREY_TANK,ID_DBP_BLACK_TANK,
	ID_DBP_RSA, ID_DBP_START_BATTERY_VOLTS, ID_DBP_HOUSE_BATTERY_VOLTS,
	ID_DBP_START_BATTERY_TEMP, ID_DBP_HOUSE_BATTERY_TEMP, ID_DBP_LAST_ENTRY //this has a reference in one of the routines; defining a "LAST_ENTRY" and setting the reference to it, is one codeline less to change (and find) when adding new instruments :-)
};

// Retrieve a caption for each instrument
wxString GetInstrumentCaption(unsigned int id) {
    switch(id) {
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
		case ID_DBP_MAIN_ENGINE_VOLTS:
			return _("Main Alternator Voltage");
		case ID_DBP_PORT_ENGINE_VOLTS:
			return _("Port Alternator Voltage");
		case ID_DBP_STBD_ENGINE_VOLTS:
			return _("Stbd Alternator Voltage");
		case ID_DBP_FUEL_TANK:
			return _("Fuel");
		case ID_DBP_WATER_TANK:
			return _("Water");
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
        case ID_DBP_START_BATTERY_TEMP:
			return _("Start Battery Temperature");
		case ID_DBP_HOUSE_BATTERY_TEMP:
			return _("House Battery Temperature");
		default:
			return _T("");
    }
}

// Populate an index, caption and image for each instrument for use in a list control
void GetListItemForInstrument(wxListItem &item, unsigned int id) {
    item.SetData(id);
    item.SetText(GetInstrumentCaption(id));
   
	switch(id) {
		// All the engine dashboard instruments use either the speedometer control (derived from the dial control)
		// or the rudder control, so display a gauge icon. No need to display a "text" icon 
		// BUG BUG Find then rename or delete SetImage(0) which probably represents a text label
        case ID_DBP_MAIN_ENGINE_RPM:
		case ID_DBP_PORT_ENGINE_RPM:
		case ID_DBP_STBD_ENGINE_RPM:
		case ID_DBP_MAIN_ENGINE_OIL:
		case ID_DBP_PORT_ENGINE_OIL:
		case ID_DBP_STBD_ENGINE_OIL:
		case ID_DBP_MAIN_ENGINE_WATER:
		case ID_DBP_PORT_ENGINE_WATER:
		case ID_DBP_STBD_ENGINE_WATER:
		case ID_DBP_MAIN_ENGINE_VOLTS:
		case ID_DBP_PORT_ENGINE_VOLTS:
		case ID_DBP_STBD_ENGINE_VOLTS:
		case ID_DBP_FUEL_TANK:
		case ID_DBP_WATER_TANK:
		case ID_DBP_OIL_TANK:
		case ID_DBP_LIVEWELL_TANK:
		case ID_DBP_GREY_TANK:
		case ID_DBP_BLACK_TANK:
		case ID_DBP_RSA:
		case ID_DBP_HOUSE_BATTERY_VOLTS:
		case ID_DBP_START_BATTERY_VOLTS:
        case ID_DBP_HOUSE_BATTERY_TEMP:
		case ID_DBP_START_BATTERY_TEMP:
			item.SetImage(1);
			break;
		default:
			item.SetImage(0);
			break;
    }
}

// These two functions are used to construct a unique id for each dashboard instance

/// These two function were taken from gpxdocument.cpp
int GetRandomNumber(int range_min, int range_max) {
      long u = (long)wxRound(((double)rand() / ((double)(RAND_MAX) + 1) * (range_max - range_min)) + range_min);
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

      uuid.time_low = GetRandomNumber(0, 2147483647);//FIXME: the max should be set to something like MAXINT32, but it doesn't compile un gcc...
      uuid.time_mid = GetRandomNumber(0, 65535);
      uuid.time_hi_and_version = GetRandomNumber(0, 65535);
      uuid.clock_seq_hi_and_rsv = GetRandomNumber(0, 255);
      uuid.clock_seq_low = GetRandomNumber(0, 255);
      uuid.node_hi = GetRandomNumber(0, 65535);
      uuid.node_low = GetRandomNumber(0, 2147483647);

      // Set the two most significant bits (bits 6 and 7) of the
      // clock_seq_hi_and_rsv to zero and one, respectively.
      uuid.clock_seq_hi_and_rsv = (uuid.clock_seq_hi_and_rsv & 0x3F) | 0x80;

      // Set the four most significant bits (bits 12 through 15) of the
      // time_hi_and_version field to 4 
      uuid.time_hi_and_version = (uuid.time_hi_and_version & 0x0fff) | 0x4000;

      str.Printf(_T("%08x-%04x-%04x-%02x%02x-%04x%08x"),
      uuid.time_low,
      uuid.time_mid,
      uuid.time_hi_and_version,
      uuid.clock_seq_hi_and_rsv,
      uuid.clock_seq_low,
      uuid.node_hi,
      uuid.node_low);

      return str;
}


// Constructs a unique id for each dashboard instance
wxString MakeName() {
    return _T("ENGINE_DASHBOARD_") + GetUUID();
}

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

// Dashboard Constructor
// BUG BUG Consider renaming the class to engine_dashboard_pi to avoid confusion when programming other dashboard projects
dashboard_pi::dashboard_pi(void *ppimgr) : opencpn_plugin_116(ppimgr), wxTimer(this) {
    // Create the PlugIn icons
    initialize_images();
}

// Dashboard Destructor
dashboard_pi::~dashboard_pi(void) {
    delete _img_engine;
    delete _img_dashboard;
    delete _img_dial;
    delete _img_instrument;
    delete _img_minus;
    delete _img_plus;
}

// Initialize the Dashboard
int dashboard_pi::Init(void) {
    // BUG BUG I need to understand localization
    AddLocaleCatalog(_T("opencpn-engine_dashboard_pi"));

    // BUG BUG Not really used, as plugin only uses version 2 configuration style
    m_config_version = -1;
    
    // Load the fonts
    g_pFontTitle = new wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL);
    g_pFontData = new wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    g_pFontLabel = new wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    g_pFontSmall = new wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    // Wire up the OnClose AUI event
    m_pauimgr = GetFrameAuiManager();
    m_pauimgr->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(dashboard_pi::OnPaneClose), NULL, this);

    // Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();

    // And load the configuration items
    LoadConfig();

    // Scaleable Vector Graphics (SVG) icons are stored in the following path.
	wxString iconFolder = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();
    
    // Load my own plugin icons (refer to the data directory in the repository)
	wxString normalIcon = iconFolder + _T("engine-dashboard-colour.svg");
	wxString toggledIcon = iconFolder + _T("engine-dashboard-bw.svg");
	wxString rolloverIcon = iconFolder + _T("engine-dashboard-bw-rollover.svg");
     
    // For journeyman styles, we prefer the built-in raster icons which match the rest of the toolbar.
    // Is this the "jigsaw icon" ?? In anycase load a monochrome version of my icon
    if (GetActiveStyleName().Lower() != _T("traditional")) {
	    normalIcon = iconFolder + _T("engine-dashboard-bw.svg");
	    toggledIcon = iconFolder + _T("engine-dashboard-bw-rollover.svg");
	    rolloverIcon = iconFolder + _T("engine-dashboard-bw-rollover.svg");
     }

    // Add toolbar icon (in SVG format)
    m_toolbar_item_id = InsertPlugInToolSVG(_T(""), normalIcon, rolloverIcon, toggledIcon, wxITEM_CHECK,
	_(PLUGIN_COMMON_NAME), _T(""), NULL, DASHBOARD_TOOL_POSITION, 0, this);
    
    // Having Loaded the config, then display each of the dashboards
    ApplyConfig();

    // If we loaded a version 1 configuration, convert now to version 2, unlikely to occur for this engine dashboard
	// BUG BUG Consider removing unused code
    if(m_config_version == 1) {
        SaveConfig();
    }

    // Initialize the watchdog timer
    Start(1000, wxTIMER_CONTINUOUS);

    // Reduced from the original dashboard requests
    return (WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_PREFERENCES | WANTS_CONFIG | WANTS_NMEA_SENTENCES | USES_AUI_MANAGER);
}

bool dashboard_pi::DeInit(void) {
    // Save the current configuration
    SaveConfig();

    // Is watchdog timer started?
    if (IsRunning()) {
	Stop(); 
    }

    // This appears to close each dashboard instance
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindow *dashboard_window = m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
        if (dashboard_window) {
            m_pauimgr->DetachPane(dashboard_window);
            dashboard_window->Close();
            dashboard_window->Destroy();
            m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow = NULL;
        }
    }

    // And this appears to close each dashboard container
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindowContainer *pdwc = m_ArrayOfDashboardWindow.Item(i);
        delete pdwc;
    }

    // Unload the fonts
    delete g_pFontTitle;
    delete g_pFontData;
    delete g_pFontLabel;
    delete g_pFontSmall;

    return true;
}

// Called for each timer tick, ensures valid data and refreshes each display
void dashboard_pi::Notify()
{
    if (wxDateTime::Now() > (watchDogTime + wxTimeSpan::Seconds(5))) {
		// Zero all the instruments, Note ID_DBP_LAST_ENTRY is the last entry in the enum
		for (int i = 0, j = 0; i < ID_DBP_LAST_ENTRY; i++) {
			j = 1 << i;
			SendSentenceToAllInstruments(j,0.0f, "");
		}
    }

    // Force a repaint of each instrument
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
	    DashboardWindow *dashboard_window = m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
	    if (dashboard_window) {
	        dashboard_window->Refresh();
	    }
    }
}

int dashboard_pi::GetAPIVersionMajor() {
	return OCPN_API_VERSION_MAJOR;
}

int dashboard_pi::GetAPIVersionMinor() {
	return OCPN_API_VERSION_MINOR;
}

int dashboard_pi::GetPlugInVersionMajor() {
	return PLUGIN_VERSION_MAJOR;
}

int dashboard_pi::GetPlugInVersionMinor() {
	return PLUGIN_VERSION_MINOR;
}

// The plugin bitmap is loaded by the call to InitializeImages in icons.cpp
// Use png2wx.pl perl script to generate the binary data used in icons.cpp
wxBitmap *dashboard_pi::GetPlugInBitmap() {
    return _img_engine;
}

wxString dashboard_pi::GetCommonName() {
    return _(PLUGIN_COMMON_NAME);
}

wxString dashboard_pi::GetShortDescription() {
    return _(PLUGIN_SHORT_DESCRIPTION);
}

wxString dashboard_pi::GetLongDescription() {
    return _(PLUGIN_LONG_DESCRIPTION);
}

// Sends the data value from the parsed NMEA sentence to each gauge
void dashboard_pi::SendSentenceToAllInstruments(int st, double value, wxString unit) {
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindow *dashboard_window = m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
	if (dashboard_window) {
	    dashboard_window->SendSentenceToAllInstruments(st, value, unit);
	}
    }
}

// This method is invoked by OpenCPN when we specify WANTS_NMEA_SENTENCES
void dashboard_pi::SetNMEASentence(wxString &sentence) {
	// Parse the received NMEA 183 sentence
    m_NMEA0183 << sentence;

    if (m_NMEA0183.PreParse()) {

		// Handle NMEA0183 RSA Sentences
		// BUG BUG Rudder Display does not yet support individual gauges for port or starboard(main) rudders.
		if (m_NMEA0183.LastSentenceIDReceived == _T("RSA")) {
			if (m_NMEA0183.Parse()) {
				if (m_NMEA0183.Rsa.IsStarboardDataValid == NTrue) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_RSA, m_NMEA0183.Rsa.Starboard, _T("\u00B0"));
				}
				else if (m_NMEA0183.Rsa.IsPortDataValid == NTrue) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_RSA, -m_NMEA0183.Rsa.Port, _T("\u00B0"));
				}
			}
		}

		// Handle NMEA0183 RPM Sentences
		if (m_NMEA0183.LastSentenceIDReceived == _T("RPM")) {
			if (m_NMEA0183.Parse()) {
				if (m_NMEA0183.Rpm.IsDataValid == NTrue) {
					// Only display engine rpm 'E', not shaft rpm 'S'
					if (m_NMEA0183.Rpm.Source == 'E') {
						// Update Watchdog Timer
						watchDogTime = wxDateTime::Now();
						// Engine Numbering: 
						// 0 = Mid-line, Odd = Starboard, Even = Port (numbered from midline)
						switch (m_NMEA0183.Rpm.EngineNumber) {
							case 0:
							    SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, m_NMEA0183.Rpm.RevolutionsPerMinute, "RPM");
							    break;
							case 1:
							    SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, m_NMEA0183.Rpm.RevolutionsPerMinute, "RPM");
							    break;
							case 2:
							    SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, m_NMEA0183.Rpm.RevolutionsPerMinute, "RPM");
							    break;
							default:
							    SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, m_NMEA0183.Rpm.RevolutionsPerMinute, "RPM");
							    break;
						}
					}
				}
			}
		}

		// Handle NMEA 0183 XDR sentences
		// These are the specific XDR sentences sent by the TwoCan Plugin
		// XDR Transducer Description		Type	Units
		// Temperature Transducer			C		C (degrees Celsius)
		// Pressure Transducer				P		P (Pascal)
		// Tachometer Transducer			T		R (RPM)
		// Volume Transducer				V		P (percent capacity) rather than M (cubic metres)
		// Voltage Transducer				U		V (volts) (for Battery Status, A = Amps)
		// Generic Transducer				G		H (hours, I use this to display engine hours)
		// Switch (Not yet implemented)		S		(no units), Names customised for Status 1 & 2 codes 

		if (m_NMEA0183.LastSentenceIDReceived == _T("XDR")) { 			
			if (m_NMEA0183.Parse()) { 
				wxString xdrunit;
				double xdrdata;
				// Each NMEA 0183 XDR sentence may have up to 4 items
				for (int i = 0; i < m_NMEA0183.Xdr.TransducerCnt; i++) {
					// Copy the NMEA 183 XDR Sentence data element to a variable
					xdrdata = m_NMEA0183.Xdr.TransducerInfo[i].MeasurementData;
					
					// Now for each sentence, parse the transducer type, name and units  to determine which gauge to send the data to

					// "T" Engine RPM in unit "R" RPM
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("T")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("R")) {
							// Update Watchdog timer
							watchDogTime = wxDateTime::Now();
							// Set the units
							xdrunit = _T("RPM");
                             // TwoCan plugin transducer names
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, xdrdata, xdrunit);
							}
                            // NMEA 183 v4.11 transducer names
                            else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#1")) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, xdrdata, xdrunit);
							}
                            else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (!dualEngine)) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, xdrdata, xdrunit);
							}
                            else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (dualEngine)) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, xdrdata, xdrunit);
							}
                            // Ship Modul/Maretron transducer names
                            else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE1")) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, xdrdata, xdrunit);
							}
                            else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE0")) && (!dualEngine)) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, xdrdata, xdrunit);
							}
                            else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE0")) && (dualEngine)) {
							    SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, xdrdata, xdrunit);
							}
						}
					}

					// "C" Temperature in "C" degrees Celsius
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("C")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("C")) {
							if (g_iDashTemperatureUnit == TEMPERATURE_CELSIUS) {
								xdrunit = _T("\u00B0 C");
                                // TwoCan plugin transducer names
								if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, xdrdata, xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, xdrdata, xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, xdrdata, xdrunit);
								}
                                // NMEA 183 v4.11 Transducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, xdrdata, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, xdrdata, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, xdrdata, xdrunit);
								}
                                // Ship Modul/Maretron Transducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGTEMP1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, xdrdata, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGTEMP0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, xdrdata, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGTEMP0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, xdrdata, xdrunit);
								}
							}
							else if (g_iDashTemperatureUnit == TEMPERATURE_FAHRENHEIT) {
								xdrunit = _T("\u00B0 F");
								if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
                                // NMEA 183 v4.11 Transducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
                                // Ship Modul/Maretron Transducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGTEMP1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGTEMP0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGTEMP0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, celsius2fahrenheit(xdrdata), xdrunit);
								}
							}
						}
					}
		    
					// "P" Pressure in "P" pascal
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("P")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("P")) {
							if (g_iDashPressureUnit == PRESSURE_BAR) {
								xdrunit = _T("Bar");
								if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, xdrdata + 1e-5, xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}
                                // NMEA 183 v4.11 Transducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}
                                // Ship Modul/Maretron Transducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGOILP1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGOILP0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGOILP0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
								}

							}
							else if (g_iDashPressureUnit == PRESSURE_PSI) {
								xdrunit = _T("PSI");
                                // TwoCan Plugin Transducer Names
								if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
								else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
                                // NMEA 183 v4.11 Transducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
                                // Ship Modul/MaretronTransducer Names
                                else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGOILP1")) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGOILP0")) && (!dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
                                else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGOILP0")) && (dualEngine)) {
									SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, pascal2psi(xdrdata), xdrunit);
								}
							}
						}
					}

					// "U" Voltage in "V" volts
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("U")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("V")) {
							xdrunit = _T("Volts");
                            // TwoCan Plugin Transducer Names
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STRT")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("HOUS")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, xdrdata, xdrunit);
							}
                            // NMEA 183 v4.11 Transducer Names
                            if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ALTERNATOR#1")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ALTERNATOR#0")) && (!dualEngine)) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ALTERNATOR#0")) && (dualEngine)) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATTERY#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATTERY#1")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, xdrdata, xdrunit);
							}
                            // Ship Modul/Maretron Transducer Names
                            if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ALTVOLT1")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ALTVOLT0")) && (!dualEngine)) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ALTVOLT0")) && (dualEngine)) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATVOLT0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATVOLT1")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, xdrdata, xdrunit);
							}
						}
						// TwoCan uses "A" to indicate battery current
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("A")) {
							xdrunit = _T("Amps");
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STRT")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_AMPS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("HOUS")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, xdrdata, xdrunit);
							}
						}
					}

                    // NMEA 0183 V4 standard for current
                    if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("I")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("A")) {
							xdrunit = _T("Amps");
                            // NMEA 183 v4.11 Transducr Names
                            if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATTERY#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_AMPS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATTERY#1")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, xdrdata, xdrunit);
							}
                            // Ship Modul/Maretron Transducr Names
                            if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATCURR0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_AMPS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BATCURR1")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, xdrdata, xdrunit);
							}
						}
                    }

					// "G" Generic - Customised to use "H" as engine hours
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("G")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("H")) {	
							xdrunit = _T("Hrs");
                            // TwoCan Plugin transducer naming
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
								mainEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
								portEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
								stbdEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, xdrdata, xdrunit);
							}
						}
                        // NMEA 183 v4.11 Transducer Names, Note NMEA do not define Engine Hours
                        // So we'll just assume the same as per ShipModul
                        if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("")) {	
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#1")) {
                                xdrunit = _T("Hrs");
								mainEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (!dualEngine)) {
                                xdrunit = _T("Hrs");
								portEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGINE#0")) && (dualEngine)) {
                                xdrunit = _T("Hrs");
								stbdEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, xdrdata, xdrunit);
							}
						}
                        // Ship Modul/Maretron Transducer Names (Note does not have a unit of measurement)
                        if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("")) {	
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGHRS1")) {
                                xdrunit = _T("Hrs");
								mainEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGHRS0")) && (!dualEngine)) {
                                xdrunit = _T("Hrs");
								portEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, xdrdata, xdrunit);
							}
							else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("ENGHRS0")) && (dualEngine)) {
                                xdrunit = _T("Hrs");
								stbdEngineHours = xdrdata;
								SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, xdrdata, xdrunit);
							}
                        }

					}

                	// "V" Volume - Customised to use "P" as percent capacity
					// instead of "M" as volume in cubic metres
                    // Note that NMEA 183 v4.11 standard alo introduces 'P' as percent capacity
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("V")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("P")) {
							xdrunit = _T("Level");
                            // TwoCan Plugin Transducer Names
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FUEL")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("H2O")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("OIL")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("LIVE")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("GREY")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BLACK")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, xdrdata, xdrunit);
							}
                            // NMEA 183 v4.11 Transducer Names
                            if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FUEL#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FRESHWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("OIL#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("LIVEWELLWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("WASTEWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BLACKWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, xdrdata, xdrunit);
							}
						}
					}
                    // NMEA 0184 v4.11 Standard for volume
                    if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("E")) {
						if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("P")) {
							xdrunit = _T("Level");
                            // NMEA 183 v4.11 Transducer Names
							if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FUEL#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FRESHWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("OIL#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("LIVEWELLWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("WASTEWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BLACKWATER#0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, xdrdata, xdrunit);
							}
                            // Ship Modul/Martron Transducer Names
                            if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FUEL0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FRESHWATER0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("OIL0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("LIVEWELL0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("WASTEWATER0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, xdrdata, xdrunit);
							}
							else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("BLACKWATER0")) {
								SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, xdrdata, xdrunit);
							}
                        }
                    }
                 }
            }
        }
    }
}



// Not sure what this does or is used for. I guess we only install one toolbar item??
int dashboard_pi::GetToolbarToolCount(void) {
    return 1;
}

// Display the Dashboard Setings Dialog
void dashboard_pi::ShowPreferencesDialog(wxWindow* parent) {
	DashboardPreferencesDialog *dialog = new DashboardPreferencesDialog(parent, wxID_ANY, m_ArrayOfDashboardWindow);
	if (dialog->ShowModal() == wxID_OK) {
		// Reload the fonts in case they have been changed
		delete g_pFontTitle;
		delete g_pFontData;
		delete g_pFontLabel;
		delete g_pFontSmall;

		g_pFontTitle = new wxFont(dialog->m_pFontPickerTitle->GetSelectedFont());
		g_pFontData = new wxFont(dialog->m_pFontPickerData->GetSelectedFont());
		g_pFontLabel = new wxFont(dialog->m_pFontPickerLabel->GetSelectedFont());
		g_pFontSmall = new wxFont(dialog->m_pFontPickerSmall->GetSelectedFont());

		// OnClose should handle that for us normally but it doesn't seems to do so
		// We must save changes first
		dialog->SaveDashboardConfig();
		m_ArrayOfDashboardWindow.Clear();
		m_ArrayOfDashboardWindow = dialog->m_Config;
		// Reload the saved dashboard instruments
		ApplyConfig();
		// Save the Configuration
		SaveConfig();
		// Not exactly sure what this does. Pesumably if no dashboards are displayed, the toolbar icon is toggled/untoggled??
		SetToolbarItemState(m_toolbar_item_id, GetDashboardWindowShownCount() != 0);
	}

	// Invoke the dialog destructor
	dialog->Destroy();
}


void dashboard_pi::SetColorScheme(PI_ColorScheme cs) {
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindow *dashboard_window = m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
		if (dashboard_window) {
			dashboard_window->SetColorScheme(cs);
		}
    }
}

int dashboard_pi::GetToolbarItemId() { 
	return m_toolbar_item_id; 
}

int dashboard_pi::GetDashboardWindowShownCount() {
    int cnt = 0;

    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindow *dashboard_window = m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
        if (dashboard_window) {
            wxAuiPaneInfo &pane = m_pauimgr->GetPane(dashboard_window);
			if (pane.IsOk() && pane.IsShown()) {
				cnt++;
			}
        }
    }
    return cnt;
}

void dashboard_pi::OnPaneClose(wxAuiManagerEvent& event) {
    // if name is unique, we should use it
    DashboardWindow *dashboard_window = (DashboardWindow *) event.pane->window;
    int cnt = 0;
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
        DashboardWindow *d_w = cont->m_pDashboardWindow;
        if (d_w) {
            // we must not count this one because it is being closed
            if (dashboard_window != d_w) {
                wxAuiPaneInfo &pane = m_pauimgr->GetPane(d_w);
				if (pane.IsOk() && pane.IsShown()) {
					cnt++;
				}
            } else {
                cont->m_bIsVisible = false;
            }
        }
    }
    SetToolbarItemState(m_toolbar_item_id, cnt != 0);

    event.Skip();
}

void dashboard_pi::OnToolbarToolCallback(int id) {
    int cnt = GetDashboardWindowShownCount();
    bool b_anyviz = false;
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
        if (cont->m_bIsVisible) {
            b_anyviz = true;
            break;
        }
    }

    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
        DashboardWindow *dashboard_window = cont->m_pDashboardWindow;
        if (dashboard_window) {
            wxAuiPaneInfo &pane = m_pauimgr->GetPane(dashboard_window);
            if (pane.IsOk()) {
                bool b_reset_pos = false;

#ifdef __WXMSW__
                //  Support MultiMonitor setups which an allow negative window positions.
                //  If the requested window title bar does not intersect any installed monitor,
                //  then default to simple primary monitor positioning.
                RECT frame_title_rect;
                frame_title_rect.left = pane.floating_pos.x;
                frame_title_rect.top = pane.floating_pos.y;
                frame_title_rect.right = pane.floating_pos.x + pane.floating_size.x;
                frame_title_rect.bottom = pane.floating_pos.y + 30;

				if (NULL == MonitorFromRect(&frame_title_rect, MONITOR_DEFAULTTONULL)) {
					b_reset_pos = true;
				}
#else

                //    Make sure drag bar (title bar) of window intersects wxClient Area of screen, with a little slop...
                wxRect window_title_rect;// conservative estimate
                window_title_rect.x = pane.floating_pos.x;
                window_title_rect.y = pane.floating_pos.y;
                window_title_rect.width = pane.floating_size.x;
                window_title_rect.height = 30;

                wxRect ClientRect = wxGetClientDisplayRect();
                ClientRect.Deflate(60, 60);// Prevent the new window from being too close to the edge
				if (!ClientRect.Intersects(window_title_rect)) {
					b_reset_pos = true;
				}

#endif

				if (b_reset_pos) {
					pane.FloatingPosition(50, 50);
				}

                if (cnt == 0)
                    if (b_anyviz)
                        pane.Show(cont->m_bIsVisible);
                    else {
                       cont->m_bIsVisible = cont->m_bPersVisible;
                       pane.Show(cont->m_bIsVisible);
                    }
                else
                    pane.Show(false);
            }

            //  This patch fixes a bug in wxAUIManager
            //  FS#548
            // Dropping a DashBoard Window right on top on the (supposedly fixed) chart bar window
            // causes a resize of the chart bar, and the Dashboard window assumes some of its properties
            // The Dashboard window is no longer grabbable...
            // Workaround:  detect this case, and force the pane to be on a different Row.
            // so that the display is corrected by toggling the dashboard off and back on.
            if ((pane.dock_direction == wxAUI_DOCK_BOTTOM) && pane.IsDocked()) pane.Row(2);
        }
    }
    // Toggle is handled by the toolbar but we must keep plugin manager b_toggle updated
    // to actual status to ensure right status upon toolbar rebuild
    SetToolbarItemState(m_toolbar_item_id, GetDashboardWindowShownCount() != 0);
    m_pauimgr->Update();
}

void dashboard_pi::UpdateAuiStatus(void) {
    // This method is called after the PlugIn is initialized
    // and the frame has done its initial layout, possibly from a saved wxAuiManager "Perspective"
    // It is a chance for the PlugIn to syncronize itself internally with the state of any Panes that
    //  were added to the frame in the PlugIn ctor.

    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
        wxAuiPaneInfo &pane = m_pauimgr->GetPane(cont->m_pDashboardWindow);
        // Initialize visible state as perspective is loaded now
        cont->m_bIsVisible = (pane.IsOk() && pane.IsShown()); 
    }
    m_pauimgr->Update();
    
    // We use this callback here to keep the context menu selection in sync with the window state
    SetToolbarItemState(m_toolbar_item_id, GetDashboardWindowShownCount() != 0);
}

// Loads a saved configuration
bool dashboard_pi::LoadConfig(void) {
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if (pConf) {
        pConf->SetPath(_T("/PlugIns/Engine-Dashboard"));

        wxString version;
        pConf->Read(_T("Version"), &version, wxEmptyString);
        
		// Load the font configuration, note reuse of config variable
		wxString config;
        pConf->Read(_T("FontTitle"), &config, wxEmptyString);
		
		if (!config.IsEmpty()) {
			g_pFontTitle->SetNativeFontInfo(config);
		}
        
		pConf->Read(_T("FontData"), &config, wxEmptyString);
        
		if (!config.IsEmpty()) {
			g_pFontData->SetNativeFontInfo(config);
		}
        
		pConf->Read(_T("FontLabel"), &config, wxEmptyString);
		
		if (!config.IsEmpty()) {
			g_pFontLabel->SetNativeFontInfo(config);
		
		}
        pConf->Read(_T("FontSmall"), &config, wxEmptyString);
		
		if (!config.IsEmpty()) {
			g_pFontSmall->SetNativeFontInfo(config);
		}

		// Load the maximum tachometer value, Temperature & Pressure units and dual engine status
		pConf->Read(_T("TachometerMax"), &g_iDashTachometerMax, 6000);
		pConf->Read(_T("TemperatureUnit"), &g_iDashTemperatureUnit, TEMPERATURE_CELSIUS);
		pConf->Read(_T("PressureUnit"), &g_iDashPressureUnit, PRESSURE_BAR);
        pConf->Read(_T("DualEngine"), &dualEngine, false);
		
		// Now retrieve the number of dashboard containers and their instruments
        int d_cnt;
        pConf->Read(_T("DashboardCount"), &d_cnt, -1);
        
	// TODO: Memory leak? We should destroy everything first
        m_ArrayOfDashboardWindow.Clear();
	// BUG BUG A version 1 configuration does not include a version value
	// BUG BUG consider removing the following obsolete code.
        if (version.IsEmpty() && d_cnt == -1) {
            m_config_version = 1;
            // Let's load version 1 or default settings.
            int i_cnt;
            pConf->Read(_T("InstrumentCount"), &i_cnt, -1);
            wxArrayInt ar;
            if (i_cnt != -1) {
                for (int i = 0; i < i_cnt; i++) {
                    int id;
                    pConf->Read(wxString::Format(_T("Instrument%d"), i + 1), &id, -1);
                    if (id != -1) ar.Add(id);
                }
            } else {
                // Load a default instrument list, assumes single engined vessel
                ar.Add(ID_DBP_MAIN_ENGINE_RPM);
                ar.Add(ID_DBP_MAIN_ENGINE_OIL);
                ar.Add(ID_DBP_MAIN_ENGINE_WATER);
		ar.Add(ID_DBP_MAIN_ENGINE_VOLTS);
            }
	    
	    // Note generate a unique GUID for each dashboard container
            DashboardWindowContainer *cont = new DashboardWindowContainer(NULL, MakeName(), _("Engine-Dashboard"), _T("V"), ar);
            cont->m_bPersVisible = true;
            m_ArrayOfDashboardWindow.Add(cont);
            
        } else {
            // Configuration Version 2
            m_config_version = 2;
            bool b_onePersisted = false;

            for (int i = 0; i < d_cnt; i++) {
                pConf->SetPath(wxString::Format(_T("/PlugIns/Engine-Dashboard/Dashboard%d"), i + 1));
                wxString name;
                pConf->Read(_T("Name"), &name, MakeName());
                wxString caption;
                pConf->Read(_T("Caption"), &caption, _("Dashboard"));
                wxString orient;
                pConf->Read(_T("Orientation"), &orient, _T("V"));
                int i_cnt;
                pConf->Read(_T("InstrumentCount"), &i_cnt, -1);
                bool b_persist;
                pConf->Read(_T("Persistence"), &b_persist, 1);
                
                wxArrayInt ar;
                for (int i = 0; i < i_cnt; i++) {
                    int id;
                    pConf->Read(wxString::Format(_T("Instrument%d"), i + 1), &id, -1);
                    if (id != -1) ar.Add(id);
                }

				// TODO: Do not add if GetCount == 0

                DashboardWindowContainer *cont = new DashboardWindowContainer(NULL, name, caption, orient, ar);
                cont->m_bPersVisible = b_persist;

		if (b_persist) {
		    b_onePersisted = true;
		}
                
                m_ArrayOfDashboardWindow.Add(cont);

            }
            
            // Make sure at least one dashboard is scheduled to be visible
            if (m_ArrayOfDashboardWindow.Count() && !b_onePersisted){
                DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(0);
		if (cont) {
		    cont->m_bPersVisible = true;
		}
            }   
        }

        return true;
    } else
        return false;
}

// Save the current configuration
// Note this is a version 2 configuration
bool dashboard_pi::SaveConfig(void) {
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if (pConf) {
        pConf->SetPath(_T("/PlugIns/Engine-Dashboard"));
        pConf->Write(_T("Version"), _T("2"));
        pConf->Write(_T("FontTitle"), g_pFontTitle->GetNativeFontInfoDesc());
        pConf->Write(_T("FontData"), g_pFontData->GetNativeFontInfoDesc());
        pConf->Write(_T("FontLabel"), g_pFontLabel->GetNativeFontInfoDesc());
        pConf->Write(_T("FontSmall"), g_pFontSmall->GetNativeFontInfoDesc());

	    pConf->Write(_T("TachometerMax"), g_iDashTachometerMax);
	    pConf->Write(_T("TemperatureUnit"), g_iDashTemperatureUnit);
	    pConf->Write(_T("PressureUnit"), g_iDashPressureUnit);
        pConf->Write(_T("DualEngine"), dualEngine);

        pConf->Write(_T("DashboardCount"), (int) m_ArrayOfDashboardWindow.GetCount());
        for (unsigned int i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
            DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
            pConf->SetPath(wxString::Format(_T("/PlugIns/Engine-Dashboard/Dashboard%d"), i + 1));
            pConf->Write(_T("Name"), cont->m_sName);
            pConf->Write(_T("Caption"), cont->m_sCaption);
            pConf->Write(_T("Orientation"), cont->m_sOrientation);
            pConf->Write(_T("Persistence"), cont->m_bPersVisible);
            pConf->Write(_T("InstrumentCount"), (int) cont->m_aInstrumentList.GetCount());
	        for (unsigned int j = 0; j < cont->m_aInstrumentList.GetCount(); j++) {
	    	    pConf->Write(wxString::Format(_T("Instrument%d"), j + 1), cont->m_aInstrumentList.Item(j));
	        }
        }

        return true;
	}

	else {
		return false;
	}
}

// Load current dashboard containers and their instruments
void dashboard_pi::ApplyConfig(void) {
    // Reverse order to handle deletes
    for (size_t i = m_ArrayOfDashboardWindow.GetCount(); i > 0; i--) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i - 1);
        int orient = (cont->m_sOrientation == _T("V") ? wxVERTICAL : wxHORIZONTAL);
        if (cont->m_bIsDeleted) {
            if (cont->m_pDashboardWindow) {
                m_pauimgr->DetachPane(cont->m_pDashboardWindow);
                cont->m_pDashboardWindow->Close();
                cont->m_pDashboardWindow->Destroy();
                cont->m_pDashboardWindow = NULL;
            }
            m_ArrayOfDashboardWindow.Remove(cont);
            delete cont;

        } else if(!cont->m_pDashboardWindow) {
            // A new dashboard is created
            cont->m_pDashboardWindow = new DashboardWindow(GetOCPNCanvasWindow(), wxID_ANY,
                    m_pauimgr, this, orient, cont);
            cont->m_pDashboardWindow->SetInstrumentList(cont->m_aInstrumentList);
            bool vertical = orient == wxVERTICAL;
            wxSize sz = cont->m_pDashboardWindow->GetMinSize();
// Mac has a little trouble with initial Layout() sizing...
#ifdef __WXOSX__
            if (sz.x == 0)
                sz.IncTo(wxSize(160, 388));
#endif
                wxAuiPaneInfo p = wxAuiPaneInfo().Name(cont->m_sName).Caption(cont->m_sCaption).CaptionVisible(false).TopDockable(
                    !vertical).BottomDockable(!vertical).LeftDockable(vertical).RightDockable(vertical).MinSize(
                        sz).BestSize(sz).FloatingSize(sz).FloatingPosition(100, 100).Float().Show(cont->m_bIsVisible).Gripper(false) ;
            
            m_pauimgr->AddPane(cont->m_pDashboardWindow, p);
                //wxAuiPaneInfo().Name(cont->m_sName).Caption(cont->m_sCaption).CaptionVisible(false).TopDockable(
               // !vertical).BottomDockable(!vertical).LeftDockable(vertical).RightDockable(vertical).MinSize(
               // sz).BestSize(sz).FloatingSize(sz).FloatingPosition(100, 100).Float().Show(cont->m_bIsVisible));
        } else {
            wxAuiPaneInfo& pane = m_pauimgr->GetPane(cont->m_pDashboardWindow);
            pane.Caption(cont->m_sCaption).Show(cont->m_bIsVisible);
            if (!cont->m_pDashboardWindow->isInstrumentListEqual(cont->m_aInstrumentList)) {
                cont->m_pDashboardWindow->SetInstrumentList(cont->m_aInstrumentList);
                wxSize sz = cont->m_pDashboardWindow->GetMinSize();
                pane.MinSize(sz).BestSize(sz).FloatingSize(sz);
            }
            if (cont->m_pDashboardWindow->GetSizerOrientation() != orient) {
                cont->m_pDashboardWindow->ChangePaneOrientation(orient, false);
            }
        }
    }
    m_pauimgr->Update();
}

void dashboard_pi::PopulateContextMenu(wxMenu* menu) {
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
        wxMenuItem* item = menu->AppendCheckItem(i+1, cont->m_sCaption);
        item->Check(cont->m_bIsVisible);
    }
}

void dashboard_pi::ShowDashboard(size_t id, bool visible) {
    if (id < m_ArrayOfDashboardWindow.GetCount()) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(id);
        m_pauimgr->GetPane(cont->m_pDashboardWindow).Show(visible);
        cont->m_bIsVisible = visible;
        cont->m_bPersVisible = visible;
        m_pauimgr->Update();
    }
}


// BUG BUG Should refactor and place into a separate class file

//
// DashboardPreferencesDialog
//
//

DashboardPreferencesDialog::DashboardPreferencesDialog(wxWindow *parent, wxWindowID id, wxArrayOfDashboard config) :
        wxDialog(parent, id, _("Engine Dashboard Settings"), wxDefaultPosition, wxDefaultSize,  wxDEFAULT_DIALOG_STYLE) {
    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(DashboardPreferencesDialog::OnCloseDialog), NULL, this);

    // Copy original config
    m_Config = wxArrayOfDashboard(config);
    // Build Dashboard Page for Toolbox
    int border_size = 2;

    wxBoxSizer* itemBoxSizerMainPanel = new wxBoxSizer(wxVERTICAL);
    SetSizer(itemBoxSizerMainPanel);

    wxNotebook *itemNotebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxNB_TOP);
    itemBoxSizerMainPanel->Add(itemNotebook, 1, wxALL | wxEXPAND, border_size);

    wxPanel *itemPanelNotebook01 = new wxPanel(itemNotebook, wxID_ANY, wxDefaultPosition,
            wxDefaultSize, wxTAB_TRAVERSAL);
    wxFlexGridSizer *itemFlexGridSizer01 = new wxFlexGridSizer(2);
    itemFlexGridSizer01->AddGrowableCol(1);
    itemPanelNotebook01->SetSizer(itemFlexGridSizer01);
    itemNotebook->AddPage(itemPanelNotebook01, _("Dashboard"));

    wxBoxSizer *itemBoxSizer01 = new wxBoxSizer(wxVERTICAL);
    itemFlexGridSizer01->Add(itemBoxSizer01, 1, wxEXPAND | wxTOP | wxLEFT, border_size);

    wxImageList *imglist1 = new wxImageList(32, 32, true, 1);
    imglist1->Add(*_img_dashboard);

    m_pListCtrlDashboards = new wxListCtrl(itemPanelNotebook01, wxID_ANY, wxDefaultPosition,
            wxSize(50, 200), wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);
    m_pListCtrlDashboards->AssignImageList(imglist1, wxIMAGE_LIST_SMALL);
    m_pListCtrlDashboards->InsertColumn(0, _T(""));
    m_pListCtrlDashboards->Connect(wxEVT_COMMAND_LIST_ITEM_SELECTED,
            wxListEventHandler(DashboardPreferencesDialog::OnDashboardSelected), NULL, this);
    m_pListCtrlDashboards->Connect(wxEVT_COMMAND_LIST_ITEM_DESELECTED,
            wxListEventHandler(DashboardPreferencesDialog::OnDashboardSelected), NULL, this);
    itemBoxSizer01->Add(m_pListCtrlDashboards, 1, wxEXPAND, 0);

    wxBoxSizer *itemBoxSizer02 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer01->Add(itemBoxSizer02);

    m_pButtonAddDashboard = new wxBitmapButton(itemPanelNotebook01, wxID_ANY, *_img_plus,
            wxDefaultPosition, wxDefaultSize);
    itemBoxSizer02->Add(m_pButtonAddDashboard, 0, wxALIGN_CENTER, 2);
    m_pButtonAddDashboard->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DashboardPreferencesDialog::OnDashboardAdd), NULL, this);
    m_pButtonDeleteDashboard = new wxBitmapButton(itemPanelNotebook01, wxID_ANY, *_img_minus,
            wxDefaultPosition, wxDefaultSize);
    itemBoxSizer02->Add(m_pButtonDeleteDashboard, 0, wxALIGN_CENTER, 2);
    m_pButtonDeleteDashboard->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DashboardPreferencesDialog::OnDashboardDelete), NULL, this);

    m_pPanelDashboard = new wxPanel(itemPanelNotebook01, wxID_ANY, wxDefaultPosition,
            wxDefaultSize, wxBORDER_SUNKEN);
    itemFlexGridSizer01->Add(m_pPanelDashboard, 1, wxEXPAND | wxTOP | wxRIGHT, border_size);

    wxBoxSizer* itemBoxSizer03 = new wxBoxSizer(wxVERTICAL);
    m_pPanelDashboard->SetSizer(itemBoxSizer03);

    wxStaticBox* itemStaticBox02 = new wxStaticBox(m_pPanelDashboard, wxID_ANY, _("Dashboard"));
    wxStaticBoxSizer* itemStaticBoxSizer02 = new wxStaticBoxSizer(itemStaticBox02, wxHORIZONTAL);
    itemBoxSizer03->Add(itemStaticBoxSizer02, 0, wxEXPAND | wxALL, border_size);
    wxFlexGridSizer *itemFlexGridSizer = new wxFlexGridSizer(2);
    itemFlexGridSizer->AddGrowableCol(1);
    itemStaticBoxSizer02->Add(itemFlexGridSizer, 1, wxEXPAND | wxALL, 0);

    m_pCheckBoxIsVisible = new wxCheckBox(m_pPanelDashboard, wxID_ANY, _("show this dashboard"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer->Add(m_pCheckBoxIsVisible, 0, wxEXPAND | wxALL, border_size);
    wxStaticText *itemDummy01 = new wxStaticText(m_pPanelDashboard, wxID_ANY, _T(""));
    itemFlexGridSizer->Add(itemDummy01, 0, wxEXPAND | wxALL, border_size);

    wxStaticText* itemStaticText01 = new wxStaticText(m_pPanelDashboard, wxID_ANY, _("Caption:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer->Add(itemStaticText01, 0, wxEXPAND | wxALL, border_size);
    m_pTextCtrlCaption = new wxTextCtrl(m_pPanelDashboard, wxID_ANY, _T(""), wxDefaultPosition,
            wxDefaultSize);
    itemFlexGridSizer->Add(m_pTextCtrlCaption, 0, wxEXPAND | wxALL, border_size);

    wxStaticText* itemStaticText02 = new wxStaticText(m_pPanelDashboard, wxID_ANY,
            _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer->Add(itemStaticText02, 0, wxEXPAND | wxALL, border_size);
    m_pChoiceOrientation = new wxChoice(m_pPanelDashboard, wxID_ANY, wxDefaultPosition,
            wxSize(120, -1));
    m_pChoiceOrientation->Append(_("Vertical"));
    m_pChoiceOrientation->Append(_("Horizontal"));
    itemFlexGridSizer->Add(m_pChoiceOrientation, 0, wxALIGN_RIGHT | wxALL, border_size);

    wxImageList *imglist = new wxImageList(20, 20, true, 2);
    imglist->Add(*_img_instrument);
    imglist->Add(*_img_dial);

    wxStaticBox* itemStaticBox03 = new wxStaticBox(m_pPanelDashboard, wxID_ANY, _("Instruments"));
    wxStaticBoxSizer* itemStaticBoxSizer03 = new wxStaticBoxSizer(itemStaticBox03, wxHORIZONTAL);
    itemBoxSizer03->Add(itemStaticBoxSizer03, 1, wxEXPAND | wxALL, border_size);

    m_pListCtrlInstruments = new wxListCtrl(m_pPanelDashboard, wxID_ANY, wxDefaultPosition,
            wxSize(-1, 200), wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);
    itemStaticBoxSizer03->Add(m_pListCtrlInstruments, 1, wxEXPAND | wxALL, border_size);
    m_pListCtrlInstruments->AssignImageList(imglist, wxIMAGE_LIST_SMALL);
    m_pListCtrlInstruments->InsertColumn(0, _("Instruments"));
    m_pListCtrlInstruments->Connect(wxEVT_COMMAND_LIST_ITEM_SELECTED,
            wxListEventHandler(DashboardPreferencesDialog::OnInstrumentSelected), NULL, this);
    m_pListCtrlInstruments->Connect(wxEVT_COMMAND_LIST_ITEM_DESELECTED,
            wxListEventHandler(DashboardPreferencesDialog::OnInstrumentSelected), NULL, this);

    wxBoxSizer* itemBoxSizer04 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer03->Add(itemBoxSizer04, 0, wxALIGN_TOP | wxALL, border_size);
    m_pButtonAdd = new wxButton(m_pPanelDashboard, wxID_ANY, _("Add"), wxDefaultPosition,
            wxSize(20, -1));
    itemBoxSizer04->Add(m_pButtonAdd, 0, wxEXPAND | wxALL, border_size);
    m_pButtonAdd->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentAdd), NULL, this);

/* TODO  Instrument Properties
    m_pButtonEdit = new wxButton(m_pPanelDashboard, wxID_ANY, _("Edit"), wxDefaultPosition,
            wxDefaultSize);
    itemBoxSizer04->Add(m_pButtonEdit, 0, wxEXPAND | wxALL, border_size);
    m_pButtonEdit->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentEdit), NULL, this);
*/
    m_pButtonDelete = new wxButton(m_pPanelDashboard, wxID_ANY, _("Delete"), wxDefaultPosition,
            wxSize(20, -1));
    itemBoxSizer04->Add(m_pButtonDelete, 0, wxEXPAND | wxALL, border_size);
    m_pButtonDelete->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentDelete), NULL, this);
    itemBoxSizer04->AddSpacer(10);
    m_pButtonUp = new wxButton(m_pPanelDashboard, wxID_ANY, _("Up"), wxDefaultPosition,
            wxDefaultSize);
    itemBoxSizer04->Add(m_pButtonUp, 0, wxEXPAND | wxALL, border_size);
    m_pButtonUp->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentUp), NULL, this);
    m_pButtonDown = new wxButton(m_pPanelDashboard, wxID_ANY, _("Down"), wxDefaultPosition,
            wxDefaultSize);
    itemBoxSizer04->Add(m_pButtonDown, 0, wxEXPAND | wxALL, border_size);
    m_pButtonDown->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentDown), NULL, this);

    wxPanel *itemPanelNotebook02 = new wxPanel(itemNotebook, wxID_ANY, wxDefaultPosition,
            wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer* itemBoxSizer05 = new wxBoxSizer(wxVERTICAL);
    itemPanelNotebook02->SetSizer(itemBoxSizer05);
    itemNotebook->AddPage(itemPanelNotebook02, _("Appearance"));

    wxStaticBox* itemStaticBoxFonts = new wxStaticBox(itemPanelNotebook02, wxID_ANY, _("Fonts"));
    wxStaticBoxSizer* itemStaticBoxSizer01 = new wxStaticBoxSizer(itemStaticBoxFonts, wxHORIZONTAL);
    itemBoxSizer05->Add(itemStaticBoxSizer01, 0, wxEXPAND | wxALL, border_size);
    wxFlexGridSizer *itemFlexGridSizer03 = new wxFlexGridSizer(2);
    itemFlexGridSizer03->AddGrowableCol(1);
    itemStaticBoxSizer01->Add(itemFlexGridSizer03, 1, wxEXPAND | wxALL, 0);
    wxStaticText* itemStaticText04 = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Title:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer03->Add(itemStaticText04, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerTitle = new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, *g_pFontTitle,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer03->Add(m_pFontPickerTitle, 0, wxALIGN_RIGHT | wxALL, 0);
    wxStaticText* itemStaticText05 = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Data:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer03->Add(itemStaticText05, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerData = new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, *g_pFontData,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer03->Add(m_pFontPickerData, 0, wxALIGN_RIGHT | wxALL, 0);
    wxStaticText* itemStaticText06 = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Label:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer03->Add(itemStaticText06, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerLabel = new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, *g_pFontLabel,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer03->Add(m_pFontPickerLabel, 0, wxALIGN_RIGHT | wxALL, 0);
    wxStaticText* itemStaticText07 = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Small:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer03->Add(itemStaticText07, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerSmall = new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, *g_pFontSmall,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer03->Add(m_pFontPickerSmall, 0, wxALIGN_RIGHT | wxALL, 0);
	
	// wxColourPickerCtrl
    wxStaticBox* itemStaticBoxUnits = new wxStaticBox(itemPanelNotebook02, wxID_ANY, _("Units, Ranges, Formats"));
    wxStaticBoxSizer* itemStaticBoxSizer04 = new wxStaticBoxSizer(itemStaticBoxUnits, wxHORIZONTAL);
    itemBoxSizer05->Add(itemStaticBoxSizer04, 0, wxEXPAND | wxALL, border_size);
    wxFlexGridSizer *itemFlexGridSizer04 = new wxFlexGridSizer(2);
    itemFlexGridSizer04->AddGrowableCol(1);
    itemStaticBoxSizer04->Add(itemFlexGridSizer04, 1, wxEXPAND | wxALL, 0);
    
    // Sets the maximum RPM in the tachometer control
    wxStaticText* itemStaticTextTachometerM = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Tachometer Maximum RPM:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer04->Add(itemStaticTextTachometerM, 0, wxEXPAND | wxALL, border_size);
    m_pSpinSpeedMax = new wxSpinCtrl(itemPanelNotebook02, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, g_iDashTachometerMax);
    itemFlexGridSizer04->Add(m_pSpinSpeedMax, 0, wxALIGN_RIGHT | wxALL, 0);

    // Enable the user to specify the temperature display in Celsius or Fahrenheit
    wxStaticText* itemStaticTextTemperatureU = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Temperature units:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer04->Add(itemStaticTextTemperatureU, 0, wxEXPAND | wxALL, border_size);
    wxString m_TemperatureUnitChoices[] = { _("Celsius"), _("Fahrenheit") };
    int m_TemperatureUnitNChoices = sizeof(m_TemperatureUnitChoices) / sizeof(wxString);
    m_pChoiceTemperatureUnit = new wxChoice(itemPanelNotebook02, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TemperatureUnitNChoices, m_TemperatureUnitChoices, 0);
    m_pChoiceTemperatureUnit->SetSelection(g_iDashTemperatureUnit);
    itemFlexGridSizer04->Add(m_pChoiceTemperatureUnit, 0, wxALIGN_RIGHT | wxALL, 0);

    // Enable the user to specify the engine oil pressure display in Bar or PSI
    wxStaticText* itemStaticTextPressureU = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Pressure units:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer04->Add(itemStaticTextPressureU, 0, wxEXPAND | wxALL, border_size);
    wxString m_PressureUnitChoices[] = { _("Bar"), _("PSI") };
    int m_PressureUnitNChoices = sizeof(m_PressureUnitChoices) / sizeof(wxString);
    m_pChoicePressureUnit = new wxChoice(itemPanelNotebook02, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PressureUnitNChoices, m_PressureUnitChoices, 0);
    m_pChoicePressureUnit->SetSelection(g_iDashPressureUnit);
    itemFlexGridSizer04->Add(m_pChoicePressureUnit, 0, wxALIGN_RIGHT | wxALL, 0);

    m_pCheckBoxDualengine = new wxCheckBox(itemPanelNotebook02, wxID_ANY, _("Dual Engine Vessel"),
            wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    m_pCheckBoxDualengine->SetValue(dualEngine);
    itemFlexGridSizer04->Add(m_pCheckBoxDualengine, 0, wxALIGN_RIGHT | wxALL, 0);

	wxStdDialogButtonSizer* DialogButtonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    itemBoxSizerMainPanel->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

    curSel = -1;
    for (size_t i = 0; i < m_Config.GetCount(); i++) {
        m_pListCtrlDashboards->InsertItem(i, 0);
        // Using data to store m_Config index for managing deletes
        m_pListCtrlDashboards->SetItemData(i, i);
    }
    m_pListCtrlDashboards->SetColumnWidth(0, wxLIST_AUTOSIZE);

    UpdateDashboardButtonsState();
    UpdateButtonsState();
    SetMinSize(wxSize(450, -1));
    Fit();
}

void DashboardPreferencesDialog::OnCloseDialog(wxCloseEvent& event) {
    SaveDashboardConfig();
    event.Skip();
}

void DashboardPreferencesDialog::SaveDashboardConfig(void) {
	
    g_iDashTachometerMax = m_pSpinSpeedMax->GetValue();
    g_iDashTemperatureUnit = m_pChoiceTemperatureUnit->GetSelection();
    g_iDashPressureUnit = m_pChoicePressureUnit->GetSelection();
    dualEngine = m_pCheckBoxDualengine->IsChecked();
    
    if (curSel != -1) {
        DashboardWindowContainer *cont = m_Config.Item(curSel);
        cont->m_bIsVisible = m_pCheckBoxIsVisible->IsChecked();
        cont->m_sCaption = m_pTextCtrlCaption->GetValue();
        cont->m_sOrientation = m_pChoiceOrientation->GetSelection() == 0 ? _T("V") : _T("H");
        cont->m_aInstrumentList.Clear();
        for (int i = 0; i < m_pListCtrlInstruments->GetItemCount(); i++)
            cont->m_aInstrumentList.Add((int) m_pListCtrlInstruments->GetItemData(i));
    }
}

void DashboardPreferencesDialog::OnDashboardSelected(wxListEvent& event) {
    SaveDashboardConfig();
    UpdateDashboardButtonsState();
}

void DashboardPreferencesDialog::UpdateDashboardButtonsState() {
    long item = -1;
    item = m_pListCtrlDashboards->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    bool enable = (item != -1);

    // Disable the Dashboard Delete button if the parent(Dashboard) of this dialog is selected.
    bool delete_enable = enable;
    if (item != -1) {
        int sel = m_pListCtrlDashboards->GetItemData(item);
        DashboardWindowContainer *cont = m_Config.Item(sel);
        DashboardWindow *dash_sel = cont->m_pDashboardWindow;
        if(dash_sel == GetParent())
            delete_enable = false;
    }
    m_pButtonDeleteDashboard->Enable(delete_enable);

    m_pPanelDashboard->Enable(enable);

    if (item != -1) {
        curSel = m_pListCtrlDashboards->GetItemData(item);
        DashboardWindowContainer *cont = m_Config.Item(curSel);
        m_pCheckBoxIsVisible->SetValue(cont->m_bIsVisible);
        m_pTextCtrlCaption->SetValue(cont->m_sCaption);
        m_pChoiceOrientation->SetSelection(cont->m_sOrientation == _T("V") ? 0 : 1);
        m_pListCtrlInstruments->DeleteAllItems();
        for (size_t i = 0; i < cont->m_aInstrumentList.GetCount(); i++) {
            wxListItem item;
            GetListItemForInstrument(item, cont->m_aInstrumentList.Item(i));
            item.SetId(m_pListCtrlInstruments->GetItemCount());
            m_pListCtrlInstruments->InsertItem(item);
        }

        m_pListCtrlInstruments->SetColumnWidth(0, wxLIST_AUTOSIZE);
    } else {
        curSel = -1;
        m_pCheckBoxIsVisible->SetValue(false);
        m_pTextCtrlCaption->SetValue(_T(""));
        m_pChoiceOrientation->SetSelection(0);
        m_pListCtrlInstruments->DeleteAllItems();
    }
	// UpdateButtonsState();
}

void DashboardPreferencesDialog::OnDashboardAdd(wxCommandEvent& event) {
    int idx = m_pListCtrlDashboards->GetItemCount();
    m_pListCtrlDashboards->InsertItem(idx, 0);
    // Data is index in m_Config
    m_pListCtrlDashboards->SetItemData(idx, m_Config.GetCount());
    wxArrayInt ar;
    DashboardWindowContainer *dwc = new DashboardWindowContainer(NULL, MakeName(), _("Dashboard"), _T("V"), ar);
    dwc->m_bIsVisible = true;
    m_Config.Add(dwc);
}

void DashboardPreferencesDialog::OnDashboardDelete(wxCommandEvent& event) {
    long itemID = -1;
    itemID = m_pListCtrlDashboards->GetNextItem(itemID, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    int idx = m_pListCtrlDashboards->GetItemData(itemID);
    m_pListCtrlDashboards->DeleteItem(itemID);
    m_Config.Item(idx)->m_bIsDeleted = true;
    UpdateDashboardButtonsState();
}

void DashboardPreferencesDialog::OnInstrumentSelected(wxListEvent& event) {
    UpdateButtonsState();
}

void DashboardPreferencesDialog::UpdateButtonsState() {
    long item = -1;
    item = m_pListCtrlInstruments->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    bool enable = (item != -1);

    m_pButtonDelete->Enable(enable);
	// m_pButtonEdit->Enable(false); // TODO: Properties
    m_pButtonUp->Enable(item > 0);
    m_pButtonDown->Enable(item != -1 && item < m_pListCtrlInstruments->GetItemCount() - 1);
}

void DashboardPreferencesDialog::OnInstrumentAdd(wxCommandEvent& event) {
    AddInstrumentDlg pdlg((wxWindow *) event.GetEventObject(), wxID_ANY);

    if (pdlg.ShowModal() == wxID_OK) {
        wxListItem item;
        GetListItemForInstrument(item, pdlg.GetInstrumentAdded());
        item.SetId(m_pListCtrlInstruments->GetItemCount());
        m_pListCtrlInstruments->InsertItem(item);
        m_pListCtrlInstruments->SetColumnWidth(0, wxLIST_AUTOSIZE);
        UpdateButtonsState();
    }
}

void DashboardPreferencesDialog::OnInstrumentDelete(wxCommandEvent& event) {
    long itemID = -1;
    itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    m_pListCtrlInstruments->DeleteItem(itemID);
    UpdateButtonsState();
}

void DashboardPreferencesDialog::OnInstrumentEdit(wxCommandEvent& event) {
// TODO: Instument options
}

void DashboardPreferencesDialog::OnInstrumentUp(wxCommandEvent& event) {
    long itemID = -1;
    itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    wxListItem item;
    item.SetId(itemID);
    item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_DATA);
    m_pListCtrlInstruments->GetItem(item);
    item.SetId(itemID - 1);
    m_pListCtrlInstruments->DeleteItem(itemID);
    m_pListCtrlInstruments->InsertItem(item);
    m_pListCtrlInstruments->SetItemState(itemID - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    UpdateButtonsState();
}

void DashboardPreferencesDialog::OnInstrumentDown(wxCommandEvent& event) {
    long itemID = -1;
    itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    wxListItem item;
    item.SetId(itemID);
    item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_DATA);
    m_pListCtrlInstruments->GetItem(item);
    item.SetId(itemID + 1);
    m_pListCtrlInstruments->DeleteItem(itemID);
    m_pListCtrlInstruments->InsertItem(item);
    m_pListCtrlInstruments->SetItemState(itemID + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    UpdateButtonsState();
}

//----------------------------------------------------------------
//
//    Add Instrument Dialog Implementation
//
//----------------------------------------------------------------

AddInstrumentDlg::AddInstrumentDlg(wxWindow *pparent, wxWindowID id) :
        wxDialog(pparent, id, _("Add instrument"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* itemBoxSizer01 = new wxBoxSizer(wxVERTICAL);
    SetSizer(itemBoxSizer01);
    wxStaticText* itemStaticText01 = new wxStaticText(this, wxID_ANY,
            _("Select instrument to add:"), wxDefaultPosition, wxDefaultSize, 0);
    itemBoxSizer01->Add(itemStaticText01, 0, wxEXPAND | wxALL, 5);

    wxImageList *imglist = new wxImageList(20, 20, true, 2);
    imglist->Add(*_img_instrument);
    imglist->Add(*_img_dial);

    m_pListCtrlInstruments = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(250, 180),
            wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_SORT_ASCENDING);
    itemBoxSizer01->Add(m_pListCtrlInstruments, 0, wxEXPAND | wxALL, 5);
    m_pListCtrlInstruments->AssignImageList(imglist, wxIMAGE_LIST_SMALL);
    m_pListCtrlInstruments->InsertColumn(0, _("Instruments"));
    wxStdDialogButtonSizer* DialogButtonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    itemBoxSizer01->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

	// Perhaps there should be a dummy first entry
	// Final loop, Do not reference an instrument, but the last dummy entry in the list
    for (unsigned int i = ID_DBP_MAIN_ENGINE_RPM; i < ID_DBP_LAST_ENTRY; i++) { 
        wxListItem item;
        GetListItemForInstrument(item,i);
        item.SetId(i);
        m_pListCtrlInstruments->InsertItem(item);
    }

    m_pListCtrlInstruments->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_pListCtrlInstruments->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

    Fit();
}

unsigned int AddInstrumentDlg::GetInstrumentAdded() {
    long itemID = -1;
    itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    return (int) m_pListCtrlInstruments->GetItemData(itemID);
}

//----------------------------------------------------------------
//
//    Dashboard Window Implementation
//
//----------------------------------------------------------------

// wxWS_EX_VALIDATE_RECURSIVELY required to push events to parents
DashboardWindow::DashboardWindow(wxWindow *pparent, wxWindowID id, wxAuiManager *auimgr,
        dashboard_pi* plugin, int orient, DashboardWindowContainer* mycont) :
        wxWindow(pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, _T("Dashboard")) {
    m_pauimgr = auimgr;
    m_plugin = plugin;
    m_Container = mycont;

	// wx2.9 itemBoxSizer = new wxWrapSizer(orient);
    itemBoxSizer = new wxBoxSizer(orient);
    SetSizer(itemBoxSizer);
    Connect(wxEVT_SIZE, wxSizeEventHandler(DashboardWindow::OnSize), NULL, this);
    Connect(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(DashboardWindow::OnContextMenu), NULL,
            this);
    Connect(wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(DashboardWindow::OnContextMenuSelect), NULL, this);
}

DashboardWindow::~DashboardWindow() {
    for (size_t i = 0; i < m_ArrayOfInstrument.GetCount(); i++) {
        DashboardInstrumentContainer *pdic = m_ArrayOfInstrument.Item(i);
        delete pdic;
    }
}

void DashboardWindow::OnSize(wxSizeEvent& event) {
    event.Skip();
    for (unsigned int i=0; i<m_ArrayOfInstrument.size(); i++) {
        DashboardInstrument* inst = m_ArrayOfInstrument.Item(i)->m_pInstrument;
        inst->SetMinSize(inst->GetSize(itemBoxSizer->GetOrientation(), GetClientSize()));
    }
    Layout();
    Refresh();
}

void DashboardWindow::OnContextMenu(wxContextMenuEvent& event) {
    wxMenu* contextMenu = new wxMenu();

    wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);
    if (pane.IsOk() && pane.IsDocked()) {
        contextMenu->Append(ID_DASH_UNDOCK, _("Undock"));
    }
    wxMenuItem* btnVertical = contextMenu->AppendRadioItem(ID_DASH_VERTICAL, _("Vertical"));
    btnVertical->Check(itemBoxSizer->GetOrientation() == wxVERTICAL);
    wxMenuItem* btnHorizontal = contextMenu->AppendRadioItem(ID_DASH_HORIZONTAL, _("Horizontal"));
    btnHorizontal->Check(itemBoxSizer->GetOrientation() == wxHORIZONTAL);
    contextMenu->AppendSeparator();

    m_plugin->PopulateContextMenu(contextMenu);

    contextMenu->AppendSeparator();
    contextMenu->Append(ID_DASH_PREFS, _("Preferences..."));
    PopupMenu(contextMenu);
    delete contextMenu;
}

void DashboardWindow::OnContextMenuSelect(wxCommandEvent& event) {
    if (event.GetId() < ID_DASH_PREFS) { 
	// Toggle dashboard visibility
        m_plugin->ShowDashboard(event.GetId()-1, event.IsChecked());
        SetToolbarItemState(m_plugin->GetToolbarItemId(), m_plugin->GetDashboardWindowShownCount() != 0);
    }

    switch(event.GetId()) {
        case ID_DASH_PREFS: {
            m_plugin->ShowPreferencesDialog(this);
            return; // Does it's own save.
        }
        case ID_DASH_VERTICAL: {
            ChangePaneOrientation(wxVERTICAL, true);
            m_Container->m_sOrientation = _T("V");
            break;
        }
        case ID_DASH_HORIZONTAL: {
            ChangePaneOrientation(wxHORIZONTAL, true);
            m_Container->m_sOrientation = _T("H");
            break;
        }
        case ID_DASH_UNDOCK: {
            ChangePaneOrientation(GetSizerOrientation(), true);
            return;     // Nothing changed so nothing need be saved
        }
    }
    
    m_plugin->SaveConfig();
}

void DashboardWindow::SetColorScheme(PI_ColorScheme cs) {
    DimeWindow(this);
    
    // Improve appearance, especially in DUSK or NIGHT palette
    wxColour col;
    GetGlobalColor(_T("DASHL"), &col);
    SetBackgroundColour(col);
    Refresh(false);
}

void DashboardWindow::ChangePaneOrientation(int orient, bool updateAUImgr) {
    m_pauimgr->DetachPane(this);
    SetSizerOrientation(orient);
    bool vertical = orient == wxVERTICAL;
    // wxSize sz = GetSize(orient, wxDefaultSize);
    wxSize sz = GetMinSize();
    // We must change Name to reset AUI perpective
    m_Container->m_sName = MakeName();
    m_pauimgr->AddPane(this, wxAuiPaneInfo().Name(m_Container->m_sName).Caption(
        m_Container->m_sCaption).CaptionVisible(true).TopDockable(!vertical).BottomDockable(
        !vertical).LeftDockable(vertical).RightDockable(vertical).MinSize(sz).BestSize(
        sz).FloatingSize(sz).FloatingPosition(100, 100).Float().Show(m_Container->m_bIsVisible));
    if (updateAUImgr) m_pauimgr->Update();
}

void DashboardWindow::SetSizerOrientation(int orient) {
    itemBoxSizer->SetOrientation(orient);
    // We must reset all MinSize to ensure we start with new default
    wxWindowListNode* node = GetChildren().GetFirst();
    while(node) {
        node->GetData()->SetMinSize(wxDefaultSize);
        node = node->GetNext();
    }
    SetMinSize(wxDefaultSize);
    Fit();
    SetMinSize(itemBoxSizer->GetMinSize());
}

int DashboardWindow::GetSizerOrientation() {
    return itemBoxSizer->GetOrientation();
}

bool isArrayIntEqual(const wxArrayInt& l1, const wxArrayOfInstrument &l2) {
    if (l1.GetCount() != l2.GetCount()) return false;

    for (size_t i = 0; i < l1.GetCount(); i++)
        if (l1.Item(i) != l2.Item(i)->m_ID) return false;

    return true;
}

bool DashboardWindow::isInstrumentListEqual(const wxArrayInt& list) {
    return isArrayIntEqual(list, m_ArrayOfInstrument);
}

// Create and display each instrument in a dashboard container
void DashboardWindow::SetInstrumentList(wxArrayInt list) {
    m_ArrayOfInstrument.Clear();
    itemBoxSizer->Clear(true);
    for (size_t i = 0; i < list.GetCount(); i++) {
        int id = list.Item(i);
        DashboardInstrument *instrument = NULL;
        switch (id) {
			case ID_DBP_MAIN_ENGINE_RPM:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_MAIN_ENGINE_RPM, 0, g_iDashTachometerMax);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(1000, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(200, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionExtraValue(OCPN_DBP_STC_MAIN_ENGINE_HOURS, _T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_PORT_ENGINE_RPM:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_PORT_ENGINE_RPM, 0, g_iDashTachometerMax);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(1000, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(200, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionExtraValue(OCPN_DBP_STC_PORT_ENGINE_HOURS, _T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_STBD_ENGINE_RPM:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_STBD_ENGINE_RPM, 0, g_iDashTachometerMax);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(1000, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(200, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionExtraValue(OCPN_DBP_STC_STBD_ENGINE_HOURS, _T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_MAIN_ENGINE_OIL:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_MAIN_ENGINE_OIL, 0, g_iDashPressureUnit == PRESSURE_BAR ? 5 : 80);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(g_iDashPressureUnit == PRESSURE_BAR ? 1 : 20,	DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(g_iDashPressureUnit == PRESSURE_BAR ? 0.5 : 10, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_PORT_ENGINE_OIL:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_PORT_ENGINE_OIL, 0, g_iDashPressureUnit == PRESSURE_BAR ? 5 : 80);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(g_iDashPressureUnit == PRESSURE_BAR ? 1 : 20, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(g_iDashPressureUnit == PRESSURE_BAR ? 0.5 : 10, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_STBD_ENGINE_OIL:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_STBD_ENGINE_OIL, 0, g_iDashPressureUnit == PRESSURE_BAR ? 5 : 80);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(g_iDashPressureUnit == PRESSURE_BAR ? 1 : 20, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(g_iDashPressureUnit == PRESSURE_BAR ? 0.5 : 10, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_MAIN_ENGINE_WATER:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_MAIN_ENGINE_WATER, g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 60 : 100 , g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 120 : 250);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_PORT_ENGINE_WATER:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_PORT_ENGINE_WATER, 60, 120);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_STBD_ENGINE_WATER:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_STBD_ENGINE_WATER, 60, 120);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(g_iDashTemperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_MAIN_ENGINE_VOLTS:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_MAIN_ENGINE_VOLTS, 8, 16);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_PORT_ENGINE_VOLTS:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_PORT_ENGINE_VOLTS, 8, 16);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(2,	DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_STBD_ENGINE_VOLTS:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_STBD_ENGINE_VOLTS, 8, 16);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(2,	DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_FUEL_TANK:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_FUEL, 0, 100);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(12.5,	DIAL_MARKER_WARNING_LOW, 1);
				break;
			case ID_DBP_WATER_TANK:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_WATER, 0, 100);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(12.5,	DIAL_MARKER_WARNING_LOW, 1);
				break;
			case ID_DBP_OIL_TANK:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_OIL, 0, 100);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
				break;
			case ID_DBP_LIVEWELL_TANK:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, 0, 100);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
				break;
			case ID_DBP_GREY_TANK:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_GREY, 0, 100);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_HIGH, 1);
				break;
			case ID_DBP_BLACK_TANK:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
					GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_BLACK, 0, 100);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_HIGH, 1);
				break;
			case ID_DBP_START_BATTERY_VOLTS:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY, GetInstrumentCaption(id), 
				OCPN_DBP_STC_START_BATTERY_VOLTS, 8, 16);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(1, DIAL_MARKER_GREEN_MID, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionExtraValue(OCPN_DBP_STC_START_BATTERY_AMPS, _T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_HOUSE_BATTERY_VOLTS:
				instrument = new DashboardInstrument_Speedometer(this, wxID_ANY, GetInstrumentCaption(id), 
				OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, 8, 16);
				((DashboardInstrument_Dial *)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
				((DashboardInstrument_Dial *)instrument)->SetOptionMarker(1, DIAL_MARKER_GREEN_MID, 1);
				((DashboardInstrument_Dial *)instrument)->SetOptionExtraValue(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, _T("%.1f"), DIAL_POSITION_INSIDE);
				break;
			case ID_DBP_RSA:
				instrument = new DashboardInstrument_RudderAngle(this, wxID_ANY, GetInstrumentCaption(id));
				((DashboardInstrument_RudderAngle *)instrument)->SetOptionMarker(5, DIAL_MARKER_REDGREEN, 2);
				wxString labels[] = {_T("40"), _T("30"), _T("20"), _T("10"), _T("0"), _T("10"), _T("20"), _T("30"), _T("40")};
				((DashboardInstrument_RudderAngle *)instrument)->SetOptionLabel(10, DIAL_LABEL_HORIZONTAL, wxArrayString(9,labels));
				break;
		}
        if (instrument) {
            instrument->instrumentTypeId = id;
            m_ArrayOfInstrument.Add(new DashboardInstrumentContainer(id, instrument,instrument->GetCapacity()));
            itemBoxSizer->Add(instrument, 0, wxEXPAND, 0);
            if (itemBoxSizer->GetOrientation() == wxHORIZONTAL) {
                itemBoxSizer->AddSpacer(5);
            }
        }
    }
    Fit();
    Layout();
    SetMinSize(itemBoxSizer->GetMinSize());
}

void DashboardWindow::SendSentenceToAllInstruments(int st, double value, wxString unit) {
    for (size_t i = 0; i < m_ArrayOfInstrument.GetCount(); i++) {
		if (m_ArrayOfInstrument.Item(i)->m_cap_flag & st) {
			m_ArrayOfInstrument.Item(i)->m_pInstrument->SetData(st, value, unit);
		}
    }
}
