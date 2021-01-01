
// ============================================================================
// declarations
// ============================================================================

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include <wx/srchctrl.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/artprov.h"
#include "wx/dataview.h"
#include "wx/datetime.h"
#include "wx/splitter.h"
#include "wx/aboutdlg.h"
#include "wx/colordlg.h"
#include "wx/choicdlg.h"
#include "wx/numdlg.h"
#include "wx/spinctrl.h"
#include "wx/imaglist.h"
#include "wx/itemattr.h"
#include "wx/notebook.h"
#include "wx/dataview.h"
#include "wx/popupwin.h"
#include "wx/listctrl.h"
#include "wx/textcompleter.h"
#include "FileTreeModel.h"
#include "wx/timer.h"
#include <mhclient.h>

#include <rapidjson.h>
#include <document.h>     // rapidjson's DOM-style API
#include <prettywriter.h> // for stringify JSON
#include <filereadstream.h>
#include <windows.h>
#include <istreamwrapper.h>
#include <ostreamwrapper.h>
#include <prettywriter.h>
#include <fstream>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>
#include <future>
#include "Command.h"
#include "Dialogs.h"
#include "entityviewmodel.h"
#include "EntityHelper.h"
#include "EventDescription.h"

using namespace rapidjson;
using namespace std;

wxString gVersion = "0.7";
std::map<std::string, std::set<std::string>> ValueMap;
int DecompressEntities(std::istream* input, char** OutDecompressedData, size_t& OutSize, size_t InSize);
int CompressEntities(const char* destFilename, byte* uncompressedData, size_t size);
int CompressEntities(byte* uncompressedData, size_t size, char** output, size_t& outputSize);

void ConstructInsertSubMenu(wxMenu* Menu, EntityTreeModelNode* Node, std::map<int, std::string> &MenuMap);
EntityTreeModelNode* ConstructInsertionTree(wxString Name, rapidjson::Document &Document);

#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    #include "wx/headerctrl.h"
#endif // wxHAS_GENERIC_DATAVIEWCTRL

#include "entityviewmodel.h"
#include "ResourcesParser.h"

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "../sample.xpm"
#endif

#include "wx_small.xpm"

class DuplicateSubTreeCommand : public _CommandPattern {
    wxDataViewItem m_Item;
    Document &m_JsonDocument;
    wxObjectDataPtr<EntityTreeModel> m_Model;
public:
    EntityTreeModelNode *m_NewItem;
    wxDataViewItem m_ParentItem;
    size_t m_Position;
public:
    DuplicateSubTreeCommand(wxDataViewItem Item, wxObjectDataPtr<EntityTreeModel> Model, Document& JsonDocument) :
        m_Item(Item), m_Model(Model), m_JsonDocument(JsonDocument), m_NewItem(nullptr) {
    }

    void Execute() {
        if (m_NewItem == nullptr) {
            m_NewItem = m_Model->Duplicate(&m_Item, m_JsonDocument);
            m_ParentItem = m_Model->GetParent(m_Item);
            m_Position = m_NewItem->GetParent()->GetChildIndex(m_NewItem);
        } else {
            m_Model->Insert(&m_ParentItem, m_Position, &(wxDataViewItem(m_NewItem)), m_JsonDocument, INSERT_TYPE_AUTO);
        }
    }

    void Revert() {
        m_Model->Delete(wxDataViewItem(m_NewItem));
    }
};

class DeleteSubTreeCommand : public _CommandPattern {
    wxDataViewItem m_Item;
    wxDataViewItem m_ParentItem;
    Document& m_JsonDocument;
    wxObjectDataPtr<EntityTreeModel> m_Model;
    size_t m_Position;
    bool m_Deleted;
    bool m_WrappedNode;
    rapidjson::Value m_Key;
    rapidjson::Value m_Value;
public:
    DeleteSubTreeCommand(wxDataViewItem Item, wxObjectDataPtr<EntityTreeModel> Model, Document& JsonDocument) :
        m_Item(Item), m_Model(Model), m_JsonDocument(JsonDocument) {

        EntityTreeModelNode* Node = (EntityTreeModelNode*)Item.GetID();
        m_Position = Node->GetParent()->GetChildIndex(Node);
        m_ParentItem = m_Model->GetParent(Item);
        m_Deleted = false;
        m_Key = *Node->m_keyRef;
        m_Value = *Node->m_valueRef;
        m_WrappedNode = Node->IsWrapped();
    }
    ~DeleteSubTreeCommand() {
        if (m_Deleted != false) {
            delete (EntityTreeModelNode*)m_Item.GetID();
        }
    }

    void Execute() {
        m_Model->Delete(m_Item);
        m_Deleted = true;
        EntityTreeModelNode* Node = (EntityTreeModelNode*)m_Item.GetID();
        Node->m_keyRef = &m_Key;
        Node->m_valueRef = &m_Value;
    }

    void Revert() {
        m_Model->Insert(&m_ParentItem, m_Position, &m_Item, m_JsonDocument, m_WrappedNode ? INSERT_TYPE_WRAP : INSERT_TYPE_NO_WRAP);
        m_Deleted = false;
    }
};

class InsertSubTreeCommand : public _CommandPattern {
    wxDataViewItem m_Item;
    wxDataViewItem m_ParentItem;
    Document& m_JsonDocument;
    wxObjectDataPtr<EntityTreeModel> m_Model;
    size_t m_Position;
    bool m_Deleted;
    rapidjson::Value m_Key;
    rapidjson::Value m_Value;
    bool m_AsObject;
    bool m_Owner;
public:
    InsertSubTreeCommand(wxDataViewItem Item, wxDataViewItem Parent, size_t Position, wxObjectDataPtr<EntityTreeModel> Model, Document& JsonDocument, bool AsObject = false, bool Owner = true) :
        m_Item(Item), m_ParentItem(Parent), m_Position(Position), m_Model(Model), m_JsonDocument(JsonDocument), m_AsObject(AsObject), m_Owner(Owner) {

        EntityTreeModelNode* Node = (EntityTreeModelNode*)Item.GetID();
        m_Position = Position;
        m_ParentItem = Parent;
        m_Deleted = false;
        m_Key = *Node->m_keyRef;
        m_Value = *Node->m_valueRef;
    }

    ~InsertSubTreeCommand() {
        if ((m_Owner != false) && (m_Deleted != false)) {
            delete (EntityTreeModelNode*)m_Item.GetID();
        }
    }

    void Execute() {
        m_Model->Insert(&m_ParentItem, m_Position, &m_Item, m_JsonDocument, m_AsObject ? INSERT_TYPE_WRAP : INSERT_TYPE_AUTO);
        m_Deleted = false;
    }

    void Revert() {
        m_Model->Delete(m_Item);
        m_Deleted = true;
        EntityTreeModelNode* Node = (EntityTreeModelNode*)m_Item.GetID();
        Node->m_keyRef = &m_Key;
        Node->m_valueRef = &m_Value;
    }
};

class ChangeItemCommand : public _CommandPattern {
    wxString m_OldValue;
    wxString m_NewValue;
    wxDataViewItem m_Item;
    wxObjectDataPtr<EntityTreeModel> m_Model;
    size_t m_Column;
public:
    ChangeItemCommand(wxDataViewItem Item, wxObjectDataPtr<EntityTreeModel> Model, wxString NewValue, size_t Column) :
        m_Item(Item), m_Model(Model), m_Column(Column), m_NewValue(NewValue) {

        wxVariant Value;
        m_Model->GetJsonValue(Value, Item, Column);
        m_OldValue = wxString(Value);
    }

    wxString GetOldValue() {
        return m_OldValue;
    }

    void SetNewValue(wxString NewValue) {
        m_NewValue = NewValue;
    }

    void Execute() {
        m_Model->SetValue(m_NewValue, m_Item, m_Column);
        m_Model->ValueChanged(m_Item, m_Column);
    }

    void Revert() {
        m_Model->SetValue(m_OldValue, m_Item, m_Column);
        m_Model->ValueChanged(m_Item, m_Column);
    }
};

class MoveItemCommand : public _CommandPattern {
    // Move from x to y.
};

class RenameVisualCommand : public _CommandPattern {
    wxString m_OldValue;
    wxString m_NewValue;
    wxDataViewItem m_Item;
    wxObjectDataPtr<EntityTreeModel> m_Model;
    size_t m_Column;
public:
    RenameVisualCommand(wxDataViewItem Item, wxObjectDataPtr<EntityTreeModel> Model, wxString NewValue, size_t Column) :
        m_Item(Item), m_Model(Model), m_Column(Column), m_NewValue(NewValue) {

        if (Column == 0) {
            m_OldValue = m_Model->GetKey(Item);

        } else {
            m_OldValue = m_Model->GetValue(Item);
        }
    }
    
    void Execute() {
        EntityTreeModelNode *Node = (EntityTreeModelNode*)(m_Item.GetID());
        if (m_Column == 0) {
            Node->m_key = m_NewValue;

        } else {
            Node->m_value = m_NewValue;
        }

        m_Model->ValueChanged(m_Item, m_Column);
    }

    void Revert() {
        EntityTreeModelNode *Node = (EntityTreeModelNode*)(m_Item.GetID());
        if (m_Column == 0) {
            Node->m_key = m_OldValue;

        } else {
            Node->m_value = m_OldValue;
        }

        m_Model->ValueChanged(m_Item, m_Column);
    }
};


// ----------------------------------------------------------------------------
// MyApp
// ----------------------------------------------------------------------------

class MyApp: public wxApp
{
public:
    virtual bool OnInit() wxOVERRIDE;
};

// ----------------------------------------------------------------------------
// MyFrame
// ----------------------------------------------------------------------------

class MyFrame : public wxFrame
{
public:
    MyFrame(wxFrame *frame, const wxString &title, int x, int y, int w, int h);
    ~MyFrame();

    void BuildDataViewCtrl(wxPanel* parent,
                           wxSizer* sizer,
                           unsigned int nPanel,
                           unsigned long style = 0,
                           wxString Filter = "*");

private:
    // event handlers
    void OnStyleChange(wxCommandEvent& event);
    void OnSetBackgroundColour(wxCommandEvent& event);
    void OnCustomHeaderAttr(wxCommandEvent& event);
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    void OnCustomHeaderHeight(wxCommandEvent& event);
#endif // wxHAS_GENERIC_DATAVIEWCTRL
    void OnGetPageInfo(wxCommandEvent& event);
    void OnDisable(wxCommandEvent& event);
    void OnSetForegroundColour(wxCommandEvent& event);
    void OnIncIndent(wxCommandEvent& event);
    void OnDecIndent(wxCommandEvent& event);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void SaveFile(wxCommandEvent& event);
    void SaveFileAs(wxCommandEvent& event);
    void ExportFile(wxCommandEvent& event);
    void OpenFile(wxCommandEvent& event);
    void OpenFromMeathook(wxCommandEvent& event);
    void ImportFile(wxCommandEvent& event);

    void Undo(wxCommandEvent& event);
    void Redo(wxCommandEvent& event);

    void OnClearLog(wxCommandEvent& event);
    void OnPageChanged(wxBookCtrlEvent& event);
    void OnDeleteSelected(wxCommandEvent& event);
    void OnCollapse(wxCommandEvent& event);
    void OnExpand(wxCommandEvent& event);
    void OnShowCurrent(wxCommandEvent& event);

    void OnValueChanged( wxDataViewEvent &event );

    void OnActivated( wxDataViewEvent &event );
    void OnExpanding( wxDataViewEvent &event );
    void OnExpanded( wxDataViewEvent &event );
    void OnCollapsing( wxDataViewEvent &event );
    void OnCollapsed( wxDataViewEvent &event );
    void OnSelectionChanged( wxDataViewEvent &event );

    void OnStartEditing( wxDataViewEvent &event );
    void OnEditingStarted( wxDataViewEvent &event );
    void OnEditingDone( wxDataViewEvent &event );

    void OnHeaderClick( wxDataViewEvent &event );
    void OnHeaderRightClick( wxDataViewEvent &event );
    void OnSorted( wxDataViewEvent &event );
    void OnColumnReordered( wxDataViewEvent &event);

    void OnContextMenu( wxDataViewEvent &event );
    void OnContextFileMenu(wxDataViewEvent& event);

    void OnFilterType(wxCommandEvent& event);
    void OnFilterSearch(wxCommandEvent& event);
    void OnContextMenuSelect(wxCommandEvent& event);
    void OnContextEncounterSelect(wxCommandEvent& event);
    void OnFilterTypeResources(wxCommandEvent& event);
    void OnFilterSearchResources(wxCommandEvent& event);
    void ProgressCancel(wxCommandEvent& event);

    void QuickFind(wxCommandEvent& event);

#if wxUSE_DRAG_AND_DROP
    void OnBeginDrag( wxDataViewEvent &event );
    void OnDropPossible( wxDataViewEvent &event );
    void OnDrop( wxDataViewEvent &event );
#endif // wxUSE_DRAG_AND_DROP

    void OnDataViewChar(wxKeyEvent& event);
    void ConstructTreeView();

    // helper used by both OnDeleteSelected() and OnDataViewChar()
    void DeleteSelectedItems();

    void PushCommand(CommandPattern Command);
    wxString FindUnusedName(wxString Base);
    void OpenEntitiesFromResources(IDCT_FILE& FileInfo);

    void NavBackward(wxCommandEvent& event);
    void NavForward(wxCommandEvent& event);
    void SearchBackward(wxCommandEvent& event);
    void SearchForward(wxCommandEvent& event);
    void MHPause(wxCommandEvent& event);
    void MHReload(wxCommandEvent& event);
    void MHGotoCurrentEncounter(wxCommandEvent& event);
    void MHGotoCurrentCheckpoint(wxCommandEvent& event);
    void MHStatusCheck(wxTimerEvent& event);
    void ResolveEncounterSpawnChange(EntityTreeModelNode* EncounterNode, wxString OldValue);
    bool SetLocationNodeFromMH(EntityTreeModelNode* Node);
    bool SetRotationNodeFromMH(EntityTreeModelNode* Node);
    bool OpenFileInternal(wxString FilePath);
    void Copy(wxCommandEvent& event);
    void Paste(wxCommandEvent& event);
    void InsertFromClipBoard(EntityTreeModelNode* ParentNode);
    void CopyToClipBoard(EntityTreeModelNode* Node);
    void ScheduleBuildEncounterToEntityMap(void);
    void ReverseCommitExitTriggers(wxCommandEvent& event);
    void ReverseEncounter(wxCommandEvent& event);
    void TrackLocation(wxCommandEvent& event);
    bool SetSpawnPosition(EntityTreeModelNode* Node, float x, float y, float z, shared_ptr< _GroupedCommand> ExternalGroup = nullptr);
    void SelectEncounterManager(wxString ActiveEncounter);
    void SelectCheckpointByName(wxString CheckpointName);

    wxNotebook* m_notebook;

    // the controls stored in the various tabs of the main notebook:
    enum Page
    {
        Page_EntityView,
        Page_ResourcesView,
        Page_Max
    };

    wxDataViewCtrl* m_ctrl[Page_Max];

    // Some of the models associated with the controls:

    wxObjectDataPtr<EntityTreeModel> m_entity_view_model;
    wxObjectDataPtr<FileTreeModel> m_file_view_model;

    // other data:

    wxDataViewColumn* m_col;
    wxDataViewColumn* m_attributes;
    IDCLReader m_ResourceReader;

    wxTextCtrl* m_log;
    wxLog *m_logOld;
    Document m_Document;
    wxTextCtrl* m_FilterCtrl;
    wxTextCtrl* m_ResourceCtrl;
    wxPopupWindow *m_Popup;
    wxString m_CurrentlyLoadedFileName;
    bool m_CurrentlyLoadedFileCompressed;
    std::vector<CommandPattern> m_UndoStack;
    std::vector<CommandPattern> m_RedoStack;
    CommandPattern m_ChangeCmd;
    wxMenu* m_file_menu;
    wxString m_OldValue;
    wxCheckBox *m_MatchCaseCheck;
    std::vector<wxDataViewItem> m_LastNavigation;
    std::vector<wxDataViewItem> m_NextNavigation;
    MeathookInterface m_MeatHook;
    wxTimer m_MHStatusTimer;
    wxStaticText* m_MHInterfaceStatus;
    wxString m_MhText;
    EntityTreeModelNode* m_DraggingNow;
    std::map<int, std::string> m_MenuMap;
    std::future<void> m_Thread;
    std::future<void> m_FileOpenThread;
    std::future<void> m_EndThread;
    std::future<void> m_UpdateTimerThread;
    std::vector<wxString> m_Encounters;

private:
    // Flag used by OnListValueChanged(), see there.
    bool m_eventFromProgram;
    bool m_BuildingEntitiesMap;
    size_t m_ProgressCurrent;
    size_t m_ProgressEnd;
    wxGauge* m_ProgressGuage;
    wxStaticText* m_ProgressText;
    wxBoxSizer* m_ProgressSizer;
    wxSizer* mainSizer;

    wxDECLARE_EVENT_TABLE();
};


// ----------------------------------------------------------------------------
// MyCustomRenderer
// ----------------------------------------------------------------------------

