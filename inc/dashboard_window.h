/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - DashboardWindow
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _DASHBOARD_WINDOW_H_
#define _DASHBOARD_WINDOW_H_

#include "wx/wx.h"
#include <wx/aui/aui.h>
#include <ocpn_plugin.h>
#include "dashboard_window_container.h"
#include "instrument.h"
#include <nmea0183.h>

#ifdef __OCPN__ANDROID__
#include <wx/qt/private/wxQtGesture.h>
#endif

class Dashboard;

enum {
    ID_DASHBOARD_WINDOW
};

enum {
    ID_DASH_PREFS      = 999,
    ID_DASH_VERTICAL,
    ID_DASH_HORIZONTAL,
    ID_DASH_RESIZE,
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

enum {
	VOLUME_LITRE,
	VOLUME_GALLON
};


//  DashboardWindow
//  The actual panel that hosts a column/row of instruments.

class DashboardWindow : public wxWindow {
public:
    DashboardWindow(wxWindow *pparent, wxWindowID id,
                    wxAuiManager *auimgr, Dashboard *plugin,
                    int orient, DashboardWindowContainer *mycont);
    ~DashboardWindow();

    void SetColorScheme(PI_ColorScheme cs);
    void SetSizerOrientation(int orient);
    int  GetSizerOrientation();
    void OnSize(wxSizeEvent &evt);
    void OnContextMenu(wxContextMenuEvent &evt);
    void OnContextMenuSelect(wxCommandEvent &evt);
    void OnMouseEvent(wxMouseEvent &event);

#ifdef __OCPN__ANDROID__
    void OnEvtPinchGesture(wxQT_PinchGestureEvent &event);
    void OnEvtPanGesture(wxQT_PanGestureEvent &event);
#endif

    bool IsInstrumentListEqual(const wxArrayInt &list);
    void SetInstrumentList(wxArrayInt list,
                           wxArrayOfInstrumentProperties *InstrumentPropertyList);
    void SendSentenceToAllInstruments(DASH_CAP cap_flag, double value, wxString unit);

    // Default FloatingPosition (100,100) included.
    void ChangePaneOrientation(int orient, bool updateAUImgr,
                               int fpx = 100, int fpy = 100);

    DashboardWindowContainer *m_Container;

    bool    m_binPinch;
    bool    m_binPan;
    wxPoint m_resizeStartPoint;
    wxSize  m_resizeStartSize;
    bool    m_binResize;
    bool    m_binResize2;

private:
    wxAuiManager        *m_pauimgr;
    Dashboard        *m_plugin;
    wxBoxSizer          *itemBoxSizer;
    wxArrayOfInstrument  m_ArrayOfInstrument;
    wxButton            *m_tButton;
};

#endif  // _DASHBOARD_WINDOW_H_
