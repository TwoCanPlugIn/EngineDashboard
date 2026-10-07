#include "dashboard_pi.h"
#include "edit_dialog.h"
EditDialog::EditDialog(wxWindow *parent, InstrumentProperties &Properties,
                       wxWindowID id, const wxString &title, const wxPoint &pos,
                       const wxSize &size, long style)
    : wxDialog(parent, id, title, pos, size, style) {
  this->SetSizeHints(wxDefaultSize, wxDefaultSize);

  wxBoxSizer *bSizer5;
  bSizer5 = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer *fgSizer2;
  fgSizer2 = new wxFlexGridSizer(0, 2, 0, 0);
  fgSizer2->SetFlexibleDirection(wxBOTH);
  fgSizer2->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

  m_staticText1 = new wxStaticText(this, wxID_ANY, _("Title:"),
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText1->Wrap(-1);
  fgSizer2->Add(m_staticText1, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_fontPicker2 = new wxFontPickerCtrl(this, wxID_ANY, Properties.m_USTitleFont,
                                       wxDefaultPosition, wxDefaultSize);
  fgSizer2->Add(m_fontPicker2, 0, wxALL, 5);

  m_staticText5 = new wxStaticText(this, wxID_ANY, _("Title background color:"),
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText5->Wrap(-1);
  fgSizer2->Add(m_staticText5, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_colourPicker1 = new wxColourPickerCtrl(
      this, wxID_ANY, Properties.m_TitleBackgroundColour, wxDefaultPosition,
      wxDefaultSize, wxCLRP_DEFAULT_STYLE);
  fgSizer2->Add(m_colourPicker1, 0, wxALL, 5);

  m_staticText2 = new wxStaticText(this, wxID_ANY, _("Data:"),
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText2->Wrap(-1);
  fgSizer2->Add(m_staticText2, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_fontPicker4 = new wxFontPickerCtrl(this, wxID_ANY, Properties.m_USDataFont,
                                       wxDefaultPosition, wxDefaultSize);
  fgSizer2->Add(m_fontPicker4, 0, wxALL, 5);

  m_staticText6 = new wxStaticText(this, wxID_ANY, _("Data background color:"),
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText6->Wrap(-1);
  fgSizer2->Add(m_staticText6, 0, wxALL, 5);

  m_colourPicker2 = new wxColourPickerCtrl(
      this, wxID_ANY, Properties.m_DataBackgroundColour, wxDefaultPosition,
      wxDefaultSize, wxCLRP_DEFAULT_STYLE);
  fgSizer2->Add(m_colourPicker2, 0, wxALL, 5);

  m_staticText3 = new wxStaticText(this, wxID_ANY, _("Label:"),
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText3->Wrap(-1);
  fgSizer2->Add(m_staticText3, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_fontPicker5 = new wxFontPickerCtrl(this, wxID_ANY, Properties.m_USLabelFont,
                                       wxDefaultPosition, wxDefaultSize);
  fgSizer2->Add(m_fontPicker5, 0, wxALL, 5);

  m_staticText4 = new wxStaticText(this, wxID_ANY, _("Small:"),
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText4->Wrap(-1);
  fgSizer2->Add(m_staticText4, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_fontPicker6 = new wxFontPickerCtrl(this, wxID_ANY, Properties.m_USSmallFont,
                                       wxDefaultPosition, wxDefaultSize);
  fgSizer2->Add(m_fontPicker6, 0, wxALL, 5);

  m_staticText9 = new wxStaticText(this, wxID_ANY, _("Arrow 1 Color :"),
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText9->Wrap(-1);
  fgSizer2->Add(m_staticText9, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_colourPicker3 = new wxColourPickerCtrl(
      this, wxID_ANY, Properties.m_Arrow_First_Colour, wxDefaultPosition,
      wxDefaultSize, wxCLRP_DEFAULT_STYLE);
  fgSizer2->Add(m_colourPicker3, 0, wxALL, 5);

  m_staticText10 = new wxStaticText(this, wxID_ANY, _("Arrow 2 Color :"),
                                    wxDefaultPosition, wxDefaultSize, 0);
  m_staticText10->Wrap(-1);
  fgSizer2->Add(m_staticText10, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_colourPicker4 = new wxColourPickerCtrl(
      this, wxID_ANY, Properties.m_Arrow_Second_Colour, wxDefaultPosition,
      wxDefaultSize, wxCLRP_DEFAULT_STYLE);
  fgSizer2->Add(m_colourPicker4, 0, wxALL, 5);

  m_staticline1 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxLI_HORIZONTAL);
  fgSizer2->Add(m_staticline1, 0, wxEXPAND | wxALL, 5);

  m_staticline2 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxLI_HORIZONTAL);
  fgSizer2->Add(m_staticline2, 0, wxEXPAND | wxALL, 5);

  fgSizer2->Add(0, 5, 1, wxEXPAND, 5);

  fgSizer2->Add(0, 0, 1, wxEXPAND, 5);

  m_staticText7 = new wxStaticText(this, wxID_ANY, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize, 0);
  m_staticText7->Wrap(-1);
  fgSizer2->Add(m_staticText7, 0, wxALL, 5);

  m_button1 = new wxButton(this, wxID_ANY, _("Set default"), wxDefaultPosition,
                           wxDefaultSize, 0);
  fgSizer2->Add(m_button1, 0, wxALL, 5);

  fgSizer2->Add(0, 5, 1, wxEXPAND, 5);

  fgSizer2->Add(5, 0, 1, wxEXPAND, 5);

  bSizer5->Add(fgSizer2, 1, wxALL | wxEXPAND, 5);

  m_sdbSizer3 = new wxStdDialogButtonSizer();
  m_sdbSizer3OK = new wxButton(this, wxID_OK);
  m_sdbSizer3->AddButton(m_sdbSizer3OK);
  m_sdbSizer3Cancel = new wxButton(this, wxID_CANCEL);
  m_sdbSizer3->AddButton(m_sdbSizer3Cancel);
  m_sdbSizer3->Realize();

  bSizer5->Add(m_sdbSizer3, 0, 0, 1);

  bSizer5->Add(0, 10, 0, wxEXPAND, 5);

  this->SetSizer(bSizer5);
  this->Layout();
  bSizer5->Fit(this);

  this->Centre(wxBOTH);

  // Connect Events
  m_button1->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                     wxCommandEventHandler(EditDialog::OnSetdefault), NULL,
                     this);
}

EditDialog::~EditDialog() {
  // Disconnect Events
  m_button1->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED,
                        wxCommandEventHandler(EditDialog::OnSetdefault), NULL,
                        this);
}

void EditDialog::OnSetdefault(wxCommandEvent &event) {
  m_fontPicker2->SetSelectedFont(g_USFontTitle.GetChosenFont());
  m_fontPicker2->SetSelectedColour(g_USFontTitle.GetColour());
  m_fontPicker4->SetSelectedFont(g_USFontData.GetChosenFont());
  m_fontPicker4->SetSelectedColour(g_USFontData.GetColour());
  m_fontPicker5->SetSelectedFont(g_USFontLabel.GetChosenFont());
  m_fontPicker5->SetSelectedColour(g_USFontLabel.GetColour());
  m_fontPicker6->SetSelectedFont(g_USFontSmall.GetChosenFont());
  m_fontPicker6->SetSelectedColour(g_USFontSmall.GetColour());
  wxColour dummy;
  GetGlobalColor("DASHL", &dummy);
  m_colourPicker1->SetColour(dummy);
  GetGlobalColor("DASHB", &dummy);
  m_colourPicker2->SetColour(dummy);
  GetGlobalColor("DASHN", &dummy);
  m_colourPicker3->SetColour(dummy);
  GetGlobalColor("BLUE3", &dummy);
  m_colourPicker4->SetColour(dummy);
  Update();
}
