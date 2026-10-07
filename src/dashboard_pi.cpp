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
// 1.3. 20-12-2020 - Add support for additional transducer names
// 1.4. 10-10-2021 - Support lower/camel/upper case transducer names
//                 - Additional Fuel & Water tanks (for NMEA 0183 V4.11 standard names, Eg. FreshWater#2)
//                 - Tank level & battery voltage gauges are no longer zeroed when the engine is off (no RPM's)
//                 - Support for SignalK data
// 1.4.1 16-12-2021 - Fix uninitailzed watchdog timers
// 1.4.2 20-05-2022 - Add Yacht Devices engine hours transducer name (EngineHours#x), 
//                  - New gauges for Engine Exhaust (EngineExhaust#n)
// 1.5   30-10-2022 - Add support for OpenCPN v5.8 native NMEA 2000 network connections
// 1.6   30-08-2023 - Fix CircleCI build platforms, Add NMEA 183 Listeners, Add tank guage controls
// 1.7   01/02/2024 - Fix for multicanvas docking 
// 1.8   08/05/2024 - Fix for engine hours (decimal units), Add PGN 127245 Rudder Angle
// 1.81  05/06/2024 - Fix for incorrect display of rudder angle
// 1.9	 01/12/2024 - Add icon display in tachometer for engine faults, added corresponding SignalK notifications
// 1.91  22/05/2026 - Add PGN 127506 DC Detailed Status for State of Charge & Amp Hours
// 2.0   01/10/2026 - Adopted current Dashboard model (for Android support), Use SignalK observer model,
//                    and refactored
// 
// Please send bug reports to twocanplugin@hotmail.com or to the opencpn forum
//
/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - dashboard_pi class implementation
 * Author:   Jean-Eudes Onfray
 * expanded: Bernd Cirotzki 2023 (special colour design)
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "dashboard_pi.h"


// BUG BUG Commented out as it broke build. What needs to be fixed ?
//#include "../../../gui/include/gui/ocpn_fontdlg.h"
// 
// Intended to load OpenCPN offline manual
//#include "manual.h"

#ifdef __OCPN__ANDROID__
#include "qdebug.h"
#include <QtWidgets/QScroller>
#endif

Dashboard::Dashboard(void *ppimgr) : wxTimer(this), opencpn_plugin_120(ppimgr) {
     
  // Uses SVG icons instead of PNG files
  g_pluginFolder = GetPluginDataDir(PKG_NAME) + wxFileName::GetPathSeparator() + "data" + wxFileName::GetPathSeparator();
  // The plugin icon
  g_pluginBitmap = GetBitmapFromSVGFile(g_pluginFolder + "engine-dashboard-colour.svg", 32, 32);

  // Existing icons used in preferences dialog
  g_dashboardBitmap = GetBitmapFromSVGFile(g_pluginFolder + "dashboard.svg", 32, 32);
  g_dialBitmap = GetBitmapFromSVGFile(g_pluginFolder + "dial.svg", 16, 16);
  g_instrumentBitmap = GetBitmapFromSVGFile(g_pluginFolder + "instrument.svg", 16, 16);
  g_minusBitmap = GetBitmapFromSVGFile(g_pluginFolder + "minus.svg", 16, 16);
  g_plusBitmap = GetBitmapFromSVGFile(g_pluginFolder + "plus.svg", 16, 16);

}

Dashboard::~Dashboard(void) {
}

int Dashboard::Init(void) {
	AddLocaleCatalog(_T("opencpn-engine_dashboard_pi"));

  
  g_pFontTitle = new wxFontData();
  g_pFontTitle->SetChosenFont(
      wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL));

  g_pFontData = new wxFontData();
  g_pFontData->SetChosenFont(
      wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

  g_pFontLabel = new wxFontData();
  g_pFontLabel->SetChosenFont(
      wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

  g_pFontSmall = new wxFontData();
  g_pFontSmall->SetChosenFont(
      wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

  g_pUSFontTitle = &g_USFontTitle;
  g_pUSFontData = &g_USFontData;
  g_pUSFontLabel = &g_USFontLabel;
  g_pUSFontSmall = &g_USFontSmall;

  // Initialize wxWidgets Advanced User Interface (wxAUI)
  m_pauimgr = GetFrameAuiManager();
  m_pauimgr->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(Dashboard::OnPaneClose), NULL, this);

  // Get a pointer to the opencpn configuration object
  m_pconfig = GetOCPNConfigObject();

  // And load the configuration items
  LoadConfig();

  // Initialize the dashboard toolbar button
  wxString normalIcon = g_pluginFolder + "engine-dashboard-normal.svg";
  wxString toggledIcon = g_pluginFolder + "engine-dashboard-toggled.svg";
  wxString rolloverIcon = g_pluginFolder + "engine-dashboard-rollover.svg";

  m_toolbar_item_id = InsertPlugInToolSVG("Engine Dashboard", normalIcon,
      rolloverIcon, toggledIcon, wxITEM_CHECK, "Engine Dashboard", "Engine Dashboard - refactored", NULL, DASHBOARD_TOOL_POSITION, 0, this);

  ApplyConfig();

  //  If we loaded a version 1 config setup, convert now to version 2
  if (m_config_version == 1) {
    SaveConfig();
  }

 // Initialize listeners
 // PGN 127488 Engine Parameters Rapid Update
  wxDEFINE_EVENT(EVT_N2K_127488, ObservedEvt);
  NMEA2000Id id_127488 = NMEA2000Id(127488);
  listener_127488 = std::move(GetListener(id_127488, EVT_N2K_127488, this));
  Bind(EVT_N2K_127488, [&](ObservedEvt ev) {
	  HandleN2K_127488(ev);
	  });

  // PGN 127489 Engine Parameters Dynamic
  wxDEFINE_EVENT(EVT_N2K_127489, ObservedEvt);
  NMEA2000Id id_127489 = NMEA2000Id(127489);
  listener_127489 = std::move(GetListener(id_127489, EVT_N2K_127489, this));
  Bind(EVT_N2K_127489, [&](ObservedEvt ev) {
	  HandleN2K_127489(ev);
	  });

  // PGN 127505 Fluid Levels
  wxDEFINE_EVENT(EVT_N2K_127505, ObservedEvt);
  NMEA2000Id id_127505 = NMEA2000Id(127505);
  listener_127505 = std::move(GetListener(id_127505, EVT_N2K_127505, this));
  Bind(EVT_N2K_127505, [&](ObservedEvt ev) {
	  HandleN2K_127505(ev);
	  });

  // PGN 127506 DC Detailed Status
  wxDEFINE_EVENT(EVT_N2K_127506, ObservedEvt);
  NMEA2000Id id_127506 = NMEA2000Id(127506);
  listener_127506 = std::move(GetListener(id_127506, EVT_N2K_127506, this));
  Bind(EVT_N2K_127506, [&](ObservedEvt ev) {
	  HandleN2K_127506(ev);
	  });

  // PGN 127508 Battery Status
  wxDEFINE_EVENT(EVT_N2K_127508, ObservedEvt);
  NMEA2000Id id_127508 = NMEA2000Id(127508);
  listener_127508 = std::move(GetListener(id_127508, EVT_N2K_127508, this));
  Bind(EVT_N2K_127508, [&](ObservedEvt ev) {
	  HandleN2K_127508(ev);
	  });

  // PGN 127245 Rudder Angle
  wxDEFINE_EVENT(EVT_N2K_127245, ObservedEvt);
  NMEA2000Id id_127245 = NMEA2000Id(127245);
  listener_127245 = std::move(GetListener(id_127245, EVT_N2K_127245, this));
  Bind(EVT_N2K_127245, [&](ObservedEvt ev) {
	  HandleN2K_127245(ev);
	  });

  // Initialize NMEA 183 Listeners
  // $--XDR Transducers
  wxDEFINE_EVENT(EVT_183_XDR, ObservedEvt);
  NMEA0183Id id_xdr = NMEA0183Id("XDR");
  listener_xdr = std::move(GetListener(id_xdr, EVT_183_XDR, this));
  Bind(EVT_183_XDR, [&](ObservedEvt ev) {
	  HandleXDR(ev);
	  });

  // $--RPM
  wxDEFINE_EVENT(EVT_183_RPM, ObservedEvt);
  NMEA0183Id id_rpm = NMEA0183Id("RPM");
  listener_rpm = std::move(GetListener(id_rpm, EVT_183_RPM, this));
  Bind(EVT_183_RPM, [&](ObservedEvt ev) {
	  HandleRPM(ev);
	  });

  // $--RSA
  wxDEFINE_EVENT(EVT_183_RSA, ObservedEvt);
  NMEA0183Id id_rsa = NMEA0183Id("RSA");
  listener_rsa = std::move(GetListener(id_rsa, EVT_183_RSA, this));
  Bind(EVT_183_RSA, [&](ObservedEvt ev) {
	  HandleRSA(ev);
	  });

  // Initialize SignalK Listeners
  // self.vessels.propulsion
  wxDEFINE_EVENT(EVT_SIGNALK, ObservedEvt);
  SignalkId id_signalk = SignalkId("self");
  listener_signalk = std::move(GetListener(id_signalk, EVT_SIGNALK, this));
  Bind(EVT_SIGNALK, [&](ObservedEvt ev) {
	  HandleSignalK(ev);
	  });

  // Initialize the watchdog timers
  // Engine watchdog zeros tachometer, oil pressure & engine temperature if no RPM's received
  // Tank level watchdog zeroes tanks if no tank level data is received
  m_engineWatchDog = wxDateTime::Now() - wxTimeSpan::Seconds(5);
  m_tankLevelWatchDog = wxDateTime::Now() - wxTimeSpan::Seconds(5);
  
  Start(1000, wxTIMER_CONTINUOUS);

  return ( WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_CONFIG |
          WANTS_PREFERENCES  | USES_AUI_MANAGER);
}

bool Dashboard::DeInit(void) {
  SaveConfig();
  // If timer is started, stop it
  if (IsRunning())  {
    Stop(); 
  }

  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindow *dashboard_window =
        m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
    if (dashboard_window) {
      m_pauimgr->DetachPane(dashboard_window);
      dashboard_window->Close();
      dashboard_window->Destroy();
      m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow = NULL;
    }
  }

  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindowContainer *pdwc = m_ArrayOfDashboardWindow.Item(i);
    delete pdwc;
  }

  return true;
}

// Invoked by the timer
void Dashboard::Notify() {
  
  //  Manage the watchdogs
  // BUG BUG Consider using OCPN_DBP_STC as the for loop constraints
  if (wxDateTime::Now() > (m_engineWatchDog + wxTimeSpan::Seconds(5))) {
	  // Zero the engine instruments
	  // We go from zero to ID_DBP_FUEL_TANK_01 + 3, because there are three additional values
	  // in OCPN_DBP_STC_... (instrument.h) for the engine hours, which 
	  // do not have their own gauge, but populate the engine rpm gauges
	  for (int i = 0; i < ID_DBP_FUEL_TANK_01 + 3; i++) {
		  SendSentenceToAllInstruments((DASH_CAP)i, 0.0f, "");
	  }
  }

  if (wxDateTime::Now() > (m_tankLevelWatchDog + wxTimeSpan::Seconds(5))) {
	  // Zero the tank instruments
	  // We go from ID_DBP_FUEL_TANK_01 + 3 to IDP_LAST_ENTRY + 3, 
	  // because there are three additional values
	  // in OCPN_DBP_STC_... (instrument.h) for the engine hours, which 
	  // do not have their own gauge, but populate the engine rpm gauges
	  for (int i = ID_DBP_FUEL_TANK_01 + 3; i < ID_DBP_LAST_ENTRY + 3; i++) {
		  SendSentenceToAllInstruments((DASH_CAP)i, 0.0f, "");
	  }
  }

  // Force a repaint of all the instruments
  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
	  DashboardWindow* dashboard_window =
		  m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
	  if (dashboard_window) {
		  dashboard_window->Refresh();
#ifdef __OCPN__ANDROID__
		  wxWindowList list = dashboard_window->GetChildren();
		  wxWindowListNode* node = list.GetFirst();
		  for (size_t i = 0; i < list.GetCount(); i++) {
			  wxWindow* win = node->GetData();
			  // qDebug() << "Refresh Dash child:" << i;
			  win->Refresh();
			  node = node->GetNext();
		  }
#endif
	  }
  }
}

