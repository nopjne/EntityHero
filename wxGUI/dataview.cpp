
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
#include "Command.h"
#include "Dialogs.h"

using namespace rapidjson;
using namespace std;

std::map<std::string, std::set<std::string>> ValueMap;
int DecompressEntities(std::istream* input, char** OutDecompressedData, size_t& OutSize);
int CompressEntities(const char* destFilename, byte* uncompressedData, size_t size);
int CompressEntities(byte* uncompressedData, size_t size, char** output, size_t& outputSize);

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
            m_Model->Insert(&m_ParentItem, m_Position, &(wxDataViewItem(m_NewItem)), m_JsonDocument);
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
        m_Model->Insert(&m_ParentItem, m_Position, &m_Item, m_JsonDocument);
        m_Deleted = false;
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

        if (Column == 0) {
            m_OldValue = m_Model->GetKey(Item);

        } else {
            m_OldValue = m_Model->GetValue(Item);
        }
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
    void OnFilterTypeResources(wxCommandEvent& event);
    void OnFilterSearchResources(wxCommandEvent& event);

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
    std::vector<CommandPattern> m_UndoStack;
    std::vector<CommandPattern> m_RedoStack;
    CommandPattern m_ChangeCmd;
    wxMenu* m_file_menu;
    wxString m_OldValue;
    wxCheckBox *m_MatchCaseCheck;
    std::vector<wxDataViewItem> m_LastNavigation;
    std::vector<wxDataViewItem> m_NextNavigation;

private:
    // Flag used by OnListValueChanged(), see there.
    bool m_eventFromProgram;

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

    MyFrame *frame =
        new MyFrame(NULL, "EntityHero v0.1 (by Scorp0rX0r)", 40, 40, 1000, 540);

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

    // Quick find menu
    ID_QF_INTRO_CUTSCENE,
    ID_QF_SPAWN_LOCATION,
    ID_QF_FIRST_ENCOUNTER,

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
    ID_SHOW_CURRENT,
    ID_SET_NINTH_CURRENT,
    ID_CHANGE_NINTH_TITLE,
    ID_FILTER_SEARCH,
    ID_FILTER_SEARCH_RESOURCES,

    ID_NAVIGTE_BACKWARD,
    ID_NAVIGTE_FORWARD,
    ID_SEARCH_BACKWARD,
    ID_SEARCH_FORWARD,
    ID_SEARCH_MATCH_CASE,

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
    EVT_MENU(ID_OPEN_TEXT_FILE, MyFrame::ImportFile)
    EVT_MENU( ID_CLEARLOG, MyFrame::OnClearLog )

    EVT_MENU(ID_REDO, MyFrame::Redo)
    EVT_MENU(ID_UNDO, MyFrame::Undo)

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
    
    EVT_NOTEBOOK_PAGE_CHANGED( wxID_ANY, MyFrame::OnPageChanged )

    EVT_BUTTON( ID_COLLAPSE, MyFrame::OnCollapse )
    EVT_BUTTON( ID_EXPAND, MyFrame::OnExpand )
    EVT_BUTTON( ID_SHOW_CURRENT, MyFrame::OnShowCurrent )
    EVT_BUTTON(ID_SEARCH_BACKWARD, MyFrame::SearchBackward)
    EVT_BUTTON(ID_SEARCH_FORWARD, MyFrame::SearchForward)


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

wxEND_EVENT_TABLE()

MyFrame::MyFrame(wxFrame *frame, const wxString &title, int x, int y, int w, int h):
  wxFrame(frame, wxID_ANY, title, wxPoint(x, y), wxSize(w, h)),
    m_Popup(nullptr)
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

    wxMenu* quickfind_menu = new wxMenu;
    quickfind_menu->Append(ID_QF_INTRO_CUTSCENE, "Intro cutscene (intro_game_info_logic)");
    quickfind_menu->Append(ID_QF_SPAWN_LOCATION, "Spawn point (intro_game_target_relay_player_start)");
    quickfind_menu->Append(ID_QF_FIRST_ENCOUNTER, "First encounter");

    wxMenuBar *menu_bar = new wxMenuBar;
    menu_bar->Append(file_menu, "&File");
    menu_bar->Append(edit_menu, "&Edit");
    menu_bar->Append(quickfind_menu, "&Quick Find");

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

    sizerCurrent->Add(new wxButton(firstPanel, ID_SHOW_CURRENT,
                                   "&Pause"), border);
    sizerCurrent->Add(new wxButton(firstPanel, ID_SHOW_CURRENT,
                                   "&Reload level"), border);
    sizerCurrent->Add(new wxButton(firstPanel, ID_SET_NINTH_CURRENT,
                                   "Copy current &location"), border);
    sizerCurrent->Add(new wxButton(firstPanel, ID_CHANGE_NINTH_TITLE,
                                   "Copy current &rotation"), border);

    // 
    // Per request from the creator of meathook, randomize the displayed meathook name.
    //
    srand(timeGetTime());
    struct PickRand_ { static char PickRand(const char *str) { size_t len = strlen(str); return str[rand() % (len - 1)]; } };
    wxString InterfaceText = wxString::Format("%c%c%c%c%c%c%c%c interface: %s",
        PickRand_::PickRand("Mm"),
        PickRand_::PickRand("eE3é"),
        PickRand_::PickRand("aA4"),
        PickRand_::PickRand("tT7"),
        PickRand_::PickRand("hH"),
        PickRand_::PickRand("oO0"),
        PickRand_::PickRand("oO0"),
        PickRand_::PickRand("kK"),
        MeathookActive ? "" : "(inactive)");

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
    firstPanelSz->Add(
        new wxStaticText(firstPanel, wxID_ANY, InterfaceText),
        0, wxGROW|wxALL, 5);

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

    wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->Add( m_notebook, 1, wxGROW );
