/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - Dashboard Window implementation
 * Author:   Jean-Eudes Onfray
 * expanded: Bernd Cirotzki 2023 (special colour design)
 *
 *   Copyright (C) 2010 by David S. Register
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 ***************************************************************************/
 
#include "dashboard_window.h"
#include "dashboard_globals.h"
#include "dashboard_pi.h"

DashboardWindow::DashboardWindow(wxWindow *pparent, wxWindowID id,
                                 wxAuiManager *auimgr, Dashboard *plugin,
                                 int orient, DashboardWindowContainer *mycont)
    : wxWindow(pparent, id, wxDefaultPosition, wxDefaultSize, 0) {
  // wxDialog::Create(pparent, id, _("tileMine"), wxDefaultPosition,
  // wxDefaultSize, wxDEFAULT_DIALOG_STYLE, "Dashboard");

  m_pauimgr = auimgr;
  m_plugin = plugin;
  m_Container = mycont;

  // wx2.9      itemBoxSizer = new wxWrapSizer( orient );
  itemBoxSizer = new wxBoxSizer(orient);
  SetSizer(itemBoxSizer);
  Connect(wxEVT_SIZE, wxSizeEventHandler(DashboardWindow::OnSize), NULL, this);
  Connect(wxEVT_CONTEXT_MENU,
          wxContextMenuEventHandler(DashboardWindow::OnContextMenu), NULL,
          this);
  Connect(wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(DashboardWindow::OnContextMenuSelect), NULL,
          this);

#ifdef __OCPN__ANDROID__
  Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(DashboardWindow::OnMouseEvent));
  Connect(wxEVT_LEFT_UP, wxMouseEventHandler(DashboardWindow::OnMouseEvent));
  Connect(wxEVT_MOTION, wxMouseEventHandler(DashboardWindow::OnMouseEvent));

  GetHandle()->setAttribute(Qt::WA_AcceptTouchEvents);
  GetHandle()->grabGesture(Qt::PinchGesture);
  GetHandle()->grabGesture(Qt::PanGesture);

  Connect(wxEVT_QT_PINCHGESTURE,
          (wxObjectEventFunction)(wxEventFunction)&DashboardWindow::
              OnEvtPinchGesture,
          NULL, this);
  Connect(
      wxEVT_QT_PANGESTURE,
      (wxObjectEventFunction)(wxEventFunction)&DashboardWindow::OnEvtPanGesture,
      NULL, this);
#endif

  Hide();

  m_binResize = false;
  m_binPinch = false;
}

DashboardWindow::~DashboardWindow() {
  for (size_t i = 0; i < m_ArrayOfInstrument.GetCount(); i++) {
    DashboardInstrumentContainer *pdic = m_ArrayOfInstrument.Item(i);
    delete pdic;
  }
}

