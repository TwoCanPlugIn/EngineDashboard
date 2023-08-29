//
// Author: Steven Adler
// 
// Modified the existing dashboard plugin to create an "Engine Dashboard"
// Parses NMEA 0183 RSA, RPM & XDR sentences and displays Engine RPM, Oil Pressure, Water Temperature, 
// Alternator Voltage, Engine Hours andFluid Levels in a dashboard
//
// Version 1.0
// 10-10-2019
// 1.1 30-08-2023 Added simple Gauge display using wxGauge control
// 
// Please send bug reports to twocanplugin@hotmail.com or to the opencpn forum
//
/******************************************************************************
 * $Id: instrument.cpp, v1.0 2010/08/30 SethDart Exp $
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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers
#include <cmath>

#include "instrument.h"
//#include "wx28compat.h"

//----------------------------------------------------------------
//
//    Generic DashboardInstrument Implementation
//
//----------------------------------------------------------------

DashboardInstrument::DashboardInstrument(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap_flag)
      :wxControl(pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {
      m_title = title;
      m_cap_flag.set(cap_flag);

      SetBackgroundStyle(wxBG_STYLE_CUSTOM);
      SetDrawSoloInPane(false);
      wxClientDC dc(this);
      int width;
      dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);

      Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(DashboardInstrument::OnEraseBackground));
      Connect(wxEVT_PAINT, wxPaintEventHandler(DashboardInstrument::OnPaint));
      
      //  On OSX, there is an orphan mouse event that comes from the automatic
      //  exEVT_CONTEXT_MENU synthesis on the main wxWindow mouse handler.
      //  The event goes to an instrument window (here) that may have been deleted by the
      //  preferences dialog.  Result is NULL deref.
      //  Solution:  Handle right-click here, and DO NOT skip()
      //  Strangely, this does not work for GTK...
      //  See: http://trac.wxwidgets.org/ticket/15417
      
#ifdef __WXOSX__
      Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(DashboardInstrument::MouseEvent), NULL, this);
#endif      
}

void DashboardInstrument::MouseEvent(wxMouseEvent &event) {
    if (event.GetEventType() == wxEVT_RIGHT_DOWN) {
		wxContextMenuEvent evtCtx(wxEVT_CONTEXT_MENU, this->GetId(), this->ClientToScreen(event.GetPosition()));
        evtCtx.SetEventObject(this);
        GetParent()->GetEventHandler()->AddPendingEvent(evtCtx);
    }
}

CapType DashboardInstrument::GetCapacity() {
	return m_cap_flag;
}

void DashboardInstrument::SetDrawSoloInPane(bool value) {
    m_drawSoloInPane = value;
}

void DashboardInstrument::OnEraseBackground(wxEraseEvent& WXUNUSED(evt)) {
        // intentionally empty
}


void DashboardInstrument::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxAutoBufferedPaintDC pdc(this);
    if (!pdc.IsOk()) {
        wxLogMessage(_T("DashboardInstrument::OnPaint() fatal: wxAutoBufferedPaintDC.IsOk() false."));
        return;
    }

    wxSize size = GetClientSize();
    if (size.x == 0 || size.y == 0) {
        wxLogMessage(_T("DashboardInstrument::OnPaint() fatal: Zero size DC."));
        return;
    }

#if wxUSE_GRAPHICS_CONTEXT
    wxGCDC dc(pdc);
#else
    wxDC &dc(pdc);
#endif

    wxColour cl;
    GetGlobalColor(_T("DASHB"), &cl);
    dc.SetBackground(cl);
#ifdef __WXGTK__
    dc.SetBrush(cl);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, size.x, size.y);
#endif
    dc.Clear();

    Draw(&dc);

    if (!m_drawSoloInPane) {

    //  Windows GCDC does a terrible job of rendering small texts
    //  Workaround by using plain old DC for title box if text size is too small
#ifdef __WXMSW__
        if (g_pFontTitle->GetPointSize() > 12)
#endif
        {
            wxPen pen;
            pen.SetStyle(wxPENSTYLE_SOLID);
            GetGlobalColor(_T("DASHL"), &cl);
            pen.SetColour(cl);
            dc.SetPen(pen);
            dc.SetBrush(cl);
            dc.DrawRoundedRectangle(0, 0, size.x, m_TitleHeight, 3);

            dc.SetFont(*g_pFontTitle);
            GetGlobalColor(_T("DASHF"), &cl);
            dc.SetTextForeground(cl);
            dc.DrawText(m_title, 5, 0);
        }

#ifdef __WXMSW__
        if (g_pFontTitle->GetPointSize() <= 12) {
            wxColour cl;
            GetGlobalColor(_T("DASHB"), &cl);
            pdc.SetBrush(cl);
            pdc.DrawRectangle(0, 0, size.x, m_TitleHeight);

            wxPen pen;
            pen.SetStyle(wxPENSTYLE_SOLID);
            GetGlobalColor(_T("DASHL"), &cl);
            pen.SetColour(cl);
            pdc.SetPen(pen);
            pdc.SetBrush(cl);
            pdc.DrawRoundedRectangle(0, 0, size.x, m_TitleHeight, 3);

            pdc.SetFont(*g_pFontTitle);
            GetGlobalColor(_T("DASHF"), &cl);
            pdc.SetTextForeground(cl);
            pdc.DrawText(m_title, 5, 0);
        }
#endif
    }
}

//----------------------------------------------------------------
//
//    DashboardInstrument_Simple Implementation
//
//----------------------------------------------------------------

DashboardInstrument_Single::DashboardInstrument_Single(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap_flag, wxString format)
      :DashboardInstrument(pparent, id, title, cap_flag) {
      m_format = format;
      m_data = _T("---");
      m_DataHeight = 0;
}

wxSize DashboardInstrument_Single::GetSize(int orient, wxSize hint) {
      wxClientDC dc(this);
      int w;
      dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
      dc.GetTextExtent(_T("000"), &w, &m_DataHeight, 0, 0, g_pFontData);

      if (orient == wxHORIZONTAL) {
          return wxSize(DefaultWidth, wxMax(hint.y, m_TitleHeight+m_DataHeight));
      } else {
          return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight+m_DataHeight);
      }
}

void DashboardInstrument_Single::Draw(wxGCDC* dc) {
      wxColour cl;
#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_DataHeight, -1);
      wxMemoryDC tdc(tbm);
      wxColour c2;
      GetGlobalColor(_T("DASHB"), &c2);
      tdc.SetBackground(c2);
      tdc.Clear();

      tdc.SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      tdc.SetTextForeground(cl);

      tdc.DrawText(m_data, 10, 0);

      tdc.SelectObject(wxNullBitmap);

      dc->DrawBitmap(tbm, 0, m_TitleHeight, false);
#else
      dc->SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      dc->SetTextForeground(cl);

      dc->DrawText(m_data, 10, m_TitleHeight);

#endif

}

void DashboardInstrument_Single::SetData(DASH_CAP st, double data, wxString unit) {
      if (m_cap_flag.test(st)) {
            if (!std::isnan(data) && (data < 9999)) {
                if (unit == _T("C"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_T("C");
                else if (unit == _T("\u00B0"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN;
                else if (unit == _T("\u00B0T"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_(" true");
                else if (unit == _T("\u00B0M"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_(" mag");
                else if (unit == _T("\u00B0L"))
                  m_data = _T(">")+ wxString::Format(m_format, data)+DEGREE_SIGN;
                else if (unit == _T("\u00B0R"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_T("<");
                else if (unit == _T("N")) //Knots
                  m_data = wxString::Format(m_format, data)+_T(" Kts");
/* maybe in the future ...
                else if (unit == _T("M")) // m/s
                  m_data = wxString::Format(m_format, data)+_T(" m/s");
                else if (unit == _T("K")) // km/h
                  m_data = wxString::Format(m_format, data)+_T(" km/h");
 ... to be completed
 */
                else
                  m_data = wxString::Format(m_format, data)+_T(" ")+unit;
            }
            else
                m_data = _T("---");
      }
}