#ifdef _DEBUG
    mainSizer->Add( m_log, 0, wxGROW );
#else
    m_log->Hide();
#endif

    SetSizerAndFit(mainSizer);

    wxAcceleratorEntry entries[5];
    entries[0].Set(wxACCEL_CTRL, (int)'z', ID_UNDO);
    entries[1].Set(wxACCEL_CTRL, (int)'y', ID_REDO);
    entries[2].Set(wxACCEL_CTRL, (int)'s', ID_SAVE_FILE);
    entries[3].Set(wxACCEL_CTRL, (int)',', ID_NAVIGTE_BACKWARD);
    entries[4].Set(wxACCEL_CTRL, (int)'.', ID_NAVIGTE_FORWARD);

    wxAcceleratorTable accel(ARRAYSIZE(entries), entries);
    SetAcceleratorTable(accel);
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


#if wxUSE_DRAG_AND_DROP && wxUSE_UNICODE && _DEBUG
            m_ctrl[Page_EntityView]->EnableDragSource( wxDF_UNICODETEXT );
            m_ctrl[Page_EntityView]->EnableDropTarget( wxDF_UNICODETEXT );
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
    info.SetName(_("EntityHero v0.1"));
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
    if (m_entity_view_model->IsContainer( item ) == false)
    {
        wxLogMessage("Forbidding starting dragging");
        event.Veto();
        return;
    }

    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    wxTextDataObject *obj = new wxTextDataObject;
    obj->SetText( node->m_key );
    event.SetDataObject( obj );
    event.SetDragFlags(wxDrag_AllowMove); // allows both copy and move

    wxLogMessage("Starting dragging \"%s\"", node->m_key);
}

void MyFrame::OnDropPossible( wxDataViewEvent &event )
{
    if (event.GetDataFormat() != wxDF_UNICODETEXT)
        event.Veto();
    else
        event.SetDropEffect(wxDragMove);	// check 'move' drop effect
}