// OpenCPN Mandatory plugin functions
int Dashboard::GetAPIVersionMajor() { 
    return atoi(API_VERSION);
}

int Dashboard::GetAPIVersionMinor() { 
    std::string v(API_VERSION);
    size_t dotpos = v.find('.');
    return atoi(v.substr(dotpos + 1).c_str());
}

int Dashboard::GetPlugInVersionMajor() { 
    return PLUGIN_VERSION_MAJOR;
}

int Dashboard::GetPlugInVersionMinor() { 
    return PLUGIN_VERSION_MINOR;
}

wxBitmap *Dashboard::GetPlugInBitmap() { 
    return &g_pluginBitmap; 
}

wxString Dashboard::GetCommonName() { 
    return PLUGIN_API_NAME;
}

wxString Dashboard::GetShortDescription() {
    return PKG_SUMMARY;
}

wxString Dashboard::GetLongDescription() {
    return wxString(PKG_DESCRIPTION);
}

// a few conversion functions
double Dashboard::Celsius2Fahrenheit(double temperature) {
	return (temperature * 9 / 5) + 32;
}

double Dashboard::Fahrenheit2Celsius(double temperature) {
	return (temperature - 32) * 5 / 9;
}

double Dashboard::Pascal2Psi(double pressure) {
	return pressure * 0.000145f;
}

double Dashboard::Psi2Pascal(double pressure) {
	return pressure * 6894.745f;
}

// Update all of the displayed instruments
void Dashboard::SendSentenceToAllInstruments(DASH_CAP cap_flag, double value,
                                                wxString unit) {
  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindow *dashboard_window =
        m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
    if (dashboard_window)
      dashboard_window->SendSentenceToAllInstruments(cap_flag, value, unit);
  }
}

// Receive SignalK update using observer/listener model
void Dashboard::HandleSignalK(ObservedEvt ev) {
	// OpenCPN "packages" up the SignalK update, including the self context
	auto payload = GetSignalkPayload(ev);
	const auto signalKMessage = *std::static_pointer_cast<const wxJSONValue>(payload);
	auto errorCount = signalKMessage.ItemAt("ErrorCount");
	if (errorCount.AsInt() > 0) {
		wxLogMessage("Demo Plugin, SignalK Error Count: %d", errorCount.AsInt());
		return;
	}

	// Retrieve the Self Context and the SignalK Data
	wxJSONValue self = signalKMessage.ItemAt("ContextSelf");
	wxJSONValue root = signalKMessage.ItemAt("Data");

	// Only interested in displaying data for our own vessel
	if (root.HasMember("context") && root["context"].IsString()) {
		wxString context = root["context"].AsString();
		if (context == self.AsString()) {
			// Parse the data
			if (root.HasMember("updates") && root["updates"].IsArray()) {
				wxJSONValue updates = root["updates"];
				for (int i = 0; i < updates.Size(); i++) {
					ParseSignalK(updates[i]);
				}
			}
		}
	}
}

