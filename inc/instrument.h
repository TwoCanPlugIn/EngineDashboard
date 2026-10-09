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

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif  // precompiled headers

#if !wxUSE_GRAPHICS_CONTEXT
#define wxGCDC wxDC
#endif

// Required GetGlobalColor
#include <ocpn_plugin.h>
#include <wx/dcbuffer.h>
// Supplemental for Mac
#include <wx/dcgraph.h>  

#include <bitset>
#include <wx/fontdata.h>

// This is the degree sign in UTF8. It should be correctly handled on both Win & Unix
const wxString DEGREE_SIGN = wxString::Format("%c", 0x00B0);  
                    
#define DefaultWidth 150

extern wxFontData *g_pFontTitle;
extern wxFontData *g_pFontData;
extern wxFontData *g_pFontLabel;
extern wxFontData *g_pFontSmall;

extern wxFontData *g_pUSFontTitle;
extern wxFontData *g_pUSFontData;
extern wxFontData *g_pUSFontLabel;
extern wxFontData *g_pUSFontSmall;

extern wxAlignment g_TitleAlignment;
extern double g_TitleVerticalOffset;
extern int g_iTitleMargin;
extern bool g_bShowUnit;
extern wxAlignment g_DataAlignment;
extern int g_iDataMargin;
extern int g_iInstrumentSpacing;

extern bool g_highContrast;

class DashboardInstrument;
class DashboardInstrument_Single;
class DashboardInstrument_Position;
class DashboardInstrument_Sun;

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
	OCPN_DBP_STC_MAIN_ENGINE_FUEL_RATE,
	OCPN_DBP_STC_PORT_ENGINE_FUEL_RATE,
	OCPN_DBP_STC_STBD_ENGINE_FUEL_RATE,
	OCPN_DBP_STC_START_BATTERY_SOC,
	OCPN_DBP_STC_START_BATTERY_HOURS,
	OCPN_DBP_STC_HOUSE_BATTERY_SOC,
	OCPN_DBP_STC_HOUSE_BATTERY_HOURS,
	OCPN_DBP_STC_LAST
};

// Number of instrument capability flags
#define N_INSTRUMENTS ((int)OCPN_DBP_STC_LAST)  
using CapType = std::bitset<N_INSTRUMENTS>;

wxColour GetColourSchemeBackgroundColour(wxColour co);
wxColour GetColourSchemeFont(wxColour co);

class InstrumentProperties {
public:
  InstrumentProperties() { SetDefault(); }
  InstrumentProperties(int aInstrument, int Listplace) {
    m_aInstrument = aInstrument;
    m_Listplace = Listplace;
    m_ShowUnit = -1;
    m_DataAlignment = wxALIGN_INVALID;
    m_DataMargin = -1;
    m_InstrumentSpacing = -1;
    m_Format = "";
    m_Title = "";
    m_TitleFont = *(g_pFontTitle);
    m_USTitleFont = *(g_pUSFontTitle);
    m_DataFont = *(g_pFontData);
    m_USDataFont = *(g_pUSFontData);
    m_LabelFont = *(g_pFontLabel);
    m_USLabelFont = *(g_pUSFontLabel);
    m_SmallFont = *(g_pFontSmall);
    m_USSmallFont = *(g_pUSFontSmall);
    GetGlobalColor("DASHL", &m_TitleBackgroundColour);
    GetGlobalColor("DASHB", &m_DataBackgroundColour);
    GetGlobalColor("DASHN", &m_Arrow_First_Colour);
    GetGlobalColor("BLUE3", &m_Arrow_Second_Colour);
  }
  ~InstrumentProperties() {}
  void SetDefault() {
    m_aInstrument = -1;
    m_Listplace = -1;
    m_ShowUnit = -1;
    m_DataAlignment = wxALIGN_INVALID;
    m_DataMargin = -1;
    m_InstrumentSpacing = -1;
    m_Format = "";
    m_Title = "";
    m_TitleFont = *(g_pFontTitle);
    m_USTitleFont = *(g_pUSFontTitle);
    m_DataFont = *(g_pFontData);
    m_USDataFont = *(g_pUSFontData);
    m_LabelFont = *(g_pFontLabel);
    m_USLabelFont = *(g_pUSFontLabel);
    m_SmallFont = *(g_pFontSmall);
    m_USSmallFont = *(g_pUSFontSmall);
    GetGlobalColor("DASHL", &m_TitleBackgroundColour);
    GetGlobalColor("DASHB", &m_DataBackgroundColour);
    GetGlobalColor("DASHN", &m_Arrow_First_Colour);
    GetGlobalColor("BLUE3", &m_Arrow_Second_Colour);
  };
  int m_aInstrument;
  int m_Listplace;
  int m_ShowUnit;
  wxAlignment m_DataAlignment;
  int m_DataMargin;
  int m_InstrumentSpacing;
  wxString m_Format;
  wxString m_Title;
  wxFontData m_TitleFont;
  wxFontData m_USTitleFont;
  wxColour m_TitleBackgroundColour;
  wxFontData m_DataFont;
  wxFontData m_USDataFont;
  wxColour m_DataBackgroundColour;
  wxFontData m_LabelFont;
  wxFontData m_USLabelFont;
  wxFontData m_SmallFont;
  wxFontData m_USSmallFont;
  wxColour m_Arrow_First_Colour;
  wxColour m_Arrow_Second_Colour;
};