class MyCustomRenderer: public wxDataViewCustomRenderer
{
public:
    // This renderer can be either activatable or editable, for demonstration
    // purposes. In real programs, you should select whether the user should be
    // able to activate or edit the cell and it doesn't make sense to switch
    // between the two -- but this is just an example, so it doesn't stop us.
    explicit MyCustomRenderer(wxDataViewCellMode mode)
        : wxDataViewCustomRenderer("string", mode, wxALIGN_CENTER)
       { }

    virtual bool Render( wxRect rect, wxDC *dc, int state ) wxOVERRIDE
    {
        dc->SetBrush( *wxLIGHT_GREY_BRUSH );
        dc->SetPen( *wxTRANSPARENT_PEN );

        rect.Deflate(2);
        dc->DrawRoundedRectangle( rect, 5 );

        RenderText(m_value,
                   0, // no offset
                   wxRect(dc->GetTextExtent(m_value)).CentreIn(rect),
                   dc,
                   state);
        return true;
    }

    virtual bool ActivateCell(const wxRect& WXUNUSED(cell),
                              wxDataViewModel *WXUNUSED(model),
                              const wxDataViewItem &WXUNUSED(item),
                              unsigned int WXUNUSED(col),
                              const wxMouseEvent *mouseEvent) wxOVERRIDE
    {
        wxString position;
        if ( mouseEvent )
            position = wxString::Format("via mouse at %d, %d", mouseEvent->m_x, mouseEvent->m_y);
        else
            position = "from keyboard";
        wxLogMessage("MyCustomRenderer ActivateCell() %s", position);
        return false;
    }

    virtual wxSize GetSize() const wxOVERRIDE
    {
        return wxSize(60,20);
    }

    virtual bool SetValue( const wxVariant &value ) wxOVERRIDE
    {
        m_value = value.GetString();
        return true;
    }

    virtual bool GetValue( wxVariant &WXUNUSED(value) ) const wxOVERRIDE { return true; }

#if wxUSE_ACCESSIBILITY
    virtual wxString GetAccessibleDescription() const wxOVERRIDE
    {
        return m_value;
    }
#endif // wxUSE_ACCESSIBILITY

    virtual bool HasEditorCtrl() const wxOVERRIDE { return true; }

    virtual wxWindow*
    CreateEditorCtrl(wxWindow* parent,
                     wxRect labelRect,
                     const wxVariant& value) wxOVERRIDE
    {
        wxTextCtrl* text = new wxTextCtrl(parent, wxID_ANY, value,
                                          labelRect.GetPosition(),
                                          labelRect.GetSize(),
                                          wxTE_PROCESS_ENTER);
        text->SetInsertionPointEnd();

        return text;
    }

    virtual bool
    GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) wxOVERRIDE
    {
        wxTextCtrl* text = wxDynamicCast(ctrl, wxTextCtrl);
        if ( !text )
            return false;

        value = text->GetValue();

        return true;
    }

private:
    wxString m_value;
};


// ----------------------------------------------------------------------------
// MultiLineCustomRenderer
// ----------------------------------------------------------------------------

class MultiLineCustomRenderer : public wxDataViewCustomRenderer
{
public:
    // a simple renderer that wraps each word on a new line
    explicit MultiLineCustomRenderer()
        : wxDataViewCustomRenderer("string", wxDATAVIEW_CELL_INERT, 0)
    { }

    virtual bool Render(wxRect rect, wxDC *dc, int state) wxOVERRIDE
    {
        RenderText(m_value, 0, rect, dc, state);
        return true;
    }

    virtual wxSize GetSize() const wxOVERRIDE
    {
        wxSize txtSize = GetTextExtent(m_value);
        int lines = m_value.Freq('\n') + 1;
        txtSize.SetHeight(txtSize.GetHeight() * lines);
        return txtSize;
    }

    virtual bool SetValue(const wxVariant &value) wxOVERRIDE
    {
        m_value = value.GetString();
        m_value.Replace(" ", "\n");
        return true;
    }

    virtual bool GetValue(wxVariant &WXUNUSED(value)) const wxOVERRIDE { return true; }

#if wxUSE_ACCESSIBILITY
    virtual wxString GetAccessibleDescription() const wxOVERRIDE
    {
        return m_value;
    }
#endif // wxUSE_ACCESSIBILITY

private:
    wxString m_value;
};


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MyApp
// ----------------------------------------------------------------------------

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    if (LoadEventDescriptor() == false) {
        wxMessageBox(wxT("Could not load eternalevents.txt put it into the same folder as EntityHero.exe"), wxT("Error"), wxOK);
    }

    if (LoadMenuDescriptor() == false) {
        wxMessageBox(wxT("Could not load insert_desc.txt put it into the same folder as EntityHero.exe"), wxT("Error"), wxOK);
    }

    MyFrame *frame =
        new MyFrame(NULL, "EntityHero v" + gVersion + " (by Scorp0rX0r)", 40, 40, 1000, 540);

    frame->Show(true);
    return true;
}

// ----------------------------------------------------------------------------
// MyFrame
// ----------------------------------------------------------------------------

enum
{
    ID_CLEARLOG = wxID_HIGHEST+1,
    ID_GET_PAGE_INFO,
    ID_DISABLE,
    ID_BACKGROUND_COLOUR,
    ID_FOREGROUND_COLOUR,
    ID_CUSTOM_HEADER_ATTR,
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    ID_CUSTOM_HEADER_HEIGHT,
#endif // wxHAS_GENERIC_DATAVIEWCTRL
    ID_STYLE_MENU,
    ID_INC_INDENT,
    ID_DEC_INDENT,
    ID_OPEN_FILE,
    ID_OPEN_MEATHOOK,
    ID_SAVE_FILE,
    ID_SAVE_FILE_AS,
    ID_SAVE_TEXT_FILE,
    ID_OPEN_TEXT_FILE,

    // file menu
    //ID_SINGLE,        wxDV_SINGLE==0 so it's always present
    ID_MULTIPLE,
    ID_ROW_LINES,
    ID_HORIZ_RULES,
    ID_VERT_RULES,

    // Edit menu
    ID_UNDO,
    ID_REDO,

    //
    ID_COPY,
    ID_PASTE,


    // Quick find menu
    ID_QF_INTRO_CUTSCENE,
    ID_QF_SPAWN_LOCATION,
    ID_QF_FIRST_ENCOUNTER,

    ID_SP_REVERSE_COMMIT_EXIT_TRIGGERS,
    ID_SP_TRACK_LOCATION,
    ID_SP_REVERSE_ENCOUNTER,

    ID_EXIT = wxID_EXIT,

    // about menu
    ID_ABOUT = wxID_ABOUT,


    // control IDs

    ID_ENTITY_CTRL       = 50,
    ID_FILE_CTRL         = 51,

    ID_DELETE_SEL       = 101,
    ID_DELETE_YEAR      = 102,
    ID_SELECT_NINTH     = 103,
    ID_COLLAPSE         = 104,
    ID_EXPAND           = 105,
    ID_MH_PAUSE,
    ID_MH_RELOAD,
    ID_MH_GOTO_CURRENT_ENCOUNTER,
    ID_MH_GOTO_CURRENT_CHECKPOINT,
    ID_FILTER_SEARCH,
    ID_FILTER_SEARCH_RESOURCES,
    ID_PROGRESS_CANCEL,

    ID_NAVIGTE_BACKWARD,
    ID_NAVIGTE_FORWARD,
    ID_SEARCH_BACKWARD,
    ID_SEARCH_FORWARD,
    ID_SEARCH_MATCH_CASE,
    ID_CHECK_MH_STATUS,

    ID_PREPEND_LIST     = 200,
    ID_DELETE_LIST      = 201,
    ID_GOTO             = 202,
    ID_ADD_MANY         = 203,
    ID_HIDE_ATTRIBUTES  = 204,
    ID_SHOW_ATTRIBUTES  = 205,
    ID_MULTIPLE_SORT    = 206,
    ID_SORT_BY_FIRST_COLUMN,

    // Fourth page.
    ID_DELETE_TREE_ITEM = 400,
    ID_DELETE_ALL_TREE_ITEMS = 401,
    ID_ADD_TREE_ITEM = 402,
    ID_ADD_TREE_CONTAINER_ITEM = 403,

    // Index list model page
    ID_INDEX_LIST_USE_ENGLISH = 500,
    ID_INDEX_LIST_USE_FRENCH,
    ID_INDEX_LIST_RESET_MODEL
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU_RANGE( ID_MULTIPLE, ID_VERT_RULES, MyFrame::OnStyleChange )
    EVT_MENU(ID_SAVE_FILE, MyFrame::SaveFile )
    EVT_MENU(ID_SAVE_FILE_AS, MyFrame::SaveFileAs)
    EVT_MENU(ID_SAVE_TEXT_FILE, MyFrame::ExportFile)

    EVT_MENU( ID_OPEN_FILE, MyFrame::OpenFile )
    EVT_MENU(ID_OPEN_MEATHOOK, MyFrame::OpenFromMeathook)
    EVT_MENU(ID_OPEN_TEXT_FILE, MyFrame::ImportFile)
    EVT_MENU( ID_CLEARLOG, MyFrame::OnClearLog )

    EVT_MENU(ID_REDO, MyFrame::Redo)
    EVT_MENU(ID_UNDO, MyFrame::Undo)

    EVT_MENU(ID_COPY, MyFrame::Copy)
    EVT_MENU(ID_PASTE, MyFrame::Paste)

    EVT_MENU(ID_NAVIGTE_BACKWARD, MyFrame::NavBackward)
    EVT_MENU(ID_NAVIGTE_FORWARD, MyFrame::NavForward)


    EVT_MENU( ID_GET_PAGE_INFO, MyFrame::OnGetPageInfo )
    EVT_MENU( ID_DISABLE, MyFrame::OnDisable )
    EVT_MENU( ID_FOREGROUND_COLOUR, MyFrame::OnSetForegroundColour )
    EVT_MENU( ID_BACKGROUND_COLOUR, MyFrame::OnSetBackgroundColour )
    EVT_MENU( ID_CUSTOM_HEADER_ATTR, MyFrame::OnCustomHeaderAttr )
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    EVT_MENU( ID_CUSTOM_HEADER_HEIGHT, MyFrame::OnCustomHeaderHeight )
#endif // wxHAS_GENERIC_DATAVIEWCTRL
    EVT_MENU( ID_INC_INDENT, MyFrame::OnIncIndent )
    EVT_MENU( ID_DEC_INDENT, MyFrame::OnDecIndent )

    EVT_MENU(ID_QF_INTRO_CUTSCENE, MyFrame::QuickFind)
    EVT_MENU(ID_QF_SPAWN_LOCATION, MyFrame::QuickFind)
    EVT_MENU(ID_QF_FIRST_ENCOUNTER, MyFrame::QuickFind)

    EVT_MENU(ID_SP_REVERSE_COMMIT_EXIT_TRIGGERS, MyFrame::ReverseCommitExitTriggers)
    EVT_MENU(ID_SP_TRACK_LOCATION, MyFrame::TrackLocation)
    EVT_MENU(ID_SP_REVERSE_ENCOUNTER, MyFrame::ReverseEncounter)
    
    EVT_NOTEBOOK_PAGE_CHANGED( wxID_ANY, MyFrame::OnPageChanged )

    EVT_BUTTON( ID_COLLAPSE, MyFrame::OnCollapse )
    EVT_BUTTON( ID_EXPAND, MyFrame::OnExpand )
    EVT_BUTTON(ID_SEARCH_BACKWARD, MyFrame::SearchBackward)
    EVT_BUTTON(ID_SEARCH_FORWARD, MyFrame::SearchForward)
    EVT_BUTTON(ID_MH_PAUSE, MyFrame::MHPause)
    EVT_BUTTON(ID_MH_RELOAD, MyFrame::MHReload)
    EVT_BUTTON(ID_MH_GOTO_CURRENT_ENCOUNTER, MyFrame::MHGotoCurrentEncounter)
    EVT_BUTTON(ID_MH_GOTO_CURRENT_CHECKPOINT, MyFrame::MHGotoCurrentCheckpoint)
    EVT_BUTTON(ID_PROGRESS_CANCEL, MyFrame::ProgressCancel)
    
    EVT_DATAVIEW_ITEM_VALUE_CHANGED( ID_ENTITY_CTRL, MyFrame::OnValueChanged )

    EVT_DATAVIEW_ITEM_ACTIVATED(ID_ENTITY_CTRL, MyFrame::OnActivated )
    EVT_DATAVIEW_ITEM_EXPANDING(ID_ENTITY_CTRL, MyFrame::OnExpanding)
    EVT_DATAVIEW_ITEM_EXPANDED(ID_ENTITY_CTRL, MyFrame::OnExpanded)
    EVT_DATAVIEW_ITEM_COLLAPSING(ID_ENTITY_CTRL, MyFrame::OnCollapsing)
    EVT_DATAVIEW_ITEM_COLLAPSED(ID_ENTITY_CTRL, MyFrame::OnCollapsed)
    EVT_DATAVIEW_SELECTION_CHANGED(ID_ENTITY_CTRL, MyFrame::OnSelectionChanged)

    EVT_DATAVIEW_ITEM_START_EDITING(ID_ENTITY_CTRL, MyFrame::OnStartEditing)
    EVT_DATAVIEW_ITEM_EDITING_STARTED(wxID_ANY, MyFrame::OnEditingStarted)
    EVT_DATAVIEW_ITEM_EDITING_DONE(wxID_ANY, MyFrame::OnEditingDone)

    EVT_DATAVIEW_COLUMN_HEADER_CLICK(ID_ENTITY_CTRL, MyFrame::OnHeaderClick)
    EVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK(ID_ENTITY_CTRL, MyFrame::OnHeaderRightClick)
    EVT_DATAVIEW_COLUMN_SORTED(ID_ENTITY_CTRL, MyFrame::OnSorted)
    EVT_DATAVIEW_COLUMN_REORDERED(wxID_ANY, MyFrame::OnColumnReordered)

    EVT_DATAVIEW_ITEM_CONTEXT_MENU(ID_ENTITY_CTRL, MyFrame::OnContextMenu)
    EVT_DATAVIEW_ITEM_CONTEXT_MENU(ID_FILE_CTRL, MyFrame::OnContextFileMenu)

#if wxUSE_DRAG_AND_DROP
    EVT_DATAVIEW_ITEM_BEGIN_DRAG( ID_ENTITY_CTRL, MyFrame::OnBeginDrag )
    EVT_DATAVIEW_ITEM_DROP_POSSIBLE( ID_ENTITY_CTRL, MyFrame::OnDropPossible )
    EVT_DATAVIEW_ITEM_DROP( ID_ENTITY_CTRL, MyFrame::OnDrop )
#endif // wxUSE_DRAG_AND_DROP

    EVT_TEXT(ID_FILTER_SEARCH, MyFrame::OnFilterType)
    EVT_TEXT_ENTER(ID_FILTER_SEARCH, MyFrame::OnFilterSearch)

