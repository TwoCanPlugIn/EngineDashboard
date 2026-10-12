/******************************************************************************
 * $Id: dial.h, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 *           (Inspired by original work from Andreas Heiming)
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */

#ifndef __Dial_H__
#define __Dial_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "instrument.h"

// BUG BUG constexpr

#define ANGLE_OFFSET 90  // 0 degrees are at 12 o'clock, but screen orogin is 3 o'clock

// BUG BUG enum class
typedef enum {
  DIAL_LABEL_NONE,
  DIAL_LABEL_HORIZONTAL,
  DIAL_LABEL_ROTATED,
  DIAL_LABEL_FRACTIONS
} DialLabelOption;

typedef enum {
  DIAL_MARKER_NONE,
  DIAL_MARKER_SIMPLE,
  DIAL_MARKER_REDGREEN,
  DIAL_MARKER_REDGREENBAR,
  DIAL_MARKER_WARNING_HIGH,
  DIAL_MARKER_WARNING_LOW,
  DIAL_MARKER_GREEN_MID
} DialMarkerOption;

typedef enum {
  DIAL_POSITION_NONE,
  DIAL_POSITION_INSIDE,
  DIAL_POSITION_TOPLEFT,
  DIAL_POSITION_TOPRIGHT,
  DIAL_POSITION_BOTTOMLEFT,
  DIAL_POSITION_BOTTOMRIGHT,
  DIAL_POSITION_BOTTOMMIDDLE
} DialPositionOption;

// BUG BUG move to namespace
extern double rad2deg(double angle);
extern double deg2rad(double angle);

// BUG BUG Investigate elsewhere function foo(const wxString& value) if read only
extern wxString g_pluginFolder;

// Highcontrast colours inseat of day/dusk/night
extern bool g_highContrast;

// DashboardInstrument_Dial
// Implements a speedometer style gauge
class DashboardInstrument_Dial : public DashboardInstrument {
public:
  DashboardInstrument_Dial(wxWindow* parent, wxWindowID id, wxString title,
                           InstrumentProperties* Properties, DASH_CAP cap_flag,
                           int s_angle, int r_angle, int s_value, int e_value);

  ~DashboardInstrument_Dial(void);

  wxSize GetSize(int orient, wxSize hint);

  // BUG BUG These could possibly go into the base class ??
  wxFont GetSmallFont() const;
  wxFont GetLabelFont() const;
  wxColor GetSmallFontColour() const;
  wxColor GetLabelFontColour() const;

  // BUG BUG Investigate foo(const wxString& value) if read only
  void SetData(DASH_CAP, double, wxString);

  void SetOptionMarker(double step, DialMarkerOption option, int offset);

  void SetOptionLabel(double step, DialLabelOption option, wxArrayString labels = wxArrayString());

  void SetOptionMainValue(wxString format, DialPositionOption option);

  void SetOptionExtraValue(DASH_CAP cap_flag, wxString format, DialPositionOption option);

  void SetOptionWarningValue(DASH_CAP cap_flag);

private:
protected:
  int m_cx, m_cy, m_radius;
  int m_AngleStart;
  int m_AngleRange;
  bool m_gpsWD;
  double m_MainValue;
  DASH_CAP m_MainValueCap;
  double m_MainValueMin, m_MainValueMax;
  wxString m_MainValueFormat;
  wxString m_MainValueUnit;
  DialPositionOption m_MainValueOption;
  double m_ExtraValue;
  DASH_CAP m_ExtraValueCap;
  DASH_CAP m_WarningValueCap;
  wxString m_ExtraValueFormat;
  wxString m_ExtraValueUnit;
  DialPositionOption m_ExtraValueOption;
  DialMarkerOption m_MarkerOption;
  int m_MarkerOffset;
  double m_MarkerStep, m_LabelStep;
  DialLabelOption m_LabelOption;
  wxArrayString m_LabelArray;
  wxString m_warningIconFilename = wxEmptyString;
  wxBitmap m_warningImageBitmap = wxNullBitmap;


  virtual void Draw(wxGCDC* dc);
  virtual void DrawFrame(wxGCDC* dc);
  virtual void DrawMarkers(wxGCDC* dc);
  virtual void DrawLabels(wxGCDC* dc);
  virtual void DrawBackground(wxGCDC* dc);
  virtual void DrawData(wxGCDC* dc, double value, wxString unit, wxString format, DialPositionOption position);
  virtual void DrawForeground(wxGCDC* dc);
  virtual void DrawWarning(wxGCDC* dc);
};

#endif  // __Dial_H__
