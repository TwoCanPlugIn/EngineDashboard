/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - AddInstrumentDlg
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#ifndef _ADD_INSTRUMENT_DLG_H_
#define _ADD_INSTRUMENT_DLG_H_

#include "wx/wx.h"
#include <wx/listctrl.h>

// ============================================================
//  AddInstrumentDlg
//  Shows the list of available instrument types so the user
//  can pick one to add to a dashboard panel.
// ============================================================
class AddInstrumentDlg : public wxDialog {
public:
    AddInstrumentDlg(wxWindow *pparent, wxWindowID id);
    ~AddInstrumentDlg() {}

    unsigned int GetInstrumentAdded();

private:
    wxListCtrl *m_pListCtrlInstruments;
};

#endif  // _ADD_INSTRUMENT_DLG_H_
