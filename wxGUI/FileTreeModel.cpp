// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dataview.h"

#include "FileTreeModel.h"
#include "ResourcesParser.h"

// ----------------------------------------------------------------------------
// EntityTreeModel
// ----------------------------------------------------------------------------

FileTreeModel::FileTreeModel(IDCLReader Reader, wxString Filter) : m_Reader(Reader)
{
    m_Index = -1;
    auto List = Reader.GetFileList();
    IDCT_FILE Name = {};
    Name.FileName = "";
    m_root = new FileTreeModelNode(nullptr, Name);

    for (size_t i = 0; i < List.size(); i += 1) {
        m_AllFiles.push_back(new FileTreeModelNode(m_root, List[i]));
    }

    SetFilter(Filter);
}

void FileTreeModel::SetFilter(wxString Filter)
{
    FileTreeModelNodePtrArray Children = m_root->GetChildren();
    wxDataViewItemArray ItemArray;
    for (size_t i = 0; i < Children.size(); i += 1) {
        ItemArray.push_back(wxDataViewItem(Children[i]));
    }
    ItemsDeleted(wxDataViewItem(m_root), ItemArray);

    ItemArray.clear();
    m_root->GetChildren().clear();
    FileTreeModelNodePtrArray &FileViewArray = m_AllFiles;
    for (size_t i = 0; i < FileViewArray.size(); i += 1) {
        wxString Name(FileViewArray[i]->GetIDFile().FileName);
        if (Name.Matches("*" + Filter + "*") ||
            Name.Matches(Filter + "*") ||
            Name.Matches("*" + Filter) ||
            Name.Matches(Filter)) {

            m_root->GetChildren().push_back(m_AllFiles[i]);
            ItemArray.push_back(wxDataViewItem(m_AllFiles[i]));
        }
    }

    ItemsAdded(wxDataViewItem(m_root), ItemArray);
}

bool PartialMatch(wxString& a, wxString& b);

FileTreeModelNode* FileTreeModelNode::FindByName(size_t Depth, size_t MaxDepth, wxString& Text, size_t IndexToFind, size_t& Index, bool Exact)
{
    wxString ConvertedString(m_File.FileName.c_str());
    if ((Text == ConvertedString) || ((Exact == false) && (PartialMatch(Text, ConvertedString)))) {
        if (Index == IndexToFind) {
            return this;
        }

        Index += 1;
    }

    if ((MaxDepth != 0) && (Depth == MaxDepth)) {
        return nullptr;
    }

    size_t Count = GetChildren().size();
    for (size_t i = 0; i < Count; i += 1) {
        FileTreeModelNode* Result = GetChildren().at(i)->FindByName(Depth + 1, MaxDepth, Text, IndexToFind, Index, Exact);
        if (Result != nullptr) {
            return Result;
        }
    }

    return nullptr;
}

wxDataViewItem FileTreeModel::SelectText(wxString& Text, bool Next, bool Exact)
{
    if (Next == false) {
        m_Index = 0;
    }
    else {
        m_Index += 1;
    }

    if (m_PreviousToFind != Text) {
        m_Index = 0;
    }

    m_PreviousToFind = Text;
    size_t Counter = 0;
    FileTreeModelNode* Result = m_root->FindByName(0, 0, Text, m_Index, Counter, Exact);
    if (Result == nullptr) {
        m_Index = -1;
    }

    return wxDataViewItem(Result);
}

size_t FileTreeModel::CountByName(wxString& Text, bool Exact, size_t MaxDepth)
{
    size_t Counter = 0;
    FileTreeModelNode* Result = nullptr;
    do {
        Result = m_root->FindByName(0, MaxDepth, Text, m_Index, Counter, Exact);
    } while (Result != nullptr);

    return Counter;
}

int FileTreeModel::Compare(const wxDataViewItem& item1, const wxDataViewItem& item2,
    unsigned int column, bool ascending) const
{
    wxASSERT(item1.IsOk() && item2.IsOk());
    // should never happen

    if (IsContainer(item1) && IsContainer(item2))
    {
        wxVariant value1, value2;
        GetValue(value1, item1, 0);
        GetValue(value2, item2, 0);

        wxString str1 = value1.GetString();
        wxString str2 = value2.GetString();
        int res = str1.Cmp(str2);
        if (res) return res;

        // items must be different
        wxUIntPtr litem1 = (wxUIntPtr)item1.GetID();
        wxUIntPtr litem2 = (wxUIntPtr)item2.GetID();

        return litem1 - litem2;
    }

    return wxDataViewModel::Compare(item1, item2, column, ascending);
}

