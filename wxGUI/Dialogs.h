#pragma once
#ifndef __DIALOGSH__
#define __DIALOGSH__

#include <wx/string.h>
#include <wx/modalhook.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
// Maybe not needed ?
#include <wx/checkbox.h>
#include <wx/radiobox.h>

#ifdef __WXUNIVERSAL__
#define USE_WXUNIVERSAL 1
#else
#define USE_WXUNIVERSAL 0
#endif

#ifdef WXUSINGDLL
#define USE_DLL 1
#else
#define USE_DLL 0
#endif

#if defined(__WXMSW__)
#define USE_WXMSW 1
#else
#define USE_WXMSW 0
#endif

#ifdef __WXMAC__
#define USE_WXMAC 1
#else
#define USE_WXMAC 0
#endif

#if USE_NATIVE_FONT_DIALOG_FOR_MACOSX
#define USE_WXMACFONTDLG 1
#else
#define USE_WXMACFONTDLG 0
#endif

#ifdef __WXGTK__
#define USE_WXGTK 1
#else
#define USE_WXGTK 0
#endif

#define USE_GENERIC_DIALOGS (!USE_WXUNIVERSAL && !USE_DLL)

#define USE_COLOURDLG_GENERIC \
    ((USE_WXMSW || USE_WXMAC) && USE_GENERIC_DIALOGS && wxUSE_COLOURDLG)
#define USE_DIRDLG_GENERIC \
    ((USE_WXMSW || USE_WXMAC) && USE_GENERIC_DIALOGS && wxUSE_DIRDLG)
#define USE_FILEDLG_GENERIC \
    ((USE_WXMSW || USE_WXMAC) && USE_GENERIC_DIALOGS  && wxUSE_FILEDLG)
#define USE_FONTDLG_GENERIC \
    ((USE_WXMSW || USE_WXMACFONTDLG) && USE_GENERIC_DIALOGS && wxUSE_FONTDLG)

// Turn USE_MODAL_PRESENTATION to 0 if there is any reason for not presenting difference
// between modal and modeless dialogs (ie. not implemented it in your port yet)
#if !wxUSE_BOOKCTRL
#define USE_MODAL_PRESENTATION 0
#else
#define USE_MODAL_PRESENTATION 1
#endif


// Turn USE_SETTINGS_DIALOG to 0 if supported
#if wxUSE_BOOKCTRL
#define USE_SETTINGS_DIALOG 1
#else
#define USE_SETTINGS_DIALOG 0
#endif

#if wxUSE_LOG

class AddItemDialog : public wxDialog
{
public:
    AddItemDialog(wxWindow* parent);

    bool Create();

protected:
    wxString GetBoxTitle() { return m_textTitle->GetValue(); }
    wxString GetMessage() { return m_textMsg->GetValue(); }

    virtual void AddAdditionalTextOptions(wxSizer* WXUNUSED(sizer)) { }
    virtual void AddAdditionalFlags(wxSizer* WXUNUSED(sizer)) { }

    void OnApply(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);

private:
    enum
    {
        Btn_Yes,
        Btn_No,
        Btn_Ok,
        Btn_Cancel,
        Btn_Help,
        Btn_Max
    };

    struct BtnInfo
    {
        int flag;
        const char* name;
    };

    //static const BtnInfo ms_btnInfo[Btn_Max];

    enum
    {
        MsgDlgIcon_No,
        MsgDlgIcon_None,
        MsgDlgIcon_Info,
        MsgDlgIcon_Question,
        MsgDlgIcon_Warning,
        MsgDlgIcon_Error,
        MsgDlgIcon_AuthNeeded,
        MsgDlgIcon_Max
    };

    wxTextCtrl* m_textTitle;
    wxTextCtrl* m_textMsg;
    wxTextCtrl* m_textExtMsg;

    wxCheckBox* m_buttons[Btn_Max];
    wxTextCtrl* m_labels[Btn_Max];

    wxRadioBox* m_icons;

    wxCheckBox* m_chkNoDefault,
        * m_chkCentre;

    wxStaticText* m_labelResult;

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(AddItemDialog);
};

class RenameKeyDialog : public wxDialog
{
public:
    RenameKeyDialog(wxWindow* parent);

    void OnListBoxDClick(wxCommandEvent& event);
    void OnDisableOK(wxCommandEvent& event);
    void OnDisableCancel(wxCommandEvent& event);
    void OnCatchListBoxDClick(wxCommandEvent& event);
    void OnTextEnter(wxCommandEvent& event);

private:
    bool   m_catchListBoxDClick;

private:
    wxDECLARE_EVENT_TABLE();
};

#endif
#endif