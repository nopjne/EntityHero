#pragma once
#include "wx/hashmap.h"
#include "wx/vector.h"
#include "ResourcesParser.h"


class FileTreeModelNode;
WX_DEFINE_ARRAY_PTR(FileTreeModelNode*, FileTreeModelNodePtrArray);

class FileTreeModelNode
{
    IDCT_FILE m_File;
    FileTreeModelNode* m_parent;
    FileTreeModelNodePtrArray m_children;
    bool m_IsContainer;

public:
    FileTreeModelNode(FileTreeModelNode* Parent, IDCT_FILE File) : m_parent(Parent), m_File(File) {}
    ~FileTreeModelNode() {
        for (size_t i = 0; i < m_children.size(); i += 1) {
            delete m_children[i];
        }
    }

    FileTreeModelNode* GetParent()
    {
        return m_parent;
    }
    FileTreeModelNodePtrArray& GetChildren()
    {
        return m_children;
    }
    FileTreeModelNode* GetNthChild(unsigned int n)
    {
        return m_children.Item(n);
    }

    IDCT_FILE GetIDFile() {
        return m_File;
    }

    unsigned int GetChildCount() const
    {
        return m_children.GetCount();
    }

    bool IsContainer() { return m_IsContainer; }
    size_t GetChildIndex(FileTreeModelNode* Node);
    FileTreeModelNode* FindByName(size_t Depth, size_t MaxDepth, wxString& Text, size_t IndexToFind, size_t& Index, bool Exact);

};

class FileTreeModel : public wxDataViewModel
{
public:
    FileTreeModel(IDCLReader Reader, wxString Filter);
    ~FileTreeModel()
    {
        m_root->GetChildren().clear();
        delete m_root;

        for (auto x : m_AllFiles) {
            delete x;
        }
    }

    size_t CountByName(wxString& Text, bool Exact, size_t MaxDepth);
    wxDataViewItem SelectText(wxString& Text, bool Next, bool Exact);

    void SetFilter(wxString Filter);

    int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2,
        unsigned int column, bool ascending) const wxOVERRIDE;

    virtual unsigned int GetColumnCount() const wxOVERRIDE
    {
        return 4;
    }

    virtual wxString GetColumnType(unsigned int col) const wxOVERRIDE
    {
        if (col == 1)
            return "size";
        if (col == 2)
            return "compressed size";
        if (col == 3)
            return "file offset";

        return "name";
    }
    FileTreeModelNode* GetRoot() { return m_root; }

    virtual void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const wxOVERRIDE;
    virtual bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) wxOVERRIDE;
    virtual bool IsEnabled(const wxDataViewItem& item, unsigned int col) const wxOVERRIDE;
    virtual wxDataViewItem GetParent(const wxDataViewItem& item) const wxOVERRIDE;
    virtual bool IsContainer(const wxDataViewItem& item) const wxOVERRIDE;
    virtual unsigned int GetChildren(const wxDataViewItem& parent, wxDataViewItemArray& array) const wxOVERRIDE;
private:
    wxString m_PreviousToFind;
    size_t m_Index;
    IDCLReader m_Reader;
    FileTreeModelNode* m_root;
    FileTreeModelNodePtrArray m_AllFiles;
};