#ifdef __OCPN__ANDROID__
void DashboardWindow::OnEvtPinchGesture(wxQT_PinchGestureEvent &event) {
  float zoom_gain = 0.3;
  float zoom_val;
  float total_zoom_val;

  if (event.GetScaleFactor() > 1)
    zoom_val = ((event.GetScaleFactor() - 1.0) * zoom_gain) + 1.0;
  else
    zoom_val = 1.0 - ((1.0 - event.GetScaleFactor()) * zoom_gain);

  if (event.GetTotalScaleFactor() > 1)
    total_zoom_val = ((event.GetTotalScaleFactor() - 1.0) * zoom_gain) + 1.0;
  else
    total_zoom_val = 1.0 - ((1.0 - event.GetTotalScaleFactor()) * zoom_gain);

  wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);

  wxSize currentSize = wxSize(pane.floating_size.x, pane.floating_size.y);
  double aRatio = (double)currentSize.y / (double)currentSize.x;

  wxSize par_size = GetOCPNCanvasWindow()->GetClientSize();
  wxPoint par_pos = wxPoint(pane.floating_pos.x, pane.floating_pos.y);

  switch (event.GetState()) {
    case GestureStarted:
      m_binPinch = true;
      break;

    case GestureUpdated:
      currentSize.y *= zoom_val;
      currentSize.x *= zoom_val;

      if ((par_pos.y + currentSize.y) > par_size.y)
        currentSize.y = par_size.y - par_pos.y;

      if ((par_pos.x + currentSize.x) > par_size.x)
        currentSize.x = par_size.x - par_pos.x;

      /// vertical
      currentSize.x = currentSize.y / aRatio;

      currentSize.x = wxMax(currentSize.x, 150);
      currentSize.y = wxMax(currentSize.y, 150);

      pane.FloatingSize(currentSize);
      m_pauimgr->Update();

      break;

    case GestureFinished: {
      if (itemBoxSizer->GetOrientation() == wxVERTICAL) {
        currentSize.y *= total_zoom_val;
        currentSize.x = currentSize.y / aRatio;
      } else {
        currentSize.x *= total_zoom_val;
        currentSize.y = currentSize.x * aRatio;
      }

      //  Bound the resulting size
      if ((par_pos.y + currentSize.y) > par_size.y)
        currentSize.y = par_size.y - par_pos.y;

      if ((par_pos.x + currentSize.x) > par_size.x)
        currentSize.x = par_size.x - par_pos.x;

      // not too small
      currentSize.x = wxMax(currentSize.x, 150);
      currentSize.y = wxMax(currentSize.y, 150);

      //  Try a manual layout of the window, to estimate a good primary size..

      // vertical
      if (itemBoxSizer->GetOrientation() == wxVERTICAL) {
        int total_y = 0;
        for (unsigned int i = 0; i < m_ArrayOfInstrument.size(); i++) {
          DashboardInstrument *inst =
              m_ArrayOfInstrument.Item(i)->m_pInstrument;
          wxSize is =
              inst->GetSize(itemBoxSizer->GetOrientation(), currentSize);
          total_y += is.y;
        }

        currentSize.y = total_y;
      }

      pane.FloatingSize(currentSize);

      // Reshow the window
      for (unsigned int i = 0; i < m_ArrayOfInstrument.size(); i++) {
        DashboardInstrument *inst = m_ArrayOfInstrument.Item(i)->m_pInstrument;
        inst->Show();
      }

      m_pauimgr->Update();

      m_binPinch = false;
      m_binResize = false;

      break;
    }

    case GestureCanceled:
      m_binPinch = false;
      m_binResize = false;
      break;

    default:
      break;
  }
}

void DashboardWindow::OnEvtPanGesture(wxQT_PanGestureEvent &event) {
  if (m_binPinch) return;

  if (m_binResize) return;

  int x = event.GetOffset().x;
  int y = event.GetOffset().y;

  int lx = event.GetLastOffset().x;
  int ly = event.GetLastOffset().y;

  int dx = x - lx;
  int dy = y - ly;

  switch (event.GetState()) {
    case GestureStarted:
      if (m_binPan) break;

      m_binPan = true;
      break;

    case GestureUpdated:
      if (m_binPan) {
        wxSize par_size = GetOCPNCanvasWindow()->GetClientSize();
        wxPoint par_pos_old = ClientToScreen(wxPoint(0, 0));  // GetPosition();

        wxPoint par_pos = par_pos_old;
        par_pos.x += dx;
        par_pos.y += dy;

        par_pos.x = wxMax(par_pos.x, 0);
        par_pos.y = wxMax(par_pos.y, 0);

        wxSize mySize = GetSize();

        if ((par_pos.y + mySize.y) > par_size.y)
          par_pos.y = par_size.y - mySize.y;

        if ((par_pos.x + mySize.x) > par_size.x)
          par_pos.x = par_size.x - mySize.x;

        wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);
        pane.FloatingPosition(par_pos).Float();
        m_pauimgr->Update();
      }
      break;

    case GestureFinished:
      if (m_binPan) {
      }
      m_binPan = false;

      break;

    case GestureCanceled:
      m_binPan = false;
      break;

    default:
      break;
  }
}

