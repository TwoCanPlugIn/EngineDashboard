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
// Please send bug reports to twocanplugin@hotmail.com orr to the opencpn forum
//
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

#ifndef _DIAL_H_
#define _DIAL_H_

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

// 0 degrees are at 12 o´clock
#define ANGLE_OFFSET 90

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
	DIAL_POSITION_BOTTOMRIGHT
} DialPositionOption;

extern double rad2deg(double angle);
extern double deg2rad(double angle);

extern wxString iconFolder;

//+------------------------------------------------------------------------------
//|
//| CLASS:
//|    DashboardInstrument_Dial
//|
//| DESCRIPTION:
//|    This class creates a speedometer style control
//|
//+------------------------------------------------------------------------------
class DashboardInstrument_Dial: public DashboardInstrument {
public:
	DashboardInstrument_Dial(wxWindow *parent, wxWindowID id, wxString title, DASH_CAP cap_flag, int s_angle, int r_angle, int s_value, int e_value);
	~DashboardInstrument_Dial(void);

	wxSize GetSize(int orient, wxSize hint);
	void SetData(DASH_CAP cap, double units, wxString format);
	void SetOptionMarker(double step, DialMarkerOption option, int offset);
	void SetOptionLabel(double step, DialLabelOption option, wxArrayString labels = wxArrayString()); // { m_LabelStep = step; m_LabelOption = option; m_LabelArray = labels; }
	void SetOptionMainValue(wxString format, DialPositionOption option);
	void SetOptionExtraValue(DASH_CAP cap, wxString format, DialPositionOption option);
	void SetOptionWarningValue(DASH_CAP cap);
private:

protected:
	int m_cx, m_cy, m_radius;
	int m_AngleStart, m_AngleRange;
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
	// Engine Warning images
	wxString imageFilename;
	wxBitmap imageBitmap;
	
	virtual void Draw(wxGCDC* dc);
	virtual void DrawFrame(wxGCDC* dc);
	virtual void DrawMarkers(wxGCDC* dc);
	virtual void DrawLabels(wxGCDC* dc);
	virtual void DrawBackground(wxGCDC* dc);
	virtual void DrawData(wxGCDC* dc, double value, wxString unit, wxString format, DialPositionOption position);
	virtual void DrawForeground(wxGCDC* dc);
	virtual void DrawWarning(wxGCDC* dc);
};

#endif // _DIAL_H_

