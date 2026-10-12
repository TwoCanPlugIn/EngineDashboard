#include "ocpn_font_button.h"
#include "ocpn_fontdlg.h"
#include <ocpn_plugin.h>

bool OCPNFontButton::Create(wxWindow *parent, wxWindowID id,
                            const wxFontData &initial, const wxPoint &pos,
                            const wxSize &size, long style,
                            const wxValidator &validator,
                            const wxString &name) {
  wxString label = (style & wxFNTP_FONTDESC_AS_LABEL)
                       ? wxString()
                       :  // label will be updated by UpdateFont
                       _("Choose font");
  label = name;
  // create this button
  if (!wxButton::Create(parent, id, label, pos, size, style, validator, name)) {
    wxFAIL_MSG("OCPNFontButton creation failed");
    return false;
  }

  // and handle user clicks on it
  Connect(GetId(), wxEVT_BUTTON,
          wxCommandEventHandler(OCPNFontButton::OnButtonClick), NULL, this);

  m_data = initial;
  m_selectedFont =
      initial.GetChosenFont().IsOk() ? initial.GetChosenFont() : *wxNORMAL_FONT;
  UpdateFont();

  return true;
}

void OCPNFontButton::OnButtonClick(wxCommandEvent &WXUNUSED(ev)) {
  // update the wxFontData to be shown in the dialog
  m_data.SetInitialFont(m_selectedFont);
  wxFont *pF = OCPNGetFont(_("Dialog"), m_selectedFont.GetPointSize());

#ifdef __WXGTK__
  // Use a smaller font picker dialog (ocpnGenericFontDialog) for small displays
  int display_height = wxGetDisplaySize().y;
  if (display_height < 800) {
    // create the font dialog and display it
    ocpnGenericFontDialog dlg(this, m_data);
    dlg.SetFont(*pF);
    if (dlg.ShowModal() == wxID_OK) {
      m_data = dlg.GetFontData();
      m_selectedFont = m_data.GetChosenFont();
      // fire an event
      wxFontPickerEvent event(this, GetId(), m_selectedFont);
      GetEventHandler()->ProcessEvent(event);
      UpdateFont();
    }
  } else {
    // create the font dialog and display it
    wxFontDialog dlg(this, m_data);
    dlg.SetFont(*pF);
    if (dlg.ShowModal() == wxID_OK) {
      m_data = dlg.GetFontData();
      m_selectedFont = m_data.GetChosenFont();
      // fire an event
      wxFontPickerEvent event(this, GetId(), m_selectedFont);
      GetEventHandler()->ProcessEvent(event);
      UpdateFont();
    }
  }

#else  // Not __GTK__
  // create the font dialog and display it
  wxFontDialog dlg(this, m_data);
  dlg.SetFont(*pF);

#ifdef __WXQT__
  // Make sure that font dialog will fit on the screen without scrolling
  // We do this by setting the dialog font size "small enough" to show "n" lines
  wxSize proposed_size = GetParent()->GetSize();
  float n_lines = 30;
  float font_size = pF->GetPointSize();

  if ((proposed_size.y / font_size) < n_lines) {
    float new_font_size = proposed_size.y / n_lines;
    wxFont *smallFont = new wxFont(*pF);
    smallFont->SetPointSize(new_font_size);
    dlg.SetFont(*smallFont);
  }

  dlg.SetSize(GetParent()->GetSize());
  dlg.Centre();
#endif

  if (dlg.ShowModal() == wxID_OK) {
    m_data = dlg.GetFontData();
    m_selectedFont = m_data.GetChosenFont();

    // fire an event
    wxFontPickerEvent event(this, GetId(), m_selectedFont);
    GetEventHandler()->ProcessEvent(event);

    UpdateFont();
  }
#endif
}

void OCPNFontButton::UpdateFont() {
  if (!m_selectedFont.IsOk()) return;

  //  Leave black, until Instruments are modified to accept color fonts
  // SetForegroundColour(m_data.GetColour());

  if (HasFlag(wxFNTP_USEFONT_FOR_LABEL)) {
    // use currently selected font for the label...
    wxButton::SetFont(m_selectedFont);
    wxButton::SetForegroundColour(GetSelectedColour());
  }

  wxString label =
      wxString::Format("%s, %d", m_selectedFont.GetFaceName().c_str(),
                       m_selectedFont.GetPointSize());

  if (HasFlag(wxFNTP_FONTDESC_AS_LABEL)) {
    SetLabel(label);
  }

  auto minsize = GetTextExtent(label);
  SetSize(minsize);

  GetParent()->Layout();
  GetParent()->Fit();
}

// Edit Dialog