void DashboardWindow::OnMouseEvent(wxMouseEvent &event) {
  if (m_binPinch) return;

  if (m_binResize) {
    wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);
    wxSize currentSize = wxSize(pane.floating_size.x, pane.floating_size.y);
    double aRatio = (double)currentSize.y / (double)currentSize.x;

    wxSize par_size = GetOCPNCanvasWindow()->GetClientSize();
    wxPoint par_pos = wxPoint(pane.floating_pos.x, pane.floating_pos.y);

    if (event.LeftDown()) {
      m_resizeStartPoint = event.GetPosition();
      m_resizeStartSize = currentSize;
      m_binResize2 = true;
    }

    if (m_binResize2) {
      if (event.Dragging()) {
        wxPoint p = event.GetPosition();

        wxSize dragSize = m_resizeStartSize;

        dragSize.y += p.y - m_resizeStartPoint.y;
        dragSize.x += p.x - m_resizeStartPoint.x;
        ;

        if ((par_pos.y + dragSize.y) > par_size.y)
          dragSize.y = par_size.y - par_pos.y;

        if ((par_pos.x + dragSize.x) > par_size.x)
          dragSize.x = par_size.x - par_pos.x;

        /// vertical
        // dragSize.x = dragSize.y / aRatio;

        // not too small
        dragSize.x = wxMax(dragSize.x, 150);
        dragSize.y = wxMax(dragSize.y, 150);

        pane.FloatingSize(dragSize);
        m_pauimgr->Update();
      }

      if (event.LeftUp()) {
        wxPoint p = event.GetPosition();

        wxSize dragSize = m_resizeStartSize;

        dragSize.y += p.y - m_resizeStartPoint.y;
        dragSize.x += p.x - m_resizeStartPoint.x;
        ;

        if ((par_pos.y + dragSize.y) > par_size.y)
          dragSize.y = par_size.y - par_pos.y;

        if ((par_pos.x + dragSize.x) > par_size.x)
          dragSize.x = par_size.x - par_pos.x;

        // not too small
        dragSize.x = wxMax(dragSize.x, 150);
        dragSize.y = wxMax(dragSize.y, 150);
        /*
                        for( unsigned int i=0; i<m_ArrayOfInstrument.size(); i++
           ) { DashboardInstrument* inst =
           m_ArrayOfInstrument.Item(i)->m_pInstrument; inst->Show();
                        }
        */

#include "dashboard_pi.h"
#include "dashboard_window.h"
        pane.FloatingSize(dragSize);
        m_pauimgr->Update();

        m_binResize = false;
        m_binResize2 = false;
      }
    }
  }
}
#endif

void DashboardWindow::OnSize(wxSizeEvent &event) {
  event.Skip();
  for (unsigned int i = 0; i < m_ArrayOfInstrument.size(); i++) {
    DashboardInstrument *inst = m_ArrayOfInstrument.Item(i)->m_pInstrument;
    inst->SetMinSize(
        inst->GetSize(itemBoxSizer->GetOrientation(), GetClientSize()));
  }
  Layout();
  Refresh();
  // Capture the user adjusted docked Dashboard size
  this->m_Container->m_best_size = event.GetSize();
}

void DashboardWindow::OnContextMenu(wxContextMenuEvent &event) {
  wxMenu *contextMenu = new wxMenu();

#ifdef __WXQT__
  wxFont *pf = OCPNGetFont(_("Menu"));

  // add stuff
  wxMenuItem *item1 =
      new wxMenuItem(contextMenu, ID_DASH_PREFS, _("Preferences..."));
  item1->SetFont(*pf);
  contextMenu->Append(item1);

  wxMenuItem *item2 =
      new wxMenuItem(contextMenu, ID_DASH_RESIZE, _("Resize..."));
  item2->SetFont(*pf);
  contextMenu->Append(item2);

#else

  wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);
  if (pane.IsOk() && pane.IsDocked()) {
    contextMenu->Append(ID_DASH_UNDOCK, _("Undock"));
  }
  wxMenuItem *btnVertical =
      contextMenu->AppendRadioItem(ID_DASH_VERTICAL, _("Vertical"));
  btnVertical->Check(itemBoxSizer->GetOrientation() == wxVERTICAL);
  wxMenuItem *btnHorizontal =
      contextMenu->AppendRadioItem(ID_DASH_HORIZONTAL, _("Horizontal"));
  btnHorizontal->Check(itemBoxSizer->GetOrientation() == wxHORIZONTAL);
  contextMenu->AppendSeparator();

  m_plugin->PopulateContextMenu(contextMenu);

  contextMenu->AppendSeparator();
  contextMenu->Append(ID_DASH_PREFS, _("Preferences..."));

#endif

  PopupMenu(contextMenu);
  delete contextMenu;
}