    EVT_TEXT(ID_FILTER_SEARCH_RESOURCES, MyFrame::OnFilterTypeResources)
    EVT_TEXT_ENTER(ID_FILTER_SEARCH_RESOURCES, MyFrame::OnFilterSearchResources)
    EVT_TIMER(ID_CHECK_MH_STATUS, MyFrame::MHStatusCheck)
wxEND_EVENT_TABLE()

MyFrame::MyFrame(wxFrame *frame, const wxString &title, int x, int y, int w, int h):
  wxFrame(frame, wxID_ANY, title, wxPoint(x, y), wxSize(w, h)),
    m_Popup(nullptr),
    m_BuildingEntitiesMap(false)
{
    m_log = NULL;
    m_col = NULL;

    for ( int page = 0; page < Page_Max; ++page )
        m_ctrl[page] = NULL;

    m_eventFromProgram = false;
    SetIcon(wx_small_xpm);

#ifdef _DEBUG
    // Quickload something for debugging.

    ifstream InputStream("../output_edited.txt");
    if (InputStream.good() != false) {
        rapidjson::IStreamWrapper InStream(InputStream);
        m_Document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(InStream);
        if (m_Document.HasParseError() != false) {
            wxMessageBox(wxT("Could not parse: ../output_edited.txt"), wxT("Unresolved"), wxICON_INFORMATION | wxOK);
        }
    }
#endif

    auto oodle = LoadLibraryA("./oo2core_8_win64.dll");
    if (oodle == nullptr) {
        wxMessageBox(wxT("You can still open uncompressed entities text files but will have to decompress them separately.\nPut oo2core_8_win64.dll in the same folder as EntityHero.exe"), wxT("Warning: oo2core_8_win64.dll could not be found."), wxICON_WARNING | wxOK);
    }

    bool MeathookActive = false;

    // build the menus
    // ----------------
    wxMenu *file_menu = new wxMenu;
    file_menu->Append(ID_OPEN_FILE, "&Open");
    file_menu->Append(ID_OPEN_MEATHOOK, "Open from MH");
    file_menu->Append(ID_SAVE_FILE, "&Save");
    file_menu->Append(ID_SAVE_FILE_AS, "Save&As");
    file_menu->AppendSeparator();
    file_menu->Append(ID_SAVE_TEXT_FILE, "&Export to text");
    file_menu->Append(ID_OPEN_TEXT_FILE, "&Import from text");
    file_menu->Enable(ID_SAVE_FILE, false);
    file_menu->Enable(ID_SAVE_FILE_AS, false);
    m_file_menu = file_menu;

    wxMenu* edit_menu = new wxMenu;
    edit_menu->Append(ID_REDO, "Redo");
    edit_menu->Append(ID_UNDO, "Undo");
    edit_menu->Append(ID_NAVIGTE_BACKWARD, "Navigate Previous (Ctrl+,)");
    edit_menu->Append(ID_NAVIGTE_FORWARD, "Navigate Next (Ctrl+.)");
    edit_menu->AppendSeparator();
    edit_menu->Append(ID_COPY, "Copy");
    edit_menu->Append(ID_PASTE, "Paste");

    wxMenu* quickfind_menu = new wxMenu;
    quickfind_menu->Append(ID_QF_INTRO_CUTSCENE, "Intro cutscene (intro_game_info_logic)");
    quickfind_menu->Append(ID_QF_SPAWN_LOCATION, "Initial Spawn point");
#if 0
    quickfind_menu->Append(ID_QF_FIRST_ENCOUNTER, "First encounter");
#endif

    wxMenu* special_menu = new wxMenu;
    special_menu->Append(ID_SP_REVERSE_COMMIT_EXIT_TRIGGERS, "Reverse Commit and Exit triggers");
#if 0
    special_menu->Append(ID_SP_REVERSE_ENCOUNTER, "Reverse Encounter");
    special_menu->Append(ID_SP_TRACK_LOCATION, "Track Location");
#endif

    wxMenuBar *menu_bar = new wxMenuBar;
    menu_bar->Append(file_menu, "&File");
    menu_bar->Append(edit_menu, "&Edit");
    menu_bar->Append(quickfind_menu, "&Quick Find");
    menu_bar->Append(special_menu, "&Special");

    SetMenuBar(menu_bar);
    CreateStatusBar();

    // redirect logs from our event handlers to text control
    m_log = new wxTextCtrl( this, wxID_ANY, wxString(), wxDefaultPosition,
                            wxDefaultSize, wxTE_MULTILINE );
    m_log->SetMinSize(wxSize(-1, 100));
    m_logOld = wxLog::SetActiveTarget(new wxLogTextCtrl(m_log));
    wxLogMessage( "This is the log window" );
    m_notebook = new wxNotebook( this, wxID_ANY );

    wxPanel *firstPanel = new wxPanel( m_notebook, wxID_ANY );
    BuildDataViewCtrl(firstPanel, nullptr, Page_EntityView);

    const wxSizerFlags border = wxSizerFlags().DoubleBorder();
    wxBoxSizer *sizerCurrent = new wxBoxSizer(wxHORIZONTAL);
    sizerCurrent->SetMinSize(wxSize(500,0));
    sizerCurrent->Add(new wxButton(firstPanel, ID_MH_PAUSE,
                                   "&Pause"), border);
    sizerCurrent->Add(new wxButton(firstPanel, ID_MH_RELOAD,
                                   "&Reload level"), border);
    sizerCurrent->Add(new wxButton(firstPanel, ID_MH_GOTO_CURRENT_ENCOUNTER,
                                   "&Goto current encounter"), border);
    sizerCurrent->Add(new wxButton(firstPanel, ID_MH_GOTO_CURRENT_CHECKPOINT,
                                   "&Goto Checkpoint"), border);
    // 
    // Per request from the creator of meathook, randomize the displayed meathook name.
    //
    srand(timeGetTime());
    struct PickRand_ { static char PickRand(const char *str) { size_t len = strlen(str); return str[rand() % (len - 1)]; } };
    m_MhText = wxString::Format("%c%c%c%c%c%c%c%c interface:",
        PickRand_::PickRand("Mm"),
        PickRand_::PickRand("eE3�"),
        PickRand_::PickRand("aA4"),
        PickRand_::PickRand("tT7"),
        PickRand_::PickRand("hH"),
        PickRand_::PickRand("oO0"),
        PickRand_::PickRand("oO0"),
        PickRand_::PickRand("kK"));

    wxString InterfaceText = wxString().Format("%s %s", m_MhText, MeathookActive ? L"" : L"(inactive)");
    m_file_menu->Enable(ID_OPEN_MEATHOOK, m_MeatHook.m_Initialized);
    m_MHInterfaceStatus = new wxStaticText(firstPanel, wxID_ANY, InterfaceText);

    wxSizer* navigationSizer = new wxBoxSizer(wxHORIZONTAL);
    m_FilterCtrl = new wxTextCtrl(firstPanel, ID_FILTER_SEARCH, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_FilterCtrl->SetHint("Search");
    navigationSizer->Add(new wxButton(firstPanel, ID_SEARCH_BACKWARD, "<", wxDefaultPosition, wxSize(20, 20)), 0, wxGROW | wxALL, 1);
    navigationSizer->Add(new wxButton(firstPanel, ID_SEARCH_FORWARD, ">", wxDefaultPosition, wxSize(20, 20)), 0, wxGROW | wxALL, 1);
    navigationSizer->Add(m_FilterCtrl, 1, wxGROW | wxALL, 1);
    m_MatchCaseCheck = new wxCheckBox(firstPanel, ID_SEARCH_MATCH_CASE, "Match case");
    navigationSizer->Add(m_MatchCaseCheck, 0, wxGROW | wxALL, 1);

    wxSizer *firstPanelSz = new wxBoxSizer( wxVERTICAL );
    m_ctrl[Page_EntityView]->SetMinSize(wxSize(-1, 200));
    firstPanelSz->Add(navigationSizer, 0, wxGROW | wxALL, 5);
    firstPanelSz->Add(m_ctrl[Page_EntityView], 1, wxGROW|wxALL, 5);
    firstPanelSz->Add(m_MHInterfaceStatus, 0, wxGROW | wxALL, 5);

    firstPanelSz->Add(sizerCurrent);
    firstPanel->SetSizerAndFit(firstPanelSz);
    m_notebook->AddPage(firstPanel, "EntityView");

    wxPanel* resourcesPanel = new wxPanel(m_notebook, wxID_ANY);
    BuildDataViewCtrl(resourcesPanel, nullptr, Page_ResourcesView);

    m_ResourceCtrl = new wxTextCtrl(resourcesPanel, ID_FILTER_SEARCH_RESOURCES, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_ResourceCtrl->SetHint("Filter");
    wxSizer* resourcesPanelSz = new wxBoxSizer(wxVERTICAL);
    m_ctrl[Page_ResourcesView]->SetMinSize(wxSize(-1, 200));
    resourcesPanelSz->Add(m_ResourceCtrl, 0, wxGROW | wxALL, 5);
    resourcesPanelSz->Add(m_ctrl[Page_ResourcesView], 1, wxGROW | wxALL, 5);
    resourcesPanel->SetSizerAndFit(resourcesPanelSz);
    m_notebook->AddPage(resourcesPanel, "ResourceView");

    mainSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->Add( m_notebook, 1, wxGROW );
#ifdef _DEBUG
    mainSizer->Add( m_log, 0, wxGROW );
#else
    m_log->Hide();
#endif

    m_ProgressSizer = new wxBoxSizer(wxHORIZONTAL);
    m_ProgressGuage = new wxGauge(this, 0, 100);
    m_ProgressGuage->SetValue(50);
    m_ProgressGuage->SetRange(75);
    m_ProgressText = new wxStaticText(this, wxID_ANY, "test");
    m_ProgressSizer->Add(m_ProgressText, 0, wxGROW | wxALL, 5);
    m_ProgressSizer->Add(m_ProgressGuage, 1, wxGROW | wxALL, 5);
    m_ProgressSizer->Add(new wxButton(this, ID_PROGRESS_CANCEL, "Cancel", wxDefaultPosition, wxSize(50, 15)), 0, wxALL, 5);
    mainSizer->Add(m_ProgressSizer, 0, wxGROW);
    m_ProgressSizer->Show(false);
    SetSizerAndFit(mainSizer);

    wxAcceleratorEntry entries[7];
    entries[0].Set(wxACCEL_CTRL, (int)'z', ID_UNDO);
    entries[1].Set(wxACCEL_CTRL, (int)'y', ID_REDO);
    entries[2].Set(wxACCEL_CTRL, (int)'s', ID_SAVE_FILE);
    entries[3].Set(wxACCEL_CTRL, (int)',', ID_NAVIGTE_BACKWARD);
    entries[4].Set(wxACCEL_CTRL, (int)'.', ID_NAVIGTE_FORWARD);
    entries[5].Set(wxACCEL_CTRL, (int)'c', ID_COPY);
    entries[6].Set(wxACCEL_CTRL, (int)'v', ID_PASTE);

    wxAcceleratorTable accel(ARRAYSIZE(entries), entries);
    SetAcceleratorTable(accel);

    m_MHStatusTimer.SetOwner(this, ID_CHECK_MH_STATUS);
    m_MHStatusTimer.Start(5000);

#if 0
//    std::string TestClip = \
//R"(item[1] = {
//	eventCall = {
//		eventDef = "spawnSingleAI";
//		args = {
//			num = 3;
//			item[0] = {
//				eEncounterSpawnType_t = "ENCOUNTER_SPAWN_ZOMBIE_TIER_1";
//			}
//			item[1] = {
//				entity = "barge_target_spawn_intro_2";
//			}
//			item[2] = {
//				string = "priest_room_zombies";
//			}
//		}
//	}
//})";
//
//    OpenClipboard(GetHWND());
//    EmptyClipboard();
//    HGLOBAL Memory = GlobalAlloc(GMEM_MOVEABLE, TestClip.size());
//    HGLOBAL StringHandle = GlobalLock(Memory);
//    memcpy((char*)StringHandle, TestClip.c_str(), TestClip.size());
//    GlobalUnlock(StringHandle);
//    SetClipboardData(CF_TEXT, StringHandle);
//    CloseClipboard();

    EntityTreeModelNode* ParentNode = (EntityTreeModelNode*)m_entity_view_model->GetRoot().GetID();
    ParentNode = ParentNode->Find("barge_encounter_manager_priest_room_no_gk:entityDef barge_encounter_manager_priest_room_no_gk:edit:encounterComponent:entityEvents:events");
    size_t Index = 0;
    wxString eventCallStr("eventCall");
    ParentNode = ParentNode->FindByName(0, 0, eventCallStr, 1, Index, true, true);
    InsertFromClipBoard(ParentNode);
#endif
}

MyFrame::~MyFrame()
{
    delete wxLog::SetActiveTarget(m_logOld);
}

void MyFrame::BuildDataViewCtrl(wxPanel* parent, wxSizer* sizer, unsigned int nPanel, unsigned long style, wxString Filter)
{
    wxASSERT(!m_ctrl[nPanel]); // should only be initialized once

    switch (nPanel)
    {
    case Page_EntityView:
        {
            m_ctrl[Page_EntityView] =
                new wxDataViewCtrl(parent, ID_ENTITY_CTRL, wxDefaultPosition,
                                    wxDefaultSize, style );

            m_ctrl[Page_EntityView]->Bind(wxEVT_CHAR, &MyFrame::OnDataViewChar, this);

            m_entity_view_model = new EntityTreeModel(m_Document);
            m_ctrl[Page_EntityView]->AssociateModel( m_entity_view_model.get() );
            m_ctrl[0]->Expand(m_entity_view_model->GetRoot());


#if wxUSE_DRAG_AND_DROP && wxUSE_UNICODE
            m_ctrl[Page_EntityView]->EnableDragSource(wxDF_UNICODETEXT);
            m_ctrl[Page_EntityView]->EnableDropTarget(wxDF_UNICODETEXT);
#endif // wxUSE_DRAG_AND_DROP && wxUSE_UNICODE

            // column 0 of the view control:

            wxDataViewTextRenderer *tr =
                new wxDataViewTextRenderer( "string", wxDATAVIEW_CELL_EDITABLE);
            wxDataViewColumn *column0 =
                new wxDataViewColumn( "key", tr, 0, FromDIP(400), wxALIGN_LEFT,
                                      wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE );
            m_ctrl[Page_EntityView]->AppendColumn( column0 );
#if 0
            // Call this and sorting is enabled
            // immediately upon start up.
            column0->SetAsSortKey();
#endif

            // column 1 of the view control:

            tr = new wxDataViewTextRenderer( "string", wxDATAVIEW_CELL_EDITABLE );
            wxDataViewColumn *column1 =
                new wxDataViewColumn( "value", tr, 1, FromDIP(700), wxALIGN_LEFT,
                                      wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE |
                                      wxDATAVIEW_COL_RESIZABLE );
            column1->SetMinWidth(FromDIP(150)); // this column can't be resized to be smaller
            m_ctrl[Page_EntityView]->AppendColumn( column1 );

            ScheduleBuildEncounterToEntityMap();
        }
        break;
    case Page_ResourcesView:
        {
            m_ctrl[Page_ResourcesView] =
                new wxDataViewCtrl(parent, ID_FILE_CTRL, wxDefaultPosition,
                    wxDefaultSize, style);

            m_ctrl[Page_ResourcesView]->Bind(wxEVT_CHAR, &MyFrame::OnDataViewChar, this);

            m_file_view_model = new FileTreeModel(m_ResourceReader, "*");
            m_ctrl[Page_ResourcesView]->AssociateModel(m_file_view_model.get());
            m_ctrl[Page_ResourcesView]->Expand(wxDataViewItem(m_file_view_model->GetRoot()));

            wxDataViewTextRenderer *tr =
                new wxDataViewTextRenderer( "string", wxDATAVIEW_CELL_INERT );
            wxDataViewColumn *column0 =
                new wxDataViewColumn( "name", tr, 0, FromDIP(200), wxALIGN_LEFT,
                                      wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE );
            m_ctrl[Page_ResourcesView]->AppendColumn( column0 );
#if 0
            // Call this and sorting is enabled
            // immediately upon start up.
            column0->SetAsSortKey();
#endif

            // column 1 of the view control:

            tr = new wxDataViewTextRenderer( "long", wxDATAVIEW_CELL_INERT);
            wxDataViewColumn *column1 =
                new wxDataViewColumn( "compressed size", tr, 1, FromDIP(100), wxALIGN_LEFT,
                                      wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE |
                                      wxDATAVIEW_COL_RESIZABLE );
            column1->SetMinWidth(FromDIP(150)); // this column can't be resized to be smaller
            m_ctrl[Page_ResourcesView]->AppendColumn(column1);

            // column 2 of the view control:
            tr = new wxDataViewTextRenderer("long", wxDATAVIEW_CELL_INERT);
            wxDataViewColumn* column2 =
                new wxDataViewColumn("size", tr, 2, FromDIP(100), wxALIGN_LEFT,
                    wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE |
                    wxDATAVIEW_COL_RESIZABLE);
            column2->SetMinWidth(FromDIP(150)); // this column can't be resized to be smaller
            m_ctrl[Page_ResourcesView]->AppendColumn( column2 );

            tr = new wxDataViewTextRenderer("long", wxDATAVIEW_CELL_INERT);
            wxDataViewColumn* column3 =
                new wxDataViewColumn("offset", tr, 3, FromDIP(100), wxALIGN_LEFT,
                    wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE |
                    wxDATAVIEW_COL_RESIZABLE);
            column3->SetMinWidth(FromDIP(150)); // this column can't be resized to be smaller
            m_ctrl[Page_ResourcesView]->AppendColumn(column3);
        }
        break;
    }
}


// ----------------------------------------------------------------------------
// MyFrame - generic event handlers
// ----------------------------------------------------------------------------

void MyFrame::OnClearLog( wxCommandEvent& WXUNUSED(event) )
{
    m_log->Clear();
}

void MyFrame::OnGetPageInfo(wxCommandEvent& WXUNUSED(event))
{
    wxDataViewCtrl* const dvc = m_ctrl[m_notebook->GetSelection()];

    const wxDataViewItem top = dvc->GetTopItem();
    wxString topDesc;
    if ( top.IsOk() )
    {
        wxVariant value;
        dvc->GetModel()->GetValue(value, top, 0);
        topDesc.Printf("Top item is \"%s\"", value.GetString());
    }
    else
    {
        topDesc = "There is no top item";
    }

    wxLogMessage("%s and there are %d items per page",
                 topDesc,
                 dvc->GetCountPerPage());
}

void MyFrame::OnDisable(wxCommandEvent& event)
{
    m_ctrl[m_notebook->GetSelection()]->Enable(!event.IsChecked());
}

void MyFrame::OnSetForegroundColour(wxCommandEvent& WXUNUSED(event))
{
    wxDataViewCtrl * const dvc = m_ctrl[m_notebook->GetSelection()];
    wxColour col = wxGetColourFromUser(this, dvc->GetForegroundColour());
    if ( col.IsOk() )
    {
        dvc->SetForegroundColour(col);
        Refresh();
    }
}

void MyFrame::OnSetBackgroundColour(wxCommandEvent& WXUNUSED(event))
{
    wxDataViewCtrl * const dvc = m_ctrl[m_notebook->GetSelection()];
    wxColour col = wxGetColourFromUser(this, dvc->GetBackgroundColour());
    if ( col.IsOk() )
    {
        dvc->SetBackgroundColour(col);
        Refresh();
    }
}

void MyFrame::OnCustomHeaderAttr(wxCommandEvent& event)
{
    wxItemAttr attr;
    if ( event.IsChecked() )
    {
        attr.SetTextColour(*wxRED);
        attr.SetFont(wxFontInfo(20).Bold());
    }
    //else: leave it as default to disable custom header attributes

    wxDataViewCtrl * const dvc = m_ctrl[m_notebook->GetSelection()];
    if ( !dvc->SetHeaderAttr(attr) )
        wxLogMessage("Sorry, header attributes not supported on this platform");
}

#ifdef wxHAS_GENERIC_DATAVIEWCTRL
void MyFrame::OnCustomHeaderHeight(wxCommandEvent& event)
{
    wxDataViewCtrl * const dvc = m_ctrl[m_notebook->GetSelection()];
    wxHeaderCtrl* const header = dvc->GenericGetHeader();
    if ( !header )
    {
        wxLogMessage("No header");
        return;
    }

    // Use a big height to show that this works.
    wxSize size = event.IsChecked() ? FromDIP(wxSize(0, 80)) : wxDefaultSize;
    header->SetMinSize(size);
    header->Refresh();

    dvc->Layout();
}
#endif // wxHAS_GENERIC_DATAVIEWCTRL

void MyFrame::OnIncIndent(wxCommandEvent& WXUNUSED(event))
{
    wxDataViewCtrl * const dvc = m_ctrl[m_notebook->GetSelection()];
    dvc->SetIndent(dvc->GetIndent() + 5);
    wxLogMessage("Indent is now %d", dvc->GetIndent());
}

void MyFrame::OnDecIndent(wxCommandEvent& WXUNUSED(event))
{
    wxDataViewCtrl * const dvc = m_ctrl[m_notebook->GetSelection()];
    dvc->SetIndent(wxMax(dvc->GetIndent() - 5, 0));
    wxLogMessage("Indent is now %d", dvc->GetIndent());
}

void MyFrame::OnPageChanged( wxBookCtrlEvent& WXUNUSED(event) )
{
    unsigned int nPanel = m_notebook->GetSelection();

    //GetMenuBar()->FindItem(ID_STYLE_MENU)->SetItemLabel(
    //            wxString::Format("Style of panel #%d", nPanel+1));

    for (unsigned int id = ID_MULTIPLE; id <= ID_VERT_RULES; id++)
    {
        unsigned long style = 0;
        switch (id)
        {
        /*case ID_SINGLE:
            style = wxDV_SINGLE;
            break;*/
        case ID_MULTIPLE:
            style = wxDV_MULTIPLE;
            break;
        case ID_ROW_LINES:
            style = wxDV_ROW_LINES;
            break;
        case ID_HORIZ_RULES:
            style = wxDV_HORIZ_RULES;
            break;
        case ID_VERT_RULES:
            style = wxDV_VERT_RULES;
            break;
        default:
            wxFAIL;
        }

        //GetMenuBar()->FindItem(id)->Check( m_ctrl[nPanel]->HasFlag(style) );
    }

    //GetMenuBar()->FindItem(ID_DISABLE)->Check(!m_ctrl[nPanel]->IsEnabled());
}

void MyFrame::OnStyleChange( wxCommandEvent& WXUNUSED(event) )
{
    unsigned int nPanel = m_notebook->GetSelection();

    // build the style
    unsigned long style = 0;
    /*if (GetMenuBar()->FindItem(ID_SINGLE)->IsChecked())
        style |= wxDV_SINGLE;*/
    if (GetMenuBar()->FindItem(ID_MULTIPLE)->IsChecked())
        style |= wxDV_MULTIPLE;
    if (GetMenuBar()->FindItem(ID_ROW_LINES)->IsChecked())
        style |= wxDV_ROW_LINES;
    if (GetMenuBar()->FindItem(ID_HORIZ_RULES)->IsChecked())
        style |= wxDV_HORIZ_RULES;
    if (GetMenuBar()->FindItem(ID_VERT_RULES)->IsChecked())
        style |= wxDV_VERT_RULES;

    wxSizer* sz = m_ctrl[nPanel]->GetContainingSizer();
    wxASSERT(sz);

    sz->Detach(m_ctrl[nPanel]);
    wxDELETE(m_ctrl[nPanel]);
    m_ctrl[nPanel] = NULL;

    if (nPanel == 0)
        m_entity_view_model.reset(NULL);

    // rebuild the DVC for the selected panel:
    BuildDataViewCtrl((wxPanel*)m_notebook->GetPage(nPanel), nullptr, nPanel, style);

    sz->Prepend(m_ctrl[nPanel], 1, wxGROW|wxALL, 5);
    sz->Layout();
}

void MyFrame::OnQuit( wxCommandEvent& WXUNUSED(event) )
{
    Close(true);
}

void MyFrame::OnAbout( wxCommandEvent& WXUNUSED(event) )
{
    wxAboutDialogInfo info;
    info.SetName(_("EntityHero v") + gVersion);
    info.SetDescription(_("Doom Eternal entity file editor"));
    info.SetCopyright("(C) 2020");
    info.AddDeveloper("by Scorp0rX0r");

    wxAboutBox(info, this);
}

// ----------------------------------------------------------------------------
// MyFrame - event handlers for the first page
// ----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

void MyFrame::OnBeginDrag( wxDataViewEvent &event )
{
    wxDataViewItem item( event.GetItem() );

    // only allow drags for item, not containers
    if (m_entity_view_model->IsArrayElement( item ) == false)
    {
        wxLogMessage("Forbidding starting dragging");
        event.Veto();
        return;
    }

    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    m_DraggingNow = node;
    wxTextDataObject* obj = new wxTextDataObject;
    obj->SetText(node->m_key);
    event.SetDataObject(obj);
    event.SetDragFlags(wxDrag_AllowMove); // allows both copy and move

    wxLogMessage("Starting dragging \"%s\"", node->m_key);
}

void MyFrame::OnDropPossible( wxDataViewEvent &event )
{
    // Only items that are part of an array may be reordered.
    // Item is array item and has to be the same level (have same parent).
    //wxLogMessage("Proposed drop index = %i", "", event.GetProposedDropIndex());

    if (event.GetDataFormat() != wxDF_UNICODETEXT) {
        event.Veto();
    }

    // Node dropped on parent.
    // Node dropped on other child.
    // Node dropped on other location.
    wxDataViewItem item(event.GetItem());
    EntityTreeModelNode* Node = (EntityTreeModelNode*)item.GetID();
    if (m_DraggingNow->GetParent() == Node->GetParent()) {
        // Allow
        event.SetDropEffect(wxDragMove); // check 'move' drop effect

    } else if (m_DraggingNow->GetParent() == Node) {
        // Allow
        event.SetDropEffect(wxDragMove); // check 'move' drop effect

    } else {
        event.Veto();
    }
}

void MyFrame::OnDrop( wxDataViewEvent &event )
{
    wxDataViewItem item(event.GetItem());
    if (event.GetDataFormat() != wxDF_UNICODETEXT) {
        event.Veto();
        return;
    }

    EntityTreeModelNode* Node = (EntityTreeModelNode*)item.GetID();
    //if ( item.IsOk() )
    //{
        // These are invalid..
        // if (m_entity_view_model->IsContainer(item))
        // {
        //     wxLogMessage("Text '%s' dropped in container '%s' (proposed index = %i)",
        //                  obj.GetText(), m_entity_view_model->GetKey(item), event.GetProposedDropIndex());
        // }
        // else
        //     wxLogMessage("Text '%s' dropped on item '%s'", obj.GetText(), m_entity_view_model->GetKey(item));
    //}
    //else
        wxLogMessage("Text '%s' dropped on background (proposed index = %i)", Node->m_key, event.GetProposedDropIndex());

    // Node dropped on parent.
    size_t DropIndex = 0;
    if (m_DraggingNow->GetParent() == Node) {
        DropIndex = event.GetProposedDropIndex();
        if (DropIndex == (-1)) {
            return;
        }

        size_t OriginalIndex = Node->GetChildIndex(m_DraggingNow);
        if (OriginalIndex < DropIndex) {
            DropIndex -= 1;
        }

    } else if (m_DraggingNow == Node) {
        return;

    } else if (m_DraggingNow->GetParent() == Node->GetParent()) {
        // Node dropped on other child.
        DropIndex = Node->GetParent()->GetChildIndex(Node);
        size_t OriginalIndex = Node->GetParent()->GetChildIndex(m_DraggingNow);
        if (OriginalIndex < DropIndex) {
            if (event.GetProposedDropIndex() == -1) {
                DropIndex -= 1;
            }

        } else {
            if (event.GetProposedDropIndex() != -1) {
                DropIndex += 1;
            }
        }

    } else {
        // Node dropped on other location.
        return;
    }

    // Construct a command group.
    GroupedCommand Group = make_shared<_GroupedCommand>();
    
    // Construct a remove command.
    CommandPattern Delete = make_shared<DeleteSubTreeCommand>(wxDataViewItem(m_DraggingNow), m_entity_view_model, m_Document);

    // Construct an insert command.
    bool NeedsObjectWrapper = m_DraggingNow->IsWrapped();
    CommandPattern Insert = make_shared<InsertSubTreeCommand>(wxDataViewItem(m_DraggingNow), wxDataViewItem(m_DraggingNow->GetParent()), DropIndex, m_entity_view_model, m_Document, NeedsObjectWrapper, false);

    Group->PushCommand(Delete);
    Group->PushCommand(Insert);
    Group->Execute();
    PushCommand(Group);
}

#endif // wxUSE_DRAG_AND_DROP

void MyFrame::DeleteSelectedItems()
{
    wxDataViewItemArray items;
    GroupedCommand Group = std::make_shared<_GroupedCommand>();
    
    int len = m_ctrl[Page_EntityView]->GetSelections( items );
    for (int i = 0; i < len; i += 1) {
        if (items[i].IsOk()) {
            CommandPattern Command = std::make_shared<DeleteSubTreeCommand>(items[i], m_entity_view_model, m_Document);
            Group->PushCommand(Command);
            Command->Execute();
        }
    }

    if (Group->Empty() == false) {
        PushCommand(Group);
    }
}

void MyFrame::OnDeleteSelected( wxCommandEvent& WXUNUSED(event) )
{
    DeleteSelectedItems();
}

void MyFrame::OnCollapse( wxCommandEvent& WXUNUSED(event) )
{
    wxDataViewItem item = m_ctrl[Page_EntityView]->GetSelection();
    if (item.IsOk())
        m_ctrl[Page_EntityView]->Collapse( item );
}

void MyFrame::OnExpand( wxCommandEvent& WXUNUSED(event) )
{
    wxDataViewItem item = m_ctrl[Page_EntityView]->GetSelection();
    if (item.IsOk())
        m_ctrl[Page_EntityView]->Expand( item );
}

void MyFrame::OnShowCurrent(wxCommandEvent& WXUNUSED(event))
{
    wxDataViewItem item = m_ctrl[Page_EntityView]->GetCurrentItem();
    if ( item.IsOk() )
    {
        wxLogMessage("Current item: \"%s\" by %s",
                     m_entity_view_model->GetKey(item),
                     m_entity_view_model->GetValue(item));
    }
    else
    {
        wxLogMessage("There is no current item.");
    }

    wxDataViewColumn *col = m_ctrl[Page_EntityView]->GetCurrentColumn();
    if ( col )
    {
        wxLogMessage("Current column: %d",
                     m_ctrl[Page_EntityView]->GetColumnPosition(col));
    }
    else
    {
        wxLogMessage("There is no current column.");
    }
}

void MyFrame::OnValueChanged(wxDataViewEvent& event)
{
    wxString title = m_entity_view_model->GetKey(event.GetItem());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, Item Id: %s;  Column: %d",
        title, event.GetColumn());
}

void MyFrame::OnActivated(wxDataViewEvent& event)
{
    wxString title = m_entity_view_model->GetKey(event.GetItem());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_ACTIVATED, Item: %s; Column: %d",
        title, event.GetColumn());

    if (m_ctrl[Page_EntityView]->IsExpanded(event.GetItem()))
    {
        wxLogMessage("Item: %s is expanded", title);
    }
}

void MyFrame::OnSelectionChanged(wxDataViewEvent& event)
{
    if (event.GetModel() != m_entity_view_model.get()) {
        return;
    }

    m_LastNavigation.push_back(event.GetItem());

    wxString title = m_entity_view_model->GetKey(event.GetItem());
    if (title.empty())
        title = "None";

    wxLogMessage("wxEVT_DATAVIEW_SELECTION_CHANGED, First selected Item: %s", title);
}

void MyFrame::OnExpanding(wxDataViewEvent& event)
{
    wxString title = m_entity_view_model->GetKey(event.GetItem());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_EXPANDING, Item: %s", title);
}


void MyFrame::OnStartEditing(wxDataViewEvent& event)
{
    if (m_BuildingEntitiesMap != false) {
        wxMessageBox("Please wait for the entity-map to finish building before editing.", "Still building entity map.");
        event.Veto();
    }

    // if ((event.GetColumn() == 0) && (m_entity_view_model->IsArrayElement(&(event.GetItem())) == false)) {
    //     event.Veto();
    // } else
    if ((event.GetColumn() == 0) && (event.GetItem() == m_entity_view_model->GetRoot())) {
        event.Veto();
    } else if ((event.GetColumn() == 1) && (m_entity_view_model->HasContainerColumns(event.GetItem()))) {
        event.Veto();
    } else {
        wxString value = m_entity_view_model->GetValue(event.GetItem());
        wxLogMessage("wxEVT_DATAVIEW_ITEM_START_EDITING not vetoed. Value: %s", value);
    }
}

class MyTextCompleter : public wxTextCompleterSimple
{
    wxString m_key;

public:
    MyTextCompleter(wxString key): m_key(key) {}
    virtual void GetCompletions(const wxString& prefix, wxArrayString& res)
    {
        auto Entry = ValueMap.find(std::string(m_key.c_str().AsChar()));
        if (Entry != ValueMap.end()) {
            for (auto iter = Entry->second.begin(); iter != Entry->second.end(); iter++) {
                res.push_back(*iter);
            }
        }
    }
};

void MyFrame::OnEditingStarted(wxDataViewEvent& event)
{
    wxDataViewModel* const model = event.GetModel();
    wxVariant value;
    m_entity_view_model->GetJsonValue(value, event.GetItem(), event.GetColumn());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_EDITING_STARTED, current value %s",
        value.GetString());

