#include <wx/apptrait.h>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/bookctrl.h>
#include <wx/artprov.h>
#include <wx/imaglist.h>
#include <wx/minifram.h>
#include <wx/sysopt.h>
#include <wx/notifmsg.h>
#include <wx/generic/notifmsg.h>
#include <wx/modalhook.h>
#include <wx/sizer.h>
#include "Dialogs.h"

wxBEGIN_EVENT_TABLE(AddItemDialog, wxDialog)
EVT_BUTTON(wxID_APPLY, AddItemDialog::OnApply)
EVT_BUTTON(wxID_CLOSE, AddItemDialog::OnClose)
wxEND_EVENT_TABLE()

AddItemDialog::AddItemDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Message Box Test Dialog",
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
}

bool AddItemDialog::Create()
{
    wxSizer* const sizerTop = new wxBoxSizer(wxVERTICAL);

    // this sizer allows to configure the messages shown in the message box
    wxSizer* const
        sizerMsgs = new wxStaticBoxSizer(wxVERTICAL, this, "&Messages");
    sizerMsgs->Add(new wxStaticText(this, wxID_ANY, "&Title:"));
    m_textTitle = new wxTextCtrl(this, wxID_ANY, "Test Message Box");
    sizerMsgs->Add(m_textTitle, wxSizerFlags().Expand().Border(wxBOTTOM));

    sizerMsgs->Add(new wxStaticText(this, wxID_ANY, "&Main message:"));
    m_textMsg = new wxTextCtrl(this, wxID_ANY, "Hello from a box!",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE);
    sizerMsgs->Add(m_textMsg, wxSizerFlags(1).Expand().Border(wxBOTTOM));

    sizerMsgs->Add(new wxStaticText(this, wxID_ANY, "&Extended message:"));
    m_textExtMsg = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE);
    sizerMsgs->Add(m_textExtMsg, wxSizerFlags().Expand());

    sizerTop->Add(sizerMsgs, wxSizerFlags(1).Expand().Border());

    // if a derived class provides more message configurations, add these.
    AddAdditionalTextOptions(sizerTop);

    // this one is for configuring the buttons
    wxSizer* const
        sizerBtnsBox = new wxStaticBoxSizer(wxVERTICAL, this, "&Buttons");

    wxFlexGridSizer* const sizerBtns = new wxFlexGridSizer(2, 5, 5);
    sizerBtns->AddGrowableCol(1);

    sizerBtns->Add(new wxStaticText(this, wxID_ANY, "Button(s)"));
    sizerBtns->Add(new wxStaticText(this, wxID_ANY, "Custom label"));

    for (int n = 0; n < Btn_Max; n++)
    {
        // m_buttons[n] = new wxCheckBox(this, wxID_ANY, ms_btnInfo[n].name);
        // sizerBtns->Add(m_buttons[n], wxSizerFlags().Centre().Left());

        m_labels[n] = new wxTextCtrl(this, wxID_ANY);
        sizerBtns->Add(m_labels[n], wxSizerFlags().Expand());
    }

    sizerBtnsBox->Add(sizerBtns, wxSizerFlags().Expand());
    sizerTop->Add(sizerBtnsBox, wxSizerFlags().Expand().Border());


    // icon choice
    const wxString icons[] =
    {
        "&Not specified",
        "E&xplicitly none",
        "&Information icon",
        "&Question icon",
        "&Warning icon",
        "&Error icon",
        "A&uth needed icon"
    };

    wxCOMPILE_TIME_ASSERT(WXSIZEOF(icons) == MsgDlgIcon_Max, IconMismatch);

    m_icons = new wxRadioBox(this, wxID_ANY, "&Icon style",
        wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(icons), icons,
        2, wxRA_SPECIFY_ROWS);
    // Make the 'Information' icon the default one:
    m_icons->SetSelection(MsgDlgIcon_Info);
    sizerTop->Add(m_icons, wxSizerFlags().Expand().Border());


    // miscellaneous other stuff
    wxSizer* const
        sizerFlags = new wxStaticBoxSizer(wxHORIZONTAL, this, "&Other flags");

    m_chkNoDefault = new wxCheckBox(this, wxID_ANY, "Make \"No\" &default");
    sizerFlags->Add(m_chkNoDefault, wxSizerFlags().Border());

    m_chkCentre = new wxCheckBox(this, wxID_ANY, "Centre on &parent");
    sizerFlags->Add(m_chkCentre, wxSizerFlags().Border());

    // add any additional flag from subclasses
    AddAdditionalFlags(sizerFlags);

    sizerTop->Add(sizerFlags, wxSizerFlags().Expand().Border());

    // add the currently unused zone for displaying the dialog result
    m_labelResult = new wxStaticText(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxST_NO_AUTORESIZE | wxALIGN_CENTRE);
    m_labelResult->SetForegroundColour(*wxBLUE);
    sizerTop->Add(m_labelResult, wxSizerFlags().Expand().DoubleBorder());

    // finally buttons to show the resulting message box and close this dialog
    sizerTop->Add(CreateStdDialogButtonSizer(wxAPPLY | wxCLOSE),
        wxSizerFlags().Right().Border());

    SetSizerAndFit(sizerTop);

    //m_buttons[Btn_Ok]->SetValue(true);

    CentreOnScreen();

    return true;
}

void AddItemDialog::OnApply(wxCommandEvent& WXUNUSED(event))
{
    // wxMessageDialog dlg(this, GetMessage(), GetBoxTitle(), GetStyle());
    // PrepareMessageDialog(dlg);
    // 
    // ShowResult(dlg.ShowModal());
}

void AddItemDialog::OnClose(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}