void MyFrame::OnDrop( wxDataViewEvent &event )
{
    wxDataViewItem item( event.GetItem() );

    if (event.GetDataFormat() != wxDF_UNICODETEXT)
    {
        event.Veto();
        return;
    }

    wxTextDataObject obj;
    obj.SetData( wxDF_UNICODETEXT, event.GetDataSize(), event.GetDataBuffer() );

    if ( item.IsOk() )
    {
        if (m_entity_view_model->IsContainer(item))
        {
            wxLogMessage("Text '%s' dropped in container '%s' (proposed index = %i)",
                         obj.GetText(), m_entity_view_model->GetKey(item), event.GetProposedDropIndex());
        }
        else
            wxLogMessage("Text '%s' dropped on item '%s'", obj.GetText(), m_entity_view_model->GetKey(item));
    }
    else
        wxLogMessage("Text '%s' dropped on background (proposed index = %i)", obj.GetText(), event.GetProposedDropIndex());
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
    if ((event.GetColumn() == 0) && (m_entity_view_model->IsArrayElement(&(event.GetItem())))) {
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
    model->GetValue(value, event.GetItem(), event.GetColumn());
    wxLogMessage("wxEVT_DATAVIEW_ITEM_EDITING_STARTED, current value %s",
        value.GetString());

    m_OldValue = value.GetString();
    model->GetValue(value, event.GetItem(), 0);
    auto EditBox = ((wxTextCtrl*)event.GetDataViewColumn()->GetRenderer()->GetEditorCtrl());
    EditBox->AutoComplete(new MyTextCompleter(value.GetString()));
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
        PushCommand(m_ChangeCmd);
        OutputDebugString(L"Saved change command\n");
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
    CID_REINJECT_FILE
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
    int Result = DecompressEntities(&Stream, &DecompressedData, DecompressedSize);
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

    wxMenu menu;
    menu.Append(CID_GOTO_REFERENCE, "Goto reference");
    menu.Append(CID_DUPLICATE, "Duplicate");
    menu.Append(CID_DELETE, "Delete");
#ifdef _DEBUG
    menu.Append(CID_EDIT_KEY_NAME, "Edit key name");
    menu.Append(CID_ADD_ITEM, "Add new node");
#endif

    if (m_entity_view_model->IsContainer(event.GetItem()) == false) {
        if (m_entity_view_model->IsArrayElement(&(event.GetItem())) == false) {
            menu.Enable(CID_DUPLICATE, false);
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
    switch (Id) {
        case ID_QF_INTRO_CUTSCENE:
            str = "entityDef intro_game_info_logic";
        break;
        case ID_QF_SPAWN_LOCATION:
            str = "entityDef intro_game_target_relay_player_start";
        break;
        case ID_QF_FIRST_ENCOUNTER:
            str = "entityDef spawn";
        break;
    };

    wxDataViewItem Select = m_entity_view_model->SelectText(str, eSearchDirection::NEXT, false, true);
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
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
    wxDataViewItem Select = m_entity_view_model->SelectText(Text, eSearchDirection::NEXT, false, m_MatchCaseCheck->IsChecked());

    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
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
        byte *Data = nullptr;
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

void MyFrame::ConstructTreeView()
{
    m_ctrl[0]->ClearColumns();
    m_entity_view_model = new EntityTreeModel(m_Document);

    // Call build instead ? (Or separate this function to prevent duplication)
    m_ctrl[0]->AssociateModel(m_entity_view_model.get());
    //m_entity_view_model->DecRef();
    m_ctrl[0]->Expand(m_entity_view_model->GetRoot());

#ifdef _DEBUG
    m_ctrl[Page_EntityView]->EnableDragSource(wxDF_UNICODETEXT);
    m_ctrl[Page_EntityView]->EnableDropTarget(wxDF_UNICODETEXT);
#endif

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

    bool ResourcesFile = false;
    IDCLReader Resources;
    ResourcesFile = Resources.OpenFile(openFileDialog.GetPath());
    if (ResourcesFile != false) {
        // Find entities strings.
        m_ResourceReader.OpenFile(openFileDialog.GetPath());
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
            wxMessageBox(_("There are multiple entities file in the resources archive please select one from the resource view."), _("Multiple entities"), wxICON_QUESTION | wxOK, this);
            m_ResourceCtrl->SetHelpText(".entities");
            m_file_view_model->SetFilter(".entities");
            m_ctrl[Page_ResourcesView]->Expand(wxDataViewItem(m_file_view_model->GetRoot()));
        }

        m_file_menu->Enable(ID_SAVE_FILE, false);
        m_file_menu->Enable(ID_SAVE_FILE_AS, true);
        return;
    }

    ifstream InputStreamBinary(openFileDialog.GetPath().c_str().AsChar(), std::ios_base::binary);
    InputStreamBinary.seekg(InputStreamBinary.end);
    size_t Size = InputStreamBinary.tellg();
    InputStreamBinary.seekg(InputStreamBinary.beg);

    // Oodle decompress.
    char *DecompressedData = nullptr;
    size_t DecompressedSize;
    int Result = DecompressEntities(&InputStreamBinary, &DecompressedData, DecompressedSize);
    if (Result == S_OK) {
        MemoryStream memstream(DecompressedData, DecompressedSize);
        m_Document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(memstream);
        if (m_Document.HasParseError() != false) {
            wxString error = wxString::Format("Error parsing the entities definition file. (Syntax error at offset: %llu", (long long)m_Document.GetErrorOffset());
            wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR, this);
        }

    } else {
        wxMessageBox(_("Error parsing the entities definition file. (Could not decompress)."), _("Error"), wxOK, this);
    }

    if (DecompressedData != nullptr) {
        delete[] DecompressedData;
    }

    if (m_Document.HasParseError() == false) {
        m_CurrentlyLoadedFileName = openFileDialog.GetPath();
    }

    ConstructTreeView();

    m_file_menu->Enable(ID_SAVE_FILE, true);
    m_file_menu->Enable(ID_SAVE_FILE_AS, true);
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

    rapidjson::IStreamWrapper InStream(InputStream);
    m_Document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(InStream);
    if (m_Document.HasParseError() != false) {
        wxMessageBox(_("Error parsing the entities syntax."), _("Error"), wxOK, this);
    }

    ConstructTreeView();
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

    // Select and make visible.
    if (CurrentNavLocation.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(CurrentNavLocation);
        m_ctrl[0]->EnsureVisible(CurrentNavLocation);
    }
}

void MyFrame::SearchBackward(wxCommandEvent& event)
{
    wxString str = m_FilterCtrl->GetValue().c_str();
    wxDataViewItem Select = m_entity_view_model->SelectText(str, eSearchDirection::PREV, true, m_MatchCaseCheck->IsChecked());
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
    }
}

void MyFrame::SearchForward(wxCommandEvent& event)
{
    wxString str = m_FilterCtrl->GetValue().c_str();
    wxDataViewItem Select = m_entity_view_model->SelectText(str, eSearchDirection::NEXT, true, m_MatchCaseCheck->IsChecked());
    if (Select.IsOk() != false) {
        m_ctrl[0]->SetCurrentItem(Select);
        m_ctrl[0]->EnsureVisible(Select);
    }
}