// Parse SignalK updates
void Dashboard::ParseSignalK(wxJSONValue & update) {
	if (update.HasMember("values") && update["values"].IsArray()) {
		for (int i = 0; i < update["values"].Size(); i++) {
			wxJSONValue& item = update["values"][i];
			if (item.HasMember("path") && item.HasMember("value")) {
				const wxString& update_path = item["path"].AsString();
				wxJSONValue& value = item["value"];

				if (update_path.StartsWith("propulsion")) {
					m_engineWatchDog = wxDateTime::Now();
				}

				// Units in revolutions per second
				if ((update_path == _T("propulsion.port.revolutions")) && (!g_dualEngine)) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, value.AsDouble() * 60, "RPM");
				}

				if ((update_path == _T("propulsion.port.revolutions")) && (g_dualEngine)) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, value.AsDouble() * 60, "RPM");
				}

				if (update_path == _T("propulsion.starboard.revolutions")) {
					// dualEngine = TRUE;
					SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, value.AsDouble() * 60, "RPM");
				}

				// Units in volts
				if ((update_path == _T("propulsion.port.alternatorVoltage")) && (!g_dualEngine)) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_VOLTS, value.AsDouble(), "Volts");
				}

				if ((update_path == _T("propulsion.port.alternatorVoltage")) && (g_dualEngine)) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_VOLTS, value.AsDouble(), "Volts");
				}

				if (update_path == _T("propulsion.starboard.alternatorVoltage")) {
					// dualEngine = TRUE;
					SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_VOLTS, value.AsDouble(), "Volts");
				}

				if (g_pressureUnit == PRESSURE_BAR) {
					// Units are in Pascals. 100000 Pascals = 1 Bar
					// No idea why current version of SignalK encodes oil pressure as an Int ?
					if ((update_path == _T("propulsion.port.oilPressure")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, value.AsInt() * 1e-5, "Bar");
					}

					if ((update_path == _T("propulsion.port.oilPressure")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, value.AsInt() * 1e-5, "Bar");
					}

					if (update_path == _T("propulsion.starboard.oilPressure")) {
						// dualEngine = TRUE;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, value.AsInt() * 1e-5, "Bar");
					}
				}

				else if (g_pressureUnit == PRESSURE_PSI) {
					if ((update_path == _T("propulsion.port.oilPressure")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, Pascal2Psi(value.AsDouble()), "Psi");
					}

					if ((update_path == _T("propulsion.port.oilPressure")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, Pascal2Psi(value.AsDouble()), "Psi");
					}

					if (update_path == _T("propulsion.starboard.oilPressure")) {
						// dualEngine = TRUE;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, Pascal2Psi(value.AsDouble()), "Psi");
					}
				}

				if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
					// Units are in Kelvin
					if ((update_path == _T("propulsion.port.temperature")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, CONVERT_KELVIN(value.AsDouble()), _T("\u00B0 C"));
					}

					if ((update_path == _T("propulsion.port.temperature")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, CONVERT_KELVIN(value.AsDouble()), _T("\u00B0 C"));
					}

					if (update_path == _T("propulsion.starboard.temperature")) {
						// dualEngine = TRUE;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, CONVERT_KELVIN(value.AsDouble()), _T("\u00B0 C"));
					}

					if ((update_path == _T("propulsion.port.exhaustTemperature")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_EXHAUST, CONVERT_KELVIN(value.AsDouble()), _T("\u00B0 C"));
					}

					if ((update_path == _T("propulsion.port.exhaustTemperature")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_EXHAUST, CONVERT_KELVIN(value.AsDouble()), _T("\u00B0 C"));
					}

					if (update_path == _T("propulsion.starboard.exhaustTemperature")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_EXHAUST, CONVERT_KELVIN(value.AsDouble()), _T("\u00B0 C"));
					}
				}
				else if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
					if ((update_path == _T("propulsion.port.temperature")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, Celsius2Fahrenheit(CONVERT_KELVIN(value.AsDouble())), _T("\u00B0 F"));
					}

					if ((update_path == _T("propulsion.port.temperature")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, Celsius2Fahrenheit(CONVERT_KELVIN(value.AsDouble())), _T("\u00B0 F"));
					}

					if (update_path == _T("propulsion.starboard.temperature")) {
						// dualEngine = TRUE;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, Celsius2Fahrenheit(CONVERT_KELVIN(value.AsDouble())), _T("\u00B0 F"));
					}

					if ((update_path == _T("propulsion.port.exhaustTemperature")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_EXHAUST, Celsius2Fahrenheit(CONVERT_KELVIN(value.AsDouble())), _T("\u00B0 F"));
					}

					if ((update_path == _T("propulsion.port.exhaustTemperature")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_EXHAUST, Celsius2Fahrenheit(CONVERT_KELVIN(value.AsDouble())), _T("\u00B0 F"));
					}

					if (update_path == _T("propulsion.starboard.exhaustTemperature")) {
						// dualEngine = TRUE;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_EXHAUST, Celsius2Fahrenheit(CONVERT_KELVIN(value.AsDouble())), _T("\u00B0 F"));
					}
				}
				// Units are in seconds
				if ((update_path == _T("propulsion.port.runTime")) && (!g_dualEngine)) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, value.AsInt() / 3600.0, "Hrs");
				}

				if ((update_path == _T("propulsion.port.runTime")) && (g_dualEngine)) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, value.AsInt() / 3600.0, "Hrs");
				}

				if (update_path == _T("propulsion.starboard.runTime")) {
					// dualEngine = TRUE;
					SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, value.AsInt() / 3600.0, "Hrs");
				}

				if (update_path == _T("electrical.batteries.0.voltage")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, value.AsDouble(), "Volts");
				}

				if (update_path == _T("electrical.batteries.0.current")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_AMPS, value.AsDouble(), "Amps");
				}

				if (update_path == _T("electrical.batteries.1.voltage")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, value.AsDouble(), "Volts");
				}

				if (update_path == _T("electrical.batteries.1.current")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, value.AsDouble(), "Amps");
				}

				if (update_path == _T("electrical.batteries.0.capacity.stateOfCharge")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_SOC, value.AsDouble(), "%");
				}

				if (update_path == _T("electrical.batteries.0.capacity.remaining")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_HOURS, value.AsDouble(), "Hours");
				}

				if (update_path == _T("electrical.batteries.1.capacity.stateOfCharge")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_SOC, value.AsDouble(), "%");
				}

				if (update_path == _T("electrical.batteries.1.capacity.remaining")) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_HOURS, value.AsDouble(), "Hours");
				}

				if (update_path.StartsWith(_T("steering.rudderAngle"))) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_RSA, RADIANS_TO_DEGREES(value.AsDouble()), _T("\u00B0"));
				}

				// Engine Warning state = "alarm" or "normal"
				if (update_path.StartsWith("notifications.propulsion", NULL)) {
					// Status One Alarm conditions
					// Main Engine
					// Bit 0
					if ((update_path == "notifications.propulsion.port.checkEngine") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 1, wxEmptyString);
						}
					}
					// Bit 1
					if ((update_path == "notifications.propulsion.port.overTemperature") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 2, wxEmptyString);
						}
					}
					// Bit 2
					if ((update_path == "notifications.propulsion.port.lowOilPressure") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 4, wxEmptyString);
						}
					}
					// Bit 3
					if ((update_path == "notifications.propulsion.port.lowOilLevel") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 8, wxEmptyString);
						}
					}
					// Bit 4
					if ((update_path == "notifications.propulsion.port.lowFuelPressure") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 16, wxEmptyString);
						}
					}
					// Bit 5
					if ((update_path == "notifications.propulsion.port.lowSystemVoltage") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 32, wxEmptyString);
						}
					}
					// Bit 6
					if ((update_path == "notifications.propulsion.port.lowCoolantLevel") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 64, wxEmptyString);
						}
					}
					// Bit 7
					if ((update_path == "notifications.propulsion.port.waterFlow") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 128, wxEmptyString);
						}
					}
					// Bit 8
					if ((update_path == "notifications.propulsion.port.waterInFuel") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 256, wxEmptyString);
						}
					}
					// Bit 9
					if ((update_path == "notifications.propulsion.port.chargeIndicator") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 512, wxEmptyString);
						}
					}
					// Bit 10
					if ((update_path == "notifications.propulsion.port.preheatIndicator") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 1024, wxEmptyString);
						}
					}
					// Bit 11
					if ((update_path == "notifications.propulsion.port.highBoostPressure") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 2048, wxEmptyString);
						}
					}
					// Bit 12
					if ((update_path == "notifications.propulsion.port.revLimitExceeded") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 4096, wxEmptyString);
						}
					}
					// Bit 13
					if ((update_path == "notifications.propulsion.port.eGRSystem") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 8192, wxEmptyString);
						}
					}
					// Bit 14
					if ((update_path == "notifications.propulsion.port.throttlePositionSensor") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 16384, wxEmptyString);
						}
					}
					//Bit 15
					if ((update_path == "notifications.propulsion.port.emergencyStopMode") && (!g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, 32768, wxEmptyString);
						}
					}
					// Port Engine
					// Bit 0
					if ((update_path == "notifications.propulsion.port.checkEngine") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 1, wxEmptyString);
						}
					}
					// Bit 1
					if ((update_path == "notifications.propulsion.port.overTemperature") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 2, wxEmptyString);
						}
					}
					// Bit 2
					if ((update_path == "notifications.propulsion.port.lowOilPressure") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 4, wxEmptyString);
						}
					}
					// Bit 3
					if ((update_path == "notifications.propulsion.port.lowOilLevel") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 8, wxEmptyString);
						}
					}
					// Bit 4
					if ((update_path == "notifications.propulsion.port.lowFuelPressure") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 16, wxEmptyString);
						}
					}
					// Bit 5
					if ((update_path == "notifications.propulsion.port.lowSystemVoltage") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 32, wxEmptyString);
						}
					}
					// Bit 6
					if ((update_path == "notifications.propulsion.port.lowCoolantLevel") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 64, wxEmptyString);
						}
					}
					// Bit 7
					if ((update_path == "notifications.propulsion.port.waterFlow") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 128, wxEmptyString);
						}
					}
					// Bit 8
					if ((update_path == "notifications.propulsion.port.waterInFuel") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 256, wxEmptyString);
						}
					}
					// Bit 9
					if ((update_path == "notifications.propulsion.port.chargeIndicator") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 512, wxEmptyString);
						}
					}
					// Bit 10
					if ((update_path == "notifications.propulsion.port.preheatIndicator") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 1024, wxEmptyString);
						}
					}
					// Bit 11
					if ((update_path == "notifications.propulsion.port.highBoostPressure") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 2048, wxEmptyString);
						}
					}
					// Bit 12
					if ((update_path == "notifications.propulsion.port.revLimitExceeded") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 4096, wxEmptyString);
						}
					}
					// Bit 13
					if ((update_path == "notifications.propulsion.port.eGRSystem") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 8192, wxEmptyString);
						}
					}
					// Bit 14
					if ((update_path == "notifications.propulsion.port.throttlePositionSensor") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 16384, wxEmptyString);
						}
					}
					//Bit 15
					if ((update_path == "notifications.propulsion.port.emergencyStopMode") && (g_dualEngine)) {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, 32768, wxEmptyString);
						}
					}

					// Starboard Engine
					// Bit 0
					if (update_path == "notifications.propulsion.starboard.checkEngine") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 1, wxEmptyString);
						}
					}
					// Bit 1
					if (update_path == "notifications.propulsion.starboard.overTemperature") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 2, wxEmptyString);
						}
					}
					// Bit 2
					if (update_path == "notifications.propulsion.starboard.lowOilPressure") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 4, wxEmptyString);
						}
					}
					// Bit 3
					if (update_path == "notifications.propulsion.starboard.lowOilLevel") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 8, wxEmptyString);
						}
					}
					// Bit 4
					if (update_path == "notifications.propulsion.starboard.lowFuelPressure") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 16, wxEmptyString);
						}
					}
					// Bit 5
					if (update_path == "notifications.propulsion.starboard.lowSystemVoltage") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 32, wxEmptyString);
						}
					}
					// Bit 6
					if (update_path == "notifications.propulsion.starboard.lowCoolantLevel") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 64, wxEmptyString);
						}
					}
					// Bit 7
					if (update_path == "notifications.propulsion.starboard.waterFlow") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 128, wxEmptyString);
						}
					}
					// Bit 8
					if (update_path == "notifications.propulsion.starboard.waterInFuel") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 256, wxEmptyString);
						}
					}
					// Bit 9
					if (update_path == "notifications.propulsion.starboard.chargeIndicator") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 512, wxEmptyString);
						}
					}
					// Bit 10
					if (update_path == "notifications.propulsion.starboard.preheatIndicator") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 1024, wxEmptyString);
						}
					}
					// Bit 11
					if (update_path == "notifications.propulsion.starboard.highBoostPressure") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 2048, wxEmptyString);
						}
					}
					// Bit 12
					if (update_path == "notifications.propulsion.starboard.revLimitExceeded") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 4096, wxEmptyString);
						}
					}
					// Bit 13
					if (update_path == "notifications.propulsion.starboard.eGRSystem") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 8192, wxEmptyString);
						}
					}
					// Bit 14
					if (update_path == "notifications.propulsion.starboard.throttlePositionSensor") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 16384, wxEmptyString);
						}
					}
					//Bit 15
					if (update_path == "notifications.propulsion.starboard.emergencyStopMode") {
						if (CheckAlarmState(value)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, 32768, wxEmptyString);
						}
					}
					/////////////////

					// Status Two Error Codes
					// Currently Don't have icons for these, nor do I handle these in native NMEA 2000
					// Bit 0
					if (update_path == "notifications.propulsion.port.warningLevel1") {

					}
					// Bit 1
					if (update_path == "notifications.propulsion.port.warningLevel2") {

					}
					// Bit 2
					if (update_path == "notifications.propulsion.port.powerReduction") {

					}
					// Bit 3
					if (update_path == "notifications.propulsion.port.maintenanceNeeded") {

					}
					// Bit 4
					if (update_path == "notifications.propulsion.port.commError") {

					}
					// Bit 5
					if (update_path == "notifications.propulsion.port.subOrSecondaryThrottle") {

					}
					// Bit 6
					if (update_path == "notifications.propulsion.port.neutralStartProtect") {

					}
					// Bit 7
					if (update_path == "notifications.propulsion.port.shuttingDown") {

					}
				}

				// Fluid Levels
				if (update_path.StartsWith("tanks", NULL)) {
					m_tankLevelWatchDog = wxDateTime::Now();
					wxString xdrunit = "Level";

					if (update_path == _T("tanks.freshWater.0.currentLevel")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_01, value.AsDouble() * 100, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01, value.AsDouble() * 100, xdrunit);
					}

					if (update_path == _T("tanks.freshWater.1.currentLevel")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_02, value.AsDouble() * 100, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_02, value.AsDouble() * 100, xdrunit);
					}

					if (update_path == _T("tanks.freshWater.2.currentLevel")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_03, value.AsDouble() * 100, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_03, value.AsDouble() * 100, xdrunit);
					}

					if (update_path == _T("tanks.wasteWater.0.currentLevel")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, value.AsDouble() * 100, xdrunit);
					}

					if (update_path == _T("tanks.blackWater.0.currentLevel")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, value.AsDouble() * 100, xdrunit);
					}

					if (update_path == _T("tanks.fuel.0.currentLevel")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_01, value.AsDouble() * 100, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01, value.AsDouble() * 100, xdrunit);
					}

					if (update_path == _T("tanks.fuel.1.currentLevel")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_02, value.AsDouble() * 100, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_02, value.AsDouble() * 100, xdrunit);
					}
				}
			}
		}
	}
}
/*
void Dashboard::HandleSKUpdate(wxJSONValue& update) {
	if (update.HasMember("values") && update["values"].IsArray()) {
		for (int j = 0; j < update["values"].Size(); ++j) {
			wxJSONValue& item = update["values"][j];
			UpdateSKItem(item);
		}
	}
}
*/

//void Dashboard::UpdateSKItem(wxJSONValue& item) {
//	if (item.HasMember("path") && item.HasMember("value")) {

//	}
//}		

// SignalK Engine Notifications
// "state": "normal | alarm"
// "method: ["visual", "sound"]
// "message": "Port Engine Charge Indicator is normal"
bool Dashboard::CheckAlarmState(wxJSONValue& value) {
	if (value.HasMember("state")) {
		if (value["state"].AsString() == "alarm") {
			return true;
		}
	}
	return false;
}

