/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - DashboardPreferencesDialog
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _DASHBOARD_PREFERENCES_DIALOG_H_
#define _DASHBOARD_PREFERENCES_DIALOG_H_

#include "wx/wx.h"
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include "ocpn_font_button.h"
#include "dashboard_window_container.h"

//  DashboardPreferencesDialog
//  Plugin-level settings: fonts, units, dashboard list,
//  per-dashboard instrument list.

class DashboardPreferencesDialog : public wxDialog {
public:
    DashboardPreferencesDialog(wxWindow *pparent, wxWindowID id,
                               wxArrayOfDashboard config);
    ~DashboardPreferencesDialog() {}

    void OnCloseDialog(wxCloseEvent &event);
    void OnDashboardSelected(wxListEvent &event);
    void OnDashboardAdd(wxCommandEvent &event);
    void OnDashboardDelete(wxCommandEvent &event);
    void OnInstrumentSelected(wxListEvent &event);
    void OnInstrumentAdd(wxCommandEvent &event);
    void OnInstrumentEdit(wxCommandEvent &event);
    void OnInstrumentDelete(wxCommandEvent &event);
    void OnInstrumentUp(wxCommandEvent &event);
    void OnInstrumentDown(wxCommandEvent &event);
    void OnDashboarddefaultFont(wxCommandEvent &event);
    void SaveDashboardConfig();
    void RecalculateSize(void);

    wxArrayOfDashboard    m_Config;
    wxFontPickerCtrl     *m_pFontPickerTitle;
    wxFontPickerCtrl     *m_pFontPickerData;
    wxFontPickerCtrl     *m_pFontPickerLabel;
    wxFontPickerCtrl     *m_pFontPickerSmall;
    wxChoice             *m_pChoicePressureUnit;
    wxSpinCtrl           *m_pSpinSpeedMax;
    wxCheckBox           *m_pCheckBoxTwentyFourVolts;
    wxCheckBox           *m_pCheckBoxDualengine;
    wxChoice             *m_pChoiceTemperatureUnit;
    wxChoice             *m_pChoiceSpeedUnit;
    wxChoice             *m_pChoiceDepthUnit;
    wxSpinCtrlDouble     *m_pSpinDBTOffset;
    wxChoice             *m_pChoiceDistanceUnit;
    wxChoice             *m_pChoiceWindSpeedUnit;
    wxCheckBox           *m_pUseInternSumLog;
    wxStaticText         *m_SumLogUnit;
    wxTextCtrl           *m_pSumLogValue;
    wxCheckBox           *m_pUseTrueWinddata;
    wxChoice             *m_pChoiceTempUnit;

private:
    void UpdateDashboardButtonsState(void);
    void UpdateButtonsState(void);

    int            curSel;
    wxListCtrl    *m_pListCtrlDashboards;
    wxBitmapButton*m_pButtonAddDashboard;
    wxBitmapButton*m_pButtonDeleteDashboard;
    wxPanel       *m_pPanelDashboard;
    wxTextCtrl    *m_pTextCtrlCaption;
    wxCheckBox    *m_pCheckBoxIsVisible;
    wxChoice      *m_pChoiceOrientation;
    wxListCtrl    *m_pListCtrlInstruments;
    wxButton      *m_pButtonAdd;
    wxButton      *m_pButtonEdit;
    wxButton      *m_pButtonDelete;
    wxButton      *m_pButtonUp;
    wxButton      *m_pButtonDown;
    wxButton      *m_pButtondefaultFont;
	wxCheckBox	  *m_pCheckBoxHighContrast;
};

#endif  // _DASHBOARD_PREFERENCES_DIALOG_H_
