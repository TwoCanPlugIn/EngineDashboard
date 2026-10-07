
//
// Author: Steven Adler
// 
// Modified the existing dashboard plugin to create an "Engine Dashboard"
// Parses NMEA 0183 RSA, RPM & XDR sentences, NMEA 2000 messages & SignalK Deltas.
// Displays Engine RPM, Oil Pressure, Water Temperature, Alternator Voltage, Engine Hours, Fluid Levels 
// and Battery Status in a dashboard
//
// Version History
// 1.0. 10-10-2019 - Oiginal Release
// 1.1. 23-11-2019 - Fixed original dashboard resize bug (linux with cairo libs only), battery status, gauge background 
// 1.9	01/12/2024 - Add icon display in tachometer for engine faults
// 2.0   01/10/2026 - Adopted current Dashboard model (for Android support) and refactored
// 
// Please send bug reports to twocanplugin@hotmail.com or to the opencpn forum
//

/******************************************************************************
 * $Id: dial.cpp, v1.0 2010/08/05 SethDart Exp $
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif  // precompiled headers

#include "dial.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <cmath>
#include "wx/tokenzr.h"

#ifdef __OCPN__ANDROID__
#include "qdebug.h"
#endif

double rad2deg(double angle) { return angle * 180.0 / M_PI; }
double deg2rad(double angle) { return angle / 180.0 * M_PI; }

DashboardInstrument_Dial::DashboardInstrument_Dial(wxWindow* parent, wxWindowID id, 
	wxString title, InstrumentProperties* Properties, DASH_CAP cap_flag, 
	int s_angle, int r_angle, int s_value, int e_value)
    : DashboardInstrument(parent, id, title, cap_flag, Properties) {
	m_AngleStart = s_angle;
	m_AngleRange = r_angle;
	m_MainValueMin = s_value;
	m_MainValueMax = e_value;
	m_MainValueCap = cap_flag;

	m_MainValue = s_value;
	m_ExtraValue = 0;
	m_MainValueFormat = "%d";
	m_MainValueUnit = "";
	m_MainValueOption = DIAL_POSITION_NONE;
	m_ExtraValueFormat = "%d";
	m_ExtraValueUnit = "";
	m_ExtraValueOption = DIAL_POSITION_NONE;
	m_MarkerOption = DIAL_MARKER_SIMPLE;
	m_MarkerStep = 1;
	m_LabelStep = 1;
	m_MarkerOffset = 1;
	m_LabelOption = DIAL_LABEL_HORIZONTAL;
}

DashboardInstrument_Dial::~DashboardInstrument_Dial(void) {
}

void DashboardInstrument_Dial::SetOptionMarker(double step, DialMarkerOption option, int offset) {
	m_MarkerStep = step;
	m_MarkerOption = option;
	m_MarkerOffset = offset;
}

void DashboardInstrument_Dial::SetOptionLabel(double step, DialLabelOption option, wxArrayString labels) {
	m_LabelStep = step;
	m_LabelOption = option;
	m_LabelArray = labels;
}
void DashboardInstrument_Dial::SetOptionMainValue(wxString format, DialPositionOption option) {
	m_MainValueFormat = format;
	m_MainValueOption = option;
}

void DashboardInstrument_Dial::SetOptionExtraValue(DASH_CAP cap_flag, wxString format,
	DialPositionOption option) {
	m_ExtraValueCap = cap_flag;
	m_cap_flag.set(cap_flag);
	m_ExtraValueFormat = format;
	m_ExtraValueOption = option;
}

void DashboardInstrument_Dial::SetOptionWarningValue(DASH_CAP cap_flag) {
	m_WarningValueCap = cap_flag;
	m_cap_flag.set(cap_flag);
}

wxSize DashboardInstrument_Dial::GetSize(int orient, wxSize hint) {
	InitTitleSize();
	InitTitleAndDataPosition(DefaultWidth);
	int w;
	if (orient == wxHORIZONTAL) {
		w = wxMax(hint.y, GetFullHeight(DefaultWidth));
	return wxSize(w - m_TitleHeight, w);
	} 
	else {
		w = wxMax(hint.x, DefaultWidth);
		return wxSize(w, GetFullHeight(w));
	}
}

wxFont DashboardInstrument_Dial::GetSmallFont() const {
	return m_Properties ? m_Properties->m_SmallFont.GetChosenFont() : g_pFontSmall->GetChosenFont();
}

wxColor DashboardInstrument_Dial::GetSmallFontColour() const {
	return m_Properties	? m_Properties->m_SmallFont.GetColour()	: g_pFontSmall->GetColour();
}

wxColor DashboardInstrument_Dial::GetLabelFontColour() const {
	return m_Properties	? m_Properties->m_LabelFont.GetColour()	: g_pFontLabel->GetColour();
}

wxFont DashboardInstrument_Dial::GetLabelFont() const {
	return m_Properties	? m_Properties->m_LabelFont.GetChosenFont()	: g_pFontLabel->GetChosenFont();
}

void DashboardInstrument_Dial::SetData(DASH_CAP cap_flag, double data, wxString unit) {
	if (cap_flag == m_MainValueCap) {
		m_MainValue = data;
		m_MainValueUnit = unit;
	} 
	else if (cap_flag == m_ExtraValueCap) {
		m_ExtraValue = data;
		m_ExtraValueUnit = unit;
	}
	else if (cap_flag == m_WarningValueCap) {
	// These are bit values set from a 2 byte value
	// BUG BUG Unsure if it is possible to have multiple alarms
	switch (static_cast<int>(data)) {
		case 0:
			m_warningIconFilename.Empty();
			break;
		case 1: // "Check Engine" 
			m_warningIconFilename = "engine.svg";
			break;
		case 2: // "Over Temperature" 
			m_warningIconFilename = "temperature.svg";
			break;
		case 4: // "Low Oil Pressure" 
			m_warningIconFilename = "oil.svg";
			break;
		case 8: // "Low Oil Level"
			m_warningIconFilename = "oil-level.svg";
			break;
		case 16: // "Low Fuel Pressure" 
			m_warningIconFilename = "fuel.svg";
			break;
		case 32: // "Low System Voltage" 
			m_warningIconFilename = "battery.svg";
			break;
		case 64: // "Low Coolant Level" 
			m_warningIconFilename = "coolant.svg";
			break;
		case 128: // "Water Flow" 
			m_warningIconFilename = "water.svg";
			break;
		case 256: // "Water In Fuel" 
			m_warningIconFilename = "contamination.svg";
			break;
		case 512: // "Charge Indicator"
			m_warningIconFilename = "alternator.svg";
			break;
		case 1024: // "Preheat Indicator" 
			m_warningIconFilename = "preheat.svg";
			break;
		case 2048: // "High Boost Pressure" 
			m_warningIconFilename = "turbo.svg";
			break;
		case 4096: // "Rev Limit Exceeded"
			m_warningIconFilename = "rev-limit.svg";
			break;
		case 8192: // "EGR System" 
			m_warningIconFilename = "exhaust.svg";
			break;
		case 16384: // "Throttle Position Sensor" 
			m_warningIconFilename = "throttle.svg";
			break;
		case 32768: // "Emergency Stop" 
			m_warningIconFilename = "stop.svg";
			break;
		default:
			m_warningIconFilename.Empty();
			break;
		}
	}
}

void DashboardInstrument_Dial::Draw(wxGCDC* bdc) {
	if (m_Properties) {
		wxBrush b1(GetColourSchemeBackgroundColour(m_Properties->m_DataBackgroundColour));
		if (g_highContrast) {
			b1.SetColour(*wxBLACK);
		}
		bdc->SetBackground(b1);
	} 
	else {
		wxColour c1;
		GetGlobalColor("DASHB", &c1);
		if (g_highContrast) {
			c1 = *wxBLACK;
		} 
		wxBrush b1(c1);
		bdc->SetBackground(b1);
	}

	bdc->Clear();

	wxSize size = GetClientSize();
	m_cx = size.x / 2;
	int availableHeight = GetDataBottom(size.y) - m_DataTop;
	InitTitleAndDataPosition(availableHeight);
	availableHeight -= 6;
	int width, height;
	wxFont f;
	if (m_Properties) {
		f = m_Properties->m_LabelFont.GetChosenFont();
	}
	else {
		f = g_pFontLabel->GetChosenFont();
	}
	bdc->GetTextExtent("000", &width, &height, 0, 0, &f);
	m_cy = m_DataTop + 2;
	m_cy += availableHeight / 2;
	m_radius = availableHeight / 2;

	DrawFrame(bdc);
	DrawLabels(bdc);
	DrawMarkers(bdc);
	DrawBackground(bdc);
	DrawForeground(bdc);
	DrawData(bdc, m_MainValue, m_MainValueUnit, m_MainValueFormat, m_MainValueOption);
	DrawData(bdc, m_ExtraValue, m_ExtraValueUnit, m_ExtraValueFormat, m_ExtraValueOption);
	if (!m_warningIconFilename.IsEmpty()) {
		DrawWarning(bdc);
	}
}


void DashboardInstrument_Dial::DrawFrame(wxGCDC* dc) {
	wxSize size = GetClientSize();
	wxColour cl;

	// BUG BUG DEDUP with GetColour
	if (m_Properties) {
		cl = GetColourSchemeBackgroundColour(m_Properties->m_TitleBackgroundColour);
	} 
	else {
		GetGlobalColor("DASHL", &cl);
	}
	if (g_highContrast) {
		 cl = *wxWHITE;
	}
	dc->SetTextForeground(cl);
	dc->SetBrush(*wxTRANSPARENT_BRUSH);

	int penwidth = 1 + size.x / 100;
	wxPen pen(cl, penwidth, wxPENSTYLE_SOLID);

	// Red warning is at lower limit of the range. Eg. Fuel Gauges
	if (m_MarkerOption == DIAL_MARKER_WARNING_LOW) {
		pen.SetWidth(penwidth * 2);
		GetGlobalColor(_T("DASHR"), &cl);
		if (g_highContrast) {
			cl = *wxRED;
		}
		pen.SetColour(cl);
		dc->SetPen(pen);
		double angle1 = deg2rad(168); // 135 + 1/8 of270
		double angle2 = deg2rad(135); // 90 + 45
		int radi = m_radius - 1 - penwidth;
		wxCoord x1 = m_cx + ((radi)*cos(angle1));
		wxCoord y1 = m_cy + ((radi)*sin(angle1));
		wxCoord x2 = m_cx + ((radi)*cos(angle2));
		wxCoord y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);

		// Some platforms have trouble with transparent pen.
		// so we simply draw arcs for the outer ring.
		GetGlobalColor(_T("DASHF"), &cl);
		if (g_highContrast) {
			cl = *wxWHITE;
		}
		pen.SetWidth(penwidth);
		pen.SetColour(cl);
		dc->SetPen(pen);
		angle1 = deg2rad(0);
		angle2 = deg2rad(180);
		radi = m_radius - 1;

		x1 = m_cx + ((radi)*cos(angle1));
		y1 = m_cy + ((radi)*sin(angle1));
		x2 = m_cx + ((radi)*cos(angle2));
		y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);
		dc->DrawArc(x2, y2, x1, y1, m_cx, m_cy);

		// BUG BUG Why not draw circle ??

	}

	// The red warning section is at the upper limit of the range. Eg.Black Water tank 
	else if (m_MarkerOption == DIAL_MARKER_WARNING_HIGH) {
		pen.SetWidth(penwidth * 2);
		GetGlobalColor(_T("DASHR"), &cl);
		if (g_highContrast) {
			cl = *wxRED;
		}
		pen.SetColour(cl);
		dc->SetPen(pen);
		double angle1 = deg2rad(45); // 45
		double angle2 = deg2rad(12); // 45 - 1/8 of 270
		int radi = m_radius - 1 - penwidth;
		wxCoord x1 = m_cx + ((radi)*cos(angle1));
		wxCoord y1 = m_cy + ((radi)*sin(angle1));
		wxCoord x2 = m_cx + ((radi)*cos(angle2));
		wxCoord y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);

		// Some platforms have trouble with transparent pen.
		// so we simply draw arcs for the outer ring.
		GetGlobalColor(_T("DASHF"), &cl);
		if (g_highContrast) {
			cl = *wxWHITE;
		}
		pen.SetWidth(penwidth);
		pen.SetColour(cl);
		dc->SetPen(pen);
		angle1 = deg2rad(0);
		angle2 = deg2rad(180);
		radi = m_radius - 1;

		x1 = m_cx + ((radi)*cos(angle1));
		y1 = m_cy + ((radi)*sin(angle1));
		x2 = m_cx + ((radi)*cos(angle2));
		y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);
		dc->DrawArc(x2, y2, x1, y1, m_cx, m_cy);

		// Again Why not Draw Circle

	}
	//  For battery status, the green OK section is in the middle of the range
	else if (m_MarkerOption == DIAL_MARKER_GREEN_MID) {
		pen.SetWidth(penwidth * 2);
		GetGlobalColor(_T("DASHG"), &cl);
		if (g_highContrast) {
			cl = *wxGREEN;
		}
		pen.SetColour(cl);
		dc->SetPen(pen);
		double angle1 = deg2rad(330); // 270 + 1/4 of 270
		double angle2 = deg2rad(270);
		int radi = m_radius - 1 - penwidth;
		wxCoord x1 = m_cx + ((radi)*cos(angle1));
		wxCoord y1 = m_cy + ((radi)*sin(angle1));
		wxCoord x2 = m_cx + ((radi)*cos(angle2));
		wxCoord y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);

		// Some platforms have trouble with transparent pen.
		// so we simply draw arcs for the outer ring.
		GetGlobalColor(_T("DASHF"), &cl);
		if (g_highContrast) {
			cl = *wxWHITE;
		}
		pen.SetWidth(penwidth);
		pen.SetColour(cl);
		dc->SetPen(pen);
		angle1 = deg2rad(0);
		angle2 = deg2rad(180);
		radi = m_radius - 1;

		x1 = m_cx + ((radi)*cos(angle1));
		y1 = m_cy + ((radi)*sin(angle1));
		x2 = m_cx + ((radi)*cos(angle2));
		y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);
		dc->DrawArc(x2, y2, x1, y1, m_cx, m_cy);

		// Again Draw Circle

	}

	// Used to indicate left/right, port starboard
	if (m_MarkerOption == DIAL_MARKER_REDGREENBAR) {
		pen.SetWidth(penwidth * 2);
		GetGlobalColor("DASHR", &cl);
		if (g_highContrast) {
			cl = *wxRED;
		}
		pen.SetColour(cl);
		dc->SetPen(pen);
		double angle1 = deg2rad(270);  // 305-ANGLE_OFFSET
		double angle2 = deg2rad(90);   // 55-ANGLE_OFFSET
		int radi = m_radius - 1 - penwidth;
		wxCoord x1 = m_cx + ((radi)*cos(angle1));
		wxCoord y1 = m_cy + ((radi)*sin(angle1));
		wxCoord x2 = m_cx + ((radi)*cos(angle2));
		wxCoord y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);
		GetGlobalColor("DASHG", &cl);
		if (g_highContrast) {
			cl = *wxGREEN;
		}
		pen.SetColour(cl);
		dc->SetPen(pen);
		angle1 = deg2rad(89);   // 305-ANGLE_OFFSET
		angle2 = deg2rad(271);  // 55-ANGLE_OFFSET
		x1 = m_cx + ((radi)*cos(angle1));
		y1 = m_cy + ((radi)*sin(angle1));
		x2 = m_cx + ((radi)*cos(angle2));
		y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);

		// Some platforms have trouble with transparent pen.
		// so we simply draw arcs for the outer ring.
		GetGlobalColor("DASHF", &cl);
		if (g_highContrast) {
			cl = *wxWHITE;
		}
		pen.SetWidth(penwidth);
		pen.SetColour(cl);
		dc->SetPen(pen);
		angle1 = deg2rad(0);
		angle2 = deg2rad(180);
		radi = m_radius - 1;

		x1 = m_cx + ((radi)*cos(angle1));
		y1 = m_cy + ((radi)*sin(angle1));
		x2 = m_cx + ((radi)*cos(angle2));
		y2 = m_cy + ((radi)*sin(angle2));
		dc->DrawArc(x1, y1, x2, y2, m_cx, m_cy);
		dc->DrawArc(x2, y2, x1, y1, m_cx, m_cy);

		// Again Why not draw circle

	} 
	else {
		GetGlobalColor("DASHF", &cl);
		if (g_highContrast) {
			cl = *wxWHITE;
		}
		pen.SetColour(cl);
		dc->SetPen(pen);
		dc->DrawCircle(m_cx, m_cy, m_radius);
	}
}

void DashboardInstrument_Dial::DrawMarkers(wxGCDC* dc) {
	if (m_MarkerOption == DIAL_MARKER_NONE) {
		return;
	}

	wxColour cl;
	GetGlobalColor("DASHF", &cl);
	if (g_highContrast) {
		cl = *wxWHITE;
	}
	int penwidth = GetClientSize().x / 100;
	wxPen pen(cl, penwidth, wxPENSTYLE_SOLID);
	dc->SetPen(pen);

	int diff_angle = m_AngleStart + m_AngleRange - ANGLE_OFFSET;
	// angle between markers
	double abm = m_AngleRange * m_MarkerStep / (m_MainValueMax - m_MainValueMin);
	// don't draw last value, it's already done as first
	if (m_AngleRange == 360) {
		diff_angle -= abm;
	}

	int offset = 0;
	for (double angle = m_AngleStart - ANGLE_OFFSET; angle <= diff_angle;
		angle += abm) {
	if (m_MarkerOption == DIAL_MARKER_REDGREEN) {
		int a = int(angle + ANGLE_OFFSET) % 360;
		if (a > 180) {
			GetGlobalColor("DASHR", &cl);
			if (g_highContrast) {
				cl = *wxRED;
			}
		}
		else if ((a > 0) && (a < 180)) {
			GetGlobalColor("DASHG", &cl);
			if (g_highContrast) {
				cl = *wxGREEN;
			}
		}
		else {
			GetGlobalColor("DASHF", &cl);
			if (g_highContrast) {
				cl = *wxWHITE;
			}
		}

		pen.SetColour(cl);
		dc->SetPen(pen);
	}

	double size = 0.92;
	if (offset % m_MarkerOffset) {
		size = 0.96;
	}
	offset++;

	dc->DrawLine(m_cx + ((m_radius - 1) * size * cos(deg2rad(angle))),
					m_cy + ((m_radius - 1) * size * sin(deg2rad(angle))),
					m_cx + ((m_radius - 1) * cos(deg2rad(angle))),
					m_cy + ((m_radius - 1) * sin(deg2rad(angle))));
	}
	// We must reset pen color so following drawings are fine
	if (m_MarkerOption == DIAL_MARKER_REDGREEN) {
		GetGlobalColor("DASHF", &cl);
		if (g_highContrast) {
			cl = *wxWHITE;
		}
		pen.SetStyle(wxPENSTYLE_SOLID);
		pen.SetColour(cl);
		dc->SetPen(pen);
	}
}
/*
void DashboardInstrument_Dial::DrawLabels(wxGCDC* dc) {
	if (m_LabelOption == DIAL_LABEL_NONE)
		return;

	const wxFont font = GetSmallFont();
	dc->SetFont(font);
	dc->SetTextForeground(GetSmallFontColour());

	const double angleStep = ...;
	const int valueStep = m_LabelStep;

	for (...) {
		const wxString label = GetLabel(value, offset);

		...

			if (m_LabelOption == DIAL_LABEL_ROTATED)
				DrawRotatedLabel(...);
			else
				DrawHorizontalLabel(...);
	}
}

*/