//size_t FileTreeModelNode::

size_t FileTreeModelNode::GetChildIndex(FileTreeModelNode* Node)
{
    if (IsContainer() == false) {
        return -1;
    }

    size_t count = m_children.GetCount();
    for (size_t i = 0; i < count; i++)
    {
        FileTreeModelNode* child = m_children[i];
        if (child == Node) {
            return i;
        }
    }

    return -1;
}

// wxString FileTreeModel::GetKey(const wxDataViewItem& item) const
// {
//     FileTreeModelNode* node = (FileTreeModelNode*)item.GetID();
//     if (!node)      // happens if item.IsOk()==false
//         return wxEmptyString;
// 
//     return node->m_key;
// }
// 
// wxString FileTreeModel::GetValue(const wxDataViewItem& item) const
// {
//     FileTreeModelNode* node = (FileTreeModelNode*)item.GetID();
//     if (!node)      // happens if item.IsOk()==false
//         return wxEmptyString;
// 
//     return node->m_value;
// }
// 
// int FileTreeModel::Insert(wxDataViewItem* ParentItem, size_t Index, wxDataViewItem* Item)
// {
//     FileTreeModelNode* Node = (FileTreeModelNode*)Item->GetID();
//     if (!Node)      // happens if item.IsOk()==false
//         return 0;
// 
//     FileTreeModelNode* ParentNode = (FileTreeModelNode*)ParentItem->GetID();
//     if (!ParentNode)      // happens if item.IsOk()==false
//         return 0;
// 
//     assert(Index != size_t(-1));
// 
//     //ParentNode->Insert(Node, Index, Document);
//     ItemAdded(*ParentItem, wxDataViewItem(Node));
//     return 1;
// }

void FileTreeModel::GetValue(wxVariant& variant,
    const wxDataViewItem& item, unsigned int col) const
{
    wxASSERT(item.IsOk());

    FileTreeModelNode* node = (FileTreeModelNode*)item.GetID();
    switch (col)
    {
    case 0:
        variant = node->GetIDFile().FileName;
        break;
    case 1:
        variant = (long)(node->GetIDFile().Size);
        break;
    case 2:
        variant = (long)(node->GetIDFile().SizeUncompressed);
        break;
    case 3:
        variant = (long)(node->GetIDFile().Offset);
        break;
    default:
        wxLogError("EntityTreeModel::GetValue: wrong column %d", col);
    }
}

bool FileTreeModel::SetValue(const wxVariant& variant,
    const wxDataViewItem& item, unsigned int col)
{
    wxASSERT(item.IsOk());

    FileTreeModelNode* node = (FileTreeModelNode*)item.GetID();
    switch (col)
    {
    case 0:
        //node->m_key = variant.GetString();
        return true;
    case 1:
        //assert(node->IsContainer() == false);
        //node->m_value = variant.GetString();
        return true;
    default:
        wxLogError("EntityTreeModel::SetValue: wrong column");
    }
    return false;
}

bool FileTreeModel::IsEnabled(const wxDataViewItem& item, unsigned int col) const
{
    return true;
}

wxDataViewItem FileTreeModel::GetParent(const wxDataViewItem& item) const
{
    // the invisible root node has no parent
    if (!item.IsOk())
        return wxDataViewItem(0);

    FileTreeModelNode* node = (FileTreeModelNode*)item.GetID();

    if (node == m_root)
        return wxDataViewItem(0);

    return wxDataViewItem((void*)node->GetParent());
}

bool FileTreeModel::IsContainer(const wxDataViewItem& item) const
{
    // the invisible root node can have children
    if (!item.IsOk())
        return true;

    FileTreeModelNode* node = (FileTreeModelNode*)item.GetID();
    return node->GetParent() == nullptr;
}

unsigned int FileTreeModel::GetChildren(const wxDataViewItem& parent,
    wxDataViewItemArray& array) const
{
    FileTreeModelNode* node = (FileTreeModelNode*)parent.GetID();
    if (!node)
    {
        array.Add(wxDataViewItem((void*)m_root));
        return 1;
    }

    if (node->GetChildCount() == 0)
    {
        return 0;
    }

    unsigned int count = node->GetChildren().GetCount();
    for (unsigned int pos = 0; pos < count; pos++)
    {
        FileTreeModelNode* child = node->GetChildren().Item(pos);
        array.Add(wxDataViewItem((void*)child));
    }

    return count;
}