    m_OldValue = value.GetString();
    auto EditBox = ((wxTextCtrl*)event.GetDataViewColumn()->GetRenderer()->GetEditorCtrl());
    EditBox->SetLabelText(m_OldValue);
    wxVariant keyValue;
    m_entity_view_model->GetJsonValue(keyValue, event.GetItem(), 0);
    EditBox->AutoComplete(new MyTextCompleter(keyValue.GetString()));
    m_ChangeCmd = std::make_shared<ChangeItemCommand>(event.GetItem(), m_entity_view_model, wxT(""), event.GetColumn());
}

void MyFrame::OnEditingDone(wxDataViewEvent& event)
{
    wxLogMessage("wxEVT_DATAVIEW_ITEM_EDITING_DONE, new value %s",
        event.IsEditCancelled()
        ? wxString("unavailable because editing was cancelled")
        : event.GetValue().GetString());

    bool Changed = (event.GetValue().GetString() != m_OldValue);
    if ((event.IsEditCancelled() == false) && (Changed != false)) {
        ((ChangeItemCommand*)m_ChangeCmd.get())->SetNewValue(event.GetValue());
        m_ChangeCmd->Execute();
        PushCommand(m_ChangeCmd);
        OutputDebugString(L"Saved change command\n");

        ResolveEncounterSpawnChange((EntityTreeModelNode*)(event.GetItem().GetID()), m_OldValue);
    }
}

void MyFrame::OnExpanded(wxDataViewEvent& event)
{
    wxString title = m_entity_view_model->GetKey(event.GetItem());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_EXPANDED, Item: %s", title);
}

void MyFrame::OnCollapsing(wxDataViewEvent& event)
{
    wxString title = m_entity_view_model->GetKey(event.GetItem());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_COLLAPSING, Item: %s", title);
}

void MyFrame::OnCollapsed(wxDataViewEvent& event)
{
    wxString title = m_entity_view_model->GetKey(event.GetItem());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_COLLAPSED, Item: %s", title);
}

wxString MyFrame::FindUnusedName(wxString Base)
{
    int Index = 0;
    wxString Unused = Base;
    
    while (m_entity_view_model->CountByName(Unused, true, 2) != 0) {
        Unused = Base + wxString().Format(wxT("_%i"), Index);
        Index += 1;
    }

    return Unused;
}

enum CONTEXT_ID {
    CID_GOTO_REFERENCE,
    CID_DUPLICATE,
    CID_DELETE,
    CID_EDIT_KEY_NAME,
    CID_ADD_ITEM,
    CID_OPEN_FILE,
    CID_REINJECT_FILE,
    CID_MH_GET_POSITION,
    CID_MH_GET_ROTATION,
    CID_COPY,
    CID_PASTE,
    CID_MH_TELEPORT,
    CID_MH_TRIGGER,
    CID_MAX
};