// NMEA 0183 
void Dashboard::HandleXDR(ObservedEvt ev) {
	NMEA0183Id id_183_xdr("XDR");

	wxString sentence(GetN0183Payload(id_183_xdr, ev));
	m_NMEA0183 << sentence;

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
					m_engineWatchDog = wxDateTime::Now();
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
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, xdrdata, xdrunit);
					}
					// Ship Modul/Maretron transducer names
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE0")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE0")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, xdrdata, xdrunit);
					}
				}
			}

			// "C" Temperature in "C" degrees Celsius
			if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("C")) {
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("C")) {
					if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
						xdrunit = _T("\u00B0 C");
						// TwoCan transducer naming
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
						// Engine Temperature
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, xdrdata, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, xdrdata, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, xdrdata, xdrunit);
						}
						// Engine Exhaust
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEEXHAUST#1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_EXHAUST, xdrdata, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEEXHAUST#0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_EXHAUST, xdrdata, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEEXHAUST#0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_EXHAUST, xdrdata, xdrunit);
						}
						// Ship Modul/Maretron Transducer Names
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGTEMP1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, xdrdata, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGTEMP0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, xdrdata, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGTEMP0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, xdrdata, xdrunit);
						}
					}
					else if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
						xdrunit = _T("\u00B0 F");
						// TwoCan Transducer naming 
						if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						// NMEA 183 v4.11 Transducer Names
						// Engine Temperature
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						// Exhaust Temperature
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEEXHAUST#1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_EXHAUST, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEEXHAUST#0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_EXHAUST, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEEXHAUST#0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_EXHAUST, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						// Ship Modul/Maretron Transducer Names
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGTEMP1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGTEMP0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGTEMP0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, Celsius2Fahrenheit(xdrdata), xdrunit);
						}
					}
				}
			}

			// "P" Pressure in "P" pascal
			if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("P")) {
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("P")) {
					if (g_pressureUnit == PRESSURE_BAR) {
						xdrunit = _T("Bar");
						// TwoCan Transducer naming
						if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						// NMEA 183 v4.11 Transducer Names
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEOIL#1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEOIL#0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEOIL#0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						// Ship Modul/Maretron Transducer Names
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGOILP1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGOILP0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGOILP0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, xdrdata * 1e-5, xdrunit);
						}

					}
					else if (g_pressureUnit == PRESSURE_PSI) {
						xdrunit = _T("PSI");
						// TwoCan Plugin Transducer Names
						if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						// NMEA 183 v4.11 Transducer Names
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEOIL#1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEOIL#0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEOIL#0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						// Ship Modul/MaretronTransducer Names
						else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGOILP1")) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGOILP0")) && (!g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
						}
						else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGOILP0")) && (g_dualEngine)) {
							SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, Pascal2Psi(xdrdata), xdrunit);
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
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ALTERNATOR#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_VOLTS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ALTERNATOR#0")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_VOLTS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ALTERNATOR#0")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_VOLTS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATTERY#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATTERY#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, xdrdata, xdrunit);
					}
					// Ship Modul/Maretron Transducer Names
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ALTVOLT1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_VOLTS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ALTVOLT0")) && (!g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_VOLTS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ALTVOLT0")) && (g_dualEngine)) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_VOLTS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATVOLT0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATVOLT1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, xdrdata, xdrunit);
					}
				}
				// TwoCan also uses "A" to indicate battery current
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
					// NMEA 183 v4.11 Transducer Names
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATTERY#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_AMPS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATTERY#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, xdrdata, xdrunit);
					}
					// Ship Modul/Maretron Transducer Names
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATCURR0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_AMPS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BATCURR1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, xdrdata, xdrunit);
					}
				}
			}

			// "G" Generic - Customised to use "H" as engine hours
			if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("G")) {
				// TwoCan uses "H" as unit of measurement 
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("H")) {
					xdrunit = _T("Hrs");
					// TwoCan Plugin transducer naming
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("MAIN")) {
						m_mainEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("PORT")) {
						m_portEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("STBD")) {
						m_stbdEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, xdrdata, xdrunit);
					}
				}
				// NMEA 183 v4.11 Transducer Names, Note lack of clarity re transducer names
				// Note does not have a unit of measurement
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == wxEmptyString) {
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#1")) {
						xdrunit = _T("Hrs");
						m_stbdEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (!g_dualEngine)) {
						xdrunit = _T("Hrs");
						m_mainEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINE#0")) && (g_dualEngine)) {
						xdrunit = _T("Hrs");
						m_portEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, xdrdata, xdrunit);
					}
				}
				// NMEA 183 v4.11 Yacht Devices appear to use EngineHours
				// Note does not have a unit of measurement
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == wxEmptyString) {
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEHOURS#1")) {
						xdrunit = _T("Hrs");
						m_stbdEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEHOURS#0")) && (!g_dualEngine)) {
						xdrunit = _T("Hrs");
						m_mainEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGINEHOURS#0")) && (g_dualEngine)) {
						xdrunit = _T("Hrs");
						m_portEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, xdrdata, xdrunit);
					}
				}
				// Ship Modul/Maretron Transducer Names 
				// Note does not have a unit of measurement
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == wxEmptyString) {
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGHRS1")) {
						xdrunit = _T("Hrs");
						m_stbdEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGHRS0")) && (!g_dualEngine)) {
						xdrunit = _T("Hrs");
						m_mainEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, xdrdata, xdrunit);
					}
					else if ((m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("ENGHRS0")) && (g_dualEngine)) {
						xdrunit = _T("Hrs");
						m_portEngineHours = xdrdata;
						SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, xdrdata, xdrunit);
					}
				}

			}

			// "V" Volume - Customised to use "P" as percent capacity
			// instead of "M" as volume in cubic metres
			// Note that NMEA 183 v4.11 standard now introduces 'P' as percent capacity
			if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("V")) {
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("P")) {
					// Update Watchdog Timer
					m_tankLevelWatchDog = wxDateTime::Now();
					xdrunit = _T("Level");
					// TwoCan Plugin Transducer Names
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("FUEL")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName == _T("H2O")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01, xdrdata, xdrunit);
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
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FUEL#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01, xdrdata, xdrunit);
					}
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FUEL#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_02, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_02, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_02, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_02, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER#2")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_03, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_03, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("OIL#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("LIVEWELLWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("WASTEWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BLACKWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, xdrdata, xdrunit);
					}
				}
			}
			// NMEA 0184 v4.11 Standard for volume with percentage capacity
			if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerType == _T("E")) {
				if (m_NMEA0183.Xdr.TransducerInfo[i].UnitOfMeasurement == _T("P")) {
					// Update Watchdog Timer
					m_tankLevelWatchDog = wxDateTime::Now();
					xdrunit = _T("Level");
					// NMEA 183 v4.11 Transducer Names
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FUEL#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01, xdrdata, xdrunit);
					}
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FUEL#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_02, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_02, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER#1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_02, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_02, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER#2")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_03, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_03, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("OIL#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("LIVEWELLWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("WASTEWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BLACKWATER#0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, xdrdata, xdrunit);
					}
					// Ship Modul/Martron Transducer Names
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FUEL0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01, xdrdata, xdrunit);
					}
					if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FUEL1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_02, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_02, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_01, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER1")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_02, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_02, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("FRESHWATER2")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_03, xdrdata, xdrunit);
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_03, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("OIL0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("LIVEWELL0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("WASTEWATER0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, xdrdata, xdrunit);
					}
					else if (m_NMEA0183.Xdr.TransducerInfo[i].TransducerName.Upper() == _T("BLACKWATER0")) {
						SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, xdrdata, xdrunit);
					}
				}
			}
		}
	}
}

void Dashboard::HandleRPM(ObservedEvt ev) {
	NMEA0183Id id_183_rpm("RPM");

	wxString sentence(GetN0183Payload(id_183_rpm, ev));
	m_NMEA0183 << sentence;

	if (m_NMEA0183.Parse()) {
		if (m_NMEA0183.Rpm.IsDataValid == NTrue) {
			// Only display engine rpm 'E', not shaft rpm 'S'
			if (m_NMEA0183.Rpm.Source == _T("E")) {
				// Update Watchdog Timer
				m_engineWatchDog = wxDateTime::Now();
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

void Dashboard::HandleRSA(ObservedEvt ev) {
	NMEA0183Id id_183_rsa("RSA");

	wxString sentence(GetN0183Payload(id_183_rsa, ev));
	m_NMEA0183 << sentence;

	// Plugin does not differentiate dual rudders (port/starboard)

	if (m_NMEA0183.Parse()) {
		if (m_NMEA0183.Rsa.IsStarboardDataValid == NTrue) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_RSA, m_NMEA0183.Rsa.Starboard, _T("\u00B0"));
		}
		else if (m_NMEA0183.Rsa.IsPortDataValid == NTrue) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_RSA, m_NMEA0183.Rsa.Port, _T("\u00B0"));
		}
	}
}



// NMEA 2000
// Raw NMEA 2000 generated by OpenCPN v5.8
// Parsing routines cut and pasted from TwoCan Plugin
// Refer to twocandevice.cpp

// Note the payload is not "the payload" but an entire Actisense payload
// Actisense application data, from NGT-1 to PC
// <data code=93><length (1)><priority (1)><PGN (3)><destination(1)><source
// (1)><time (4)><len (1)><data (len)>

// As applied to a real application data element, after extraction from packet
// format: 93 13 02 01 F8 01 FF 01 76 C2 52 00 08 08 70 EB 14 E8 8E 52 D2 BB 10

// data code		0x93
// length (1):      0x13
// priority (1);    0x02
// PGN (3):         0x01 0xF8 0x01
// destination(1):  0xFF
// source (1):      0x01
// time (4):        0x76 0xC2 0x52 0x00
// len (1):         0x08
// data (len):      08 70 EB 14 E8 8E 52 D2
// packet CRC:      0xBB

// So to simplify parsing as these are copied from twocan plugin, 
// use an index into the "real" payload at byte 13 

// PGN 127488 Engine Rapid Update
void Dashboard::HandleN2K_127488(ObservedEvt ev) {
	NMEA2000Id id_127488(127488);
	std::vector<uint8_t>payload = GetN2000Payload(id_127488, ev);

	uint8_t engineInstance;
	engineInstance = payload[index + 0];

	unsigned short engineSpeed; // RPM in quarter revolutions per minute
	engineSpeed = payload[index + 1] | (payload[index + 2] << 8);

	unsigned short engineBoostPressure;
	engineBoostPressure = payload[index + 3] | (payload[index + 4] << 8);

	short engineTrim;
	engineTrim = payload[index + 5];

	if (engineInstance > 0) {
		g_dualEngine = TRUE;
	}

	m_engineWatchDog = wxDateTime::Now();

	if (IsDataValid(engineSpeed)) {
		switch (engineInstance) {
		case 0:
			if (g_dualEngine) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_RPM, engineSpeed * 0.25f, "RPM");
			}
			else {
				SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_RPM, engineSpeed * 0.25f, "RPM");
			}
			break;
		case 1:
			SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_RPM, engineSpeed * 0.25f, "RPM");
			break;
		}
	}
}

