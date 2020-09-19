#include "wx/hashmap.h"
#include "wx/vector.h"
#include <rapidjson.h>
#include <document.h>     // rapidjson's DOM-style API
#include <prettywriter.h> // for stringify JSON

WX_DECLARE_HASH_MAP(unsigned, wxString, wxIntegerHash, wxIntegerEqual,
                    IntToStringMap);

class EntityTreeModelNode;
WX_DEFINE_ARRAY_PTR(EntityTreeModelNode*, EntityTreeModelNodePtrArray );


class EntityTreeModelNode
{
public:
    EntityTreeModelNode(EntityTreeModelNode* parent,
                          const wxString &key, const wxString &value,
                          rapidjson::Value &keyRef,
                          rapidjson::Value &val,
                          rapidjson::Document& Document)
    {
        m_parent = parent;
        m_key = key;
        m_value = value;
        m_valueRef = &val;
        m_keyRef = &keyRef;

        // Not necessary.
        m_keyCopy.CopyFrom(keyRef, Document.GetAllocator(), true);
        m_valueCopy.CopyFrom(val, Document.GetAllocator(), true);
        m_container = false;
    }

    EntityTreeModelNode(EntityTreeModelNode* parent,
                          const wxString &branch,
                          rapidjson::Value& key,
                          rapidjson::Value &val,
                          rapidjson::Document& Document)
    {
        m_parent = parent;
        m_valueRef = &val;
        m_keyRef = &key;
        m_key = branch;

        // Not necessary.
        m_keyCopy.CopyFrom(key, Document.GetAllocator(), true);
        m_valueCopy.CopyFrom(val, Document.GetAllocator(), true);
        m_container = true;
    }

    ~EntityTreeModelNode()
    {
        size_t count = m_children.GetCount();
        for (size_t i = 0; i < count; i++)
        {
            EntityTreeModelNode *child = m_children[i];
            delete child;
        }
    }

    bool IsContainer() const
        { return m_container; }

    EntityTreeModelNode* GetParent()
        { return m_parent; }
    EntityTreeModelNodePtrArray& GetChildren()
        { return m_children; }
    EntityTreeModelNode* GetNthChild( unsigned int n )
        { return m_children.Item( n ); }

    void Insert(EntityTreeModelNode* child, unsigned int n, rapidjson::Document& Document)
    {
        m_children.Insert(child, n);
        if (((m_valueRef->GetFlags() & 0x4000) != 0) && (n == 0)) {
            n += 1;
        }

        m_valueRef->InsertMember(child->m_keyCopy, child->m_valueCopy, Document.GetAllocator(), n);
    }

    void Append(EntityTreeModelNode* child, rapidjson::Document& Document)
    {
        m_children.Add(child);
        m_valueRef->AddMember(child->m_keyCopy, child->m_valueCopy, Document.GetAllocator());
    }

    unsigned int GetChildCount() const
        { return m_children.GetCount(); }

    size_t GetChildIndex(EntityTreeModelNode* Node);
    EntityTreeModelNode* FindByName(size_t Depth, size_t MaxDepth, wxString& Text, size_t IndexToFind, size_t& Index, bool Exact);

public:
    wxString                m_key;
    wxString               m_value;
    bool m_container;

private:
    EntityTreeModelNode          *m_parent;
    EntityTreeModelNodePtrArray   m_children;
public:
    rapidjson::Value               m_valueCopy;
    rapidjson::Value               m_keyCopy;
    rapidjson::Value              *m_valueRef;
    rapidjson::Value              *m_keyRef;
};

class EntityTreeModel: public wxDataViewModel
{
public:
    EntityTreeModel(rapidjson::Document& Document);
    ~EntityTreeModel()
    {
        delete m_root;
    }

    // helper method for wxLog

    wxString GetKey( const wxDataViewItem &item ) const;
    wxString GetValue( const wxDataViewItem &item ) const;
    void Delete( const wxDataViewItem &item );

    int Insert(wxDataViewItem* ParentItem, size_t Index, wxDataViewItem* Item, rapidjson::Document& Document);
    void RebuildReferences(EntityTreeModelNode* Node, rapidjson::Value& Key, rapidjson::Value& Value, size_t MaxDepth = 0, size_t CurrentDepth = 0);
    size_t CountByName(wxString& Text, bool Exact, size_t MaxDepth);

    wxDataViewItem GetRoot() const
    {
        return wxDataViewItem(m_root);
    }

    wxDataViewItem SelectText(wxString& Text, bool Next = true, bool Exact = false);

    bool IsArrayElement(wxDataViewItem* Item);

    EntityTreeModelNode* Duplicate(wxDataViewItem *Item, rapidjson::Document& Document);
    int Compare( const wxDataViewItem &item1, const wxDataViewItem &item2,
                 unsigned int column, bool ascending ) const wxOVERRIDE;

    virtual unsigned int GetColumnCount() const wxOVERRIDE
    {
        return 2;
    }

    virtual wxString GetColumnType( unsigned int col ) const wxOVERRIDE
    {
        if (col == 2)
            return "long";

        return "string";
    }

    virtual void GetValue( wxVariant &variant,
                           const wxDataViewItem &item, unsigned int col ) const wxOVERRIDE;
    virtual bool SetValue( const wxVariant &variant,
                           const wxDataViewItem &item, unsigned int col ) wxOVERRIDE;

    virtual bool IsEnabled( const wxDataViewItem &item,
                            unsigned int col ) const wxOVERRIDE;

    virtual wxDataViewItem GetParent( const wxDataViewItem &item ) const wxOVERRIDE;
    virtual bool IsContainer( const wxDataViewItem &item ) const wxOVERRIDE;
    virtual unsigned int GetChildren( const wxDataViewItem &parent,
                                      wxDataViewItemArray &array ) const wxOVERRIDE;

private:
    EntityTreeModelNode*   m_root;
};
