/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - Add Instrument dialog implementation
 * Author:   Jean-Eudes Onfray
 * expanded: Bernd Cirotzki 2023 (special colour design)
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/

#include "dashboard_pi.h"
#include "add_instrument_dlg.h"
AddInstrumentDlg::AddInstrumentDlg(wxWindow *pparent, wxWindowID id)
    : wxDialog(pparent, id, _("Add instrument"), wxDefaultPosition,
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
  wxBoxSizer *itemBoxSizer01 = new wxBoxSizer(wxVERTICAL);
  SetSizer(itemBoxSizer01);
  wxStaticText *itemStaticText01 =
      new wxStaticText(this, wxID_ANY, _("Select instrument to add:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemBoxSizer01->Add(itemStaticText01, 0, wxEXPAND | wxALL, 5);

  int instImageRefSize = 20 * GetOCPNGUIToolScaleFactor_PlugIn();

  wxImageList *imglist =
      new wxImageList(instImageRefSize, instImageRefSize, true, 2);

  wxImage inst1 = g_instrumentBitmap.ConvertToImage();
  wxImage inst1s =
      inst1.Scale(instImageRefSize, instImageRefSize, wxIMAGE_QUALITY_HIGH);
  imglist->Add(wxBitmap(inst1s));

  wxImage dial1 = g_dialBitmap.ConvertToImage();
  wxImage dial1s =
      dial1.Scale(instImageRefSize, instImageRefSize, wxIMAGE_QUALITY_HIGH);
  imglist->Add(wxBitmap(dial1s));

  wxSize dsize = GetOCPNCanvasWindow()->GetClientSize();
  int vsize = dsize.y * 50 / 100;

#ifdef __OCPN__ANDROID__
  int dw, dh;
  wxDisplaySize(&dw, &dh);
  vsize = dh * 50 / 100;
#endif

  m_pListCtrlInstruments = new wxListCtrl(
      this, wxID_ANY, wxDefaultPosition, wxSize(-1, vsize /*250, 180 */),
      wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_SORT_ASCENDING);
  itemBoxSizer01->Add(m_pListCtrlInstruments, 0, wxEXPAND | wxALL, 5);
  m_pListCtrlInstruments->AssignImageList(imglist, wxIMAGE_LIST_SMALL);
  m_pListCtrlInstruments->InsertColumn(0, _("Instruments"));

  wxFont *pF = OCPNGetFont(_("Dialog"), 12);
  m_pListCtrlInstruments->SetFont(*pF);

#ifdef __OCPN__ANDROID__
  m_pListCtrlInstruments->GetHandle()->setStyleSheet(qtStyleSheet);
/// QScroller::ungrabGesture(m_pListCtrlInstruments->GetHandle());
#endif

  wxStdDialogButtonSizer *DialogButtonSizer =
      CreateStdDialogButtonSizer(wxOK | wxCANCEL);
  itemBoxSizer01->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

  long ident = 0;
  for (unsigned int i = ID_DBP_MAIN_ENGINE_RPM; i < ID_DBP_LAST_ENTRY;
       i++) {  // do not reference an instrument, but the last dummy entry in
               // the list
    wxListItem item;
    GetListItemForInstrument(item, i);
    item.SetId(ident);
    m_pListCtrlInstruments->InsertItem(item);
    id++;
  }

  m_pListCtrlInstruments->SetColumnWidth(0, wxLIST_AUTOSIZE);
  m_pListCtrlInstruments->SetItemState(0, wxLIST_STATE_SELECTED,
                                       wxLIST_STATE_SELECTED);
  Fit();
}

unsigned int AddInstrumentDlg::GetInstrumentAdded() {
  long itemID = -1;
  itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED);

  return (int)m_pListCtrlInstruments->GetItemData(itemID);
}