void MyFrame::OnContextMenuSelect(wxCommandEvent& event)
{
    void* data = static_cast<wxMenu*>(event.GetEventObject())->GetClientData();
    wxDataViewEvent* ParentEvent = (wxDataViewEvent*)event.GetEventUserData();

    wxDataViewItem Item = ParentEvent->GetItem();
    int pos = m_ctrl[0]->GetColumnPosition(ParentEvent->GetDataViewColumn());
    switch (event.GetId()) {
    case CID_GOTO_REFERENCE:
    {
        wxVariant var;
        m_entity_view_model->GetValue(var, Item, pos);
        wxString str = wxString("entityDef");
        if (var.GetString().c_str().AsChar()[0] == ' ') {
            str += var.GetString();
        } else {
            str += wxString(" ") + var.GetString();
        }
        wxDataViewItem Select = m_entity_view_model->SelectText(str, eSearchDirection::FIRST, true, true);
        if (Select.IsOk() != false) {
            m_ctrl[0]->SetCurrentItem(Select);
            m_ctrl[0]->EnsureVisible(Select);
            m_LastNavigation.push_back(Select);

        } else {
            wxMessageBox(wxT("Could not resolve:") + str, wxT("Unresolved"), wxICON_INFORMATION | wxOK);
        }
    }
        break;
    case CID_DUPLICATE:
        if ((m_entity_view_model->IsContainer(Item) != false) ||
            (m_entity_view_model->IsArrayElement(&Item) != false)) {

            CommandPattern Cmd = std::make_shared<DuplicateSubTreeCommand>(Item, m_entity_view_model, m_Document);
            Cmd->Execute();

            // Check for duplicate names.
            bool DoRename = false;
            wxString BaseName;
            EntityTreeModelNode* Node = (EntityTreeModelNode*)Item.GetID();
            if (strcmp(Node->m_keyRef->GetString(), "entity") == 0) {
                // Find entityDef node inside childeren.
                size_t Counter = 0;
                wxString EntityDefStr("entityDef ");
                auto NextEntityDef = Node->FindByName(0, 1, EntityDefStr, 0, Counter, false, true);

                assert(NextEntityDef != nullptr);

                BaseName = NextEntityDef->m_key;
                if (m_entity_view_model->CountByName(BaseName, true, 2) != 0) {
                    wxString Question = wxString().Format("Found multiple definitions of:%s\nDo you want to auto rename?", BaseName);
                    int Result = wxMessageBox(Question, wxT("Multiple definitions"), wxICON_QUESTION | wxYES | wxNO);
                    if (Result == wxYES) {
                        DoRename = true;
                    }
                }
            }

            // Ask user whether to rename.
            if (DoRename != false) {
                size_t Counter = 0;
                EntityTreeModelNode* SecondNode = ((DuplicateSubTreeCommand*)Cmd.get())->m_NewItem;
                SecondNode = SecondNode->FindByName(0, 1, BaseName, 0, Counter, true, true);
                if (SecondNode != nullptr) {
                    EntityTreeModelNode* SecondParentNode = SecondNode->GetParent();
                    GroupedCommand Group = std::make_shared<_GroupedCommand>();
                    wxString UnusedName = FindUnusedName(BaseName);

                    // Rename the new node.
                    CommandPattern RenameCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(SecondNode), m_entity_view_model, UnusedName, 0);
                    CommandPattern RenameVisualCmd = std::make_shared<RenameVisualCommand>(wxDataViewItem(SecondParentNode), m_entity_view_model, UnusedName.substr(10), 0);
                    RenameCmd->Execute();
                    RenameVisualCmd->Execute();
                    Group->PushCommand(Cmd);
                    Group->PushCommand(RenameCmd);
                    Group->PushCommand(RenameVisualCmd);
                    PushCommand(Group);
                }
            } else {
                PushCommand(Cmd);
            }
        }
        break;
        case CID_DELETE:
        {
            CommandPattern Cmd = std::make_shared<DeleteSubTreeCommand>(Item, m_entity_view_model, m_Document);
            Cmd->Execute();
            PushCommand(Cmd);
        }
        break;
        case CID_EDIT_KEY_NAME:
        {
            // Spawn a dialog with a text box to allow new name input.
            // If the current node is an entitydef node, check for duplicates.
            // Highlight red if duplicate.
#ifdef _DEBUG
            AddItemDialog dlg(this);
            dlg.Create();
            dlg.ShowModal();
#endif // DEBUG
        }
        case CID_ADD_ITEM:
        {
#ifdef _DEBUG
            // Show dialog with possible helper entities.
            // Construct a subtree.
            // Add the new subtree to the main tree.
            AddItemDialog dlg(this);
            dlg.Create();
            dlg.ShowModal();
#endif
        }
        break;
        case CID_REINJECT_FILE:
        {
            FileTreeModelNode* Node = (FileTreeModelNode*)Item.GetID();
            auto FileInfo = Node->GetIDFile();

            // Recompress the file.
            byte* Data = nullptr;
            size_t DataSize = 0;
            StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer,
                rapidjson::UTF8<char>,
                rapidjson::UTF8<char>,
                rapidjson::CrtAllocator,
                rapidjson::kWriteValidateEncodingFlag |
                rapidjson::kWriteNanAndInfFlag> writer(buffer);
            writer.SetIndent('\t', 1);
            m_Document.Accept(writer, 0);

            // Compress
            char *CompressedBuffer;
            size_t CompressedSize;
            CompressEntities((byte*)buffer.GetString(), buffer.GetSize(), &CompressedBuffer, CompressedSize);

            // Write out to .resources.
            m_ResourceReader.WriteFile(FileInfo, CompressedBuffer, CompressedSize);
        }
        break;
        case CID_OPEN_FILE:
        {
            FileTreeModelNode* Node = (FileTreeModelNode*)Item.GetID();
            auto FileInfo = Node->GetIDFile();
            OpenEntitiesFromResources(FileInfo);
        }
        break;
        case CID_MH_GET_POSITION:
        {
            EntityTreeModelNode* Node = (EntityTreeModelNode*)Item.GetID();
            SetLocationNodeFromMH(Node);
        }
        break;
        case CID_MH_GET_ROTATION:
        {
            EntityTreeModelNode* Node = (EntityTreeModelNode*)Item.GetID();
            SetRotationNodeFromMH(Node);
        }
        break;
        case CID_COPY:
        {
            CopyToClipBoard((EntityTreeModelNode*)Item.GetID());
        }
        break;
        case CID_PASTE:
        {
            InsertFromClipBoard((EntityTreeModelNode*)Item.GetID());
        }
        break;
        case CID_MH_TELEPORT:
        {
            // Find the entity def of this object.
            EntityTreeModelNode *Node = GetEntityDefNode((EntityTreeModelNode*)Item.GetID());
            if (Node != nullptr) {
                wxString Name = Node->m_key.substr(10);
                if (Name.Len() != 0) {
                    Name = "teleport " + Name;
                    m_MeatHook.ExecuteConsoleCommand((unsigned char*)Name.c_str().AsChar());
                }
            }
        }
        break;
        case CID_MH_TRIGGER:
        {
            // Find the entitydef of this trigger.
            EntityTreeModelNode* Node = GetEntityDefNode((EntityTreeModelNode*)Item.GetID());
            if (Node != nullptr) {
                wxString Name = Node->m_key.substr(10);
                if (Name.Len() != 0) {
                    Name = "trigger " + Name;
                    m_MeatHook.ExecuteConsoleCommand((unsigned char*)Name.c_str().AsChar());
                }
            }
        }
        break;
        default:
            if (m_MenuMap.find(event.GetId()) != m_MenuMap.end()) {
                wxString MenuString = m_MenuMap[event.GetId()];
                EntityTreeModelNode* Node = ConstructInsertionTree(MenuString, m_Document);
                if (Node != nullptr) {
                    EntityTreeModelNode* ItemNode = (EntityTreeModelNode*)Item.GetID();
                    size_t Index = ItemNode->GetParent()->GetChildIndex(ItemNode);
                    bool Wrapped = ItemNode->IsWrapped();
                    CommandPattern Insert = make_shared<InsertSubTreeCommand>(
                        wxDataViewItem(Node),
                        wxDataViewItem(ItemNode->GetParent()),
                        Index,
                        m_entity_view_model,
                        m_Document,
                        Wrapped
                        );

                    Insert->Execute();
                    PushCommand(Insert);
                }
            }
        break;
    }
}

void MyFrame::OpenEntitiesFromResources(IDCT_FILE &FileInfo)
{
    class OneShotReadBuf : public std::streambuf
    {
        public:
        OneShotReadBuf(char* s, std::size_t n) : streambuf()
        {
            setg(s, s, s + n);
        }
    };

    char *Membuffer = new char[FileInfo.Size];
    m_ResourceReader.ReadFile(FileInfo, Membuffer, FileInfo.Size);
    OneShotReadBuf StreamBuff(Membuffer, FileInfo.Size);
    std::istream Stream(&StreamBuff);

    // Oodle decompress.
    char* DecompressedData = nullptr;
    size_t DecompressedSize;
    int Result = DecompressEntities(&Stream, &DecompressedData, DecompressedSize, FileInfo.Size);
    if (Result == S_OK) {
        MemoryStream memstream(DecompressedData, DecompressedSize);
        m_Document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(memstream);
        if (m_Document.HasParseError() != false) {
            wxMessageBox(wxString::Format("Error parsing the entities definition file. (Syntax error offset: %llu).", m_Document.GetErrorOffset()), _("Error"), wxOK, this);
        }

    } else {
        wxMessageBox(_("Error parsing the entities definition file. (Could not decompress)."), _("Error"), wxOK, this);
    }

    if (DecompressedData != nullptr) {
        delete[] DecompressedData;
    }

    if (Membuffer != nullptr) {
        delete[] Membuffer;
    }

    if (m_Document.HasParseError() == false) {
        m_CurrentlyLoadedFileName = m_ResourceReader.m_FileName;
    }

    ConstructTreeView();
}

void MyFrame::OnContextMenu( wxDataViewEvent &event )
{
    wxString title = m_entity_view_model->GetKey( event.GetItem() );
    int pos = m_ctrl[Page_EntityView]->GetColumnPosition(event.GetDataViewColumn());
    wxLogMessage( "wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, Item: %s Column:%i", title, pos );

    wxMenu *SubMenuInsert = new wxMenu;
    wxMenu menu;
    menu.Append(CID_GOTO_REFERENCE, "Goto reference");
    menu.Append(CID_DUPLICATE, "Duplicate");
    menu.Append(CID_DELETE, "Delete");
    ConstructInsertSubMenu(SubMenuInsert, (EntityTreeModelNode*)event.GetItem().GetID(), m_MenuMap);
    // Disable insert menu if there is nothing in there.
    if (SubMenuInsert->GetMenuItemCount() == 0) {
        menu.Append(300, "Insert");
        menu.Enable(300, false);
        delete SubMenuInsert;
    } else  {
        menu.AppendSubMenu(SubMenuInsert, "Insert");
    }
    
#ifdef _DEBUG
    menu.Append(CID_EDIT_KEY_NAME, "Edit key name");
    menu.Append(CID_ADD_ITEM, "Add new node");
#endif
    menu.AppendSeparator();
    menu.Append(CID_COPY, "Copy");
    menu.Append(CID_PASTE, "Paste");

    if (title == "spawnPosition") {
        menu.Append(CID_MH_GET_POSITION, "Get position from MH");
        if (m_MeatHook.m_Initialized == false) {
            menu.Enable(CID_MH_GET_POSITION, false);
        }
    }

    if (title == "spawnOrientation") {
        menu.Append(CID_MH_GET_ROTATION, "Get rotation from MH");
        if (m_MeatHook.m_Initialized == false) {
            menu.Enable(CID_MH_GET_ROTATION, false);
        }
    }

    if ((m_entity_view_model->IsContainer(event.GetItem()) == false)) {
        if (m_entity_view_model->IsArrayElement(&(event.GetItem())) == false) {
            menu.Enable(CID_DUPLICATE, false);
        }
    }

    menu.Append(CID_MH_TELEPORT, "Teleport to entity MH (experimental - may crash Doom)");
    if (m_MeatHook.m_Initialized == false) {
        menu.Enable(CID_MH_TELEPORT, false);
    }

    EntityTreeModelNode *Node = (EntityTreeModelNode*)(event.GetItem().GetID());
    wxString ClassName = GetEntityClassName(Node);
    menu.Append(CID_MH_TRIGGER, "Trigger MH");
    menu.Enable(CID_MH_TRIGGER, false);
    if (ClassName == "idTrigger") {
        if (m_MeatHook.m_Initialized != false) {
            menu.Enable(CID_MH_TRIGGER, true);
        }
    }

    wxDataViewEvent *EventCopy = new wxDataViewEvent(event);
    menu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MyFrame::OnContextMenuSelect), EventCopy, this);
    m_ctrl[Page_EntityView]->PopupMenu(&menu);
}

void MyFrame::QuickFind(wxCommandEvent& event)
{
    int Id = event.GetId();
    wxString str("");
    wxDataViewItem Select;
    switch (Id) {
        case ID_QF_INTRO_CUTSCENE:
        {
            str = "entityDef intro_game_info_logic";
            Select = m_entity_view_model->SelectText(str, eSearchDirection::NEXT, false, true);
        }
        break;
        case ID_QF_SPAWN_LOCATION:
        {
            // Find all the class = "idPlayerStart"; nodes. Check for edit:initial=true
            EntityTreeModelNode* Root = (EntityTreeModelNode*)m_entity_view_model->GetRoot().GetID();
            for (auto Iterator = Root->GetChildren().begin(); Iterator < Root->GetChildren().end(); Iterator++) {
                wxString ClassName = GetEntityClass(*Iterator, (*Iterator)->m_key);
                if (ClassName == "idPlayerStart") {
                    size_t Index = 0;
                    wxString ToFind = "initial";
                    EntityTreeModelNode* Found = (*Iterator)->FindByName(0, 0, ToFind, 0, Index, true, true);
                    if ((Found != nullptr) && (Found->m_value == "true")) {
                        Select = wxDataViewItem(*Iterator);
                        break;
                    }
                }
            }
        }
        break;
        case ID_QF_FIRST_ENCOUNTER:
            str = "entityDef spawn";
            Select = m_entity_view_model->SelectText(str, eSearchDirection::NEXT, false, true);
        break;
    };

    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
        m_LastNavigation.push_back(Select);
    }
}

void MyFrame::OnContextFileMenu(wxDataViewEvent& event)
{
    int pos = m_ctrl[Page_ResourcesView]->GetColumnPosition(event.GetDataViewColumn());
    FileTreeModelNode* node = (FileTreeModelNode*)(event.GetItem()).GetID();

    wxMenu menu;
    menu.Append(CID_OPEN_FILE, "Open file");
    menu.Append(CID_REINJECT_FILE, "Reinject");

    wxString FileName(node->GetIDFile().FileName);
    if (FileName.Matches("*.entities") == false) {
        menu.Enable(CID_OPEN_FILE, false);
        menu.Enable(CID_REINJECT_FILE, false);
    }

    wxDataViewEvent* EventCopy = new wxDataViewEvent(event);
    menu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MyFrame::OnContextMenuSelect), EventCopy, this);
    m_ctrl[Page_ResourcesView]->PopupMenu(&menu);
}

void MyFrame::OnFilterType(wxCommandEvent& event)
{
    if (!m_FilterCtrl->IsModified())
        return;

    m_FilterCtrl->GetValue();
}

void MyFrame::OnFilterSearch(wxCommandEvent& event)
{
    wxString Text = m_FilterCtrl->GetValue();
    EntityTreeModelNode* Node = (EntityTreeModelNode*)(m_ctrl[0]->GetSelection().GetID());
    if (Node == nullptr) {
        Node = (EntityTreeModelNode*)(m_entity_view_model->GetRoot().GetID());
    }

    if (Node == nullptr) {
        return;
    }

    EntityTreeModelNode* Found = Node->FindFromHere(nullptr, Text, eSearchDirection::NEXT, false, m_MatchCaseCheck->IsChecked());
    if (Found == nullptr) {
        wxMessageBox(wxString("Could not find:") + Text, "Search", wxOK);
        return;
    }

    wxDataViewItem Select = wxDataViewItem(Found);
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
        m_LastNavigation.push_back(Select);
    }
}

void MyFrame::OnFilterTypeResources(wxCommandEvent& event)
{
    if (!m_ResourceCtrl->IsModified())
        return;

    m_ResourceCtrl->GetValue();
}

void MyFrame::OnFilterSearchResources(wxCommandEvent& event)
{
    wxString Text = m_ResourceCtrl->GetValue();
    m_file_view_model->SetFilter(Text);
    m_ctrl[Page_ResourcesView]->Expand(wxDataViewItem(m_file_view_model->GetRoot()));
}

void MyFrame::OnHeaderClick( wxDataViewEvent &event )
{
    int pos = m_ctrl[Page_EntityView]->GetColumnPosition( event.GetDataViewColumn() );

    wxLogMessage( "wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, Column position: %d", pos );
    wxLogMessage( "Column width: %d", event.GetDataViewColumn()->GetWidth() );
}

void MyFrame::OnHeaderRightClick( wxDataViewEvent &event )
{
    int pos = m_ctrl[Page_EntityView]->GetColumnPosition( event.GetDataViewColumn() );

    wxLogMessage( "wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, Column position: %d", pos );
}

void MyFrame::OnColumnReordered(wxDataViewEvent& event)
{
    wxDataViewColumn* const col = event.GetDataViewColumn();
    if ( !col )
    {
        wxLogError("Unknown column reordered?");
        return;
    }

    wxLogMessage("wxEVT_DATAVIEW_COLUMN_REORDERED: \"%s\" is now at position %d",
                 col->GetTitle(), event.GetColumn());
}

