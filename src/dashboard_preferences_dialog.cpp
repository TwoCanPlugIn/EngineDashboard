/***************************************************************************
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin - Dashboard Settings implementation
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
#include "dashboard_preferences_dialog.h"
#include "add_instrument_dlg.h"
#include "edit_dialog.h"

DashboardPreferencesDialog::DashboardPreferencesDialog(
    wxWindow *parent, wxWindowID id, wxArrayOfDashboard config)
    : wxDialog(parent, id, _("Engine Dashboard Settings"), wxDefaultPosition,
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
#ifdef __WXQT__
  wxFont *pF = OCPNGetFont(_("Dialog"));
  SetFont(*pF);
#endif

  int display_width, display_height;
  wxDisplaySize(&display_width, &display_height);


  wxString filename = g_pluginFolder + "dashboard.svg";

  Connect(wxEVT_CLOSE_WINDOW,
          wxCloseEventHandler(DashboardPreferencesDialog::OnCloseDialog), NULL,
          this);

  // Copy original config
  m_Config = wxArrayOfDashboard(config);
  // Build Dashboard Page for Toolbox
  int border_size = 2;

  wxBoxSizer *itemBoxSizerMainPanel = new wxBoxSizer(wxVERTICAL);
  SetSizer(itemBoxSizerMainPanel);

  wxWindow *dparent = this;
  wxScrolledWindow *scrollWin = new wxScrolledWindow(
      this, wxID_ANY, wxDefaultPosition, wxSize(-1, -1), wxVSCROLL | wxHSCROLL);

  scrollWin->SetScrollRate(1, 1);
  itemBoxSizerMainPanel->Add(scrollWin, 1, wxEXPAND | wxALL, 0);

  dparent = scrollWin;

  wxBoxSizer *itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
  scrollWin->SetSizer(itemBoxSizer2);

  auto *DialogButtonSizer = new wxStdDialogButtonSizer();
  DialogButtonSizer->AddButton(new wxButton(this, wxID_OK));
  DialogButtonSizer->AddButton(new wxButton(this, wxID_CANCEL));
  wxButton *help_btn = new wxButton(this, wxID_HELP);
  help_btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [&](wxCommandEvent) {
    wxString datadir = GetPluginDataDir("manual_pi");
    //Manual(this, datadir.ToStdString()).Launch("Dashboard");
  });
  DialogButtonSizer->AddButton(help_btn);
  DialogButtonSizer->Realize();

  itemBoxSizerMainPanel->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

  wxNotebook *itemNotebook = new wxNotebook(
      dparent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
  itemBoxSizer2->Add(itemNotebook, 0, wxALL | wxEXPAND, border_size);

  wxPanel *itemPanelNotebook01 =
      new wxPanel(itemNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                  wxTAB_TRAVERSAL);
  wxFlexGridSizer *itemFlexGridSizer01 = new wxFlexGridSizer(2);
  itemFlexGridSizer01->AddGrowableCol(1);
  itemPanelNotebook01->SetSizer(itemFlexGridSizer01);
  itemNotebook->AddPage(itemPanelNotebook01, _("Dashboard"));

  wxBoxSizer *itemBoxSizer01 = new wxBoxSizer(wxVERTICAL);
  itemFlexGridSizer01->Add(itemBoxSizer01, 1, wxEXPAND | wxTOP | wxLEFT,
                           border_size);

  // Scale the images in the dashboard list control
  int imageRefSize = GetCharWidth() * 4;

  wxImageList *imglist1 = new wxImageList(imageRefSize, imageRefSize, true, 1);

  wxBitmap bmDashBoard;
#ifdef ocpnUSE_SVG
  bmDashBoard = GetBitmapFromSVGFile(filename, imageRefSize, imageRefSize);
#else
  wxImage dash1 = GetBitmapFromSVGFile(filename, imageRefSize, imageRefSize).ConvertToImage();
  wxImage dash1s =
      dash1.Scale(imageRefSize, imageRefSize, wxIMAGE_QUALITY_HIGH);
  bmDashBoard = wxBitmap(dash1s);
#endif

  imglist1->Add(bmDashBoard);

  m_pListCtrlDashboards =
      new wxListCtrl(itemPanelNotebook01, wxID_ANY, wxDefaultPosition,
                     wxSize(imageRefSize * 3 / 2, 200),
                     wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);

  m_pListCtrlDashboards->AssignImageList(imglist1, wxIMAGE_LIST_SMALL);
  m_pListCtrlDashboards->InsertColumn(0, "");
  m_pListCtrlDashboards->Connect(
      wxEVT_COMMAND_LIST_ITEM_SELECTED,
      wxListEventHandler(DashboardPreferencesDialog::OnDashboardSelected), NULL,
      this);
  m_pListCtrlDashboards->Connect(
      wxEVT_COMMAND_LIST_ITEM_DESELECTED,
      wxListEventHandler(DashboardPreferencesDialog::OnDashboardSelected), NULL,
      this);
  itemBoxSizer01->Add(m_pListCtrlDashboards, 1, wxEXPAND, 0);

  wxBoxSizer *itemBoxSizer02 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer01->Add(itemBoxSizer02);

  wxBitmap bmPlus, bmMinus;
  int bmSize = imageRefSize * 100 / 275;

#ifdef __OCPN__ANDROID__
  bmSize = imageRefSize / 2;
#endif

#ifdef ocpnUSE_SVG
  bmPlus = GetBitmapFromSVGFile(pluginFolder + "plus.svg", bmSize, bmSize);
  bmMinus = GetBitmapFromSVGFile(pluginFolder + "minus.svg", bmSize, bmSize);
#else
  wxImage plus1 = g_plusBitmap.ConvertToImage();
  wxImage plus1s = plus1.Scale(bmSize, bmSize, wxIMAGE_QUALITY_HIGH);
  bmPlus = wxBitmap(plus1s);

  wxImage minus1 = g_minusBitmap.ConvertToImage();
  wxImage minus1s = minus1.Scale(bmSize, bmSize, wxIMAGE_QUALITY_HIGH);
  bmMinus = wxBitmap(minus1s);
#endif

  m_pButtonAddDashboard = new wxBitmapButton(
      itemPanelNotebook01, wxID_ANY, bmPlus, wxDefaultPosition, wxDefaultSize);
  itemBoxSizer02->Add(m_pButtonAddDashboard, 0, wxALIGN_CENTER, 2);
  m_pButtonAddDashboard->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnDashboardAdd), NULL,
      this);

  m_pButtonDeleteDashboard = new wxBitmapButton(
      itemPanelNotebook01, wxID_ANY, bmMinus, wxDefaultPosition, wxDefaultSize);
  itemBoxSizer02->Add(m_pButtonDeleteDashboard, 0, wxALIGN_CENTER, 2);
  m_pButtonDeleteDashboard->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnDashboardDelete),
      NULL, this);

  m_pPanelDashboard =
      new wxPanel(itemPanelNotebook01, wxID_ANY, wxDefaultPosition,
                  wxDefaultSize, wxBORDER_SUNKEN);
  itemFlexGridSizer01->Add(m_pPanelDashboard, 1, wxEXPAND | wxTOP | wxRIGHT,
                           border_size);

  wxBoxSizer *itemBoxSizer03 = new wxBoxSizer(wxVERTICAL);
  m_pPanelDashboard->SetSizer(itemBoxSizer03);

  wxStaticBox *itemStaticBox02 =
      new wxStaticBox(m_pPanelDashboard, wxID_ANY, _("Instruments"));
  wxStaticBoxSizer *itemStaticBoxSizer02 =
      new wxStaticBoxSizer(itemStaticBox02, wxHORIZONTAL);
  itemBoxSizer03->Add(itemStaticBoxSizer02, 0, wxEXPAND | wxALL, border_size);
  wxFlexGridSizer *itemFlexGridSizer = new wxFlexGridSizer(2);
  itemFlexGridSizer->AddGrowableCol(1);
  itemStaticBoxSizer02->Add(itemFlexGridSizer, 1, wxEXPAND | wxALL, 0);

  m_pCheckBoxIsVisible =
      new wxCheckBox(m_pPanelDashboard, wxID_ANY, _("show this dashboard"),
                     wxDefaultPosition, wxDefaultSize, 0);
  m_pCheckBoxIsVisible->SetMinSize(wxSize(25 * GetCharWidth(), -1));

  itemFlexGridSizer->Add(m_pCheckBoxIsVisible, 0, wxEXPAND | wxALL,
                         border_size);
  wxStaticText *itemDummy01 = new wxStaticText(m_pPanelDashboard, wxID_ANY, "");
  itemFlexGridSizer->Add(itemDummy01, 0, wxEXPAND | wxALL, border_size);

  wxStaticText *itemStaticText01 =
      new wxStaticText(m_pPanelDashboard, wxID_ANY, _("Caption:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer->Add(itemStaticText01, 0, wxEXPAND | wxALL, border_size);
  m_pTextCtrlCaption = new wxTextCtrl(m_pPanelDashboard, wxID_ANY, "",
                                      wxDefaultPosition, wxDefaultSize);
  m_pCheckBoxIsVisible->SetMinSize(wxSize(30 * GetCharWidth(), -1));
  itemFlexGridSizer->Add(m_pTextCtrlCaption, 0, wxALIGN_RIGHT | wxALL,
                         border_size);

  wxStaticText *itemStaticText02 =
      new wxStaticText(m_pPanelDashboard, wxID_ANY, _("Orientation:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer->Add(itemStaticText02, 0, wxEXPAND | wxALL, border_size);
  m_pChoiceOrientation = new wxChoice(m_pPanelDashboard, wxID_ANY,
                                      wxDefaultPosition, wxDefaultSize);
  m_pChoiceOrientation->SetMinSize(wxSize(15 * GetCharWidth(), -1));
  m_pChoiceOrientation->Append(_("Vertical"));
  m_pChoiceOrientation->Append(_("Horizontal"));
  itemFlexGridSizer->Add(m_pChoiceOrientation, 0, wxALIGN_RIGHT | wxALL,
                         border_size);

  int instImageRefSize = 20 * GetOCPNGUIToolScaleFactor_PlugIn();

  wxImageList *imglist =
      new wxImageList(instImageRefSize, instImageRefSize, true, 2);

  wxBitmap bmDial, bmInst;

#ifdef ocpnUSE_SVG
  bmDial = GetBitmapFromSVGFile(pluginFolder + "dial.svg", instImageRefSize,
                                instImageRefSize);
  bmInst = GetBitmapFromSVGFile(pluginFolder + "instrument.svg", instImageRefSize,
                                instImageRefSize);
#else
  wxImage dial1 = GetBitmapFromSVGFile(g_pluginFolder + "dial.svg", instImageRefSize,
      instImageRefSize).ConvertToImage();
  wxImage dial1s =
      dial1.Scale(instImageRefSize, instImageRefSize, wxIMAGE_QUALITY_HIGH);
  bmDial = wxBitmap(dial1);

  wxImage inst1 = GetBitmapFromSVGFile(g_pluginFolder + "instrument.svg", instImageRefSize,
      instImageRefSize).ConvertToImage();
  wxImage inst1s =
      inst1.Scale(instImageRefSize, instImageRefSize, wxIMAGE_QUALITY_HIGH);
  bmInst = wxBitmap(inst1s);
#endif


  imglist->Add(bmInst);
  imglist->Add(bmDial);

  wxStaticBox *itemStaticBox03 =
      new wxStaticBox(m_pPanelDashboard, wxID_ANY, _("Instruments"));
  wxStaticBoxSizer *itemStaticBoxSizer03 =
      new wxStaticBoxSizer(itemStaticBox03, wxHORIZONTAL);
  itemBoxSizer03->Add(itemStaticBoxSizer03, 1, wxEXPAND | wxALL, border_size);

  wxSize dsize = GetOCPNCanvasWindow()->GetClientSize();
  wxSize list_size = wxSize(-1, dsize.y * 35 / 100);

#ifdef __ANDROID__
  int xsize = GetCharWidth() * 30;
  list_size = wxSize(xsize, dsize.y * 50 / 100);
#endif

  m_pListCtrlInstruments =
      new wxListCtrl(m_pPanelDashboard, wxID_ANY, wxDefaultPosition, list_size,
                     wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);

  itemStaticBoxSizer03->Add(m_pListCtrlInstruments, 1, wxEXPAND | wxALL,
                            border_size);
  m_pListCtrlInstruments->AssignImageList(imglist, wxIMAGE_LIST_SMALL);
  m_pListCtrlInstruments->InsertColumn(0, _("Instruments"));
  m_pListCtrlInstruments->Connect(
      wxEVT_COMMAND_LIST_ITEM_SELECTED,
      wxListEventHandler(DashboardPreferencesDialog::OnInstrumentSelected),
      NULL, this);
  m_pListCtrlInstruments->Connect(
      wxEVT_COMMAND_LIST_ITEM_DESELECTED,
      wxListEventHandler(DashboardPreferencesDialog::OnInstrumentSelected),
      NULL, this);

  wxBoxSizer *itemBoxSizer04 = new wxBoxSizer(wxVERTICAL);
  itemStaticBoxSizer03->Add(itemBoxSizer04, 0, wxALIGN_TOP | wxALL,
                            border_size);
  m_pButtonAdd = new wxButton(m_pPanelDashboard, wxID_ANY, _("Add"),
                              wxDefaultPosition, wxSize(-1, -1));
  itemBoxSizer04->Add(m_pButtonAdd, 0, wxEXPAND | wxALL, border_size);
  m_pButtonAdd->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentAdd), NULL,
      this);

  // TODO  Instrument Properties ... done by Bernd Cirotzki
  m_pButtonEdit = new wxButton(m_pPanelDashboard, wxID_ANY, _("Edit"),
                               wxDefaultPosition, wxDefaultSize);
  itemBoxSizer04->Add(m_pButtonEdit, 0, wxEXPAND | wxALL, border_size);
  m_pButtonEdit->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentEdit), NULL,
      this);

  //    m_pFontPickerTitle =
  //        new wxFontPickerCtrl(m_pPanelDashboard, wxID_ANY, g_USFontTitle,
  //            wxDefaultPosition, wxSize(-1, -1), 0,
  //            wxDefaultValidator, "Bernd");
  //    itemBoxSizer04->Add(m_pFontPickerTitle, 0, wxALIGN_RIGHT | wxALL, 0);
  //    wxColor Farbe = m_pFontPickerTitle->GetSelectedColour();
  //    m_pFontPickerTitle->SetBackgroundColour(Farbe);

  m_pButtonDelete = new wxButton(m_pPanelDashboard, wxID_ANY, _("Delete"),
                                 wxDefaultPosition, wxSize(-1, -1));
  itemBoxSizer04->Add(m_pButtonDelete, 0, wxEXPAND | wxALL, border_size);
  m_pButtonDelete->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentDelete),
      NULL, this);
  itemBoxSizer04->AddSpacer(10);
  m_pButtonUp = new wxButton(m_pPanelDashboard, wxID_ANY, _("Up"),
                             wxDefaultPosition, wxDefaultSize);
  itemBoxSizer04->Add(m_pButtonUp, 0, wxEXPAND | wxALL, border_size);
  m_pButtonUp->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentUp), NULL,
      this);
  m_pButtonDown = new wxButton(m_pPanelDashboard, wxID_ANY, _("Down"),
                               wxDefaultPosition, wxDefaultSize);
  itemBoxSizer04->Add(m_pButtonDown, 0, wxEXPAND | wxALL, border_size);
  m_pButtonDown->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnInstrumentDown), NULL,
      this);

  wxPanel *itemPanelNotebook02 =
      new wxPanel(itemNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                  wxTAB_TRAVERSAL);
  wxBoxSizer *itemBoxSizer05 = new wxBoxSizer(wxVERTICAL);
  itemPanelNotebook02->SetSizer(itemBoxSizer05);
  itemNotebook->AddPage(itemPanelNotebook02, _("Appearance"));

  wxStaticBox *itemStaticBox01 =
      new wxStaticBox(itemPanelNotebook02, wxID_ANY, _("Fonts"));
  wxStaticBoxSizer *itemStaticBoxSizer01 =
      new wxStaticBoxSizer(itemStaticBox01, wxHORIZONTAL);
  itemBoxSizer05->Add(itemStaticBoxSizer01, 0, wxEXPAND | wxALL, border_size);
  wxFlexGridSizer *itemFlexGridSizer03 = new wxFlexGridSizer(2);
  itemFlexGridSizer03->AddGrowableCol(1);
  itemStaticBoxSizer01->Add(itemFlexGridSizer03, 1, wxEXPAND | wxALL, 0);

  wxStaticText *itemStaticText04 =
      new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Title:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer03->Add(itemStaticText04, 0, wxEXPAND | wxALL, border_size);

  m_pFontPickerTitle =
      new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, g_USFontTitle,
                           wxDefaultPosition, wxDefaultSize);
  itemFlexGridSizer03->Add(m_pFontPickerTitle, 0, wxALIGN_RIGHT | wxALL, 0);

  wxStaticText *itemStaticText05 =
      new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Data:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer03->Add(itemStaticText05, 0, wxEXPAND | wxALL, border_size);
  m_pFontPickerData =
      new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, g_USFontData,
                           wxDefaultPosition, wxDefaultSize);
  itemFlexGridSizer03->Add(m_pFontPickerData, 0, wxALIGN_RIGHT | wxALL, 0);

  wxStaticText *itemStaticText06 =
      new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Label:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer03->Add(itemStaticText06, 0, wxEXPAND | wxALL, border_size);
  m_pFontPickerLabel =
      new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, g_USFontLabel,
                           wxDefaultPosition, wxDefaultSize);
  itemFlexGridSizer03->Add(m_pFontPickerLabel, 0, wxALIGN_RIGHT | wxALL, 0);

  wxStaticText *itemStaticText07 =
      new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Small:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer03->Add(itemStaticText07, 0, wxEXPAND | wxALL, border_size);
  m_pFontPickerSmall =
      new wxFontPickerCtrl(itemPanelNotebook02, wxID_ANY, g_USFontSmall,
                           wxDefaultPosition, wxDefaultSize);
  itemFlexGridSizer03->Add(m_pFontPickerSmall, 0, wxALIGN_RIGHT | wxALL, 0);
  //      wxColourPickerCtrl

  wxStaticText *itemStaticText80 =
      new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Reset:"),
                       wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer03->Add(itemStaticText80, 0, wxEXPAND | wxALL, border_size);

  m_pButtondefaultFont = new wxButton(itemPanelNotebook02, wxID_ANY,
                                      _("Set dashboard default fonts"),
                                      wxDefaultPosition, wxSize(-1, -1));
  itemFlexGridSizer03->Add(m_pButtondefaultFont, 0, wxALIGN_RIGHT | wxALL, 0);

  wxStaticText* itemStaticTextHighContrast = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Use high contrast colours"),
	  wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer03->Add(itemStaticTextHighContrast, 0, wxEXPAND | wxALL, border_size);
  m_pCheckBoxHighContrast = new wxCheckBox(itemPanelNotebook02, wxID_ANY, _("High Contrast"),
	  wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
  m_pCheckBoxHighContrast->SetValue(g_highContrast);

  itemFlexGridSizer03->Add(m_pCheckBoxHighContrast, 0, wxALIGN_RIGHT | wxALL, 0);

  m_pButtondefaultFont->Connect(
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(DashboardPreferencesDialog::OnDashboarddefaultFont),
      NULL, this);

  wxStaticBox *itemStaticBox04 = new wxStaticBox(itemPanelNotebook02, wxID_ANY,
                                                 _("Units, Ranges, Formats"));
  wxStaticBoxSizer *itemStaticBoxSizer04 =
      new wxStaticBoxSizer(itemStaticBox04, wxHORIZONTAL);
  itemBoxSizer05->Add(itemStaticBoxSizer04, 0, wxEXPAND | wxALL, border_size);
  wxFlexGridSizer *itemFlexGridSizer04 = new wxFlexGridSizer(2);
  itemFlexGridSizer04->AddGrowableCol(1);
  itemStaticBoxSizer04->Add(itemFlexGridSizer04, 1, wxEXPAND | wxALL, 0);

  // Sets the maximum RPM in the tachometer control
  wxStaticText* itemStaticTextTachometerM = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Tachometer Maximum RPM:"),
	  wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer04->Add(itemStaticTextTachometerM, 0, wxEXPAND | wxALL, border_size);
  m_pSpinSpeedMax = new wxSpinCtrl(itemPanelNotebook02, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, g_tachometerMax);
  itemFlexGridSizer04->Add(m_pSpinSpeedMax, 0, wxALIGN_RIGHT | wxALL, 0);

  // Enable the user to specify the temperature display in Celsius or Fahrenheit
  wxStaticText* itemStaticTextTemperatureU = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Temperature units:"),
	  wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer04->Add(itemStaticTextTemperatureU, 0, wxEXPAND | wxALL, border_size);
  wxString m_TemperatureUnitChoices[] = { _("Celsius"), _("Fahrenheit") };
  int m_TemperatureUnitNChoices = sizeof(m_TemperatureUnitChoices) / sizeof(wxString);
  m_pChoiceTemperatureUnit = new wxChoice(itemPanelNotebook02, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TemperatureUnitNChoices, m_TemperatureUnitChoices, 0);
  m_pChoiceTemperatureUnit->SetSelection(g_temperatureUnit);
  itemFlexGridSizer04->Add(m_pChoiceTemperatureUnit, 0, wxALIGN_RIGHT | wxALL, 0);

  // Enable the user to specify the engine oil pressure display in Bar or PSI
  wxStaticText* itemStaticTextPressureU = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Pressure units:"),
	  wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer04->Add(itemStaticTextPressureU, 0, wxEXPAND | wxALL, border_size);
  wxString m_PressureUnitChoices[] = { _("Bar"), _("PSI") };
  int m_PressureUnitNChoices = sizeof(m_PressureUnitChoices) / sizeof(wxString);
  m_pChoicePressureUnit = new wxChoice(itemPanelNotebook02, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PressureUnitNChoices, m_PressureUnitChoices, 0);
  m_pChoicePressureUnit->SetSelection(g_pressureUnit);
  itemFlexGridSizer04->Add(m_pChoicePressureUnit, 0, wxALIGN_RIGHT | wxALL, 0);

  // Enable the user to specify volumes in litres or gallons
  wxStaticText* itemStaticTextVolumeU = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Volume units:"),
	  wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer04->Add(itemStaticTextVolumeU, 0, wxEXPAND | wxALL, border_size);
  wxString m_VolumeUnitChoices[] = { _("Litres"), _("Gallons") };
  int m_VolumeUnitNChoices = sizeof(m_VolumeUnitChoices) / sizeof(wxString);
  m_pChoiceVolumeUnit = new wxChoice(itemPanelNotebook02, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_VolumeUnitNChoices, m_VolumeUnitChoices, 0);
  m_pChoiceVolumeUnit->SetSelection(g_volumeUnit);
  itemFlexGridSizer04->Add(m_pChoiceVolumeUnit, 0, wxALIGN_RIGHT | wxALL, 0);

  wxStaticText* itemStaticTwentyFourVolts = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("Enable 24 volt range for voltmeter. Unchecked defaults to 12 volt:"),
	  wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer04->Add(itemStaticTwentyFourVolts, 0, wxEXPAND | wxALL, border_size);
  m_pCheckBoxTwentyFourVolts = new wxCheckBox(itemPanelNotebook02, wxID_ANY, _("24 volt DC"),
	  wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
  m_pCheckBoxTwentyFourVolts->SetValue(g_twentyFourVolts);
  itemFlexGridSizer04->Add(m_pCheckBoxTwentyFourVolts, 0, wxALIGN_RIGHT | wxALL, 0);

  wxStaticText* itemStaticTextDualEngine = new wxStaticText(itemPanelNotebook02, wxID_ANY, _("For dual engines, instance 0 is the port engine\nand instance 1 is the starboard engine.\nFor single engines, instance 0 is the main engine."),
	  wxDefaultPosition, wxDefaultSize, 0);
  itemFlexGridSizer04->Add(itemStaticTextDualEngine, 0, wxEXPAND | wxALL, border_size);
  m_pCheckBoxDualengine = new wxCheckBox(itemPanelNotebook02, wxID_ANY, _("Dual Engine Vessel"),
	  wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
  m_pCheckBoxDualengine->SetValue(g_dualEngine);
  itemFlexGridSizer04->Add(m_pCheckBoxDualengine, 0, wxALIGN_RIGHT | wxALL, 0);
     
  curSel = -1;
  for (size_t i = 0; i < m_Config.GetCount(); i++) {
    m_pListCtrlDashboards->InsertItem(i, 0);
    // Using data to store m_Config index for managing deletes
    m_pListCtrlDashboards->SetItemData(i, i);
  }
  m_pListCtrlDashboards->SetColumnWidth(0, wxLIST_AUTOSIZE);

  m_pListCtrlDashboards->SetItemState(0, wxLIST_STATE_SELECTED,
                                      wxLIST_STATE_SELECTED);
  curSel = 0;

  UpdateDashboardButtonsState();
  UpdateButtonsState();

  // SetMinSize(wxSize(400, -1));

  Fit();

  // Constrain size on small displays
  SetMaxSize(wxSize(display_width, display_height));

  wxSize canvas_size = GetOCPNCanvasWindow()->GetSize();
  if (display_height < 600) {
    if (g_dashPrefWidth > 0 && g_dashPrefHeight > 0)
      SetSize(wxSize(g_dashPrefWidth, g_dashPrefHeight));
    else
      SetSize(wxSize(canvas_size.x * 8 / 10, canvas_size.y * 8 / 10));
  } else {
    if (g_dashPrefWidth > 0 && g_dashPrefHeight > 0)
      SetSize(wxSize(g_dashPrefWidth, g_dashPrefHeight));
    else
      SetSize(wxSize(canvas_size.x * 3 / 4, canvas_size.y * 8 / 10));
  }

  Layout();
  CentreOnScreen();
}

void DashboardPreferencesDialog::RecalculateSize(void) {
#ifdef __OCPN__ANDROID__
  wxSize esize;
  esize.x = GetCharWidth() * 110;
  esize.y = GetCharHeight() * 40;

  wxSize dsize = GetOCPNCanvasWindow()->GetClientSize();
  esize.y = wxMin(esize.y, dsize.y - (3 * GetCharHeight()));
  esize.x = wxMin(esize.x, dsize.x - (3 * GetCharHeight()));
  SetSize(esize);

  CentreOnScreen();
#endif
}

void DashboardPreferencesDialog::OnCloseDialog(wxCloseEvent &event) {
  g_dashPrefWidth = GetSize().x;
  g_dashPrefHeight = GetSize().y;
  SaveDashboardConfig();
  event.Skip();
}

void DashboardPreferencesDialog::SaveDashboardConfig() {
  
  	g_tachometerMax = m_pSpinSpeedMax->GetValue();
	g_temperatureUnit = m_pChoiceTemperatureUnit->GetSelection();
	g_pressureUnit = m_pChoicePressureUnit->GetSelection();
	g_dualEngine = m_pCheckBoxDualengine->IsChecked();
	g_twentyFourVolts = m_pCheckBoxTwentyFourVolts->IsChecked();
	g_highContrast = m_pCheckBoxHighContrast->IsChecked();
	g_volumeUnit = m_pChoiceVolumeUnit->GetSelection();

  if (curSel != -1) {
    DashboardWindowContainer *cont = m_Config.Item(curSel);
    cont->m_bIsVisible = m_pCheckBoxIsVisible->IsChecked();
    cont->m_sCaption = m_pTextCtrlCaption->GetValue();
    cont->m_sOrientation =
        m_pChoiceOrientation->GetSelection() == 0 ? "V" : "H";
    cont->m_aInstrumentList.Clear();
    for (int i = 0; i < m_pListCtrlInstruments->GetItemCount(); i++)
      cont->m_aInstrumentList.Add((int)m_pListCtrlInstruments->GetItemData(i));
  }
}

void DashboardPreferencesDialog::OnDashboardSelected(wxListEvent &event) {
  // save changes
  SaveDashboardConfig();
  UpdateDashboardButtonsState();
}

void DashboardPreferencesDialog::UpdateDashboardButtonsState() {
  long item = -1;
  item = m_pListCtrlDashboards->GetNextItem(item, wxLIST_NEXT_ALL,
                                            wxLIST_STATE_SELECTED);
  bool enable = (item != -1);

  //  Disable the Dashboard Delete button if the parent(Dashboard) of this
  //  dialog is selected.
  bool delete_enable = enable;
  if (item != -1) {
    int sel = m_pListCtrlDashboards->GetItemData(item);
    DashboardWindowContainer *cont = m_Config.Item(sel);
    DashboardWindow *dash_sel = cont->m_pDashboardWindow;
    if (dash_sel == GetParent()) delete_enable = false;
  }
  m_pButtonDeleteDashboard->Enable(delete_enable);

  m_pPanelDashboard->Enable(enable);

  if (item != -1) {
    curSel = m_pListCtrlDashboards->GetItemData(item);
    DashboardWindowContainer *cont = m_Config.Item(curSel);
    m_pCheckBoxIsVisible->SetValue(cont->m_bIsVisible);
    m_pTextCtrlCaption->SetValue(cont->m_sCaption);
    m_pChoiceOrientation->SetSelection(cont->m_sOrientation == "V" ? 0 : 1);
    m_pListCtrlInstruments->DeleteAllItems();
    for (size_t i = 0; i < cont->m_aInstrumentList.GetCount(); i++) {
      wxListItem item;
      GetListItemForInstrument(item, cont->m_aInstrumentList.Item(i));
      item.SetId(m_pListCtrlInstruments->GetItemCount());
      m_pListCtrlInstruments->InsertItem(item);
    }

    m_pListCtrlInstruments->SetColumnWidth(0, wxLIST_AUTOSIZE);
  } else {
    curSel = -1;
    m_pCheckBoxIsVisible->SetValue(false);
    m_pTextCtrlCaption->SetValue("");
    m_pChoiceOrientation->SetSelection(0);
    m_pListCtrlInstruments->DeleteAllItems();
  }
  //      UpdateButtonsState();
}

void DashboardPreferencesDialog::OnDashboarddefaultFont(wxCommandEvent &event) {
  m_pFontPickerTitle->SetSelectedFont(
      wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL));
  m_pFontPickerTitle->SetSelectedColour(wxColour(0, 0, 0));
  m_pFontPickerData->SetSelectedFont(
      wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
  m_pFontPickerData->SetSelectedColour(wxColour(0, 0, 0));
  m_pFontPickerLabel->SetSelectedFont(
      wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
  m_pFontPickerLabel->SetSelectedColour(wxColour(0, 0, 0));
  m_pFontPickerSmall->SetSelectedFont(
      wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
  m_pFontPickerSmall->SetSelectedColour(wxColour(0, 0, 0));
  double scaler = 1.0;
  if (OCPN_GetWinDIPScaleFactor() < 1.0)
    scaler = 1.0 + OCPN_GetWinDIPScaleFactor() / 4;
  scaler = wxMax(1.0, scaler);

  g_USFontTitle = *(m_pFontPickerTitle->GetFontData());
  g_FontTitle = *g_pUSFontTitle;
  g_FontTitle.SetChosenFont(g_pUSFontTitle->GetChosenFont().Scaled(scaler));
  g_FontTitle.SetColour(g_pUSFontTitle->GetColour());
  g_USFontTitle = *g_pUSFontTitle;

  g_USFontData = *(m_pFontPickerData->GetFontData());
  g_FontData = *g_pUSFontData;
  g_FontData.SetChosenFont(g_pUSFontData->GetChosenFont().Scaled(scaler));
  g_FontData.SetColour(g_pUSFontData->GetColour());
  g_USFontData = *g_pUSFontData;

  g_USFontLabel = *(m_pFontPickerLabel->GetFontData());
  g_FontLabel = *g_pUSFontLabel;
  g_FontLabel.SetChosenFont(g_pUSFontLabel->GetChosenFont().Scaled(scaler));
  g_FontLabel.SetColour(g_pUSFontLabel->GetColour());
  g_USFontLabel = *g_pUSFontLabel;

  g_USFontSmall = *(m_pFontPickerSmall->GetFontData());
  g_FontSmall = *g_pUSFontSmall;
  g_FontSmall.SetChosenFont(g_pUSFontSmall->GetChosenFont().Scaled(scaler));
  g_FontSmall.SetColour(g_pUSFontSmall->GetColour());
  g_USFontSmall = *g_pUSFontSmall;
}

void DashboardPreferencesDialog::OnDashboardAdd(wxCommandEvent &event) {
  int idx = m_pListCtrlDashboards->GetItemCount();
  m_pListCtrlDashboards->InsertItem(idx, 0);
  // Data is index in m_Config
  m_pListCtrlDashboards->SetItemData(idx, m_Config.GetCount());
  wxArrayInt ar;
  wxArrayOfInstrumentProperties Property;
  DashboardWindowContainer *dwc = new DashboardWindowContainer(
      NULL, MakeName(), _("Instruments"), "V", ar, Property);
  dwc->m_bIsVisible = true;
  m_Config.Add(dwc);
}

void DashboardPreferencesDialog::OnDashboardDelete(wxCommandEvent &event) {
  long itemID = -1;
  itemID = m_pListCtrlDashboards->GetNextItem(itemID, wxLIST_NEXT_ALL,
                                              wxLIST_STATE_SELECTED);

  int idx = m_pListCtrlDashboards->GetItemData(itemID);
  m_pListCtrlDashboards->DeleteItem(itemID);
  m_Config.Item(idx)->m_bIsDeleted = true;
  UpdateDashboardButtonsState();
}

void DashboardPreferencesDialog::OnInstrumentSelected(wxListEvent &event) {
  UpdateButtonsState();
}

void DashboardPreferencesDialog::UpdateButtonsState() {
  long item = -1;
  item = m_pListCtrlInstruments->GetNextItem(item, wxLIST_NEXT_ALL,
                                             wxLIST_STATE_SELECTED);
  bool enable = (item != -1);

  m_pButtonDelete->Enable(enable);
  m_pButtonEdit->Enable(enable);  // TODO: Properties ... done Bernd Cirotzki
  m_pButtonUp->Enable(item > 0);
  m_pButtonDown->Enable(item != -1 &&
                        item < m_pListCtrlInstruments->GetItemCount() - 1);
}

void DashboardPreferencesDialog::OnInstrumentAdd(wxCommandEvent &event) {
  AddInstrumentDlg pdlg((wxWindow *)event.GetEventObject(), wxID_ANY);

#ifdef __OCPN__ANDROID__
  wxFont *pF = OCPNGetFont(_("Dialog"));
  pdlg.SetFont(*pF);

  wxSize esize;
  esize.x = GetCharWidth() * 110;
  esize.y = GetCharHeight() * 40;

  wxSize dsize = GetOCPNCanvasWindow()->GetClientSize();
  esize.y = wxMin(esize.y, dsize.y - (3 * GetCharHeight()));
  esize.x = wxMin(esize.x, dsize.x - (3 * GetCharHeight()));
  pdlg.SetSize(esize);

  pdlg.CentreOnScreen();
#endif
  pdlg.ShowModal();
  if (pdlg.GetReturnCode() == wxID_OK) {
    wxListItem item;
    GetListItemForInstrument(item, pdlg.GetInstrumentAdded());
    item.SetId(m_pListCtrlInstruments->GetItemCount());
    m_pListCtrlInstruments->InsertItem(item);
    m_pListCtrlInstruments->SetColumnWidth(0, wxLIST_AUTOSIZE);
    UpdateButtonsState();
  }
}

void DashboardPreferencesDialog::OnInstrumentDelete(wxCommandEvent &event) {
  long itemIDWindow = -1;
  itemIDWindow = m_pListCtrlDashboards->GetNextItem(
      itemIDWindow, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  long itemID = -1;
  itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED);
  DashboardWindowContainer *cont =
      m_Config.Item(m_pListCtrlDashboards->GetItemData(itemIDWindow));
  InstrumentProperties *InstDel = NULL;
  if (cont) {
    InstrumentProperties *Inst = NULL;
    for (unsigned int i = 0; i < (cont->m_aInstrumentPropertyList.GetCount());
         i++) {
      Inst = cont->m_aInstrumentPropertyList.Item(i);
      if (Inst->m_aInstrument ==
              (int)m_pListCtrlInstruments->GetItemData(itemID) &&
          Inst->m_Listplace == itemID) {
        cont->m_aInstrumentPropertyList.Remove(Inst);
        InstDel = Inst;
        break;
      } else {
        if (Inst->m_Listplace > itemID) Inst->m_Listplace--;
      }
    }
  }
  m_pListCtrlInstruments->DeleteItem(itemID);
  if (InstDel) {
    cont->m_pDashboardWindow->SetInstrumentList(
        cont->m_aInstrumentList, &(cont->m_aInstrumentPropertyList));
    delete InstDel;
  }
  UpdateButtonsState();
}

inline void GetFontData(OCPNFontButton *FontButton, wxFontData &UnScaledFont,
                        wxFontData &ScaledFont, double scaler) {
  UnScaledFont = *(FontButton->GetFontData());
  ScaledFont = UnScaledFont;
  ScaledFont.SetChosenFont(UnScaledFont.GetChosenFont().Scaled(scaler));
}

void DashboardPreferencesDialog::OnInstrumentEdit(wxCommandEvent &event) {
  // TODO: Instument options
  //  m_Config = Arrayofdashboardwindows.
  long itemIDWindow = -1;
  itemIDWindow = m_pListCtrlDashboards->GetNextItem(
      itemIDWindow, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  long itemID = -1;
  itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED);
  // m_Config.
  //  curSel = m_pListCtrlDashboards->GetItemData(itemWindow);
  //  DashboardWindowContainer *cont = m_Config.Item(curSel);
  //  if (cont) ....
  DashboardWindowContainer *cont =
      m_Config.Item(m_pListCtrlDashboards->GetItemData(itemIDWindow));
  if (!cont) return;
  InstrumentProperties *Inst = NULL;
  for (unsigned int i = 0; i < (cont->m_aInstrumentPropertyList.GetCount());
       i++) {
    Inst = cont->m_aInstrumentPropertyList.Item(
        i);  // m_pListCtrlInstruments->GetItemData(itemID)
    if (Inst->m_aInstrument == (int)m_pListCtrlInstruments->GetItemData(
                                   itemID))  // Is for right Instrumenttype.
    {
      if (Inst->m_Listplace == itemID) break;
    }
    Inst = NULL;
  }
  if (!Inst) {
    Inst = new InstrumentProperties(m_pListCtrlInstruments->GetItemData(itemID),
                                    itemID);
    cont->m_aInstrumentPropertyList.Add(Inst);
  }
  EditDialog *Edit = new EditDialog(this, *Inst, wxID_ANY);
  Edit->Fit();
  bool DefaultFont = false;
  if (Edit->ShowModal() == wxID_OK) {
    DefaultFont = true;
    double scaler = 1.0;
    if (OCPN_GetWinDIPScaleFactor() < 1.0)
      scaler = 1.0 + OCPN_GetWinDIPScaleFactor() / 4;
    scaler = wxMax(1.0, scaler);
    if (Edit->m_fontPicker2->GetFont().Scaled(scaler) !=
            g_FontTitle.GetChosenFont() ||
        Edit->m_fontPicker2->GetSelectedColour() != g_FontTitle.GetColour())
      DefaultFont = false;
    if (Edit->m_fontPicker4->GetFont().Scaled(scaler) !=
            g_FontData.GetChosenFont() ||
        Edit->m_fontPicker4->GetSelectedColour() != g_FontData.GetColour())
      DefaultFont = false;
    if (Edit->m_fontPicker5->GetFont().Scaled(scaler) !=
            g_FontLabel.GetChosenFont() ||
        Edit->m_fontPicker5->GetSelectedColour() != g_FontLabel.GetColour())
      DefaultFont = false;
    if (Edit->m_fontPicker6->GetFont().Scaled(scaler) !=
            g_FontSmall.GetChosenFont() ||
        Edit->m_fontPicker6->GetSelectedColour() != g_FontSmall.GetColour())
      DefaultFont = false;
    wxColour dummy;
    GetGlobalColor("DASHL", &dummy);
    if (Edit->m_colourPicker1->GetColour() != dummy) DefaultFont = false;
    GetGlobalColor("DASHB", &dummy);
    if (Edit->m_colourPicker2->GetColour() != dummy) DefaultFont = false;
    GetGlobalColor("DASHN", &dummy);
    if (Edit->m_colourPicker3->GetColour() != dummy) DefaultFont = false;
    GetGlobalColor("BLUE3", &dummy);
    if (Edit->m_colourPicker4->GetColour() != dummy) DefaultFont = false;
    if (DefaultFont)
      cont->m_aInstrumentPropertyList.Remove(Inst);
    else {
      GetFontData(Edit->m_fontPicker2, Inst->m_USTitleFont, Inst->m_TitleFont,
                  scaler);
      GetFontData(Edit->m_fontPicker4, Inst->m_USDataFont, Inst->m_DataFont,
                  scaler);
      GetFontData(Edit->m_fontPicker5, Inst->m_USLabelFont, Inst->m_LabelFont,
                  scaler);
      GetFontData(Edit->m_fontPicker6, Inst->m_USSmallFont, Inst->m_SmallFont,
                  scaler);
      Inst->m_DataBackgroundColour = Edit->m_colourPicker2->GetColour();
      Inst->m_TitleBackgroundColour = Edit->m_colourPicker1->GetColour();
      Inst->m_Arrow_First_Colour = Edit->m_colourPicker3->GetColour();
      Inst->m_Arrow_Second_Colour = Edit->m_colourPicker4->GetColour();
    }
  }
  delete Edit;
  if (cont->m_pDashboardWindow) {
    cont->m_pDashboardWindow->SetInstrumentList(
        cont->m_aInstrumentList, &(cont->m_aInstrumentPropertyList));
  }
  if (DefaultFont) delete Inst;
}

void DashboardPreferencesDialog::OnInstrumentUp(wxCommandEvent &event) {
  long itemIDWindow = -1;
  itemIDWindow = m_pListCtrlDashboards->GetNextItem(
      itemIDWindow, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  long itemID = -1;
  itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED);
  wxListItem item;
  item.SetId(itemID);
  item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_DATA);
  m_pListCtrlInstruments->GetItem(item);
  item.SetId(itemID - 1);
  // item.SetImage(0);           // image 0, by default
  // Now see if the Old itemId has an own Fontdata
  DashboardWindowContainer *cont =
      m_Config.Item(m_pListCtrlDashboards->GetItemData(itemIDWindow));
  if (cont) {
    InstrumentProperties *Inst = NULL;
    for (unsigned int i = 0; i < (cont->m_aInstrumentPropertyList.GetCount());
         i++) {
      Inst = cont->m_aInstrumentPropertyList.Item(i);
      if (Inst->m_Listplace == (itemID - 1)) Inst->m_Listplace = itemID;
      if (Inst->m_aInstrument ==
              (int)m_pListCtrlInstruments->GetItemData(itemID) &&
          Inst->m_Listplace == itemID) {
        cont->m_aInstrumentPropertyList.Item(i)->m_Listplace = itemID - 1;
      }
    }
  }
  m_pListCtrlInstruments->DeleteItem(itemID);
  m_pListCtrlInstruments->InsertItem(item);
  for (int i = 0; i < m_pListCtrlInstruments->GetItemCount(); i++)
    m_pListCtrlInstruments->SetItemState(i, 0, wxLIST_STATE_SELECTED);

  m_pListCtrlInstruments->SetItemState(itemID - 1, wxLIST_STATE_SELECTED,
                                       wxLIST_STATE_SELECTED);

  UpdateButtonsState();
}

void DashboardPreferencesDialog::OnInstrumentDown(wxCommandEvent &event) {
  long itemIDWindow = -1;
  itemIDWindow = m_pListCtrlDashboards->GetNextItem(
      itemIDWindow, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  long itemID = -1;
  itemID = m_pListCtrlInstruments->GetNextItem(itemID, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED);

  wxListItem item;
  item.SetId(itemID);
  item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_DATA);
  m_pListCtrlInstruments->GetItem(item);
  item.SetId(itemID + 1);
  // item.SetImage(0);           // image 0, by default
  // Now see if the Old itemId has an own Fontdata
  DashboardWindowContainer *cont =
      m_Config.Item(m_pListCtrlDashboards->GetItemData(itemIDWindow));
  if (cont) {
    InstrumentProperties *Inst = NULL;
    for (unsigned int i = 0; i < (cont->m_aInstrumentPropertyList.GetCount());
         i++) {
      Inst = cont->m_aInstrumentPropertyList.Item(i);
      if (Inst->m_Listplace == (itemID + 1) &&
          Inst->m_aInstrument !=
              (int)m_pListCtrlInstruments->GetItemData(itemID))
        Inst->m_Listplace = itemID;
      if (Inst->m_aInstrument ==
              (int)m_pListCtrlInstruments->GetItemData(itemID) &&
          Inst->m_Listplace == itemID) {
        cont->m_aInstrumentPropertyList.Item(i)->m_Listplace = itemID + 1;
        break;
      }
    }
  }
  m_pListCtrlInstruments->DeleteItem(itemID);
  m_pListCtrlInstruments->InsertItem(item);
  for (int i = 0; i < m_pListCtrlInstruments->GetItemCount(); i++)
    m_pListCtrlInstruments->SetItemState(i, 0, wxLIST_STATE_SELECTED);

  m_pListCtrlInstruments->SetItemState(itemID + 1, wxLIST_STATE_SELECTED,
                                       wxLIST_STATE_SELECTED);

  UpdateButtonsState();
}
