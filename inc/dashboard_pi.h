/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - plugin class declaration
 * Author:   Jean-Eudes Onfray
 * expanded: Bernd Cirotzki 2023 (special colour design)
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _DASHBOARD_PI_H_
#define _DASHBOARD_PI_H_

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

// Automagically generated via config.h.in
// API and Plugin version numbers
#include "config.h"

#include <ocpn_plugin.h>

#ifdef __OCPN__ANDROID__
#include <wx/qt/private/wxQtGesture.h>
#endif

// STL
#include <assert.h>
#include <cmath>
#include <typeinfo>

// wxWidgets
#include <wx/fileconf.h>
#include <wx/aui/aui.h>
#include <wx/filename.h>
#include <wx/fontdlg.h>

// Required libraries
#include <nmea0183.h>
#include <wx/jsonval.h>
#include <wx/jsonreader.h>
#include <wx/jsonwriter.h>
#include <N2KParser.h>

// Refactored headers
#include "dashboard_globals.h"
#include "dashboard_window_container.h"
#include "dashboard_window.h"
#include "dashboard_preferences_dialog.h"
#include "ocpn_font_button.h"
#include "edit_dialog.h"
#include "add_instrument_dlg.h"
#include "instrument.h"
#include "speedometer.h"
#include "rudder_angle.h"

#ifndef PI
#define PI 3.1415926535897931160E0
#endif

// Request default positioning of toolbar tool
#define DASHBOARD_TOOL_POSITION -1 

// If no data received in 5 seconds, zero the instrument displays
#define WATCHDOG_TIMEOUT_COUNT  5

// Kelvin to celsius
#define CONST_KELVIN 273.15
#define CONVERT_KELVIN(x) ((x) - CONST_KELVIN )

// RADIANS/DEGREES
#define RADIANS_TO_DEGREES(x) ((x) * 180 / M_PI)

// LITRE to GALLON
#define LITRES_GALLONS(x) (x / 3.7)

//  instrument_pi  — the plugin entry point
class Dashboard : public wxTimer, public opencpn_plugin_120 {
public:
    Dashboard(void *ppimgr);
    ~Dashboard(void);

    // Mandatory OpenCPN Plugin API's
    int  Init(void) override;
    bool DeInit(void) override;
    void Notify() override;
    int GetAPIVersionMajor() override;
    int GetAPIVersionMinor() override;
    int GetPlugInVersionMajor() override;
    int GetPlugInVersionMinor() override;
    wxBitmap *GetPlugInBitmap() override;
    wxString GetCommonName() override;
    wxString GetShortDescription() override;
    wxString GetLongDescription() override;

    // Optional OpenCPN Plugin API's
    int  GetToolbarToolCount(void) override;
    void OnToolbarToolCallback(int id) override;
    void ShowPreferencesDialog(wxWindow *parent) override;
    void SetColorScheme(PI_ColorScheme cs) override;
    void UpdateAuiStatus(void) override;
	
    // Plugin functions
    void OnPaneClose(wxAuiManagerEvent& event);
    bool SaveConfig(void);
    void PopulateContextMenu(wxMenu *menu);
    void ShowDashboard(size_t id, bool visible);
    int GetDashboardWindowShownCount();
	int GetToolbarItemId(void);

private:
    bool LoadConfig(void);
    void LoadFont(wxFont **target, wxString native_info);
    void ApplyConfig(void);
    void SendSentenceToAllInstruments(DASH_CAP cap_flag, double value, wxString unit);
    
	// Used to parse JSON values from SignalK
	// BUG BUG Should update to Lohmann library
	wxJSONValue root;
	wxJSONReader jsonReader;
	wxString self;
	void ParseSignalK(wxJSONValue& update);
	bool CheckAlarmState(wxJSONValue& value);

	// Used to parse NMEA Sentences
	NMEA0183 m_NMEA0183;

	// Initialize NMEA 183 Listeners
	void HandleXDR(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_xdr;

	void HandleRPM(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_rpm;

	void HandleRSA(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_rsa;

	// Initialize SignalK Listeners
	void HandleSignalK(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_signalk;

	// NMEA 2000
	// index into the payload.
	// The payload is in Actisense format, so as I've just pasted code from twocan, this simplifies 
	// accessing the data
	const int index = 13;

	// Engine Parameters - Rapid Update
	void HandleN2K_127488(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_127488;

	// Engine Parameters - Dynamic
	void HandleN2K_127489(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_127489;

	// Fluid Levels
	void HandleN2K_127505(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_127505;

	// DC Detailed Status
	void HandleN2K_127506(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_127506;

	// Battery Status
	void HandleN2K_127508(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_127508;

	// Temperature
	void HandleN2K_130312(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_130312;

	// Rudder Angle
	void HandleN2K_127245(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_127245;

	// Watchdog timer, performs two functions, firstly refresh the dashboard every second,  
	// and secondly, if no data is received, set instruments to zero (eg. Engine switched off)
	wxDateTime m_engineWatchDog;
	wxDateTime m_tankLevelWatchDog;

	// Store the current engine hours for displaying in the Tachometer Dial
	double m_mainEngineHours;
	double m_portEngineHours;
	double m_stbdEngineHours;

	// Conversion Utilities
	double Celsius2Fahrenheit(double temperature);
	double Fahrenheit2Celsius(double temperature);
	double Pascal2Psi(double pressure);
	double Psi2Pascal(double pressure);

	// NMEA 2000 Data Validation
	template<typename T>
	static bool IsDataValid(T value);

	static bool IsDataValid(uint8_t value) {
		if ((value == UCHAR_MAX) || (value == UCHAR_MAX - 1) || (value == UCHAR_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(char value) {
		if ((value == CHAR_MAX) || (value == CHAR_MAX - 1) || (value == CHAR_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(unsigned short value) {
		if ((value == USHRT_MAX) || (value == USHRT_MAX - 1) || (value == USHRT_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(short value) {
		if ((value == SHRT_MAX) || (value == SHRT_MAX - 1) || (value == SHRT_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(unsigned int value) {
		if ((value == UINT_MAX) || (value == UINT_MAX - 1) || (value == UINT_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(int value) {
		if ((value == INT_MAX) || (value == INT_MAX - 1) || (value == INT_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(unsigned long value) {
		if ((value == ULONG_MAX) || (value == ULONG_MAX - 1) || (value == ULONG_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(long value) {
		if ((value == LONG_MAX) || (value == LONG_MAX - 1) || (value == LONG_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(unsigned long long value) {
		if ((value == ULLONG_MAX) || (value == ULLONG_MAX - 1) || (value == ULLONG_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	static bool IsDataValid(long long value) {
		if ((value == LLONG_MAX) || (value == LLONG_MAX - 1) || (value == LLONG_MAX - 2)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

    wxString m_self;
    wxFileConfig *m_pconfig;
    wxAuiManager *m_pauimgr;
    int m_toolbar_item_id;

    wxArrayOfDashboard m_ArrayOfDashboardWindow;
    int m_show_id;
    int m_hide_id;
	
    int m_config_version;

    
};

#endif  // _DASHBOARD_PI_H_