class DashboardInstrument : public wxControl {
public:
  DashboardInstrument(wxWindow *pparent, wxWindowID id, wxString title,
                      DASH_CAP cap_flag,
                      InstrumentProperties *Properties = NULL);
  ~DashboardInstrument() {}

  CapType GetCapabilityFlag();
  void OnEraseBackground(wxEraseEvent &WXUNUSED(evt));
  virtual wxSize GetSize(int orient, wxSize hint) = 0;
  void OnPaint(wxPaintEvent &WXUNUSED(event));
  virtual void SetData(DASH_CAP cap_flag, double data, wxString unit) = 0;
  void SetDrawSoloInPane(bool value);
  void MouseEvent(wxMouseEvent &event);
#ifdef HAVE_WX_GESTURE_EVENTS
  void OnLongPress(wxLongPressEvent &event);
#endif
  void OnLeftUp(wxMouseEvent &event);
  void SetCapFlag(DASH_CAP cap_flag) { m_cap_flag.set(cap_flag); }
  bool HasCapFlag(DASH_CAP cap_flag) { return m_cap_flag.test(cap_flag); }
  int instrumentTypeId;
  InstrumentProperties *m_Properties;

protected:
  CapType m_cap_flag;
  int m_InstrumentSpacing;
  int m_DataTextHeight;
  int m_DataMargin;
  int m_TitleWidth;
  int m_TitleHeight;
  int m_DataTop;
  int m_TitleTop;
  bool m_DataRightAlign;
  bool m_TitleRightAlign;
  wxString m_title;
  virtual void Draw(wxGCDC *dc) = 0;
  virtual void InitDataTextHeight(const wxString &sampleText, int &sampleWidth);
  virtual void InitTitleSize();
  virtual void InitTitleAndDataPosition(int drawHeight);
  virtual int GetFullHeight(int drawHeight);
  virtual int GetDataBottom(int clientHeight);
  virtual void SetDataFont(wxGCDC *dc);

private:
  bool m_drawSoloInPane;
  bool m_popupWanted;
};

class DashboardInstrument_Single : public DashboardInstrument {
public:
  DashboardInstrument_Single(wxWindow *pparent, wxWindowID id, wxString title,
                             InstrumentProperties *Properties, DASH_CAP cap_flag,
                             wxString format);
  ~DashboardInstrument_Single() {}

  wxSize GetSize(int orient, wxSize hint);
  void SetData(DASH_CAP cap_flag, double data, wxString unit);

protected:
  wxString m_data;
  wxString m_format;
  //  int m_DataHeight;
  //  InstrumentProperties* m_Properties;

  void Draw(wxGCDC *dc);
};

class DashboardInstrument_Block : public DashboardInstrument
{
public:
	DashboardInstrument_Block(wxWindow* pparent, wxWindowID id, wxString title, DASH_CAP cap, wxString format);
	~DashboardInstrument_Block() {}

	wxSize GetSize(int orient, wxSize hint);
	void SetData(DASH_CAP cap_flag, double data, wxString unit);

protected:
	wxString m_data;
	wxString m_format;
	int m_DataHeight;
	int m_Value;

	void Draw(wxGCDC* dc);
};


#endif