void DashboardInstrument_Dial::DrawLabels(wxGCDC* dc) {
	if (m_LabelOption == DIAL_LABEL_NONE) {
		return;
	}

	wxPoint TextPoint;
	wxPen pen;
	wxColor cl;
	GetGlobalColor("DASHF", &cl);

	if (m_Properties) {
		dc->SetFont(m_Properties->m_SmallFont.GetChosenFont());
		dc->SetTextForeground(GetColourSchemeFont(m_Properties->m_SmallFont.GetColour()));
	} 
	else {
		dc->SetFont(g_pFontSmall->GetChosenFont());
		dc->SetTextForeground(GetColourSchemeFont(g_pFontSmall->GetColour()));
	}

	if (g_highContrast) {
		cl = *wxWHITE;
		dc->SetTextForeground(*wxWHITE);
	}

	int diff_angle = m_AngleStart + m_AngleRange - ANGLE_OFFSET;
	// abm: angle between markers
	double abm = m_AngleRange * m_LabelStep / (m_MainValueMax - m_MainValueMin);
	// don't draw last value, it's already done as first
	if (m_AngleRange == 360) {
		diff_angle -= abm;
	}

	int offset = 0;
	int value = m_MainValueMin;
	int width, height;
	wxString label;
	wxFont f;
	for (double angle = m_AngleStart - ANGLE_OFFSET; angle <= diff_angle; angle += abm) {
		// For fuel tanks, use different labels instead of the actual values
		if (m_LabelOption == DIAL_LABEL_FRACTIONS) {
			if (value == 0) {
				label = "0";
			}
			if (value == 25) {
				label = "1/4";
			}
			if (value == 50) {
				label = "1/2";
			}
			if (value == 75) {
				label = "3/4";
			}
			if (value == 100) {
				label = "4/4";
				}
		}
		else {
			label = (m_LabelArray.GetCount() ? m_LabelArray.Item(offset) : wxString::Format("%d", value));
		}

		// BUG BUG DEDUP With GetFont
		if (m_Properties) {
			f = m_Properties->m_SmallFont.GetChosenFont();
		}
		else {
			f = g_pFontSmall->GetChosenFont();
		}

		dc->GetTextExtent(label, &width, &height, 0, 0, &f);
		double halfW = width / 2;

		if (m_LabelOption == DIAL_LABEL_HORIZONTAL || m_LabelOption == DIAL_LABEL_FRACTIONS) {
			double halfH = height / 2;
			// double delta = sqrt(width*width+height*height);
			double delta = sqrt(halfW * halfW + halfH * halfH);
			TextPoint.x =  m_cx + ((m_radius * 0.90) - delta) * cos(deg2rad(angle)) - halfW;
			TextPoint.y =  m_cy + ((m_radius * 0.90) - delta) * sin(deg2rad(angle)) - halfH;
			dc->DrawText(label, TextPoint);
		} 

		else if (m_LabelOption == DIAL_LABEL_ROTATED) {
			// The coordinates of dc->DrawRotatedText refer to the top-left corner
			// of the rectangle bounding the string. So we must calculate the
			// correct coordinates depending on the angle.
			// Move left from the Marker so that the position is in the Middle of Text
			long double tmpangle = angle - rad2deg(asin(halfW / (0.90 * m_radius)));
			TextPoint.x = m_cx + m_radius * 0.90 * cos(deg2rad(tmpangle));
			TextPoint.y = m_cy + m_radius * 0.90 * sin(deg2rad(tmpangle));
			dc->DrawRotatedText(label, TextPoint, -90 - angle);
		}
		offset++;
		value += m_LabelStep;
	}
}