void MyFrame::OnSorted( wxDataViewEvent &event )
{
    int pos = m_ctrl[Page_EntityView]->GetColumnPosition( event.GetDataViewColumn() );

    wxLogMessage( "wxEVT_DATAVIEW_COLUMN_SORTED, Column position: %d", pos );
}

void MyFrame::OnDataViewChar(wxKeyEvent& event)
{
    if ( event.GetKeyCode() == WXK_DELETE )
        DeleteSelectedItems();
    else
        event.Skip();
}

void MyFrame::SaveFile(wxCommandEvent& event)
{
#ifdef _DEBUG
    if (m_CurrentlyLoadedFileName == "") {
        ofstream OfStream("../reloaded.txt", std::ofstream::binary);
        if (OfStream.good() == false) {
            return;
        }

        rapidjson::OStreamWrapper osw(OfStream);
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper,
            rapidjson::UTF8<char>,
            rapidjson::UTF8<char>,
            rapidjson::CrtAllocator,
            rapidjson::kWriteValidateEncodingFlag |
            rapidjson::kWriteNanAndInfFlag> writer(osw);
        writer.SetIndent('\t', 1);
        m_Document.Accept(writer, 0);
    }
#endif

    if ((m_CurrentlyLoadedFileName != "") && (m_CurrentlyLoadedFileName.rfind(".entities") != wxString::npos)) {
        // Recompress the file.
        if (m_CurrentlyLoadedFileCompressed != false) {
            byte* Data = nullptr;
            size_t DataSize = 0;
            StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer,
                rapidjson::UTF8<char>,
                rapidjson::UTF8<char>,
                rapidjson::CrtAllocator,
                rapidjson::kWriteValidateEncodingFlag |
                rapidjson::kWriteNanAndInfFlag> writer(buffer);
            writer.SetIndent('\t', 1);
            m_Document.Accept(writer, 0);
            CompressEntities(m_CurrentlyLoadedFileName.c_str().AsChar(), (byte*)buffer.GetString(), buffer.GetSize());

        } else {
            ofstream OfStream(m_CurrentlyLoadedFileName.c_str().AsChar(), std::ofstream::binary);
            if (OfStream.good() == false) {
                wxMessageBox(wxString::Format("Could not open %s for writing.", m_CurrentlyLoadedFileName), "Error", wxICON_EXCLAMATION|wxOK);
                return;
            }

            rapidjson::OStreamWrapper osw(OfStream);
            rapidjson::PrettyWriter<rapidjson::OStreamWrapper,
                rapidjson::UTF8<char>,
                rapidjson::UTF8<char>,
                rapidjson::CrtAllocator,
                rapidjson::kWriteValidateEncodingFlag |
                rapidjson::kWriteNanAndInfFlag> writer(osw);
            writer.SetIndent('\t', 1);
            m_Document.Accept(writer, 0);
        }
    }
}

void MyFrame::SaveFileAs(wxCommandEvent& event)
{
    wxFileDialog
        saveFileDialog(this, _("Save .entities file"), "", "",
            "Entities files (*.entities)|*.entities", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    // Recompress the file.
    byte* Data = nullptr;
    size_t DataSize = 0;
    StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer,
        rapidjson::UTF8<char>,
        rapidjson::UTF8<char>,
        rapidjson::CrtAllocator,
        rapidjson::kWriteValidateEncodingFlag |
        rapidjson::kWriteNanAndInfFlag> writer(buffer);
    writer.SetIndent('\t', 1);
    m_Document.Accept(writer, 0);

#ifdef _DEBUG
    FILE *file = fopen("../testoutput.txt", "wb");
    fwrite((byte*)buffer.GetString(), 1, buffer.GetSize(), file);
    fclose(file);
#endif

    CompressEntities(saveFileDialog.GetPath().c_str().AsChar(), (byte*)buffer.GetString(), buffer.GetSize());
}

void MyFrame::ExportFile(wxCommandEvent& event)
{
    wxFileDialog
        saveFileDialog(this, _("Save .txt file"), "", "",
            "Entities files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    ofstream OfStream(saveFileDialog.GetPath().c_str().AsChar(), std::ofstream::binary);
    if (OfStream.good() == false) {
        return;
    }

    rapidjson::OStreamWrapper osw(OfStream);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper,
        rapidjson::UTF8<char>,
        rapidjson::UTF8<char>,
        rapidjson::CrtAllocator,
        rapidjson::kWriteValidateEncodingFlag |
        rapidjson::kWriteNanAndInfFlag> writer(osw);
    writer.SetIndent('\t', 1);
    m_Document.Accept(writer, 0);
}

void MyFrame::ScheduleBuildEncounterToEntityMap (
    void
    )
{
    if (m_Thread.valid()) {
        m_Thread.wait();
    }

    if (m_ProgressText != nullptr) {
        m_ProgressSizer->Show(true);
        m_ProgressText->SetLabelText("Building entity map.");
        mainSizer->Layout();
    }

    m_BuildingEntitiesMap = true;
    m_Thread = std::async(BuildEncounterEntityMap2,
                          (EntityTreeModelNode*)(m_entity_view_model->GetRoot().GetID()),
                          &m_BuildingEntitiesMap,
                          &m_ProgressCurrent,
                          &m_ProgressEnd
                          );

    m_EndThread = std::async(std::launch::async, [this](){
        if (m_Thread.valid()) {
            m_Thread.wait();
        }

        if (m_ProgressSizer != nullptr) {
            m_ProgressSizer->Show(false);
            mainSizer->Layout();
        }
    });

    m_UpdateTimerThread = std::async(std::launch::async, [this]() {
        if (m_ProgressGuage == nullptr) {
            return;
        }

        while (m_BuildingEntitiesMap != false) {
            m_ProgressGuage->SetRange(m_ProgressEnd);
            m_ProgressGuage->SetValue(m_ProgressCurrent);
            Sleep(1000);
        }
    });
}

void MyFrame::ConstructTreeView()
{
    m_ProgressSizer->Show(true);
    m_ProgressText->SetLabelText("Constructing tree view.");
    mainSizer->Layout();

    m_ctrl[0]->ClearColumns();
    m_entity_view_model = new EntityTreeModel(m_Document);

    // Call build instead ? (Or separate this function to prevent duplication)
    m_ctrl[0]->AssociateModel(m_entity_view_model.get());
    //m_entity_view_model->DecRef();
    m_ctrl[0]->Expand(m_entity_view_model->GetRoot());

    m_ctrl[Page_EntityView]->EnableDragSource(wxDF_UNICODETEXT);
    m_ctrl[Page_EntityView]->EnableDropTarget(wxDF_UNICODETEXT);

    // column 0 of the view control:
    wxDataViewTextRenderer* tr =
        new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_EDITABLE);
    wxDataViewColumn* column0 =
        new wxDataViewColumn("key", tr, 0, FromDIP(400), wxALIGN_LEFT,
            wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE);
    m_ctrl[Page_EntityView]->AppendColumn(column0);

    // column 1 of the view control:
    tr = new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_EDITABLE);
    wxDataViewColumn* column1 =
        new wxDataViewColumn("value", tr, 1, FromDIP(700), wxALIGN_LEFT,
            wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE |
            wxDATAVIEW_COL_RESIZABLE);
    column1->SetMinWidth(FromDIP(150)); // this column can't be resized to be smaller
    m_ctrl[Page_EntityView]->AppendColumn(column1);

    m_UndoStack.clear();
    m_RedoStack.clear();

    ScheduleBuildEncounterToEntityMap();
}

void MyFrame::OpenFile(wxCommandEvent& event)
{
    bool SafeToLoad = true;
    if (SafeToLoad == false)
    {
        if (wxMessageBox(_("Current content has not been saved! Proceed?"), _("Please confirm"),
            wxICON_QUESTION | wxYES_NO, this) == wxNO)
            return;
        //else: proceed asking to the user the new file to open
    }

    wxFileDialog
        openFileDialog(this, _("Open .entities file"), "", "",
            "Doom files (*.entities;*.resources)|*.entities;*.resources|Resources files (*.resources)|*.resources", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString Path = openFileDialog.GetPath();
    //m_FileOpenThread = std::async(std::launch::async, [this, Path]() {OpenFileInternal(wxString(Path)); });
    OpenFileInternal(wxString(Path));
}

bool MyFrame::OpenFileInternal(wxString FilePath)
{
    bool ResourcesFile = false;
    IDCLReader Resources;
    ResourcesFile = Resources.OpenFile(FilePath);
    if (ResourcesFile != false) {
        // Find entities strings.
        m_ResourceReader.OpenFile(FilePath);
        m_file_view_model = new FileTreeModel(m_ResourceReader, "*");
        m_ctrl[Page_ResourcesView]->AssociateModel(m_file_view_model.get());
        m_ctrl[Page_ResourcesView]->Expand(wxDataViewItem(m_file_view_model->GetRoot()));

        // If only one entities file ask user whether to load automatically.
        auto FileList = m_ResourceReader.GetFileList();
        size_t EntitiesCount = 0;
        IDCT_FILE LastEntities;
        for (auto File : FileList) {
            if (File.FileName.rfind(".entities") != string::npos) {
                EntitiesCount += 1;
                LastEntities = File;
            }
        }

        // Ask user whether to Auto load entities file.
        if (EntitiesCount == 1) {
            if (wxMessageBox(_("Found one single entities file in the resources archive. Would you like to autoload?"), _("Autoload?"),
                wxICON_QUESTION | wxYES_NO, this) == wxYES) {

                m_ProgressSizer->Show(true);
                m_ProgressText->SetLabelText("Loading enties file.");
                mainSizer->Layout();
                m_ResourceCtrl->SetHelpText(".entities");
                m_ResourceCtrl->SetLabelText(".entities");
                m_ResourceCtrl->SetHint("Filter");
                m_file_view_model->SetFilter(".entities");
                m_ctrl[Page_ResourcesView]->Expand(wxDataViewItem(m_file_view_model->GetRoot()));
                OpenEntitiesFromResources(LastEntities);
            }

        } else if (EntitiesCount > 1) {
            // Multiple entities
            // Set filter to *.entities - do not load and messagebox explaining.
            wxMessageBox(_("There are multiple entities files in the resources archive please select one from the resource view."), _("Multiple entities"), wxICON_QUESTION | wxOK, this);
            m_ResourceCtrl->SetHelpText(".entities");
            m_file_view_model->SetFilter(".entities");
            m_ctrl[Page_ResourcesView]->Expand(wxDataViewItem(m_file_view_model->GetRoot()));
        }

        m_file_menu->Enable(ID_SAVE_FILE, false);
        m_file_menu->Enable(ID_SAVE_FILE_AS, true);
        return true;
    }

    ifstream InputStreamBinary(FilePath.c_str().AsChar(), std::ios_base::binary);
    InputStreamBinary.seekg(0, InputStreamBinary.end);
    size_t Size = InputStreamBinary.tellg();
    InputStreamBinary.seekg(0, InputStreamBinary.beg);

    // Oodle decompress.
    m_ProgressSizer->Show(true);
    m_ProgressText->SetLabelText("Loading enties file.");
    mainSizer->Layout();

    int Error = 0;
    char *DecompressedData = nullptr;
    size_t DecompressedSize;
    int Result = DecompressEntities(&InputStreamBinary, &DecompressedData, DecompressedSize, Size);
    if (Result == S_OK) {
        MemoryStream memstream(DecompressedData, DecompressedSize);
        m_Document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(memstream);
        if (m_Document.HasParseError() != false) {
            wxString error = wxString::Format("Error parsing the entities definition file. (Syntax error at offset: %llu) _3", (long long)m_Document.GetErrorOffset());
            wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR, this);
            Error = 1;
#if 0
            FILE *FilePtr;
            fopen_s(&FilePtr, "C:\\Programming\\EntityHero\\wxGUI\\OutputTest.txt", "wb");
            fwrite(DecompressedData, 1, DecompressedSize, FilePtr);
            fclose(FilePtr);
#endif
        } else {
            m_CurrentlyLoadedFileCompressed = true;
        }

    } else {
        ifstream InputStream(FilePath.c_str().AsChar());
        if (InputStream.good() != false) {
            rapidjson::IStreamWrapper InStream(InputStream);
            m_Document.ParseStream<rapidjson::kParseCommentsFlag |
                                   rapidjson::kParseTrailingCommasFlag |
                                   rapidjson::kParseNanAndInfFlag>(InStream);
        }

        if (m_Document.HasParseError() != false) {
            wxMessageBox(wxString::Format("Error parsing the entities definition file. (Could not decompress or parse).\n%s\n (Syntax error at offset: %llu)", FilePath, (long long)m_Document.GetErrorOffset()), _("Error"), wxOK, this);
            Error = 1;

        } else {
            m_CurrentlyLoadedFileCompressed = false;
        }
    }

    if (DecompressedData != nullptr) {
        delete[] DecompressedData;
    }

    if (m_Document.HasParseError() == false) {
        m_CurrentlyLoadedFileName = FilePath;
    }

    ConstructTreeView();

    if (Error == 0) {
        m_file_menu->Enable(ID_SAVE_FILE, true);
        m_file_menu->Enable(ID_SAVE_FILE_AS, true);
    }

    return true;
}

void MyFrame::OpenFromMeathook(wxCommandEvent& event)
{
    char Path[MAX_PATH];
    size_t PathSize = sizeof(Path);
    if (m_MeatHook.GetEntitiesFile((unsigned char*)Path, &PathSize) != false) {
        Path[PathSize] = 0;
        // Load from file.
        //m_FileOpenThread = std::async(std::launch::async, [this, Path](){OpenFileInternal(wxString(Path));} );
        OpenFileInternal(wxString(Path));
    }
}

void MyFrame::ImportFile(wxCommandEvent& event)
{
    bool SafeToLoad = true;
    if (SafeToLoad == false)
    {
        if (wxMessageBox(_("Current content has not been saved! Proceed?"), _("Please confirm"),
            wxICON_QUESTION | wxYES_NO, this) == wxNO)
            return;
        //else: proceed asking to the user the new file to open
    }

    wxFileDialog
        openFileDialog(this, _("Open .txt file"), "", "",
            "Doom Entities text files (*.txt)|*.txt", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    // proceed loading the file chosen by the user;
    // this can be done with e.g. wxWidgets input streams:

    ifstream InputStream(openFileDialog.GetPath().c_str().AsChar());
    if (InputStream.good() == false) {
        return;
    }

    bool Error = false;
    rapidjson::IStreamWrapper InStream(InputStream);
    m_Document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(InStream);
    if (m_Document.HasParseError() != false) {
        wxMessageBox(wxString::Format("Error parsing the entities syntax. (Syntax error offset : % llu).", m_Document.GetErrorOffset()), _("Error"), wxOK, this);
        Error = true;
    }

    ConstructTreeView();

    return;
}

void MyFrame::PushCommand(CommandPattern Command)
{
    m_UndoStack.push_back(Command);
    m_RedoStack.clear();
}

void MyFrame::Undo(wxCommandEvent& event)
{
    if (m_UndoStack.empty() != false) {
        return;
    }

    CommandPattern Top;
    Top = m_UndoStack.back();
    m_UndoStack.pop_back();
    Top->Revert();
    m_RedoStack.push_back(Top);
}

void MyFrame::Redo(wxCommandEvent& event)
{
    if (m_RedoStack.empty() != false) {
        return;
    }

    CommandPattern Top;
    Top = m_RedoStack.back();
    m_RedoStack.pop_back();
    Top->Execute();
    m_UndoStack.push_back(Top);
}

void MyFrame::NavBackward(wxCommandEvent& event)
{
    if (m_LastNavigation.empty() != false) {
        return;
    }

    wxDataViewItem CurrentNavLocation = m_LastNavigation.back();
    m_LastNavigation.pop_back();
    m_NextNavigation.push_back(CurrentNavLocation);

    if (m_LastNavigation.empty() != false) {
        return;
    }

    CurrentNavLocation = m_LastNavigation.back();

    // Select and make visible.
    if (CurrentNavLocation.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(CurrentNavLocation);
        m_ctrl[0]->EnsureVisible(CurrentNavLocation);
    }
}

void MyFrame::NavForward(wxCommandEvent& event)
{
    if (m_NextNavigation.empty() != false) {
        return;
    }

    wxDataViewItem CurrentNavLocation = m_NextNavigation.back();
    m_NextNavigation.pop_back();
    m_LastNavigation.push_back(CurrentNavLocation);
    if (m_LastNavigation.size() == 1) {
        if (m_NextNavigation.empty() != false) {
            return;
        }

        CurrentNavLocation = m_NextNavigation.back();
    }

    // Select and make visible.
    if (CurrentNavLocation.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(CurrentNavLocation);
        m_ctrl[0]->EnsureVisible(CurrentNavLocation);
    }
}

void MyFrame::SearchBackward(wxCommandEvent& event)
{
    wxString str = m_FilterCtrl->GetValue().c_str();
    EntityTreeModelNode* Node = (EntityTreeModelNode*)(m_ctrl[0]->GetSelection().GetID());
    if (Node == nullptr) {
        Node = (EntityTreeModelNode*)(m_entity_view_model->GetRoot().GetID());
    }

    if (Node == nullptr) {
        return;
    }

    EntityTreeModelNode* Found = Node->FindFromHere(nullptr, str, eSearchDirection::PREV, false, m_MatchCaseCheck->IsChecked());
    if (Found == nullptr) {
        wxMessageBox(wxString("Could not find:") + str, "Search", wxOK);
        return;
    }

    wxDataViewItem Select = wxDataViewItem(Found);
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
        m_LastNavigation.push_back(Select);
    }
}