// PGN 127489 Engine Dynamic 
void Dashboard::HandleN2K_127489(ObservedEvt ev) {
	NMEA2000Id id_127489(127489);
	std::vector<uint8_t>payload = GetN2000Payload(id_127489, ev);

	uint8_t engineInstance;
	engineInstance = payload[index + 0];

	unsigned short oilPressure; // hPa (1 hPa = 100Pa, 1 hPa = .001 Bar)
	oilPressure = payload[index + 1] | (payload[index + 2] << 8);

	unsigned short oilTemperature; // 0.01 degree resolution, in Kelvin
	oilTemperature = payload[index + 3] | (payload[index + 4] << 8);

	unsigned short engineTemperature; // 0.01 degree resolution, in Kelvin
	engineTemperature = payload[index + 5] | (payload[index + 6] << 8);

	unsigned short alternatorPotential; // 0.01 Volts
	alternatorPotential = payload[index + 7] | (payload[index + 8] << 8);

	unsigned short fuelRate; // 0.1 Litres/hour
	fuelRate = payload[index + 9] | (payload[index + 10] << 8);

	unsigned int totalEngineHours;  // seconds
	totalEngineHours = payload[index + 11] | (payload[index + 12] << 8) | (payload[index + 13] << 16) | (payload[index + 14] << 24);

	unsigned short coolantPressure; // hPA
	coolantPressure = payload[index + 15] | (payload[index + 16] << 8);

	unsigned short fuelPressure; // hPa
	fuelPressure = payload[index + 17] | (payload[index + 18] << 8);

	unsigned short reserved;
	reserved = payload[index + 19];

	unsigned short statusOne;
	statusOne = payload[index + 20] | (payload[index + 21] << 8);
	// Refer to dial.cpp for which SVG images match fault conditions
	// {"0": "Check Engine"},
	// { "1": "Over Temperature" },
	// { "2": "Low Oil Pressure" },
	// { "3": "Low Oil Level" },
	// { "4": "Low Fuel Pressure" },
	// { "5": "Low System Voltage" },
	// { "6": "Low Coolant Level" },
	// { "7": "Water Flow" },
	// { "8": "Water In Fuel" },
	// { "9": "Charge Indicator" },
	// { "10": "Preheat Indicator" },
	// { "11": "High Boost Pressure" },
	// { "12": "Rev Limit Exceeded" },
	// { "13": "EGR System" },
	// { "14": "Throttle Position Sensor" },
	// { "15": "Emergency Stop" }]

	unsigned short statusTwo;
	statusTwo = payload[index + 22] | (payload[index + 23] << 8);

	// {"0": "Warning Level 1"},
	// { "1": "Warning Level 2" },
	// { "2": "Power Reduction" },
	// { "3": "Maintenance Needed" },
	// { "4": "Engine Comm Error" },
	// { "5": "Sub or Secondary Throttle" },
	// { "6": "Neutral Start Protect" },
	// { "7": "Engine Shutting Down" }]

	uint8_t engineLoad;  // percentage
	engineLoad = payload[index + 24];

	uint8_t engineTorque; // percentage
	engineTorque = payload[index + 25];

	if (engineInstance > 0) {
		g_dualEngine = TRUE;
	}

	switch (engineInstance) {
	case 0:
		if (g_dualEngine) {
			if (IsDataValid(oilPressure)) {
				if (g_pressureUnit == PRESSURE_BAR) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, oilPressure * 1e-3, "Bar");
				}
				if (g_pressureUnit == PRESSURE_PSI) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_OIL, Pascal2Psi(oilPressure * 100), "Psi");
				}
			}

			if (IsDataValid(engineTemperature)) {
				if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, CONVERT_KELVIN((engineTemperature * 0.01f)), _T("\u00B0 C"));
				}
				if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_WATER, Celsius2Fahrenheit(CONVERT_KELVIN((engineTemperature * 0.01f))), _T("\u00B0 F"));
				}
			}

			if (IsDataValid(alternatorPotential)) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_VOLTS, alternatorPotential * 0.01, "Volts");
			}

			if (IsDataValid(totalEngineHours)) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_HOURS, totalEngineHours / 3600.0, "Hrs");
			}

			if (IsDataValid(statusOne)) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE, statusOne, wxEmptyString);
			}

		}
		else {
			if (IsDataValid(oilPressure)) {
				if (g_pressureUnit == PRESSURE_BAR) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, oilPressure * 1e-3, "Bar");
				}
				if (g_pressureUnit == PRESSURE_PSI) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_OIL, Pascal2Psi(oilPressure * 100), "Psi");
				}
			}
			if (IsDataValid(engineTemperature)) {
				if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, CONVERT_KELVIN((engineTemperature * 0.01f)), _T("\u00B0 C"));
				}
				if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_WATER, Celsius2Fahrenheit(CONVERT_KELVIN((engineTemperature * 0.01f))), _T("\u00B0 F"));
				}
			}

			if (IsDataValid(alternatorPotential)) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_VOLTS, alternatorPotential * 0.01, "Volts");
			}

			if (IsDataValid(totalEngineHours)) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_HOURS, totalEngineHours / 3600.0, "Hrs");
			}

			if (IsDataValid(statusOne)) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE, statusOne, wxEmptyString);
			}
		}
		break;
	case 1:
		if (IsDataValid(oilPressure)) {
			if (g_pressureUnit == PRESSURE_BAR) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, oilPressure * 1e-3, "Bar");
			}
			if (g_pressureUnit == PRESSURE_PSI) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_OIL, Pascal2Psi(oilPressure * 100), "Psi");
			}
		}
		if (IsDataValid(engineTemperature)) {
			if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, CONVERT_KELVIN((engineTemperature * 0.01f)), _T("\u00B0 C"));
			}
			if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_WATER, Celsius2Fahrenheit((CONVERT_KELVIN(engineTemperature * 0.01f))), _T("\u00B0 F"));
			}
		}

		if (IsDataValid(alternatorPotential)) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_VOLTS, alternatorPotential * 0.01, "Volts");
		}

		if (IsDataValid(totalEngineHours)) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_HOURS, totalEngineHours / 3600.0, "Hrs");
		}

		if (IsDataValid(statusOne)) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE, statusOne, wxEmptyString);
		}

		break;
	}
}

// PGN 127505 Fluid Levels
void Dashboard::HandleN2K_127505(ObservedEvt ev) {
	NMEA2000Id id_127505(127505);
	std::vector<uint8_t>payload = GetN2000Payload(id_127505, ev);

	uint8_t instance;
	instance = payload[index + 0] & 0x0F;

	uint8_t tankType;
	tankType = (payload[index + 0] & 0xF0) >> 4;

	unsigned short tankLevel; // percentage in 0.025 increments
	tankLevel = payload[index + 1] | (payload[index + 2] << 8);

	unsigned int tankCapacity; // 0.1 L
	tankCapacity = payload[index + 3] | (payload[index + 4] << 8) | (payload[index + 5] << 16) | (payload[index + 6] << 24);

	m_tankLevelWatchDog = wxDateTime::Now();

	if (IsDataValid(tankLevel)) {

		switch (tankType) {
		case 0: // Fuel
			if (instance == 0) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_01, tankLevel / 250, "Level");
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01, tankLevel / 250, "Level");
			}
			if (instance == 1) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_02, tankLevel / 250, "Level");
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_02, tankLevel / 250, "Level");
			}
			break;
		case 1: // Freshwater
			if (instance == 0) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_01, tankLevel / 250, "Level");
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01, tankLevel / 250, "Level");
			}
			if (instance == 1) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_02, tankLevel / 250, "Level");
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_02, tankLevel / 250, "Level");
			}
			if (instance == 2) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_03, tankLevel / 250, "Level");
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_03, tankLevel / 250, "Level");
			}
			break;
		case 2: // Waste water
			if (instance == 0) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_GREY, tankLevel / 250, "Level");
			}
			break;
		case 4: // Oil
			if (instance == 0) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_OIL, tankLevel / 250, "Level");
			}
			break;
		case 5: // Blackwater
			if (instance == 0) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_TANK_LEVEL_BLACK, tankLevel / 250, "Level");
			}
			break;
		}
	}
}

// PGN 127506 Battery Status
void Dashboard::HandleN2K_127506(ObservedEvt ev) {
	NMEA2000Id id_127506(127506);
	std::vector<uint8_t>payload = GetN2000Payload(id_127506, ev);

	uint8_t sid;
	sid = payload[index + 0];

	uint8_t batteryInstance;
	batteryInstance = payload[index + 1];

	uint8_t chargeSource;
	chargeSource = payload[index + 2];

	uint8_t stateOfCharge; // %
	stateOfCharge = payload[index + 3];

	uint8_t stateOfHealth; // %
	stateOfHealth = payload[index + 4];

	unsigned short timeRemaining; // Hours
	timeRemaining = payload[index + 5] | (payload[index + 6] << 8);

	unsigned short rippleVoltage; // milliVolts, so multiply by 1e-3
	rippleVoltage = payload[index + 7] | (payload[index + 8] << 8);

	unsigned short ampHours; // Hours
	ampHours = payload[index + 9] | (payload[index + 10] << 8);

	if ((IsDataValid(stateOfCharge)) && (IsDataValid(timeRemaining))) {

		if (batteryInstance == 0) {

			SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_SOC, static_cast<double>(stateOfCharge), "%");
			SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_HOURS, static_cast<double>(timeRemaining), "Hours");
		}

		if (batteryInstance == 1) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_SOC, stateOfCharge, "%");
			SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_HOURS, timeRemaining, "Hours");
		}
	}
}

// PGN 127508 Battery Status
void Dashboard::HandleN2K_127508(ObservedEvt ev) {
	NMEA2000Id id_127508(127508);
	std::vector<uint8_t>payload = GetN2000Payload(id_127508, ev);

	uint8_t batteryInstance;
	batteryInstance = payload[index + 0];

	unsigned short batteryVoltage; // 0.01 volts
	batteryVoltage = payload[index + 1] | (payload[index + 2] << 8);

	short batteryCurrent; // 0.1 amps	
	batteryCurrent = payload[index + 3] | (payload[index + 4] << 8);

	unsigned short batteryTemperature; // 0.01 degree resolution, in Kelvin
	batteryTemperature = payload[index + 5] | (payload[index + 6] << 8);

	uint8_t sid;
	sid = payload[index + 7];

	if ((IsDataValid(batteryVoltage)) && (IsDataValid(batteryCurrent))) {

		if (batteryInstance == 0) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, batteryVoltage * 0.01f, "Volts");
			SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_AMPS, batteryCurrent * 0.1f, "Amps");
		}

		if (batteryInstance == 1) {
			SendSentenceToAllInstruments(OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, batteryVoltage * 0.01f, "Volts");
			SendSentenceToAllInstruments(OCPN_DBP_STC_START_BATTERY_VOLTS, batteryCurrent * 0.1f, "Amps");
		}
	}

}

// PGN 130312 Temperature (used for Exhaust Gas Temperature)
void Dashboard::HandleN2K_130312(ObservedEvt ev) {
	NMEA2000Id id_130312(130312);
	std::vector<uint8_t>payload = GetN2000Payload(id_130312, ev);

	uint8_t sid;
	sid = payload[index + 0];

	uint8_t engineInstance;
	engineInstance = payload[index + 1];

	uint8_t source;
	source = payload[index + 2];

	unsigned short actualTemperature;
	actualTemperature = payload[index + 3] | (payload[index + 4] << 8);

	unsigned short setTemperature;
	setTemperature = payload[index + 5] | (payload[index + 6] << 8);

	if (engineInstance > 0) {
		g_dualEngine = TRUE;
	}

	// Source 14 indicates exhaust temperature
	if ((source == 14) && (IsDataValid(actualTemperature))) {

		switch (engineInstance) {
		case 0:
			if (g_dualEngine) {
				if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_EXHAUST, CONVERT_KELVIN((actualTemperature * 0.01f)), _T("\u00B0 C"));
				}
				if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_PORT_ENGINE_EXHAUST, Celsius2Fahrenheit(CONVERT_KELVIN((actualTemperature * 0.01f))), _T("\u00B0 F"));
				}
			}
			else {
				if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_EXHAUST, CONVERT_KELVIN((actualTemperature * 0.01f)), _T("\u00B0 C"));
				}
				if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
					SendSentenceToAllInstruments(OCPN_DBP_STC_MAIN_ENGINE_EXHAUST, Celsius2Fahrenheit(CONVERT_KELVIN((actualTemperature * 0.01f))), _T("\u00B0 F"));
				}
			}
			break;
		case 1:
			if (g_temperatureUnit == TEMPERATURE_CELSIUS) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_EXHAUST, CONVERT_KELVIN((actualTemperature * 0.01f)), _T("\u00B0 C"));
			}
			if (g_temperatureUnit == TEMPERATURE_FAHRENHEIT) {
				SendSentenceToAllInstruments(OCPN_DBP_STC_STBD_ENGINE_EXHAUST, Celsius2Fahrenheit(CONVERT_KELVIN((actualTemperature * 0.01f))), _T("\u00B0 F"));
			}
			break;
		}
	}
}

// PGN 127245 Rudder Angle 
void Dashboard::HandleN2K_127245(ObservedEvt ev) {
	NMEA2000Id id_127245(127245);
	std::vector<uint8_t>payload = GetN2000Payload(id_127245, ev);

	uint8_t instance;
	instance = payload[index + 0];

	uint8_t directionOrder;
	directionOrder = payload[index + 1] & 0x03;

	short angleOrder; // 0.0001 radians
	angleOrder = payload[index + 2] | (payload[index + 3] << 8);

	short position; // 0.0001 radians
	position = payload[index + 4] | (payload[index + 5] << 8);

	if (IsDataValid(position)) {
		// Ignore rudder instance assume that it refers to the main rudder
		SendSentenceToAllInstruments(OCPN_DBP_STC_RSA, RADIANS_TO_DEGREES((float)position * 1e-4), _T("\u00B0"));
	}
}