// Dashboard Gauge
// Gauge Display, Note it's height is the same as the title, and poitioned below the title
DashboardInstrument_Gauge::DashboardInstrument_Gauge(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap_flag)
	:DashboardInstrument(pparent, id, title, cap_flag)
{
	int w;
	wxClientDC dc(this);
	dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);

	gauge = new wxGauge(this, wxID_ANY, 100, wxPoint(0, m_TitleHeight), 
		wxSize(DefaultWidth, m_TitleHeight));
}

DashboardInstrument_Gauge::~DashboardInstrument_Gauge(void) {
	delete gauge;
}

wxSize DashboardInstrument_Gauge::GetSize(int orient, wxSize hint)
{
	wxClientDC dc(this);
	int w;
	dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
	
	if (orient == wxHORIZONTAL) {
		w = wxMax(hint.y, DefaultWidth);
	}
	else {
		w = wxMax(hint.x, DefaultWidth);
	}
      return wxSize(w, 2 * m_TitleHeight);
}

void DashboardInstrument_Gauge::Draw(wxGCDC* dc) {
	wxSize size = dc->GetSize();
	gauge->SetSize(size.GetWidth(), m_TitleHeight);
}

void DashboardInstrument_Gauge::SetData(DASH_CAP st, double data, wxString unit) {
	if (m_cap_flag.test(st)) {
		if (!std::isnan(data) && (data < 100)) { // Shouldn't have values greater than 100 %
			gauge->SetValue((int)data);
		}
	}
}