void MyFrame::SearchForward(wxCommandEvent& event)
{
    wxString str = m_FilterCtrl->GetValue().c_str();
    EntityTreeModelNode* Node = (EntityTreeModelNode*)(m_ctrl[0]->GetSelection().GetID());
    if (Node == nullptr) {
        Node = (EntityTreeModelNode*)(m_entity_view_model->GetRoot().GetID());
    }

    if (Node == nullptr) {
        return;
    }

    EntityTreeModelNode* Found = Node->FindFromHere(nullptr, str, eSearchDirection::NEXT, false, m_MatchCaseCheck->IsChecked());
    if (Found == nullptr) {
        wxMessageBox(wxString("Could not find:") + str, "Search", wxOK);
        return;
    }

    wxDataViewItem Select = wxDataViewItem(Found);
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
        m_LastNavigation.push_back(Select);
    }
}

void MyFrame::MHPause(wxCommandEvent& event)
{
    m_MeatHook.ExecuteConsoleCommand((unsigned char*)"noclip");
    m_MeatHook.ExecuteConsoleCommand((unsigned char*)"notarget");
}

void MyFrame::MHReload(wxCommandEvent& event)
{
    char Directory[MAX_PATH];
    GetCurrentDirectoryA(sizeof(Directory), Directory);
    strcat(Directory, "temp_entities.txt");
    ofstream OfStream(Directory, std::ofstream::binary);
    if (OfStream.good() == false) {
        return;
    }

    {
    rapidjson::OStreamWrapper osw(OfStream);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper,
        rapidjson::UTF8<char>,
        rapidjson::UTF8<char>,
        rapidjson::CrtAllocator,
        rapidjson::kWriteValidateEncodingFlag |
        rapidjson::kWriteNanAndInfFlag> writer(osw);
    writer.SetIndent('\t', 1);
    m_Document.Accept(writer, 0);
    }
    // Reduce the writer to only one and do the file write.
    byte* Data = nullptr;
    size_t DataSize = 0;
    StringBuffer buffer;
    {
    rapidjson::PrettyWriter<rapidjson::StringBuffer,
        rapidjson::UTF8<char>,
        rapidjson::UTF8<char>,
        rapidjson::CrtAllocator,
        rapidjson::kWriteValidateEncodingFlag |
        rapidjson::kWriteNanAndInfFlag> writer(buffer);
    writer.SetIndent('\t', 1);
    m_Document.Accept(writer, 0);
    }
    m_MeatHook.PushEntitiesFile(Directory, (char*)buffer.GetString(), buffer.GetSize());
}

void MyFrame::MHStatusCheck(wxTimerEvent& event)
{
    wxString InterfaceText = wxString::Format("%s %s", m_MhText, m_MeatHook.m_Initialized ? "(connected)" : "(inactive)");
    m_MHInterfaceStatus->SetLabelText(InterfaceText);
    m_file_menu->Enable(ID_OPEN_MEATHOOK, m_MeatHook.m_Initialized);
}

void MyFrame::ResolveEncounterSpawnChange(EntityTreeModelNode *EncounterNode, wxString OldValue)
{
    // Check if key is of type "eEncounterSpawnType_t" and the value being edited is at column 1.
    wxString KeyName = EncounterNode->m_key;
    if (KeyName != "eEncounterSpawnType_t") {
        return;
    }

    // Grab the encounter name
    //   1. GetRootNode 
    //   2. Find eventDef = eventDef
    //   2. Check if spawnSignleAi or SpawnAi or maintainAICount
    //      SpawnSingleAI: Entity = Item[1]
    //      SpawnAi:       Entity = Item[2]
    //    maintainAICount: Entity = Item[3]
    //   2. Find(class) == idTarget_Spawn

    auto EventCallNode = EncounterNode->GetParent()->GetParent();
    wxString eventDefine = EventCallNode->Find("eventDef")->m_value;
    auto Descriptor = EventDescriptor[eventDefine.c_str().AsChar()];
    auto Entry = Descriptor.end();
    for (auto EntryIterator = Descriptor.begin(); EntryIterator != Descriptor.end(); EntryIterator++) {
        if (EntryIterator->first == "idTargetSpawnGroup*") {
            Entry = EntryIterator;
            break;
        }
    }

    if (Entry == Descriptor.end()) {
        for (auto EntryIterator = Descriptor.begin(); EntryIterator != Descriptor.end(); EntryIterator++) {
            if (EntryIterator->first == "idTarget_Spawn*") {
                Entry = EntryIterator;
                break;
            }
        }
    }

    if (Entry == Descriptor.end()) {
        wxString MessageString = wxString::Format("Could not resolve. Make sure the eternalevents.txt file is in the same directory as EntityHero.exe");
        int result = wxMessageBox(MessageString, "Error", wxICON_EXCLAMATION | wxOK);
        return;
    }
    size_t Index = Entry->second.Index;

    // spawnSingleAI:
    //     spawnType: eEncounterSpawnType_t
    //     spawnTarget : idTarget_Spawn *
    //     group_label : char*
    // 
    // spawnAI :
    //     spawnType : eEncounterSpawnType_t
    //     spawn_count : int
    //     spawnGroup : idTargetSpawnGroup *
    //     group_label : char*
    // 
    // staggeredAISpawn :
    //     spawnType : eEncounterSpawnType_t
    //     spawn_count : int
    //     spawnGroup : idTargetSpawnGroup *
    //     group_label : char*
    //     minSpawnStagger : float
    //     maxSpawnStagger : float
    // 
    // groupBudgetSpawn :
    //     pointTotal: int
    //     spawnGroup : idTargetSpawnGroup *
    //     populationDecl : idDeclActorPopulation *
    // 
    // maintainAICount :
    //     spawnType : eEncounterSpawnType_t
    //     desired_count : int
    //     max_spawn_count : int
    //     min_spawn_delay : float
    //     min_ai_for_respawn : int
    //     spawnGroup : idTargetSpawnGroup *
    //     group_label : char*
    //     max_spawn_delay : float

    wxVariant GroupName;
    EntityTreeModelNode *SpawnGroup = EncounterNode->GetParent()->GetNthChild(Index);
    if (SpawnGroup == nullptr) {
        wxString MessageString = wxString::Format("Could not resolve. Make sure the eternalevents.txt file is in the same directory as EntityHero.exe");
        int result = wxMessageBox(MessageString, "Error", wxICON_EXCLAMATION | wxOK);
        return;
    }

    wxDataViewItem Item(SpawnGroup);
    
    wxString EntityStr = wxString::Format("entityDef %s", SpawnGroup->m_value);
    size_t SearchIndex = 0;
    EntityTreeModelNode* Root = (EntityTreeModelNode*)(m_entity_view_model->GetRoot().GetID());
    EntityTreeModelNode* EntityDefNode = Root->FindByName(0, 2, EntityStr, 0, SearchIndex, true, true);
    if (EntityDefNode == nullptr) {
        wxString MessageString = wxString::Format("Failed to get spawngroup for (%s)", SpawnGroup->m_value);
        int result = wxMessageBox(MessageString, "Error", wxICON_EXCLAMATION | wxOK);
        return;
    }

    // Get the encounter nodes layer name.
    EntityTreeModelNode *LayerNode_x = GetLayerNode(EncounterNode);
    wxString LayerValue = "";
    if (LayerNode_x != nullptr) {
        LayerValue = LayerNode_x->m_value;
    }

    // Check if the SpawnGroup supports the new enemy type.
    std::vector<bool> EncounterPresent = SpawnTypeSupportedByGroup(Root, EntityDefNode, EncounterNode);
    bool Found = true;
    for (auto Encounter : EncounterPresent) {
        Found &= Encounter;
    }

    if (Found != false) {
        // Check whether the layers are correct.
        if (IsLayerSupported(EntityDefNode, LayerValue) != false) {
            return;
        }
    }

    // Update targets, this could be adding an existing idAI2 class to the targets.
    // Or duplication an idAI2 node to if that node does not have the necessary layer useage.
    // targets = {
    //     num = 1;
    //     item[0] = "cathedral_ai_heavy_arachnotron_1";
    // }
    size_t EncounterPresentIndex = 0;
    GroupedCommand Command = make_shared<_GroupedCommand>();
    std::vector<wxString> EncounterSpawns = GetSpawnTokens(EncounterNode->m_value);
    for (auto EncounterName : EncounterSpawns) {
        if (EncounterPresent[EncounterPresentIndex] != false) {
            EncounterPresentIndex += 1;
            continue;
        }

        EncounterPresentIndex += 1;

        wxString MessageString = wxString::Format("The encounter type(%s) is not supported by spawnGroup(%s) do you want to auto resolve?", EncounterName, EntityDefNode->m_key);
        int result = wxMessageBox(MessageString, "Spawngroup not compatible", wxICON_EXCLAMATION | wxYES_NO);
        if (result == wxNO) {
            return;
        }

        EntityTreeModelNode *AI2Node = GetExistingAI2Node(Root, EncounterName, LayerValue);
        if (AI2Node == nullptr) {
            // Ask if user wants to duplicate.
            wxString MessageString = wxString::Format("The idAI2(%s) is not part of layer(%s) do you want to duplicate (%s)?", EncounterName, LayerValue, EntityDefNode->m_key, EncounterName);
            int result = wxMessageBox(MessageString, "idAI2 not in layer", wxICON_EXCLAMATION | wxYES_NO);
            if (result == wxNO) {
                return;
            }

            AI2Node = GetExistingAI2Node(Root, EncounterName, "");

            if (AI2Node == nullptr) {
                wxString MessageString = wxString::Format("Could not auto resolve..", EncounterName, EntityDefNode->m_key);
                wxMessageBox(MessageString, "Error", wxICON_EXCLAMATION | wxOK);
                return;
            }

            CommandPattern Duplicate = make_shared<DuplicateSubTreeCommand>(wxDataViewItem(AI2Node), m_entity_view_model, m_Document);
            Command->PushCommand(Duplicate);
            Duplicate->Execute();
            AI2Node = ((DuplicateSubTreeCommand*)(Duplicate.get()))->m_NewItem;

            // Overwrite the layer value.
            EntityTreeModelNode *Layer = AI2Node->Find("layers")->GetChildren()[0];
            CommandPattern ChangeCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(Layer), m_entity_view_model, wxT(""), 1);
            ((ChangeItemCommand*)ChangeCmd.get())->SetNewValue(LayerValue);
            ChangeCmd->Execute();
            Command->PushCommand(ChangeCmd);
        }

        assert(AI2Node != nullptr);

        // Add new array entry into the spawn group.
        {
            EntityTreeModelNode* GroupArrayNode = EntityDefNode->Find("edit:entityDefs");
            if (GroupArrayNode != nullptr) {
                rapidjson::Value ValKey("name", m_Document.GetAllocator());
                wxString EntityName = AI2Node->m_key;
                rapidjson::Value ValValue(EntityName.c_str().AsChar(), m_Document.GetAllocator());
                EntityTreeModelNode* ParentArrayItemNode = new EntityTreeModelNode(GroupArrayNode, "name", EntityName, ValKey, ValValue, m_Document);
                EnumChildren(ParentArrayItemNode, ValValue, m_Document);
                CommandPattern Insert = make_shared<InsertSubTreeCommand>(wxDataViewItem(ParentArrayItemNode), wxDataViewItem(GroupArrayNode), GroupArrayNode->GetChildCount(), m_entity_view_model, m_Document, true);
                Command->PushCommand(Insert);
                Insert->Execute();

            } else {
                // When no entitydefs are present check the parent.
                GroupArrayNode = nullptr;
                EntityTreeModelNode* SpawnGroupParentNode = EntityDefNode->Find("edit:spawnGroupParent");
                if (SpawnGroupParentNode != nullptr) {
                    GroupArrayNode = Root->Find(wxString::Format("%s:entityDef %s:edit:entityDefs", SpawnGroupParentNode->m_value, SpawnGroupParentNode->m_value));
                }

                if (GroupArrayNode != nullptr) {
                    rapidjson::Value ValKey("name", m_Document.GetAllocator());
                    wxString EntityName = AI2Node->m_key;
                    rapidjson::Value ValValue(EntityName.c_str().AsChar(), m_Document.GetAllocator());
                    EntityTreeModelNode* ParentArrayItemNode = new EntityTreeModelNode(GroupArrayNode, "name", EntityName, ValKey, ValValue, m_Document);
                    EnumChildren(ParentArrayItemNode, ValValue, m_Document);
                    CommandPattern Insert = make_shared<InsertSubTreeCommand>(wxDataViewItem(ParentArrayItemNode), wxDataViewItem(GroupArrayNode), GroupArrayNode->GetChildCount(), m_entity_view_model, m_Document, true);
                    Command->PushCommand(Insert);
                    Insert->Execute();
                }
            }
        }

        {
            // Add new array entry into the targets array.
            EntityTreeModelNode* GroupArrayNode = EntityDefNode->Find("edit:targets");
            if (GroupArrayNode != nullptr) {
                rapidjson::Value ValKey("name", m_Document.GetAllocator());
                wxString EntityName = AI2Node->m_key;
                rapidjson::Value ValValue(EntityName.c_str().AsChar(), m_Document.GetAllocator());
                EntityTreeModelNode* GroupArrayItemNode = new EntityTreeModelNode(GroupArrayNode, "name", EntityName, ValKey, ValValue, m_Document);
                CommandPattern Insert = make_shared<InsertSubTreeCommand>(wxDataViewItem(GroupArrayItemNode), wxDataViewItem(GroupArrayNode), GroupArrayNode->GetChildCount(), m_entity_view_model, m_Document);
                Command->PushCommand(Insert);
                Insert->Execute();
            }
        }
    }

    // Save the command.
    PushCommand(Command);
 }

bool MyFrame::SetLocationNodeFromMH(EntityTreeModelNode *Node)
{
    // Check for x,y,z
    EntityTreeModelNode *NodeX = Node->GetNthChild(0);
    EntityTreeModelNode *NodeY = Node->GetNthChild(1);
    EntityTreeModelNode *NodeZ = Node->GetNthChild(2);
    if (NodeX == nullptr || NodeY == nullptr || NodeZ == nullptr) {
        return false;
    }

    if (NodeX->m_key != "x" || NodeY->m_key != "y" || NodeZ->m_key != "z") {
        return false;
    }

    // Get the location and parse it.
    char Info[MAX_PATH];
    char x[MAX_PATH], y[MAX_PATH], z[MAX_PATH];

    m_MeatHook.GetSpawnInfo((unsigned char*)Info);
    sscanf_s(Info, "%s %s %s", x, (unsigned int)sizeof(x), y, (unsigned int)sizeof(y), z, (unsigned int)sizeof(z));

    // Adjust the current nodes.
    GroupedCommand Group = make_shared<_GroupedCommand>();
    CommandPattern ChangeCmd;

    ChangeCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(NodeX), m_entity_view_model, x, 1);
    Group->PushCommand(ChangeCmd);
    ChangeCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(NodeY), m_entity_view_model, y, 1);
    Group->PushCommand(ChangeCmd);
    ChangeCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(NodeZ), m_entity_view_model, z, 1);
    Group->PushCommand(ChangeCmd);
    Group->Execute();
    PushCommand(Group);

    return true;
}

bool MyFrame::SetRotationNodeFromMH(EntityTreeModelNode* Node)
{
    EntityTreeModelNode* Matrix = Node->GetNthChild(0);
    if (Matrix == nullptr) {
        return false;
    }

    if (Matrix->m_key != "mat") {
        return false;
    }

    float x,y,z,Pitch,Yaw;
    char Info[MAX_PATH];
    m_MeatHook.GetSpawnInfo((unsigned char*)Info);
    sscanf_s(Info, "%f %f %f %f %f", &x, &y, &z, &Yaw, &Pitch);
    EntityTreeModelNode *RotationNode = RotationMatrixFromAngle(m_Document, Yaw, Pitch, 0);

    GroupedCommand Group = make_shared<_GroupedCommand>();
    // Remove Node
    CommandPattern ChangeCmd;
    ChangeCmd = std::make_shared<DeleteSubTreeCommand>(wxDataViewItem(Matrix), m_entity_view_model, m_Document);
    Group->PushCommand(ChangeCmd);
    // Add the created RotationNode
    ChangeCmd = std::make_shared<InsertSubTreeCommand>(wxDataViewItem(RotationNode), wxDataViewItem(Node), 0, m_entity_view_model, m_Document);
    Group->PushCommand(ChangeCmd);
    // Execute group and push into undo stack.
    Group->Execute();
    PushCommand(Group);
    return true;
}