int Dashboard::GetToolbarToolCount(void) { 
	return 1; 
}

// Display our preferences dialog
void Dashboard::ShowPreferencesDialog(wxWindow *parent) {
  DashboardPreferencesDialog *dialog = new DashboardPreferencesDialog(
      parent, wxID_ANY, m_ArrayOfDashboardWindow);

  dialog->RecalculateSize();

#ifdef __OCPN__ANDROID__
  dialog->GetHandle()->setStyleSheet(qtStyleSheet);
#endif

#ifdef __OCPN__ANDROID__
  wxWindow *ccwin = GetOCPNCanvasWindow();

  if (ccwin) {
    int xmax = ccwin->GetSize().GetWidth();
    int ymax = ccwin->GetParent()
                   ->GetSize()
                   .GetHeight();  // This would be the Frame itself
    dialog->SetSize(xmax, ymax);
    dialog->Layout();

    dialog->Move(0, 0);
  }
#endif

  if (dialog->ShowModal() == wxID_OK) {
    double scaler = 1.0;
    if (OCPN_GetWinDIPScaleFactor() < 1.0)
      scaler = 1.0 + OCPN_GetWinDIPScaleFactor() / 4;
    scaler = wxMax(1.0, scaler);

    g_USFontTitle = *(dialog->m_pFontPickerTitle->GetFontData());
    g_FontTitle = *g_pUSFontTitle;
    g_FontTitle.SetChosenFont(g_pUSFontTitle->GetChosenFont().Scaled(scaler));
    g_FontTitle.SetColour(g_pUSFontTitle->GetColour());
    g_USFontTitle = *g_pUSFontTitle;

    g_USFontData = *(dialog->m_pFontPickerData->GetFontData());
    g_FontData = *g_pUSFontData;
    g_FontData.SetChosenFont(g_pUSFontData->GetChosenFont().Scaled(scaler));
    g_FontData.SetColour(g_pUSFontData->GetColour());
    g_USFontData = *g_pUSFontData;

    g_USFontLabel = *(dialog->m_pFontPickerLabel->GetFontData());
    g_FontLabel = *g_pUSFontLabel;
    g_FontLabel.SetChosenFont(g_pUSFontLabel->GetChosenFont().Scaled(scaler));
    g_FontLabel.SetColour(g_pUSFontLabel->GetColour());
    g_USFontLabel = *g_pUSFontLabel;

    g_USFontSmall = *(dialog->m_pFontPickerSmall->GetFontData());
    g_FontSmall = *g_pUSFontSmall;
    g_FontSmall.SetChosenFont(g_pUSFontSmall->GetChosenFont().Scaled(scaler));
    g_FontSmall.SetColour(g_pUSFontSmall->GetColour());
    g_USFontSmall = *g_pUSFontSmall;

    // OnClose should handle that for us normally but it doesn't seems to do so
    // We must save changes first
    g_dashPrefWidth = dialog->GetSize().x;
    g_dashPrefHeight = dialog->GetSize().y;

    dialog->SaveDashboardConfig();
    m_ArrayOfDashboardWindow.Clear();
    m_ArrayOfDashboardWindow = dialog->m_Config;

    ApplyConfig();
    SaveConfig();
    SetToolbarItemState(m_toolbar_item_id, GetDashboardWindowShownCount() != 0);
  }
  dialog->Destroy();
}

// Update our colour scheme if the user has set day, dusk or night mode
void Dashboard::SetColorScheme(PI_ColorScheme cs) {
  actualColourScheme = cs;
  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindow *dashboard_window =
        m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
    if (dashboard_window) dashboard_window->SetColorScheme(cs);
  }
}

// Return the number of dashboards displayed
int Dashboard::GetDashboardWindowShownCount() {
  int count = 0;

  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindow *dashboard_window = m_ArrayOfDashboardWindow.Item(i)->m_pDashboardWindow;
    if (dashboard_window) {
      wxAuiPaneInfo &pane = m_pauimgr->GetPane(dashboard_window);
      if (pane.IsOk() && pane.IsShown()) {
		count++;
	  }
    }
  }
  return count;
}

int Dashboard::GetToolbarItemId(void) {
    return m_toolbar_item_id;
}

// Handle the wxAUI Pane Close event
void Dashboard::OnPaneClose(wxAuiManagerEvent &event) {
  // if name is unique, we should use it
  DashboardWindow *dashboard_window = (DashboardWindow *)event.pane->window;
  int count = 0;
  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
    DashboardWindow *d_w = cont->m_pDashboardWindow;
    if (d_w) {
      // we must not count this one because it is being closed
      if (dashboard_window != d_w) {
        wxAuiPaneInfo &pane = m_pauimgr->GetPane(d_w);
        if (pane.IsOk() && pane.IsShown()) {
			count++;
		}
      } else {
        cont->m_bIsVisible = false;
      }
    }
  }
  SetToolbarItemState(m_toolbar_item_id, count != 0);

  event.Skip();
}

// Handle the toolbar button press event
void Dashboard::OnToolbarToolCallback(int id) {
  int count = GetDashboardWindowShownCount();

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
        //  Support MultiMonitor setups which an allow negative window
        //  positions. If the requested window title bar does not intersect any
        //  installed monitor, then default to simple primary monitor
        //  positioning.
        RECT frame_title_rect;
        frame_title_rect.left = pane.floating_pos.x;
        frame_title_rect.top = pane.floating_pos.y;
        frame_title_rect.right = pane.floating_pos.x + pane.floating_size.x;
        frame_title_rect.bottom = pane.floating_pos.y + 30;

        if (NULL == MonitorFromRect(&frame_title_rect, MONITOR_DEFAULTTONULL))
          b_reset_pos = true;
#else

        //    Make sure drag bar (title bar) of window intersects wxClient Area
        //    of screen, with a little slop...
        wxRect window_title_rect;  // conservative estimate
        window_title_rect.x = pane.floating_pos.x;
        window_title_rect.y = pane.floating_pos.y;
        window_title_rect.width = pane.floating_size.x;
        window_title_rect.height = 30;

        wxRect ClientRect = wxGetClientDisplayRect();
        ClientRect.Deflate(
            60, 60);  // Prevent the new window from being too close to the edge
        if (!ClientRect.Intersects(window_title_rect)) b_reset_pos = true;

#endif

        if (b_reset_pos) pane.FloatingPosition(50, 50);

        if (count == 0)
          if (b_anyviz)
            pane.Show(cont->m_bIsVisible);
          else {
            cont->m_bIsVisible = cont->m_bPersVisible;
            pane.Show(cont->m_bIsVisible);
          }
        else
          pane.Show(false);
      }

      // Restore size of docked pane
      if (pane.IsShown() && pane.IsDocked()) {
        pane.BestSize(cont->m_best_size);
        m_pauimgr->Update();
      }

      //  This patch fixes a bug in wxAUIManager
      //  FS#548
      // Dropping a DashBoard Window right on top on the (supposedly fixed)
      // chart bar window causes a resize of the chart bar, and the Dashboard
      // window assumes some of its properties The Dashboard window is no longer
      // grabbable... Workaround:  detect this case, and force the pane to be on
      // a different Row. so that the display is corrected by toggling the
      // dashboard off and back on.
      if ((pane.dock_direction == wxAUI_DOCK_BOTTOM) && pane.IsDocked())
        pane.Row(2);
    }
  }
  // Toggle is handled by the toolbar but we must keep plugin manager b_toggle
  // updated to actual status to ensure right status upon toolbar rebuild
  SetToolbarItemState(m_toolbar_item_id,  GetDashboardWindowShownCount() != 0);
  m_pauimgr->Update();
}

//    This method is called after the PlugIn is initialized
//    and the frame has done its initial layout, possibly from a saved
//    wxAuiManager "Perspective" It is a chance for the PlugIn to syncronize
//    itself internally with the state of any Panes that were added to the
//    frame in the PlugIn ctor.
void Dashboard::UpdateAuiStatus(void) {

  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
    wxAuiPaneInfo &pane = m_pauimgr->GetPane(cont->m_pDashboardWindow);
    // Initialize visible state as perspective is loaded now
    cont->m_bIsVisible = (pane.IsOk() && pane.IsShown());

    // Correct for incomplete AUIManager perspective when docked dashboard is
    //  not visible at app close.
    if (pane.IsDocked()) {
      if ((cont->m_persist_size.x > 50) && (cont->m_persist_size.y > 50))
        cont->m_pDashboardWindow->SetSize(cont->m_persist_size);
    }

#ifdef __WXQT__
    if (pane.IsShown()) {
      pane.Show(false);
      m_pauimgr->Update();
      pane.Show(true);
      m_pauimgr->Update();
    }
#endif
  }
  m_pauimgr->Update();

  // Synchronize toolbar button state
  SetToolbarItemState(m_toolbar_item_id, GetDashboardWindowShownCount() != 0);
}

