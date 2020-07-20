//
// Author: Steven Adler
// 
// Modified the existing dashboard plugin to create an "Engine Dashboard"
// Parses NMEA 0183 RSA, RPM & XDR sentences and displays Engine RPM, Oil Pressure, Water Temperature, 
// Alternator Voltage, Engine Hours andFluid Levels in a dashboard
//
// Version 1.0
// 10-10-2019
// 
// Please send bug reports to twocanplugin@hotmail.com or to the opencpn forum
//
// BUG BUG Refactor to separate each class into it's own source files
// BUG BUG Consistent style to separate class declaration in header files and class implementation in source files
/******************************************************************************
 * $Id: dashboard_pi.h, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
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

#ifndef _DASHBOARDPI_H_
#define _DASHBOARDPI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/notebook.h>
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/spinctrl.h>
#include <wx/aui/aui.h>
#include <wx/fontpicker.h>

// Defines version numbers, names etc. for this plugin
// This is automagically constructed via version.h.in from CMakeLists.txt, personally I think this is convoluted
#include "version.h"

// Differs from the built-in plugins, so that we can build outside of OpenCPN source tree
#include "ocpn_plugin.h"

// NMEA0183 Sentence parsing functions
#include "nmea0183.h"

// Dashboard instruments/dials/gauges
#include "instrument.h"
#include "speedometer.h"
#include "rudder_angle.h"

class DashboardWindow;
class DashboardWindowContainer;
class DashboardInstrumentContainer;

// Request default positioning of toolbar tool
#define DASHBOARD_TOOL_POSITION -1          

// If no data received in 5 seconds, zero the instrument displays
#define WATCHDOG_TIMEOUT_COUNT  5

// Conversion functions
double celsius2fahrenheit(double tempeature);
double fahrenheit2celsius(double tempeature);
double pascal2psi(double pressure);
double psi2pascal(double pressure);


class DashboardWindowContainer {
public:
	DashboardWindowContainer(DashboardWindow *dashboard_window, wxString name, wxString caption, wxString orientation, wxArrayInt inst) {
       m_pDashboardWindow = dashboard_window; m_sName = name; m_sCaption = caption; m_sOrientation = orientation; m_aInstrumentList = inst; m_bIsVisible = false; m_bIsDeleted = false; }

	~DashboardWindowContainer(){}

	DashboardWindow *m_pDashboardWindow;
	bool m_bIsVisible;
	bool m_bIsDeleted;
	// Persists visibility, even when Dashboard tool is toggled off.
	bool m_bPersVisible;  
	wxString m_sName;
	wxString m_sCaption;
	wxString m_sOrientation;
	wxArrayInt m_aInstrumentList;
};

class DashboardInstrumentContainer {
public:
	DashboardInstrumentContainer(int id, DashboardInstrument *instrument, int capa) {
		m_ID = id; m_pInstrument = instrument; m_cap_flag = capa; }

	~DashboardInstrumentContainer(){ delete m_pInstrument; }

	DashboardInstrument *m_pInstrument;
	int m_ID;
	int m_cap_flag;
};

// Dynamic arrays of pointers need explicit macros in wx261
#ifdef __WX261
WX_DEFINE_ARRAY_PTR(DashboardWindowContainer *, wxArrayOfDashboard);
WX_DEFINE_ARRAY_PTR(DashboardInstrumentContainer *, wxArrayOfInstrument);
#else
WX_DEFINE_ARRAY(DashboardWindowContainer *, wxArrayOfDashboard);
WX_DEFINE_ARRAY(DashboardInstrumentContainer *, wxArrayOfInstrument);
#endif


//
// Engine Dashboard PlugIn Class Definition
//

class dashboard_pi : public opencpn_plugin_116, wxTimer {
public:
	dashboard_pi(void *ppimgr);
	~dashboard_pi(void);

	// The required OpenCPN PlugIn methods
	int Init(void);
	bool DeInit(void);
	int GetAPIVersionMajor();
	int GetAPIVersionMinor();
	int GetPlugInVersionMajor();
	int GetPlugInVersionMinor();
    wxBitmap *GetPlugInBitmap();
	wxString GetCommonName();
	wxString GetShortDescription();
	wxString GetLongDescription();
	
	// As we inherit from wxTimer, the method invoked each timer interval
	// Used by the plugin to refresh the instruments and to detect stale data 
	void Notify();

	// The optional OpenCPN plugin methods
	void SetNMEASentence(wxString &sentence);
	int GetToolbarToolCount(void);
	void OnToolbarToolCallback(int id);
	void ShowPreferencesDialog(wxWindow *parent);
	void SetColorScheme(PI_ColorScheme cs);
	void OnPaneClose(wxAuiManagerEvent& event);
	void UpdateAuiStatus(void);
	bool SaveConfig(void);
	void PopulateContextMenu(wxMenu *menu);
	void ShowDashboard(size_t id, bool visible);
	int GetToolbarItemId();
	int GetDashboardWindowShownCount();
	  
private:
	// Load plugin configuraton
	bool LoadConfig(void);
	void ApplyConfig(void);
	// Send deconstructed NMEA 1083 sentence  values to each display
	void SendSentenceToAllInstruments(int st, double value, wxString unit);

	// OpenCPN goodness, pointers to Configuration, AUI Manager and Toolbar
	wxFileConfig *m_pconfig;
	wxAuiManager *m_pauimgr;
	int m_toolbar_item_id;

	// Hide/Show Dashboard Windows
	wxArrayOfDashboard m_ArrayOfDashboardWindow;
	int m_show_id;
	int m_hide_id;

	// Used to parse NMEA Sentences
	NMEA0183 m_NMEA0183;

	// For some reason in older dashboard implementations used this variable was used to differentiate config file versions
	// Engine Dashboard uses version 2 configuration settings
	int m_config_version;
};

class DashboardPreferencesDialog : public wxDialog {
public:
	DashboardPreferencesDialog(wxWindow *pparent, wxWindowID id, wxArrayOfDashboard config);
	~DashboardPreferencesDialog() {}

	void OnCloseDialog(wxCloseEvent& event);
	void OnDashboardSelected(wxListEvent& event);
	void OnDashboardAdd(wxCommandEvent& event);
	void OnDashboardDelete(wxCommandEvent& event);
	void OnInstrumentSelected(wxListEvent& event);
	void OnInstrumentAdd(wxCommandEvent& event);
	void OnInstrumentEdit(wxCommandEvent& event);
	void OnInstrumentDelete(wxCommandEvent& event);
	void OnInstrumentUp(wxCommandEvent& event);
	void OnInstrumentDown(wxCommandEvent& event);
	void SaveDashboardConfig(void);

	wxArrayOfDashboard m_Config;
	wxFontPickerCtrl *m_pFontPickerTitle;
	wxFontPickerCtrl *m_pFontPickerData;
	wxFontPickerCtrl *m_pFontPickerLabel;
	wxFontPickerCtrl *m_pFontPickerSmall;
	wxSpinCtrl *m_pSpinSpeedMax;
    wxSpinCtrl *m_pSpinCOGDamp;
    wxSpinCtrl *m_pSpinSOGDamp;
    wxChoice *m_pChoiceUTCOffset;
    wxChoice *m_pChoiceTemperatureUnit;
    wxChoice *m_pChoicePressureUnit;
    wxSpinCtrlDouble *m_pSpinDBTOffset;
    wxChoice *m_pChoiceDistanceUnit;
    wxChoice *m_pChoiceWindSpeedUnit;

private:
	void UpdateDashboardButtonsState(void);
	void UpdateButtonsState(void);
	int curSel;
	wxListCtrl *m_pListCtrlDashboards;
	wxBitmapButton *m_pButtonAddDashboard;
	wxBitmapButton *m_pButtonDeleteDashboard;
	wxPanel *m_pPanelDashboard;
	wxTextCtrl *m_pTextCtrlCaption;
	wxCheckBox *m_pCheckBoxIsVisible;
	wxChoice *m_pChoiceOrientation;
	wxListCtrl *m_pListCtrlInstruments;
	wxButton *m_pButtonAdd;
	wxButton *m_pButtonEdit;
	wxButton *m_pButtonDelete;
	wxButton *m_pButtonUp;
	wxButton *m_pButtonDown;
};

class AddInstrumentDlg : public wxDialog {
public:
	AddInstrumentDlg(wxWindow *pparent, wxWindowID id);
	~AddInstrumentDlg() {}

	unsigned int GetInstrumentAdded();

private:
	wxListCtrl *m_pListCtrlInstruments;
};

enum {
	ID_DASHBOARD_WINDOW
};

enum {
	ID_DASH_PREFS = 999,
	ID_DASH_VERTICAL,
	ID_DASH_HORIZONTAL,
	ID_DASH_UNDOCK
};

enum {
	PRESSURE_BAR,
	PRESSURE_PSI
};

enum {
	TEMPERATURE_CELSIUS,
	TEMPERATURE_FAHRENHEIT
};

class DashboardWindow : public wxWindow {
public:
	DashboardWindow(wxWindow *pparent, wxWindowID id, wxAuiManager *auimgr, dashboard_pi* plugin,
             int orient, DashboardWindowContainer* mycont);
    ~DashboardWindow();

    void SetColorScheme(PI_ColorScheme cs);
    void SetSizerOrientation(int orient);
    int GetSizerOrientation();
    void OnSize(wxSizeEvent& evt);
    void OnContextMenu(wxContextMenuEvent& evt);
    void OnContextMenuSelect(wxCommandEvent& evt);
    bool isInstrumentListEqual(const wxArrayInt& list);
    void SetInstrumentList(wxArrayInt list);
    void SendSentenceToAllInstruments(int st, double value, wxString unit);
    void ChangePaneOrientation(int orient, bool updateAUImgr);

	// TODO: OnKeyPress pass event to main window or disable focus

    DashboardWindowContainer *m_Container;

private:
	wxAuiManager *m_pauimgr;
	dashboard_pi *m_plugin;
	wxBoxSizer *itemBoxSizer;
	wxArrayOfInstrument m_ArrayOfInstrument;
};

#endif

