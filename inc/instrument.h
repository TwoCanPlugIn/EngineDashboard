//
// This file is part of Engine Dashboard, a plugin for OpenCPN.
// based on the original version of the dashboard.
// Author: Steven Adler
//
/***************************************************************************
 * $Id: instrument.h, v1.0 2010/08/30 SethDart Exp $
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

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#if !wxUSE_GRAPHICS_CONTEXT
#define wxGCDC wxDC
#endif

// Required GetGlobalColor
#include "ocpn_plugin.h"
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>         // supplemental, for Mac

// Used by the Dashboard Capability Enums
#include <bitset>

// This is the degree sign in UTF8. It should be correctly handled on both Win & Unix
const wxString DEGREE_SIGN = wxString::Format(_T("%c"), 0x00B0); 

#define DefaultWidth 150

extern wxFont *g_pFontTitle;
extern wxFont *g_pFontData;
extern wxFont *g_pFontLabel;
extern wxFont *g_pFontSmall;

wxString toSDMM(int NEflag, double a);

class DashboardInstrument;
class DashboardInstrument_Single;
class DashboardInstrument_Gauge;
class DashboardInstrument_Block;

enum DASH_CAP {
	OCPN_DBP_STC_MAIN_ENGINE_RPM = 1,
	OCPN_DBP_STC_PORT_ENGINE_RPM,
	OCPN_DBP_STC_STBD_ENGINE_RPM,
	OCPN_DBP_STC_MAIN_ENGINE_OIL,
	OCPN_DBP_STC_PORT_ENGINE_OIL,
	OCPN_DBP_STC_STBD_ENGINE_OIL,
	OCPN_DBP_STC_MAIN_ENGINE_EXHAUST,
	OCPN_DBP_STC_PORT_ENGINE_EXHAUST,
	OCPN_DBP_STC_STBD_ENGINE_EXHAUST,
	OCPN_DBP_STC_MAIN_ENGINE_WATER,
	OCPN_DBP_STC_PORT_ENGINE_WATER,
	OCPN_DBP_STC_STBD_ENGINE_WATER,
	OCPN_DBP_STC_MAIN_ENGINE_VOLTS,
	OCPN_DBP_STC_PORT_ENGINE_VOLTS,
	OCPN_DBP_STC_STBD_ENGINE_VOLTS,
	OCPN_DBP_STC_MAIN_ENGINE_HOURS,
	OCPN_DBP_STC_PORT_ENGINE_HOURS,
	OCPN_DBP_STC_STBD_ENGINE_HOURS,
	OCPN_DBP_STC_TANK_LEVEL_FUEL_01,
	OCPN_DBP_STC_TANK_LEVEL_WATER_01,
	OCPN_DBP_STC_TANK_LEVEL_OIL,
	OCPN_DBP_STC_TANK_LEVEL_LIVEWELL,
	OCPN_DBP_STC_TANK_LEVEL_GREY,
	OCPN_DBP_STC_TANK_LEVEL_BLACK,
	OCPN_DBP_STC_RSA,
	OCPN_DBP_STC_START_BATTERY_VOLTS,
	OCPN_DBP_STC_START_BATTERY_AMPS,
	OCPN_DBP_STC_HOUSE_BATTERY_VOLTS,
	OCPN_DBP_STC_HOUSE_BATTERY_AMPS,
	OCPN_DBP_STC_TANK_LEVEL_FUEL_02,
	OCPN_DBP_STC_TANK_LEVEL_WATER_02,
	OCPN_DBP_STC_TANK_LEVEL_WATER_03,
	OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01,
	OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_02,
	OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01,
	OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_02,
	OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_03,
	OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE,
	OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE,
	OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE,
	OCPN_DBP_STC_MAIN_ENGINE_FAULT_TWO,
	OCPN_DBP_STC_PORT_ENGINE_FAULT_TWO,
	OCPN_DBP_STC_STBD_ENGINE_FAULT_TWO,
	OCPN_DBP_STC_LAST
};

#define N_INSTRUMENTS  ((int)OCPN_DBP_STC_LAST)  // Number of instrument capability flags
using CapType = std::bitset<N_INSTRUMENTS>;

class DashboardInstrument : public wxControl {
public:
	DashboardInstrument(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap_flag);
	~DashboardInstrument(){}

	CapType GetCapacity();
	void OnEraseBackground(wxEraseEvent &WXUNUSED(evt));
	virtual wxSize GetSize(int orient, wxSize hint) = 0;
	void OnPaint(wxPaintEvent &WXUNUSED(event));
	virtual void SetData(DASH_CAP st, double data, wxString unit) = 0;
	void SetCapFlag(DASH_CAP val) { m_cap_flag.set(val); }
	bool HasCapFlag(DASH_CAP val) { return m_cap_flag.test(val); }
	void SetDrawSoloInPane(bool value);
	void MouseEvent(wxMouseEvent &event);
	int instrumentTypeId;

protected:
	CapType m_cap_flag;
	int m_TitleHeight;
	wxString m_title;
	virtual void Draw(wxGCDC *dc) = 0;

private:
	bool m_drawSoloInPane;
};

class DashboardInstrument_Single : public DashboardInstrument {
public:
	DashboardInstrument_Single(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap, wxString format);
	~DashboardInstrument_Single(){}

	wxSize GetSize(int orient, wxSize hint);
	void SetData(DASH_CAP st, double data, wxString unit);

protected:
	wxString m_data;
	wxString m_format;
	int m_DataHeight;
	
	void Draw(wxGCDC *dc);
};

// A simple gauge using the wxGauge Control
class DashboardInstrument_Gauge : public DashboardInstrument
{
public:
	DashboardInstrument_Gauge(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap_flag);
	~DashboardInstrument_Gauge(void);
	wxGauge *gauge;
	wxSize GetSize(int orient, wxSize hint);
	void SetData(DASH_CAP st, double data, wxString unit);

protected:
	void Draw(wxGCDC* dc);

};

class DashboardInstrument_Block : public DashboardInstrument
{
public:
	DashboardInstrument_Block(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap, wxString format);
	~DashboardInstrument_Block() {}

	wxSize GetSize(int orient, wxSize hint);
	void SetData(DASH_CAP st, double data, wxString unit);

protected:
	wxString          m_data;
	wxString          m_format;
	int               m_DataHeight;
	int 			  m_Value;

	void Draw(wxGCDC* dc);
};

#endif // _INSTRUMENT_H_