void ConstructInsertSubMenu(wxMenu *Menu, EntityTreeModelNode *Node, std::map<int, std::string> &MenuMap)
{
    MenuMap.clear();
    size_t SubMenuIndex = CID_MAX;
    for (auto Entry : MenuDescription) {
        if (GetEntityClassName(Node).c_str().AsChar() != Entry.second.Class) {
            continue;
        }

        // Class has entry description.
        EntityTreeModelNode *EntityDefNode = GetEntityDefNode(Node);
        if (EntityDefNode->Find(Entry.second.Requirements) == nullptr) {
            continue;
        }

        if (Node->GetParent()->m_key.Matches("events") == false) {
            continue;
        }

        // Create a map from search arguments.
        std::vector<std::string> SearchArguments;
        for (auto Event : EventDescriptor) {
            for (auto StringMatch : Entry.second.StringMatch) {
                if (wxString(Event.first).Lower().Matches(wxString(StringMatch).Lower()) != false) {
                    SearchArguments.push_back(Event.first);
                    break;
                }
            }
        }

        // Add the found items to the menu.
        wxMenu* SubMenu = new wxMenu;
        Menu->AppendSubMenu(SubMenu, Entry.first);
        for (auto Argument : SearchArguments) {
            SubMenu->Append(SubMenuIndex, Argument);
            MenuMap[SubMenuIndex] = Argument;
            SubMenuIndex += 1;
        }
    }
}

EntityTreeModelNode* ConstructInsertionTree(wxString Name, rapidjson::Document &Document)
{
    if (EventDescriptor.find(Name.c_str().AsChar()) == EventDescriptor.end()) {
        return nullptr;
    }

    rapidjson::Value EventCallKey("eventCall", Document.GetAllocator());
    rapidjson::Value EventCallValue("", Document.GetAllocator());
    EventCallValue.SetObject();
    EventCallValue.AddMember(rapidjson::Value("eventDef", Document.GetAllocator()), rapidjson::Value(Name.c_str().AsChar(), Document.GetAllocator()), Document.GetAllocator());
    rapidjson::Value Args("", Document.GetAllocator());
    Args.SetObject();
    auto NulValue = rapidjson::Value("0", Document.GetAllocator());
    NulValue.SetInt(0);
    Args.AddMember(rapidjson::Value("num", Document.GetAllocator()), NulValue, Document.GetAllocator());
    Args.SetFlags(true, 0x4000);
    for (auto Event : EventDescriptor[Name.c_str().AsChar()]) {
        wxString KeyName = wxString::Format("item[%i]", Event.second.Index);
        Args.AddMember(rapidjson::Value(KeyName.c_str().AsChar(), Document.GetAllocator()), rapidjson::Value("", Document.GetAllocator()), Document.GetAllocator());
    }

    for (auto Event : EventDescriptor[Name.c_str().AsChar()]) {
        wxString Translated = wxString::Format(Event.first.c_str());
        auto Override = NameOverrides.find(Translated.c_str().AsChar());
        if (Override != NameOverrides.end()) {
            Translated = Override->second;
        }

        Args.MemberBegin()[Event.second.Index + 1].value.SetObject();
        Args.MemberBegin()[Event.second.Index + 1].value.AddMember(
            rapidjson::Value(Translated.c_str().AsChar(), Document.GetAllocator()),
            rapidjson::Value(Event.second.Type.c_str(), Document.GetAllocator()),
            Document.GetAllocator()
            );
    }

    EventCallValue.AddMember(rapidjson::Value("args", Document.GetAllocator()), Args, Document.GetAllocator());
    EntityTreeModelNode* Node = new EntityTreeModelNode(nullptr, "eventCall", EventCallKey, EventCallValue, Document);
    Node->m_keyRef = &(Node->m_keyCopy);
    Node->m_valueRef = &(Node->m_valueCopy);
    EnumChildren(Node, *Node->m_valueRef, Document);
    assert(ValidateTree(Node, *Node->m_keyRef, *Node->m_valueRef, 0) != false);
    return Node;
}

void MyFrame::OnContextEncounterSelect(wxCommandEvent& event)
{
    SelectEncounterManager(m_Encounters[event.GetId()]);
}

void MyFrame::SelectEncounterManager(wxString ActiveEncounter)
{
    EntityTreeModelNode* Root = (EntityTreeModelNode*)m_entity_view_model->GetRoot().GetID();
    EntityTreeModelNode* Found = Root->Find(wxString::Format("%s:entityDef %s", ActiveEncounter, ActiveEncounter));
    if (Found == nullptr) {
        return;
    }

    wxDataViewItem Select = wxDataViewItem(Found);
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
        m_LastNavigation.push_back(Select);
    }
}

void MyFrame::SelectCheckpointByName(wxString CheckpointName)
{
    EntityTreeModelNode* Root = (EntityTreeModelNode*)m_entity_view_model->GetRoot().GetID();
    EntityTreeModelNode* Found = nullptr;
    
    for (auto Child : Root->GetChildren()) {
        auto Checkpoint = Child->Find(wxString::Format("entityDef %s:edit:checkpointName", Child->m_key));
        if ((Checkpoint != nullptr) && (Checkpoint->m_value == CheckpointName)) {
            Found = Checkpoint;
            break;
        }
    }

    if (Found == nullptr) {
        return;
    }

    wxDataViewItem Select = wxDataViewItem(Found);
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
        m_LastNavigation.push_back(Select);
    }
}


void MyFrame::MHGotoCurrentEncounter(wxCommandEvent& event)
{
    char ActiveEncounter[MAX_PATH * 20];
    int Size = sizeof(ActiveEncounter);
    memset(ActiveEncounter, 0, sizeof(ActiveEncounter));
    bool Result = m_MeatHook.GetActiveEncounter(&Size, ActiveEncounter);
    if ((Result == false) || (Size == 0)) {
        return;
    }

    wxString ActiveEncounterString = ActiveEncounter;
    if (ActiveEncounterString.empty()) {
        wxMessageBox("No active encounters", "There are currently no encounters that are active, try triggering an an encounter first.");
    }

    if (ActiveEncounterString.Freq(';') == 0) {
        SelectEncounterManager(ActiveEncounter);

    } else {
        // Construct a context menu with all the currently active encounters.
        wxMenu menu;
        size_t Index = 0;
        size_t Start = 0;
        size_t End = ActiveEncounterString.Len();
        m_Encounters.clear();
        while (Start < ActiveEncounterString.Len()) {
            End = ActiveEncounterString.find(';', Start);
            if (End == wxString::npos) {
                End = ActiveEncounterString.Len();
            }

            wxString EncounterString = ActiveEncounterString.SubString(Start, End - 1);
            Start = End + 1;
            menu.Append(Index, EncounterString);
            m_Encounters.push_back(EncounterString);
            Index += 1;
        }

        menu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MyFrame::OnContextEncounterSelect), nullptr, this);
        PopupMenu(&menu);
    }
}

void MyFrame::MHGotoCurrentCheckpoint(wxCommandEvent& event)
{
    char Checkpoint[MAX_PATH * 20];
    int Size = sizeof(Checkpoint);
    memset(Checkpoint, 0, sizeof(Checkpoint));
    bool Result = m_MeatHook.GetCurrentCheckpoint(&Size, Checkpoint);
    if ((Result == false) || (Size == 0)) {
        return;
    }

    wxString CheckpointString = Checkpoint;
    SelectCheckpointByName(CheckpointString);
}

void MyFrame::Copy(wxCommandEvent& event)
{
    EntityTreeModelNode *Node = (EntityTreeModelNode*)m_ctrl[0]->GetSelection().GetID();
    if (Node == nullptr) {
        wxMessageBox("Please select something.", "No selection");
    }

    CopyToClipBoard(Node);
}

void MyFrame::CopyToClipBoard(EntityTreeModelNode *Node)
{
    if (Node == nullptr) {
        return;
    }

    EntityTreeModelNode* Parent = Node->GetParent();
    size_t Index = Parent->GetChildIndex(Node);
    if (Parent->IsArray() != false) {
        Index += 1;
    }

    Document TempDoc;
    TempDoc.SetObject();
    TempDoc.InsertMember(Parent->m_valueRef->MemberBegin()[Index].name, Parent->m_valueRef->MemberBegin()[Index].value, m_Document.GetAllocator(), 0);
    byte* Data = nullptr;
    size_t DataSize = 0;
    StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer,
        rapidjson::UTF8<char>,
        rapidjson::UTF8<char>,
        rapidjson::CrtAllocator,
        rapidjson::kWriteValidateEncodingFlag |
        rapidjson::kWriteNanAndInfFlag> writer(buffer);
    writer.SetIndent('\t', 1);
    TempDoc.Accept(writer, 0);

    // Copy to clipboard.
    OpenClipboard(GetHWND());
    EmptyClipboard();
    HGLOBAL Memory = GlobalAlloc(GMEM_MOVEABLE, buffer.GetSize());
    HGLOBAL StringHandle = GlobalLock(Memory);
    memcpy((char*)StringHandle, buffer.GetString(), buffer.GetSize());
    GlobalUnlock(StringHandle);
    SetClipboardData(CF_TEXT, StringHandle);
    CloseClipboard();
}

void MyFrame::Paste(wxCommandEvent& event)
{
    EntityTreeModelNode *PasteNode = (EntityTreeModelNode*)m_ctrl[0]->GetSelection().GetID();
    if (PasteNode == nullptr) {
        wxMessageBox("Please select the node to paste into.", "No selection");
    }

    InsertFromClipBoard(PasteNode);
}

void MyFrame::InsertFromClipBoard(EntityTreeModelNode* ParentNode)
{
    if (ParentNode == nullptr) {
        return;
    }

    // Get from clipboard.
    OpenClipboard(GetHWND());
    char* Data = (char*)GetClipboardData(CF_TEXT);
    CloseClipboard();
    if (Data == nullptr) {
        return;
    }

    // Normalize line ending.
    wxString LineEndNormalization(Data);
    LineEndNormalization.Replace("\r\n", "\n");
    // Parse data.
    MemoryStream memstream(LineEndNormalization.c_str().AsChar(), LineEndNormalization.Len());
    Document PasteData;
    PasteData.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(memstream);
    if (PasteData.HasParseError() != false) {
        wxString error = wxString::Format("Error parsing the clipboard. (Syntax error at offset: %llu)", (long long)PasteData.GetErrorOffset());
        wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    // Insert node.
    GroupedCommand Group = make_shared<_GroupedCommand>();
    for (size_t i = 0; i < PasteData.MemberCount(); i += 1) {
        auto Member = PasteData.MemberBegin() + i;
        EntityTreeModelNode* Node = nullptr;
        size_t SubMemberCount = 1;
        bool SkipParent = false;
        if ((Member->value.IsObject() != false) && (wxString(ValueToString(Member->name)).Matches("item[*]") != false)) {
            SubMemberCount = Member->value.MemberCount();
            SkipParent = true;
        }

        auto SubMember = Member;
        for (size_t SubIndex = 0; SubIndex < SubMemberCount; SubIndex += 1) {
            if (SkipParent != false) {
                SubMember = Member->value.MemberBegin() + SubIndex;
            }

            if (SubMember->value.IsObject() != false) {
                Node = new EntityTreeModelNode(nullptr, wxString(ValueToString(SubMember->name)), SubMember->name, SubMember->value, m_Document);

            } else {
                Node = new EntityTreeModelNode(nullptr, wxString(ValueToString(SubMember->name)), wxString(ValueToString(SubMember->value)), SubMember->name, SubMember->value, m_Document);
            }

            EnumChildren(Node, SubMember->value, m_Document);
            if (ValidateTree(Node, SubMember->name, SubMember->value) == false) {
                wxMessageBox("Subtree validation failed", "The pasted item cannot be put here.");
                return;
            }
            EntityTreeModelNode* ItemNode = ParentNode;
            size_t Index = ItemNode->GetParent()->GetChildIndex(ItemNode);
            bool Wrapped = ItemNode->IsWrapped();

            CommandPattern Insert = make_shared<InsertSubTreeCommand>(
                wxDataViewItem(Node),
                wxDataViewItem(ItemNode->GetParent()),
                (Index + i),
                m_entity_view_model,
                m_Document,
                Wrapped
                );

            Group->PushCommand(Insert);
        }
    }

    Group->Execute();
    PushCommand(Group);
}

void MyFrame::ProgressCancel(wxCommandEvent& event)
{
    if (wxMessageBox("Without an entity map most of the EntityHero features will be disabled.\nAre you sure you want to cancel building the entity map?", "Cancel", wxOK | wxCANCEL) == wxOK) {
        m_ProgressText->SetLabelText("Cancelling..");
        mainSizer->Layout();
        m_ProgressCurrent = m_ProgressEnd;
    }
}

void MyFrame::ReverseCommitExitTriggers(wxCommandEvent& event)
{
    // Run through all encounter managers.
    // Follow into every entityDef:edit:commitTriggers (barge_encounter_trigger_commit_priest_room_no_gk)
    // Follow into every entityDef:edit:exitTriggers

    // Try to keep the same distances as the original.
    GroupedCommand Group = make_shared<_GroupedCommand>();
    EntityTreeModelNode* Root = (EntityTreeModelNode*)m_entity_view_model->GetRoot().GetID();
    for (auto Iterator = Root->GetChildren().begin(); Iterator < Root->GetChildren().end(); Iterator++) {
        wxString ClassName = GetEntityClass(*Iterator, (*Iterator)->m_key);
        if (ClassName == "idEncounterManager") {
            std::vector<EntityTreeModelNode*> ExitNodes;
            std::vector<wxString> ExitStrings = GetValueList((*Iterator), "exitTriggers");
            for (auto String = ExitStrings.begin(); String != ExitStrings.end(); String++) {
                EntityTreeModelNode* Position = Root->Find(*String + ":entityDef " + *String + ":edit:spawnPosition");
                if (Position != nullptr) {
                    ExitNodes.push_back(Position);
                }
            }

            float EndX,EndY,EndZ;
            float StartX, StartY, StartZ;
            if (ExitNodes.empty() == false) {
                GetSpawnPosition(ExitNodes[0], EndX, EndY, EndZ);

            } else {
                EndX = 0;
                EndY = 0;
                EndZ = 0;
            }

            std::vector<wxString> CommitStrings = GetValueList((*Iterator), "commitTriggers");
            bool First = true;
            for (auto String = CommitStrings.begin(); String != CommitStrings.end(); String++) {
                EntityTreeModelNode* Position = Root->Find(*String + ":entityDef " + *String + ":edit:spawnPosition");
                if (Position == nullptr) {
                    continue;
                }

                if (First != false) {
                    GetSpawnPosition(Position, StartX, StartY, StartZ);
                    First = false;
                }

                SetSpawnPosition(Position, EndX, EndY, EndZ, Group);
            }

            if (First != false) {
                continue;
            }

            for (auto Node = ExitNodes.begin(); Node != ExitNodes.end(); Node++) {
                SetSpawnPosition((*Node), StartX, StartY, StartZ, Group);
            }

            std::vector<wxString> UserFlagStrings = GetValueList((*Iterator), "userFlagTriggers");
            for (auto String = UserFlagStrings.begin(); String != UserFlagStrings.end(); String++) {
                EntityTreeModelNode* Position = Root->Find(*String + ":entityDef " + *String + ":edit:spawnPosition");
                if (Position == nullptr) {
                    continue;
                }

                SetSpawnPosition(Position, EndX, EndY, EndZ, Group);
            }

            std::vector<wxString> SpawnGroupStrings = GetValueList((*Iterator), "spawnGroupTouchOverride");
            for (auto String = UserFlagStrings.begin(); String != UserFlagStrings.end(); String++) {
                EntityTreeModelNode* Position = Root->Find(*String + ":entityDef " + *String + ":edit:spawnPosition");
                if (Position == nullptr) {
                    continue;
                }

                SetSpawnPosition(Position, EndX, EndY, EndZ, Group);
            }
        }
    }

    Group->Execute();
    PushCommand(Group);
}

void MyFrame::ReverseEncounter(wxCommandEvent& event)
{
    // Take a current encounter, take the last enemy and put it at the top.
    // Keep all maintain counts at the same location.
}

void MyFrame::TrackLocation(wxCommandEvent& event)
{

}

bool MyFrame::SetSpawnPosition(EntityTreeModelNode* Node, float x, float y, float z, shared_ptr< _GroupedCommand> ExternalGroup)
{
    // Check for x,y,z
    EntityTreeModelNode* NodeX = Node->GetNthChild(0);
    EntityTreeModelNode* NodeY = Node->GetNthChild(1);
    EntityTreeModelNode* NodeZ = Node->GetNthChild(2);
    if (NodeX == nullptr || NodeY == nullptr || NodeZ == nullptr) {
        return false;
    }

    if (NodeX->m_key != "x" || NodeY->m_key != "y" || NodeZ->m_key != "z") {
        return false;
    }

    // Adjust the current nodes.
    GroupedCommand Group = ExternalGroup;
    if (ExternalGroup == nullptr) {
        Group = make_shared<_GroupedCommand>();
    }

    CommandPattern ChangeCmd;
    wxString strx, stry, strz;
    strx = wxString::Format("%f", x);
    stry = wxString::Format("%f", y);
    strz = wxString::Format("%f", z);

    ChangeCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(NodeX), m_entity_view_model, strx, 1);
    Group->PushCommand(ChangeCmd);
    ChangeCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(NodeY), m_entity_view_model, stry, 1);
    Group->PushCommand(ChangeCmd);
    ChangeCmd = std::make_shared<ChangeItemCommand>(wxDataViewItem(NodeZ), m_entity_view_model, strz, 1);
    Group->PushCommand(ChangeCmd);

    if (ExternalGroup != nullptr) {
        Group->Execute();
        PushCommand(Group);
    }

    return true;
}