// Load the plugin configuration
bool Dashboard::LoadConfig(void) {
  wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

  if (pConf) {
    pConf->SetPath("/PlugIns/Engine-Dashboard");

    wxString version;
    pConf->Read("Version", &version, wxEmptyString);
    wxString config;

    // Set some sensible defaults
    wxString TitleFont;
    wxString DataFont;
    wxString LabelFont;
    wxString SmallFont;

#ifdef __OCPN__ANDROID__
    TitleFont = "Roboto,16,-1,5,50,0,0,0,0,0";
    DataFont = "Roboto,16,-1,5,50,0,0,0,0,0";
    LabelFont = "Roboto,16,-1,5,50,0,0,0,0,0";
    SmallFont = "Roboto,14,-1,5,50,0,0,0,0,0";
#else
    TitleFont = g_pFontTitle->GetChosenFont().GetNativeFontInfoDesc();
    DataFont = g_pFontData->GetChosenFont().GetNativeFontInfoDesc();
    LabelFont = g_pFontLabel->GetChosenFont().GetNativeFontInfoDesc();
    SmallFont = g_pFontSmall->GetChosenFont().GetNativeFontInfoDesc();
#endif

    double scaler = 1.0;
    wxFont DummyFont;
    wxFont *pDF = &DummyFont;

    if (OCPN_GetWinDIPScaleFactor() < 1.0)
      scaler = 1.0 + OCPN_GetWinDIPScaleFactor() / 4;
    scaler = wxMax(1.0, scaler);

    g_pFontTitle = &g_FontTitle;
    pConf->Read("FontTitle", &config, TitleFont);
    LoadFont(&pDF, config);
    wxFont DummyFontTitle = *pDF;
    pConf->Read("ColorTitle", &config, "#000000");
    wxColour DummyColor(config);
    g_pUSFontTitle->SetChosenFont(DummyFontTitle);
    g_pUSFontTitle->SetColour(DummyColor);

    g_FontTitle = *g_pUSFontTitle;
    g_FontTitle.SetChosenFont(g_pUSFontTitle->GetChosenFont().Scaled(scaler));
    g_USFontTitle = *g_pUSFontTitle;

    g_pFontData = &g_FontData;
    pConf->Read("FontData", &config, DataFont);
    LoadFont(&pDF, config);
    wxFont DummyFontData = *pDF;
    pConf->Read("ColorData", &config, "#000000");
    DummyColor.Set(config);
    g_pUSFontData->SetChosenFont(DummyFontData);
    g_pUSFontData->SetColour(DummyColor);
    g_FontData = *g_pUSFontData;
    g_FontData.SetChosenFont(g_pUSFontData->GetChosenFont().Scaled(scaler));
    g_USFontData = *g_pUSFontData;

    pConf->Read("ForceBackgroundColor", &g_ForceBackgroundColor, 0);
    pConf->Read("BackgroundColor", &config, "DASHL");
    g_BackgroundColor.Set(config);

    int alignment;
    pConf->Read("TitleAlignment", &alignment, (int)wxALIGN_LEFT);
    g_TitleAlignment = (wxAlignment)alignment;
    if (g_TitleAlignment == wxALIGN_INVALID) g_TitleAlignment = wxALIGN_LEFT;
    pConf->Read("TitleMargin", &g_iTitleMargin, 5);
    pConf->Read("DataShowUnit", &g_bShowUnit, true);
    pConf->Read("DataAlignment", &alignment, (int)wxALIGN_LEFT);
    g_DataAlignment = (wxAlignment)alignment;
    if (g_DataAlignment == wxALIGN_INVALID) g_DataAlignment = wxALIGN_LEFT;
    pConf->Read("DataMargin", &g_iDataMargin, 10);
    pConf->Read("InstrumentSpacing", &g_iInstrumentSpacing, 0);
    pConf->Read("TitleVerticalOffset", &g_TitleVerticalOffset, 0.0);

    g_pFontLabel = &g_FontLabel;
    pConf->Read("FontLabel", &config, LabelFont);
    LoadFont(&pDF, config);
    wxFont DummyFontLabel = *pDF;
    pConf->Read("ColorLabel", &config, "#000000");
    DummyColor.Set(config);
    g_pUSFontLabel->SetChosenFont(DummyFontLabel);
    g_pUSFontLabel->SetColour(DummyColor);
    g_FontLabel = *g_pUSFontLabel;
    g_FontLabel.SetChosenFont(g_pUSFontLabel->GetChosenFont().Scaled(scaler));
    g_USFontLabel = *g_pUSFontLabel;

    g_pFontSmall = &g_FontSmall;
    pConf->Read("FontSmall", &config, SmallFont);
    LoadFont(&pDF, config);
    wxFont DummyFontSmall = *pDF;
    pConf->Read("ColorSmall", &config, "#000000");
    DummyColor.Set(config);
    g_pUSFontSmall->SetChosenFont(DummyFontSmall);
    g_pUSFontSmall->SetColour(DummyColor);
    g_FontSmall = *g_pUSFontSmall;
    g_FontSmall.SetChosenFont(g_pUSFontSmall->GetChosenFont().Scaled(scaler));
    g_USFontSmall = *g_pUSFontSmall;

	// Load the maximum tachometer value, Temperature & Pressure units and dual engine status
	pConf->Read(_T("TachometerMax"), &g_tachometerMax, 6000);
	pConf->Read(_T("TemperatureUnit"), &g_temperatureUnit, TEMPERATURE_CELSIUS);
	pConf->Read(_T("PressureUnit"), &g_pressureUnit, PRESSURE_BAR);
	pConf->Read(_T("DualEngine"), &g_dualEngine, false);
	pConf->Read(_T("TwentyFourVolt"), &g_twentyFourVolts, false);
	pConf->Read(_T("HighContrast"), &g_highContrast, false);

    pConf->Read("PrefWidth", &g_dashPrefWidth, 0);
    pConf->Read("PrefHeight", &g_dashPrefHeight, 0);

    int d_cnt;
    pConf->Read("DashboardCount", &d_cnt, -1);
    // TODO: Memory leak? We should destroy everything first
    m_ArrayOfDashboardWindow.Clear();
    if (version.IsEmpty() && d_cnt == -1) {
      m_config_version = 1;
      // Let's load version 1 or default settings.
      int i_cnt;
      pConf->Read("InstrumentCount", &i_cnt, -1);
      wxArrayInt ar;
      wxArrayOfInstrumentProperties Property;
      if (i_cnt != -1) {
        for (int i = 0; i < i_cnt; i++) {
          int id;
          pConf->Read(wxString::Format("Instrument%d", i + 1), &id, -1);
          if (id != -1) ar.Add(id);
        }
      } else {
        // This is the default instrument list
        ar.Add(ID_DBP_MAIN_ENGINE_RPM);
        ar.Add(ID_DBP_MAIN_ENGINE_OIL);
        ar.Add(ID_DBP_MAIN_ENGINE_WATER);
      }

      DashboardWindowContainer *cont = new DashboardWindowContainer(
          NULL, MakeName(), _("Instruments"), "V", ar, Property);
      cont->m_bPersVisible = true;
      m_ArrayOfDashboardWindow.Add(cont);

    } else {
      // Version 2
      m_config_version = 2;
      bool b_onePersisted = false;
      wxSize best_size;
      wxSize persist_size;
      for (int k = 0; k < d_cnt; k++) {
        pConf->SetPath(
            wxString::Format("/PlugIns/Engine-Dashboard/Dashboard%d", k + 1));
        wxString name;
        pConf->Read("Name", &name, MakeName());
        wxString caption;
        pConf->Read("Caption", &caption, _("Instruments"));
        wxString orient;
        pConf->Read("Orientation", &orient, "V");
        int i_cnt;
        pConf->Read("InstrumentCount", &i_cnt, -1);
        bool b_persist;
        pConf->Read("Persistence", &b_persist, 1);
        int val;
        pConf->Read("BestSizeX", &val, DefaultWidth);
        best_size.x = val;
        pConf->Read("BestSizeY", &val, DefaultWidth);
        best_size.y = val;
        pConf->Read("PersistSizeX", &val, DefaultWidth);
        persist_size.x = val;
        pConf->Read("PersistSizeY", &val, DefaultWidth);
        persist_size.y = val;

        wxArrayInt ar;
        wxArrayOfInstrumentProperties Property;
        for (int i = 0; i < i_cnt; i++) {
          int id;
          pConf->Read(wxString::Format("Instrument%d", i + 1), &id, -1);
          if (id != -1) {
            ar.Add(id);
            InstrumentProperties *instp;
            if (pConf->Exists(wxString::Format("InstTitleFont%d", i + 1))) {
              instp = new InstrumentProperties(id, i);

              pConf->Read(wxString::Format("InstTitleFont%d", i + 1), &config,
                          TitleFont);
              LoadFont(&pDF, config);
              wxFont DummyFontTitleA = *pDF;
              pConf->Read(wxString::Format("InstTitleColor%d", i + 1), &config,
                          "#000000");
              DummyColor.Set(config);
              instp->m_USTitleFont.SetChosenFont(DummyFontTitleA);
              instp->m_USTitleFont.SetColour(DummyColor);
              instp->m_TitleFont = instp->m_USTitleFont;
              instp->m_TitleFont.SetChosenFont(
                  instp->m_USTitleFont.GetChosenFont().Scaled(scaler));

              pConf->Read(wxString::Format("InstDataShowUnit%d", i + 1),
                          &instp->m_ShowUnit, -1);
              pConf->Read(wxString::Format("InstDataMargin%d", i + 1),
                          &instp->m_DataMargin, -1);
              pConf->Read(wxString::Format("InstDataAlignment%d", i + 1),
                          &alignment, (int)wxALIGN_INVALID);
              instp->m_DataAlignment = (wxAlignment)alignment;
              pConf->Read(wxString::Format("InstInstrumentSpacing%d", i + 1),
                          &instp->m_InstrumentSpacing, -1);
              pConf->Read(wxString::Format("InstDataFormat%d", i + 1),
                          &instp->m_Format, "");
              pConf->Read(wxString::Format("InstTitle%d", i + 1),
                          &instp->m_Title, "");

              pConf->Read(wxString::Format("InstDataFont%d", i + 1), &config,
                          DataFont);
              LoadFont(&pDF, config);
              wxFont DummyFontDataA = *pDF;
              pConf->Read(wxString::Format("InstDataColor%d", i + 1), &config,
                          "#000000");
              DummyColor.Set(config);
              instp->m_USDataFont.SetChosenFont(DummyFontDataA);
              instp->m_USDataFont.SetColour(DummyColor);
              instp->m_DataFont = instp->m_USDataFont;
              instp->m_DataFont.SetChosenFont(
                  instp->m_USDataFont.GetChosenFont().Scaled(scaler));

              pConf->Read(wxString::Format("InstLabelFont%d", i + 1), &config,
                          LabelFont);
              LoadFont(&pDF, config);
              wxFont DummyFontLabelA = *pDF;
              pConf->Read(wxString::Format("InstLabelColor%d", i + 1), &config,
                          "#000000");
              DummyColor.Set(config);
              instp->m_USLabelFont.SetChosenFont(DummyFontLabelA);
              instp->m_USLabelFont.SetColour(DummyColor);
              instp->m_LabelFont = instp->m_USLabelFont;
              instp->m_LabelFont.SetChosenFont(
                  instp->m_USLabelFont.GetChosenFont().Scaled(scaler));

              pConf->Read(wxString::Format("InstSmallFont%d", i + 1), &config,
                          SmallFont);
              LoadFont(&pDF, config);
              wxFont DummyFontSmallA = *pDF;
              pConf->Read(wxString::Format("InstSmallColor%d", i + 1), &config,
                          "#000000");
              DummyColor.Set(config);
              instp->m_USSmallFont.SetChosenFont(DummyFontSmallA);
              instp->m_USSmallFont.SetColour(DummyColor);
              instp->m_SmallFont = instp->m_USSmallFont;
              instp->m_SmallFont.SetChosenFont(
                  instp->m_USSmallFont.GetChosenFont().Scaled(scaler));

              pConf->Read(wxString::Format("TitleBackColor%d", i + 1), &config,
                          "DASHL");
              instp->m_TitleBackgroundColour.Set(config);

              pConf->Read(wxString::Format("DataBackColor%d", i + 1), &config,
                          "DASHB");
              instp->m_DataBackgroundColour.Set(config);

              pConf->Read(wxString::Format("ArrowFirst%d", i + 1), &config,
                          "DASHN");
              instp->m_Arrow_First_Colour.Set(config);

              pConf->Read(wxString::Format("ArrowSecond%d", i + 1), &config,
                          "BLUE3");
              instp->m_Arrow_Second_Colour.Set(config);

              Property.Add(instp);
            }
          }
        }
        // TODO: Do not add if GetCount == 0

        DashboardWindowContainer *cont = new DashboardWindowContainer(
            NULL, name, caption, orient, ar, Property);
        cont->m_bPersVisible = b_persist;
        cont->m_conf_best_size = best_size;
        cont->m_persist_size = persist_size;

        if (b_persist) b_onePersisted = true;

        m_ArrayOfDashboardWindow.Add(cont);
      }

      // Make sure at least one dashboard is scheduled to be visible
      if (m_ArrayOfDashboardWindow.Count() && !b_onePersisted) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(0);
        if (cont) cont->m_bPersVisible = true;
      }
    }

    return true;
  } else
    return false;
}

void Dashboard::LoadFont(wxFont **target, wxString native_info) {
  if (!native_info.IsEmpty()) {
#ifdef __OCPN__ANDROID__
    wxFont *nf = new wxFont(native_info);
    *target = nf;
#else
    (*target)->SetNativeFontInfo(native_info);
#endif
  }
}