void DashboardWindow::OnContextMenuSelect(wxCommandEvent &event) {
  if (event.GetId() < ID_DASH_PREFS) {  // Toggle dashboard visibility
    if (m_plugin->GetDashboardWindowShownCount() > 1 || event.IsChecked())
      m_plugin->ShowDashboard(event.GetId() - 1, event.IsChecked());
    else
      m_plugin->ShowDashboard(event.GetId() - 1, true);

    SetToolbarItemState(m_plugin->GetToolbarItemId(),
                        m_plugin->GetDashboardWindowShownCount() != 0);
  }

  switch (event.GetId()) {
    case ID_DASH_PREFS: {
      wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);
      // Capture the dashboard's floating_pos before update.
      wxPoint fp = pane.floating_pos;
      m_plugin->ShowPreferencesDialog(this);

      // This function sets the correct size of the edited dashboard with the
      // previous floating position instead of the default that is otherwise used.
      // If the panel is docked the size is instead specified by the docking logic

      if (!pane.IsDocked()) {
        ChangePaneOrientation(GetSizerOrientation(), true, fp.x, fp.y);
	   }
    }
    case ID_DASH_RESIZE: {
      /*
                  for( unsigned int i=0; i<m_ArrayOfInstrument.size(); i++ ) {
                      DashboardInstrument* inst =
         m_ArrayOfInstrument.Item(i)->m_pInstrument; inst->Hide();
                  }
      */
      m_binResize = true;

      return;
    }
    case ID_DASH_VERTICAL: {
      ChangePaneOrientation(wxVERTICAL, true);
      m_Container->m_sOrientation = "V";
      break;
    }
    case ID_DASH_HORIZONTAL: {
      ChangePaneOrientation(wxHORIZONTAL, true);
      m_Container->m_sOrientation = "H";
      break;
    }
    case ID_DASH_UNDOCK: {
      ChangePaneOrientation(GetSizerOrientation(), true);
      return;  // Nothing changed so nothing need be saved
    }
  }
  m_plugin->SaveConfig();
}

void DashboardWindow::SetColorScheme(PI_ColorScheme cs) {
  DimeWindow(this);

  //  Improve appearance, especially in DUSK or NIGHT palette
  wxColour col = g_BackgroundColor;

  if (!g_ForceBackgroundColor) GetGlobalColor("DASHL", &col);
  SetBackgroundColour(col);

  Refresh(false);
}

void DashboardWindow::ChangePaneOrientation(int orient, bool updateAUImgr,
                                            int fpx, int fpy) {
  m_pauimgr->DetachPane(this);
  SetSizerOrientation(orient);
  bool vertical = orient == wxVERTICAL;
  // wxSize sz = GetSize( orient, wxDefaultSize );
  wxSize sz = GetMinSize();
  // We must change Name to reset AUI perpective
  m_Container->m_sName = MakeName();
  m_pauimgr->AddPane(this, wxAuiPaneInfo()
                               .Name(m_Container->m_sName)
                               .Caption(m_Container->m_sCaption)
                               .CaptionVisible(true)
                               .TopDockable(!vertical)
                               .BottomDockable(!vertical)
                               .LeftDockable(vertical)
                               .RightDockable(vertical)
                               .MinSize(sz)
                               .BestSize(sz)
                               .FloatingSize(sz)
                               .FloatingPosition(fpx, fpy)
                               .Float()
                               .Show(m_Container->m_bIsVisible));

#ifdef __OCPN__ANDROID__
  wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);
  pane.Dockable(false);
#endif

  if (updateAUImgr) m_pauimgr->Update();
}

void DashboardWindow::SetSizerOrientation(int orient) {
  itemBoxSizer->SetOrientation(orient);
  /* We must reset all MinSize to ensure we start with new default */
  wxWindowListNode *node = GetChildren().GetFirst();
  while (node) {
    node->GetData()->SetMinSize(wxDefaultSize);
    node = node->GetNext();
  }
  SetMinSize(wxDefaultSize);
  Fit();
  SetMinSize(itemBoxSizer->GetMinSize());
}

int DashboardWindow::GetSizerOrientation() {
  return itemBoxSizer->GetOrientation();
}

bool isArrayIntEqual(const wxArrayInt &l1, const wxArrayOfInstrument &l2) {
  if (l1.GetCount() != l2.GetCount()) return false;

  for (size_t i = 0; i < l1.GetCount(); i++)
    if (l1.Item(i) != l2.Item(i)->m_ID) return false;

  return true;
}

bool DashboardWindow::IsInstrumentListEqual(const wxArrayInt &list) {
  return isArrayIntEqual(list, m_ArrayOfInstrument);
}