void DashboardInstrument_Dial::DrawBackground(wxGCDC* dc) {
  // Nothing to do here right now, will be overwritten
  // by child classes if required
}

void DashboardInstrument_Dial::DrawData(wxGCDC* dc, double value, wxString unit, wxString format, 
	DialPositionOption position) {

	if (position == DIAL_POSITION_NONE) {
		return;
	}

	if (m_Properties) {
		dc->SetFont(m_Properties->m_LabelFont.GetChosenFont());
		dc->SetTextForeground(GetColourSchemeFont(m_Properties->m_LabelFont.GetColour()));
	} 
	else {
		dc->SetFont(g_pFontLabel->GetChosenFont());
		dc->SetTextForeground(GetColourSchemeFont(g_pFontLabel->GetColour()));
	}

	wxColour cl;
	if (g_highContrast) {
		dc->SetTextForeground(*wxWHITE);
	}
	wxSize size = GetClientSize();

	wxString text;
	if (!std::isnan(value)) {
		if (unit == "\u00B0") {
			text = wxString::Format(format, value) + DEGREE_SIGN;
		}
		else if (unit == "\u00B0L")  {
			text = wxString::Format(format, value) + DEGREE_SIGN;
		}
		else if (unit == "\u00B0R")  {
			text = wxString::Format(format, value) + DEGREE_SIGN;
		}
		else if (unit == "\u00B0T") {
			text = wxString::Format(format, value) + DEGREE_SIGN + "T";
		}
		else if (unit == "\u00B0M") {
			text = wxString::Format(format, value) + DEGREE_SIGN + "M";
		}
		else if (unit == "N") {
			text = wxString::Format(format, value) + " Kts";
		}
		else {
			text = wxString::Format(format, value) + " " + unit;
		}
	} 
	// Data not available
	else {
		text = "---";
	}

	int width, height;
	wxFont f;

	// BUG BUG DEDUP With GetFont
	if (m_Properties) {
		f = m_Properties->m_LabelFont.GetChosenFont();
	}
	else {
		f = g_pFontLabel->GetChosenFont();
	}

	dc->GetMultiLineTextExtent(text, &width, &height, NULL, &f);

	wxRect TextPoint;
	TextPoint.width = width;
	TextPoint.height = height;
	switch (position) {
		case DIAL_POSITION_NONE:
			// This case was already handled before, it's here just
			// to avoid compiler warning.
			return;
		case DIAL_POSITION_INSIDE: {
			TextPoint.x = m_cx - (width / 2) - 1;
			TextPoint.y = ((size.y - m_InstrumentSpacing) * .75) - height;
			if ((g_TitleAlignment & wxALIGN_BOTTOM) != 0) {
				TextPoint.y -= m_TitleHeight;
			}

			// BUG BUG DEDUP with GetColour
			if (m_Properties) {
				cl = GetColourSchemeBackgroundColour(m_Properties->m_TitleBackgroundColour);
			}
			else {
				GetGlobalColor("DASHL", &cl);
			}

			if (g_highContrast) {
				cl = *wxWHITE;
			}

			int penwidth = size.x / 100;
			wxPen* pen = wxThePenList->FindOrCreatePen(cl, penwidth, wxPENSTYLE_SOLID);
			dc->SetPen(*pen);

			if (m_Properties) {
				cl = GetColourSchemeBackgroundColour(m_Properties->m_DataBackgroundColour);
			}
			else {
				GetGlobalColor("DASHB", &cl);
			}

			if (g_highContrast) {
				cl = *wxBLACK;
			}

			dc->SetBrush(cl);
			// There might be a background drawn below/ so we must clear it first.
			dc->DrawRoundedRectangle(TextPoint.x - 2, TextPoint.y - 2, width + 4, height + 4, 3);
			break;
		}
		case DIAL_POSITION_TOPLEFT:
			TextPoint.x = 0;
			TextPoint.y = m_DataTop;
			break;
		case DIAL_POSITION_TOPRIGHT:
			TextPoint.x = size.x - width - 1;
			TextPoint.y = m_DataTop;
			break;
		case DIAL_POSITION_BOTTOMLEFT:
			TextPoint.x = 0;
			TextPoint.y = GetDataBottom(size.y) - height;
			break;
		case DIAL_POSITION_BOTTOMRIGHT:
			TextPoint.x = size.x - width - 1;
			TextPoint.y = GetDataBottom(size.y) - height;
			break;
		case DIAL_POSITION_BOTTOMMIDDLE:
			if (!std::isnan(value)) {
				TextPoint.x = m_cx - (width / 2) - 1;
				TextPoint.y = GetDataBottom(size.y) - height;
				// There might be a background drawn below so we must clear it first.
				dc->DrawRoundedRectangle(TextPoint.x - 2, TextPoint.y - 2, width + 4, height + 4, 3);
			}
			break;
	  }

	// wxColour c2;
	// GetGlobalColor(_T("DASHB"), &c2);
	wxColour c3;
	GetGlobalColor("DASHF", &c3);

	wxStringTokenizer tkz(text, "\n");
	wxString token;

	token = tkz.GetNextToken();
	while (token.Length()) {
		if (m_Properties) {
			f = m_Properties->m_LabelFont.GetChosenFont();
		}
		else {
			f = g_pFontLabel->GetChosenFont();
		}
		dc->GetTextExtent(token, &width, &height, NULL, NULL, &f);
		dc->DrawText(token, TextPoint.x, TextPoint.y);
		TextPoint.y += height;
		token = tkz.GetNextToken();
	}
}

