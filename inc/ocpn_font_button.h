/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - OCPNFontButton
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _OCPN_FONT_BUTTON_H_
#define _OCPN_FONT_BUTTON_H_

#include "wx/wx.h"
#include <wx/fontpicker.h>
#include "wx/button.h"
#include "wx/fontdata.h"
#include "wx/fontdlg.h"

// ============================================================
//  OCPNFontButton
//  A button that opens a wxFontDialog and holds the result.
//  Used as a drop-in replacement for wxFontPickerCtrl.
// ============================================================
class OCPNFontButton : public wxButton {
public:
    OCPNFontButton() {}
    OCPNFontButton(wxWindow *parent, wxWindowID id,
                   const wxFontData &initial,
                   const wxPoint &pos    = wxDefaultPosition,
                   const wxSize  &size   = wxDefaultSize,
                   long  style  = wxFONTBTN_DEFAULT_STYLE,
                   const wxValidator &validator = wxDefaultValidator,
                   const wxString &name  = wxFontPickerWidgetNameStr)
    {
        Create(parent, id, initial, pos, size, style, validator, name);
    }

    virtual ~OCPNFontButton() {}

    bool Create(wxWindow *parent, wxWindowID id,
                const wxFontData &initial,
                const wxPoint &pos    = wxDefaultPosition,
                const wxSize  &size   = wxDefaultSize,
                long           style  = wxFONTBTN_DEFAULT_STYLE,
                const wxValidator &validator = wxDefaultValidator,
                const wxString &name  = wxFontPickerWidgetNameStr);

    // --- Font / colour accessors ---
    wxFontData *GetFontData()           { return &m_data; }
    wxFont      GetSelectedFont() const { return m_selectedFont; }

    virtual wxColour GetSelectedColour() const { return m_data.GetColour(); }

    virtual void SetSelectedFont(const wxFont &font) {
        m_data.SetChosenFont(font);
        m_selectedFont = m_data.GetChosenFont();
        UpdateFont();
    }
    virtual void SetSelectedColour(const wxColour &colour) {
        m_data.SetColour(colour);
        UpdateFont();
    }

    void OnButtonClick(wxCommandEvent &);

protected:
    void UpdateFont();

    wxFontData m_data;
    wxFont     m_selectedFont;
};

// Alias so existing code that uses wxFontPickerCtrl still compiles
#define wxFontPickerCtrl OCPNFontButton

#endif  // _OCPN_FONT_BUTTON_H_
