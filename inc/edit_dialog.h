/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - EditDialog (per-instrument property editor)
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _EDIT_DIALOG_H_
#define _EDIT_DIALOG_H_

#include "wx/wx.h"
#include <wx/clrpicker.h>
#include <wx/statline.h>
#include <wx/listctrl.h>
#include "ocpn_font_button.h"
#include "instrument.h"

//  EditDialog - Edit settings for each dashboard
class EditDialog : public wxDialog {
private:
protected:
    wxStaticText *m_staticText1;
    wxStaticText *m_staticText5;
    wxStaticText *m_staticText2;
    wxStaticText *m_staticText6;
    wxStaticText *m_staticText3;
    wxStaticText *m_staticText4;
    wxStaticLine *m_staticline1;
    wxStaticLine *m_staticline2;
    wxStaticText *m_staticText7;
    wxStaticText *m_staticText9;
    wxStaticText *m_staticText10;
    wxStdDialogButtonSizer *m_sdbSizer3;
    wxButton *m_sdbSizer3OK;
    wxButton *m_sdbSizer3Cancel;

    virtual void OnSetdefault(wxCommandEvent &event);

public:
    wxFontPickerCtrl    *m_fontPicker2;
    wxColourPickerCtrl  *m_colourPicker1;
    wxFontPickerCtrl    *m_fontPicker4;
    wxColourPickerCtrl  *m_colourPicker2;
    wxFontPickerCtrl    *m_fontPicker5;
    wxFontPickerCtrl    *m_fontPicker6;
    wxColourPickerCtrl  *m_colourPicker3;
    wxColourPickerCtrl  *m_colourPicker4;
    wxButton            *m_button1;

    EditDialog(wxWindow *parent, InstrumentProperties &Properties,
               wxWindowID id    = wxID_ANY,
               const wxString &title = "Edit Instrument",
               const wxPoint  &pos   = wxDefaultPosition,
               const wxSize   &size  = wxSize(-1, -1),
               long  style = wxDEFAULT_DIALOG_STYLE);
    ~EditDialog();
};

#endif  // _EDIT_DIALOG_H_