void DashboardInstrument_Dial::DrawForeground(wxGCDC* dc) {
	// The default foreground is the arrow used in most dials
	wxColour cl;
	GetGlobalColor("DASH2", &cl);
	if (g_highContrast) {
		cl = *wxLIGHT_GREY;
	}
	wxPen pen1;
	pen1.SetStyle(wxPENSTYLE_SOLID);
	pen1.SetColour(cl);
	pen1.SetWidth(2);
	dc->SetPen(pen1);
	GetGlobalColor("DASH1", &cl);
	if (g_highContrast) {
		cl = *wxYELLOW;
	}
	wxBrush brush1;
	brush1.SetStyle(wxBRUSHSTYLE_SOLID);
	brush1.SetColour(cl);
	dc->SetBrush(brush1);
	dc->DrawCircle(m_cx, m_cy, m_radius / 8);

	dc->SetPen(*wxTRANSPARENT_PEN);
	if (m_Properties) {
		cl = GetColourSchemeFont(m_Properties->m_Arrow_First_Colour);
	}
	else {
		GetGlobalColor("DASHN", &cl);
	}

	if (g_highContrast) {
		cl = *wxYELLOW;
	}
	
	wxBrush brush;
	brush.SetStyle(wxBRUSHSTYLE_SOLID);
	brush.SetColour(cl);
	dc->SetBrush(brush);

	// This is fix for a +/-180 degrees round instrument, when m_MainValue is supplied as
	// <0..180> <L | R> for example TWA & AWA
	double data;
	if (m_MainValueUnit == "\u00B0L") {
		data = 360 - m_MainValue;
	}
	else {
		data = m_MainValue;
	}

	// The arrow should stay inside fixed limits
	double val;
	if (data < m_MainValueMin) {
		val = m_MainValueMin;
	}
	else if (data > m_MainValueMax) {
		val = m_MainValueMax;
	}
	else {
		val = data;
	}

	double value = deg2rad((val - m_MainValueMin) * m_AngleRange /
		(m_MainValueMax - m_MainValueMin)) + deg2rad(m_AngleStart - ANGLE_OFFSET);

	wxPoint points[4];
	points[0].x = m_cx + (m_radius * 0.95 * cos(value - .010));
	points[0].y = m_cy + (m_radius * 0.95 * sin(value - .010));
	points[1].x = m_cx + (m_radius * 0.95 * cos(value + .015));
	points[1].y = m_cy + (m_radius * 0.95 * sin(value + .015));
	points[2].x = m_cx + (m_radius * 0.22 * cos(value + 2.8));
	points[2].y = m_cy + (m_radius * 0.22 * sin(value + 2.8));
	points[3].x = m_cx + (m_radius * 0.22 * cos(value - 2.8));
	points[3].y = m_cy + (m_radius * 0.22 * sin(value - 2.8));
	dc->DrawPolygon(4, points, 0, 0);
}

void DashboardInstrument_Dial::DrawWarning(wxGCDC* dc) {

	wxSize size = GetClientSize();
	int dimension = size.x > 300 ? 96 : size.x > 200 ? 48 : 32;

	wxBitmap warningImage =	GetBitmapFromSVGFile(g_pluginFolder + m_warningIconFilename, 
		dimension, dimension);

	if (warningImage.IsOk()) {
		dc->DrawBitmap(warningImage, (size.x / 2) - (dimension / 2),
			(size.y / 3) - (dimension / 3),	true);
	}
}