// Simple Gauge using Unicode Block characters
DashboardInstrument_Block::DashboardInstrument_Block(wxWindow *pparent, wxWindowID id, wxString title, DASH_CAP cap_flag, wxString format)
	:DashboardInstrument(pparent, id, title, cap_flag) {
	m_format = format;
	m_data = _T("---");
	m_DataHeight = 0;

}

wxSize DashboardInstrument_Block::GetSize(int orient, wxSize hint) {
	wxClientDC dc(this);
	int w;
	dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
	dc.GetTextExtent(_T("000000000000000"), &w, &m_DataHeight, 0, 0, g_pFontData);

	if (orient == wxHORIZONTAL) {
		return wxSize(wxMax(w, DefaultWidth), wxMax(hint.y, m_TitleHeight + m_DataHeight));
	}
	else {
		return wxSize(wxMax(w, hint.x), m_TitleHeight + m_DataHeight);
	}
}

void DashboardInstrument_Block::Draw(wxGCDC* dc) {
	wxColour cl;
#ifdef __WXMSW__
	wxBitmap tbm(dc->GetSize().x, m_DataHeight, -1);
	wxMemoryDC tdc(tbm);
	wxColour c2;
	GetGlobalColor(_T("DASHB"), &c2);
	tdc.SetBackground(c2);
	tdc.Clear();

	tdc.SetFont(*g_pFontData);
	if (m_Value > 20) {
		GetGlobalColor(_T("DASHF"), &cl);
	}
	else {
		GetGlobalColor(_T("DASHR"), &cl);
	}
	tdc.SetTextForeground(cl);

	tdc.DrawText(m_data, 0, 0);

	tdc.SelectObject(wxNullBitmap);

	dc->DrawBitmap(tbm, 0, m_TitleHeight, false);
#else
	dc->SetFont(*g_pFontData);

	if (m_Value > 20) {
		GetGlobalColor(_T("DASHF"), &cl);
	}
	else {
		GetGlobalColor(_T("DASHR"), &cl);
	}
	dc->SetTextForeground(cl);

	dc->DrawText(m_data, 10, m_TitleHeight);

#endif

}

void DashboardInstrument_Block::SetData(DASH_CAP st, double data, wxString unit) {
	if (m_cap_flag.test(st)) {
		if (!std::isnan(data) && (data <= 100)) {
			if (unit == _T("Level")) {
				m_Value = (int)data; // class member used to determine foreground colour
				m_data.Clear();
				for (int i = 0; i < (int)(data / 10); i++) {
					m_data.Append(wxString::FromUTF8(u8"\u2588"));
				}
				if ((int)data < 90) {
					m_data.Append(wxString::Format(" (%d%%)", (int)data));
				}
			}
		}
		else {
			m_data = _T("---");
		}
	}
}



/**************************************************************************/
/*          Some assorted utilities                                       */
/**************************************************************************/

wxString toSDMM (int NEflag, double a)
{
      short neg = 0;
      int d;
      long m;

      if (a < 0.0)
      {
            a = -a;
            neg = 1;
      }
      d = (int) a;
      m = (long) ((a - (double) d) * 60000.0);

      if (neg)
            d = -d;

      wxString s;

      if (!NEflag)
            s.Printf (_T ("%d %02ld.%03ld'"), d, m / 1000, m % 1000);
      else
      {
            if (NEflag == 1)
            {
                  char c = 'N';

                  if (neg)
                  {
                        d = -d;
                        c = 'S';
                  }

                  s.Printf (_T ("%03d %02ld.%03ld %c"), d, m / 1000, (m % 1000), c);
            }
            else if (NEflag == 2)
            {
                  char c = 'E';

                  if (neg)
                  {
                        d = -d;
                        c = 'W';
                  }
                  s.Printf (_T ("%03d %02ld.%03ld %c"), d, m / 1000, (m % 1000), c);
            }
      }
      return s;
}