void DashboardWindow::SetInstrumentList(
    wxArrayInt list, wxArrayOfInstrumentProperties *InstrumentPropertyList) {

  InstrumentProperties *properties;
  m_ArrayOfInstrument.Clear();
  itemBoxSizer->Clear(true);
  for (size_t i = 0; i < list.GetCount(); i++) {
    int id = list.Item(i);
    properties = NULL;
    for (size_t j = 0; j < InstrumentPropertyList->GetCount(); j++) {
      if (InstrumentPropertyList->Item(j)->m_aInstrument == id &&
          InstrumentPropertyList->Item(j)->m_Listplace == (int)i) {
        properties = InstrumentPropertyList->Item(j);
        break;
      }
    }
    DashboardInstrument *instrument = NULL;
    switch (id) {
	case ID_DBP_MAIN_ENGINE_RPM:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_MAIN_ENGINE_RPM, 0, g_tachometerMax);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(1000, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(200, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionExtraValue(OCPN_DBP_STC_MAIN_ENGINE_HOURS, _T("%.1f"), DIAL_POSITION_INSIDE);
		((DashboardInstrument_Dial*)instrument)->SetOptionWarningValue(OCPN_DBP_STC_MAIN_ENGINE_FAULT_ONE);
		break;
	case ID_DBP_PORT_ENGINE_RPM:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_PORT_ENGINE_RPM, 0, g_tachometerMax);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(1000, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(200, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionExtraValue(OCPN_DBP_STC_PORT_ENGINE_HOURS, _T("%.1f"), DIAL_POSITION_INSIDE);
		((DashboardInstrument_Dial*)instrument)->SetOptionWarningValue(OCPN_DBP_STC_PORT_ENGINE_FAULT_ONE);
		break;
	case ID_DBP_STBD_ENGINE_RPM:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_STBD_ENGINE_RPM, 0, g_tachometerMax);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(1000, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(200, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionExtraValue(OCPN_DBP_STC_STBD_ENGINE_HOURS, _T("%.1f"), DIAL_POSITION_INSIDE);
		((DashboardInstrument_Dial*)instrument)->SetOptionWarningValue(OCPN_DBP_STC_STBD_ENGINE_FAULT_ONE);
		break;
	case ID_DBP_MAIN_ENGINE_OIL:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_MAIN_ENGINE_OIL, 0, g_pressureUnit == PRESSURE_BAR ? 5 : 80);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_pressureUnit == PRESSURE_BAR ? 1 : 20, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_pressureUnit == PRESSURE_BAR ? 0.5 : 10, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_PORT_ENGINE_OIL:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_PORT_ENGINE_OIL, 0, g_pressureUnit == PRESSURE_BAR ? 5 : 80);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_pressureUnit == PRESSURE_BAR ? 1 : 20, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_pressureUnit == PRESSURE_BAR ? 0.5 : 10, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_STBD_ENGINE_OIL:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_STBD_ENGINE_OIL, 0, g_pressureUnit == PRESSURE_BAR ? 5 : 80);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_pressureUnit == PRESSURE_BAR ? 1 : 20, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_pressureUnit == PRESSURE_BAR ? 0.5 : 10, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_MAIN_ENGINE_WATER:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_MAIN_ENGINE_WATER, g_temperatureUnit == TEMPERATURE_CELSIUS ? 60 : 100, g_temperatureUnit == TEMPERATURE_CELSIUS ? 120 : 250);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_temperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_temperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_PORT_ENGINE_WATER:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_PORT_ENGINE_WATER, g_temperatureUnit == TEMPERATURE_CELSIUS ? 60 : 100, g_temperatureUnit == TEMPERATURE_CELSIUS ? 120 : 250);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_temperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_temperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_STBD_ENGINE_WATER:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_STBD_ENGINE_WATER, g_temperatureUnit == TEMPERATURE_CELSIUS ? 60 : 100, g_temperatureUnit == TEMPERATURE_CELSIUS ? 120 : 250);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_temperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_temperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_MAIN_ENGINE_EXHAUST:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_MAIN_ENGINE_EXHAUST, g_temperatureUnit == TEMPERATURE_CELSIUS ? 0 : 40, g_temperatureUnit == TEMPERATURE_CELSIUS ? 80 : 190);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_temperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_temperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_PORT_ENGINE_EXHAUST:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_PORT_ENGINE_EXHAUST, g_temperatureUnit == TEMPERATURE_CELSIUS ? 0 : 40, g_temperatureUnit == TEMPERATURE_CELSIUS ? 80 : 190);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_temperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_temperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_STBD_ENGINE_EXHAUST:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_STBD_ENGINE_EXHAUST, g_temperatureUnit == TEMPERATURE_CELSIUS ? 0 : 40, g_temperatureUnit == TEMPERATURE_CELSIUS ? 80 : 190);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(g_temperatureUnit == TEMPERATURE_CELSIUS ? 10 : 30, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(g_temperatureUnit == TEMPERATURE_CELSIUS ? 5 : 15, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_MAIN_ENGINE_VOLTS:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_MAIN_ENGINE_VOLTS, g_twentyFourVolts ? 18 : 8, g_twentyFourVolts ? 32 : 16);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_PORT_ENGINE_VOLTS:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_PORT_ENGINE_VOLTS, g_twentyFourVolts ? 18 : 8, g_twentyFourVolts ? 32 : 16);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_STBD_ENGINE_VOLTS:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_STBD_ENGINE_VOLTS, g_twentyFourVolts ? 18 : 8, g_twentyFourVolts ? 32 : 16);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_FUEL_TANK_01:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_FUEL_01, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
		break;
	case ID_DBP_WATER_TANK_01:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_WATER_01, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
		break;
	case ID_DBP_FUEL_TANK_02:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_FUEL_02, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
		break;
	case ID_DBP_WATER_TANK_02:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_WATER_02, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
		break;
	case ID_DBP_WATER_TANK_03:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_WATER_03, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
		break;
	case ID_DBP_OIL_TANK:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_OIL, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
		break;
	case ID_DBP_LIVEWELL_TANK:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_LIVEWELL, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_LOW, 1);
		break;
	case ID_DBP_GREY_TANK:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_GREY, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_HIGH, 1);
		break;
	case ID_DBP_BLACK_TANK:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
			GetInstrumentCaption(id), properties, OCPN_DBP_STC_TANK_LEVEL_BLACK, 0, 100);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(25, DIAL_LABEL_FRACTIONS);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(12.5, DIAL_MARKER_WARNING_HIGH, 1);
		break;
	case ID_DBP_START_BATTERY_VOLTS:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY, 
			GetInstrumentCaption(id), properties,
			OCPN_DBP_STC_START_BATTERY_VOLTS, g_twentyFourVolts ? 18 : 8, g_twentyFourVolts ? 32 : 16);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_GREEN_MID, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionExtraValue(OCPN_DBP_STC_START_BATTERY_AMPS, _T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_HOUSE_BATTERY_VOLTS:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY, 
			GetInstrumentCaption(id), properties,
			OCPN_DBP_STC_HOUSE_BATTERY_VOLTS, g_twentyFourVolts ? 18 : 8, g_twentyFourVolts ? 32 : 16);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_GREEN_MID, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionExtraValue(OCPN_DBP_STC_HOUSE_BATTERY_AMPS, _T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_RSA: {
		instrument = new DashboardInstrument_RudderAngle(this, wxID_ANY, GetInstrumentCaption(id), properties);
		((DashboardInstrument_RudderAngle*)instrument)->SetOptionMarker(5, DIAL_MARKER_REDGREEN, 2);
		wxString labels[] = { _T("40"), _T("30"), _T("20"), _T("10"), _T("0"), _T("10"), _T("20"), _T("30"), _T("40") };
		((DashboardInstrument_RudderAngle*)instrument)->SetOptionLabel(10, DIAL_LABEL_HORIZONTAL, wxArrayString(9, labels));
		}
		break;
	case ID_DBP_MAIN_ENGINE_FUEL_RATE:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
		GetInstrumentCaption(id), properties, OCPN_DBP_STC_MAIN_ENGINE_FUEL_RATE, 0, g_volumeUnit == VOLUME_LITRE ? 10 : 4 );
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_PORT_ENGINE_FUEL_RATE:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
		GetInstrumentCaption(id), properties, OCPN_DBP_STC_STBD_ENGINE_FUEL_RATE, 0, g_volumeUnit == VOLUME_LITRE ? 10 : 4);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_STBD_ENGINE_FUEL_RATE:
		instrument = new DashboardInstrument_Speedometer(this, wxID_ANY,
		GetInstrumentCaption(id), properties, OCPN_DBP_STC_PORT_ENGINE_FUEL_RATE, 0, g_volumeUnit == VOLUME_LITRE ? 10 : 4);
		((DashboardInstrument_Dial*)instrument)->SetOptionLabel(2, DIAL_LABEL_HORIZONTAL);
		((DashboardInstrument_Dial*)instrument)->SetOptionMarker(1, DIAL_MARKER_SIMPLE, 1);
		((DashboardInstrument_Dial*)instrument)->SetOptionMainValue(_T("%.1f"), DIAL_POSITION_INSIDE);
		break;
	case ID_DBP_FUEL_TANK_GAUGE_01:
		instrument = new DashboardInstrument_Block(this, wxID_ANY, GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_01, "%s");
		break;
	case ID_DBP_FUEL_TANK_GAUGE_02:
		instrument = new DashboardInstrument_Block(this, wxID_ANY, GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_FUEL_GAUGE_02, "%s");
		break;
	case ID_DBP_WATER_TANK_GAUGE_01:
		instrument = new DashboardInstrument_Block(this, wxID_ANY, GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_01, "%s");
		break;
	case ID_DBP_WATER_TANK_GAUGE_02:
		instrument = new DashboardInstrument_Block(this, wxID_ANY, GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_02, "%s");
		break;
	case ID_DBP_WATER_TANK_GAUGE_03:
		instrument = new DashboardInstrument_Block(this, wxID_ANY, GetInstrumentCaption(id), OCPN_DBP_STC_TANK_LEVEL_WATER_GAUGE_03, "%s");
		break;
	case ID_DBP_START_BATTERY_SOC:
		instrument = new DashboardInstrument_Single(this, wxID_ANY, GetInstrumentCaption(id),
			properties, OCPN_DBP_STC_START_BATTERY_SOC, "%0.1f");
		break;
	case ID_DBP_START_BATTERY_HOURS:
		instrument = new DashboardInstrument_Single(this, wxID_ANY, GetInstrumentCaption(id),
			properties, OCPN_DBP_STC_START_BATTERY_HOURS, "%0.1f");
		break;
	case ID_DBP_HOUSE_BATTERY_SOC:
		instrument = new DashboardInstrument_Single(this, wxID_ANY, GetInstrumentCaption(id), 
			properties, OCPN_DBP_STC_HOUSE_BATTERY_SOC, "%0.1f");
		break;
	case ID_DBP_HOUSE_BATTERY_HOURS:
		instrument = new DashboardInstrument_Single(this, wxID_ANY, GetInstrumentCaption(id),
			properties, OCPN_DBP_STC_HOUSE_BATTERY_HOURS, "%0.1f");
		break;

    }
    if (instrument) {
      instrument->instrumentTypeId = id;
      m_ArrayOfInstrument.Add(new DashboardInstrumentContainer(
          id, instrument, instrument->GetCapabilityFlag()));
      itemBoxSizer->Add(instrument, 0, wxEXPAND, 0);
      if (itemBoxSizer->GetOrientation() == wxHORIZONTAL) {
        itemBoxSizer->AddSpacer(5);
      }
    }
  }

  //  In the absense of any other hints, build the default instrument sizes by
  //  taking the calculated with of the first (and succeeding) instruments as
  //  hints for the next. So, best in default loads to start with an instrument
  //  that accurately calculates its minimum width. e.g.
  //  DashboardInstrument_Position

  wxSize Hint = wxSize(DefaultWidth, DefaultWidth);

  for (unsigned int i = 0; i < m_ArrayOfInstrument.size(); i++) {
    DashboardInstrument *inst = m_ArrayOfInstrument.Item(i)->m_pInstrument;
    inst->SetMinSize(inst->GetSize(itemBoxSizer->GetOrientation(), Hint));
    Hint = inst->GetMinSize();
  }

  Fit();
  Layout();
  SetMinSize(itemBoxSizer->GetMinSize());
}

void DashboardWindow::SendSentenceToAllInstruments(DASH_CAP cap_flag, double value,
                                                   wxString unit) {
  for (size_t i = 0; i < m_ArrayOfInstrument.GetCount(); i++) {
    if (m_ArrayOfInstrument.Item(i)->m_cap_flag.test(cap_flag))
      m_ArrayOfInstrument.Item(i)->m_pInstrument->SetData(cap_flag, value, unit);
  }
}