bool Dashboard::SaveConfig(void) {
  wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

  if (pConf) {
    pConf->SetPath("/PlugIns/Engine-Dashboard");
    pConf->Write("Version", "2");
    pConf->Write("FontTitle",
                 g_pUSFontTitle->GetChosenFont().GetNativeFontInfoDesc());
    pConf->Write("ColorTitle",
                 g_pUSFontTitle->GetColour().GetAsString(wxC2S_HTML_SYNTAX));
    pConf->Write("FontData",
                 g_pUSFontData->GetChosenFont().GetNativeFontInfoDesc());
    pConf->Write("ColorData",
                 g_pUSFontData->GetColour().GetAsString(wxC2S_HTML_SYNTAX));
    pConf->Write("FontLabel",
                 g_pUSFontLabel->GetChosenFont().GetNativeFontInfoDesc());
    pConf->Write("ColorLabel",
                 g_pUSFontLabel->GetColour().GetAsString(wxC2S_HTML_SYNTAX));
    pConf->Write("FontSmall",
                 g_pUSFontSmall->GetChosenFont().GetNativeFontInfoDesc());
    pConf->Write("ColorSmall",
                 g_pUSFontSmall->GetColour().GetAsString(wxC2S_HTML_SYNTAX));
    
	pConf->Write("PrefWidth", g_dashPrefWidth);
	pConf->Write("PrefHeight", g_dashPrefHeight);

	pConf->Write(_T("TachometerMax"), g_tachometerMax);
	pConf->Write(_T("TemperatureUnit"), g_temperatureUnit);
	pConf->Write(_T("PressureUnit"), g_pressureUnit);
	pConf->Write(_T("DualEngine"), g_dualEngine);
	pConf->Write(_T("TwentyFourVolt"), g_twentyFourVolts);
	pConf->Write(_T("HighContrast"), g_highContrast);

    pConf->Write("DashboardCount", (int)m_ArrayOfDashboardWindow.GetCount());
    // Delete old Dashborads
    for (size_t i = m_ArrayOfDashboardWindow.GetCount(); i < 20; i++) {
      if (pConf->Exists(
              wxString::Format("/PlugIns/Engine-Dashboard/Dashboard%zu", i + 1))) {
        pConf->DeleteGroup(
            wxString::Format("/PlugIns/Engine-Dashboard/Dashboard%zu", i + 1));
      }
    }
    for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
      DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
      pConf->SetPath(
          wxString::Format("/PlugIns/Engine-Dashboard/Dashboard%zu", i + 1));
      pConf->Write("Name", cont->m_sName);
      pConf->Write("Caption", cont->m_sCaption);
      pConf->Write("Orientation", cont->m_sOrientation);
      pConf->Write("Persistence", cont->m_bPersVisible);
      pConf->Write("InstrumentCount", (int)cont->m_aInstrumentList.GetCount());
      pConf->Write("BestSizeX", cont->m_best_size.x);
      pConf->Write("BestSizeY", cont->m_best_size.y);
      pConf->Write("PersistSizeX", cont->m_pDashboardWindow->GetSize().x);
      pConf->Write("PersistSizeY", cont->m_pDashboardWindow->GetSize().y);

      // Delete old Instruments
      for (size_t i = cont->m_aInstrumentList.GetCount(); i < 40; i++) {
        if (pConf->Exists(wxString::Format("Instrument%zu", i + 1))) {
          pConf->DeleteEntry(wxString::Format("Instrument%zu", i + 1));
          if (pConf->Exists(wxString::Format("InstTitleFont%zu", i + 1))) {
            pConf->DeleteEntry(wxString::Format("InstTitleFont%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstTitleColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstTitle%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataShowUnit%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataMargin%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataAlignment%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataFormat%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataFont%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstLabelFont%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstLabelColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstSmallFont%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstSmallColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("TitleBackColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("DataBackColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("ArrowFirst%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("ArrowSecond%zu", i + 1));
          }
        }
      }
      for (size_t j = 0; j < cont->m_aInstrumentList.GetCount(); j++) {
        pConf->Write(wxString::Format("Instrument%zu", j + 1),
                     cont->m_aInstrumentList.Item(j));
        InstrumentProperties *Inst = NULL;
        // First delete
        if (pConf->Exists(wxString::Format("InstTitleFont%zu", j + 1))) {
          bool Delete = true;
          for (size_t i = 0; i < cont->m_aInstrumentPropertyList.GetCount();
               i++) {
            Inst = cont->m_aInstrumentPropertyList.Item(i);
            if (Inst->m_Listplace == (int)j) {
              Delete = false;
              break;
            }
          }
          if (Delete) {
            pConf->DeleteEntry(wxString::Format("InstTitleFont%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstTitleColor%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstTitle%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstDataShowUnit%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataMargin%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataAlignment%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataFormat%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("InstDataFont%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstDataColor%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstLabelFont%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstLabelColor%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstSmallFont%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("InstSmallColor%zu", j + 1));
            pConf->DeleteEntry(wxString::Format("TitleBackColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("DataBackColor%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("ArrowFirst%zu", i + 1));
            pConf->DeleteEntry(wxString::Format("ArrowSecond%zu", i + 1));
          }
        }
        Inst = NULL;
        for (size_t i = 0; i < (cont->m_aInstrumentPropertyList.GetCount());
             i++) {
          Inst = cont->m_aInstrumentPropertyList.Item(i);
          if (Inst->m_Listplace == (int)j) {
            pConf->Write(
                wxString::Format("InstTitleFont%zu", j + 1),
                Inst->m_USTitleFont.GetChosenFont().GetNativeFontInfoDesc());
            pConf->Write(
                wxString::Format("InstTitleColor%zu", j + 1),
                Inst->m_USTitleFont.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
            pConf->Write(
                wxString::Format("InstDataFont%zu", j + 1),
                Inst->m_USDataFont.GetChosenFont().GetNativeFontInfoDesc());
            pConf->Write(
                wxString::Format("InstDataColor%zu", j + 1),
                Inst->m_USDataFont.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
            pConf->Write(
                wxString::Format("InstLabelFont%zu", j + 1),
                Inst->m_USLabelFont.GetChosenFont().GetNativeFontInfoDesc());
            pConf->Write(
                wxString::Format("InstLabelColor%zu", j + 1),
                Inst->m_USLabelFont.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
            pConf->Write(
                wxString::Format("InstSmallFont%zu", j + 1),
                Inst->m_USSmallFont.GetChosenFont().GetNativeFontInfoDesc());
            pConf->Write(
                wxString::Format("InstSmallColor%zu", j + 1),
                Inst->m_USSmallFont.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
            pConf->Write(
                wxString::Format("TitleBackColor%zu", j + 1),
                Inst->m_TitleBackgroundColour.GetAsString(wxC2S_HTML_SYNTAX));
            pConf->Write(
                wxString::Format("DataBackColor%zu", j + 1),
                Inst->m_DataBackgroundColour.GetAsString(wxC2S_HTML_SYNTAX));
            pConf->Write(
                wxString::Format("ArrowFirst%zu", j + 1),
                Inst->m_Arrow_First_Colour.GetAsString(wxC2S_HTML_SYNTAX));
            pConf->Write(
                wxString::Format("ArrowSecond%zu", j + 1),
                Inst->m_Arrow_Second_Colour.GetAsString(wxC2S_HTML_SYNTAX));
            break;
          }
        }
      }
    }
    pConf->Flush();
    return true;
  } else
    return false;
}

void Dashboard::ApplyConfig(void) {
  // Reverse order to handle deletes
  for (size_t i = m_ArrayOfDashboardWindow.GetCount(); i > 0; i--) {
    DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i - 1);
    int orient = (cont->m_sOrientation == "V" ? wxVERTICAL : wxHORIZONTAL);
    if (cont->m_bIsDeleted) {
      if (cont->m_pDashboardWindow) {
        m_pauimgr->DetachPane(cont->m_pDashboardWindow);
        cont->m_pDashboardWindow->Close();
        cont->m_pDashboardWindow->Destroy();
        cont->m_pDashboardWindow = NULL;
      }
      m_ArrayOfDashboardWindow.Remove(cont);
      delete cont;

    } else if (!cont->m_pDashboardWindow) {
      // A new dashboard is created
      cont->m_pDashboardWindow = new DashboardWindow(
          GetOCPNCanvasWindow(), wxID_ANY, m_pauimgr, this, orient, cont);
      cont->m_pDashboardWindow->SetInstrumentList(
          cont->m_aInstrumentList, &(cont->m_aInstrumentPropertyList));
      bool vertical = orient == wxVERTICAL;
      wxSize sz = cont->m_pDashboardWindow->GetMinSize();
      wxSize best = cont->m_conf_best_size;
      if (best.x < 100) best = sz;

// Mac has a little trouble with initial Layout() sizing...
#ifdef __WXOSX__
      if (sz.x == 0) sz.IncTo(wxSize(160, 388));
#endif
      wxAuiPaneInfo p = wxAuiPaneInfo()
                            .Name(cont->m_sName)
                            .Caption(cont->m_sCaption)
                            .CaptionVisible(false)
                            .TopDockable(!vertical)
                            .BottomDockable(!vertical)
                            .LeftDockable(vertical)
                            .RightDockable(vertical)
                            .MinSize(sz)
                            .BestSize(best)
                            .FloatingSize(sz)
                            .FloatingPosition(100, 100)
                            .Float()
                            .Show(cont->m_bIsVisible)
                            .Gripper(false);

      m_pauimgr->AddPane(cont->m_pDashboardWindow, p);
      // wxAuiPaneInfo().Name( cont->m_sName ).Caption( cont->m_sCaption
      // ).CaptionVisible( false ).TopDockable(
      // !vertical ).BottomDockable( !vertical ).LeftDockable( vertical
      // ).RightDockable( vertical ).MinSize( sz ).BestSize( sz ).FloatingSize(
      // sz ).FloatingPosition( 100, 100 ).Float().Show( cont->m_bIsVisible ) );

#ifdef __OCPN__ANDROID__
      wxAuiPaneInfo &pane = m_pauimgr->GetPane(cont->m_pDashboardWindow);
      pane.Dockable(false);

#endif

    } else {
      wxAuiPaneInfo &pane = m_pauimgr->GetPane(cont->m_pDashboardWindow);
      pane.Caption(cont->m_sCaption).Show(cont->m_bIsVisible);
      if (!cont->m_pDashboardWindow->IsInstrumentListEqual(
              cont->m_aInstrumentList)) {
        cont->m_pDashboardWindow->SetInstrumentList(
            cont->m_aInstrumentList, &(cont->m_aInstrumentPropertyList));
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

void Dashboard::PopulateContextMenu(wxMenu *menu) {
  int nvis = 0;
  wxMenuItem *visItem = 0;
  for (size_t i = 0; i < m_ArrayOfDashboardWindow.GetCount(); i++) {
    DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(i);
    wxMenuItem *item = menu->AppendCheckItem(i + 1, cont->m_sCaption);
    item->Check(cont->m_bIsVisible);
    if (cont->m_bIsVisible) {
      nvis++;
      visItem = item;
    }
  }
  if (nvis == 1 && visItem) visItem->Enable(false);
}

void Dashboard::ShowDashboard(size_t id, bool visible) {
  if (id < m_ArrayOfDashboardWindow.GetCount()) {
    DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item(id);
    m_pauimgr->GetPane(cont->m_pDashboardWindow).Show(visible);
    cont->m_bIsVisible = visible;
    cont->m_bPersVisible = visible;
    m_pauimgr->Update();
  }
}

