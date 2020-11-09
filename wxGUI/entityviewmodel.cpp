// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/dataview.h"
#include "entityviewmodel.h"

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
#include <vector>
#include <string>
#include <set>

extern std::map<std::string, std::set<std::string>> ValueMap;

using namespace rapidjson;
using namespace std;

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

#include "null.xpm"
#include "wx_small.xpm"

char Temp[MAX_PATH];
const char* ValueToString(Value &val)
{
    switch (val.GetType()) {
        case kNullType:
            sprintf_s(Temp, "null");
            return Temp;
        case kFalseType:
            sprintf_s(Temp, "false");
            return Temp;
        case kTrueType:
            sprintf_s(Temp, "true");
            return Temp;

        case kObjectType:
        {
            sprintf_s(Temp, "object");
            return Temp;
        }
        case kArrayType:
            sprintf_s(Temp, "array");
            return Temp;

        case kStringType:
        {
            return val.GetString();
        }
        default:
            if (val.IsDouble()) {
                sprintf_s(Temp, "%lf", val.GetDouble());
                return Temp;
            }
            else if (val.IsInt())
            {
                sprintf_s(Temp, "%i", val.GetInt());
                return Temp;
            }
            else if (val.IsUint())
            {
                sprintf_s(Temp, "%u", val.GetUint());
                return Temp;
            }
            else if (val.IsInt64())
            {
                sprintf_s(Temp, "%I64i", val.GetInt64());
                return Temp;
            }
            else {
                sprintf_s(Temp, "%I64u", val.GetUint64());
                return Temp;
            }
    }
}

void ToStringValue(const char *Input, size_t Length, rapidjson::Value &Value)
{
    int UpperFlags = Value.GetFlags() & 0xE000;
    if (strcmp(Input, "null") == 0) {
        Value.SetNull();
    } else if (strcmp(Input, "false") == 0) {
        Value.SetBool(false);
    } else if (strcmp(Input, "true") == 0) {
        Value.SetBool(true);
    }

    switch (Value.GetType()) {
        case kObjectType:
        {
            assert(false);
        }
        case kArrayType:
            assert(false);

        case kStringType:
        {
            Value.SetString(Input, Length);
            break;
        }
        default:
            if (Value.IsDouble()) {
                double TempValue;
                sscanf(Input, "%lf", &TempValue);
                Value.SetDouble(TempValue);
            }
            else if (Value.IsInt())
            {
                int TempValue;
                sscanf(Input, "%i", &TempValue);
                Value.SetInt(TempValue);
            }
            else if (Value.IsUint())
            {
                unsigned int TempValue;
                sscanf(Input, "%u", &TempValue);
                Value.SetUint(TempValue);
            }
            else if (Value.IsInt64())
            {
                int64_t TempValue;
                sscanf(Input, "%I64i", &TempValue);
                Value.SetInt64(TempValue);
            }
            else {
                uint64_t TempValue;
                sscanf(Input, "%I64u", &TempValue);
                Value.SetUint64(TempValue);
            }
    }

    // Preserve the upper flags (entity def, layers)
    Value.SetFlags(true, UpperFlags);
}

void ValidateTree(EntityTreeModelNode* Node, Value& key, Value& val, size_t CurrentDepth = 0)
{
    if (Node == nullptr) {
        return;
    }

    wxASSERT(Node->m_key == ValueToString(key));
    wxASSERT(Node->IsContainer() == val.IsObject());
    wxASSERT(!(Node->m_key == "eventDef") || !Node->IsContainer());
    for (int tab = 0; tab < CurrentDepth; tab += 1) {
        OutputDebugStringA("    ");
    }

    OutputDebugStringA(ValueToString(key));
    char String[MAX_PATH];
    sprintf_s(String, " %llX", (unsigned __int64)Node);
    OutputDebugStringA(String);
    if (Node->IsContainer()) {
        OutputDebugStringA(" Container ");
    }

    if (Node->m_valueRef->IsObject()) {
        OutputDebugStringA("Object ");
    }

    OutputDebugStringA("\n");

    if (val.IsObject() == false) {
        return;
    }

    if (Node->IsContainer() == false) {
        return;
    }

    int ArrayItemCount = 0;
    int ArrayItem = 0;
    int ChildIndex = 0;
    auto Children = Node->GetChildren();
    for (auto member = val.MemberBegin(); member != val.MemberEnd(); member++) {
        if (strcmp(ValueToString(member->name), "num") == 0) {
            // skip and reduce the tree for the next x items.
            continue;
        }

        if (ChildIndex >= Children.size()) {
            break;
        }

        if (wxString(ValueToString(member->name)).Matches("item[*]") != false) {
            // skip and reduce the tree for the next x items.
            for (auto member2 = member->value.MemberBegin(); member2 != member->value.MemberEnd(); member2++) {
                ValidateTree(Children[ChildIndex], member2->name, member2->value, CurrentDepth + 1);
                ChildIndex += 1;
                if (ChildIndex >= Children.size()) {
                    break;
                }
            }
            continue;
        }

        ValidateTree(Children[ChildIndex], member->name, member->value, CurrentDepth + 1);
        ChildIndex += 1;
    }
}

void EnumChildren(EntityTreeModelNode *Parent, Value &val, rapidjson::Document& Document)
{
    if (val.IsObject() == false) {
        return;
    }

    // Copy entity naming up.
    if ((Parent->m_keyRef->IsString() != false) && (strcmp(Parent->m_keyRef->GetString(), "entity") == 0)) {
        Parent->m_key = wxString((Parent->m_valueRef->FindMember("entityDef", false)->name.GetString()) + 10);
    }

    int ArrayItemCount = 0;
    int ArrayItem = 0;
    for (auto member = val.MemberBegin(); member != val.MemberEnd(); member++) {
        if (strcmp(ValueToString(member->name), "num") == 0) {
            // skip and reduce the tree for the next x items.
            ArrayItemCount = (val.MemberEnd() - val.MemberBegin()) - 1;
            continue;
        }
        if (ArrayItemCount != 0) {
            if (member->value.GetType() == kObjectType) {
                EnumChildren(Parent, member->value, Document);

            } else {
                char Temp[256];
                sprintf(Temp, "item[%i]", ArrayItem);
                EntityTreeModelNode *child = new EntityTreeModelNode(Parent,
                                                                        wxString(Temp),
                                                                        ValueToString(member->value),
                                                                        member->value,
                                                                        member->value,
                                                                        Document);
                Parent->GetChildren().Add(child);
            }

            ArrayItem += 1;
            if (ArrayItem == ArrayItemCount) {
                ArrayItemCount = 0;
            }
            continue;
        }
        if (member->value.GetType() == kObjectType) {
            EntityTreeModelNode *child = new EntityTreeModelNode(Parent, ValueToString((*member).name), member->name, member->value, Document);
            EnumChildren(child, member->value, Document);
            Parent->GetChildren().Add(child);
        } else {
            EntityTreeModelNode *child = new EntityTreeModelNode(Parent, ValueToString((*member).name), ValueToString((*member).value), member->name, member->value, Document);
            Parent->GetChildren().Add(child);
            wxString ValueString = wxString(ValueToString(member->value));
            // TODO: support adding every separated encounter type.
            if (ValueString.Matches("ENCOUNTER_SPAWN_*") != false) {
                int Separator = ValueString.find(' ');
                if (Separator != wxString::npos) {
                    ValueString = ValueString.substr(0, Separator);
                }
            }

            ValueMap[ValueToString(member->name)].insert(ValueString.c_str().AsChar());
        }
    }
}

// ----------------------------------------------------------------------------
// EntityTreeModel
// ----------------------------------------------------------------------------

EntityTreeModel::EntityTreeModel(rapidjson::Document& Document)
{
    m_root = new EntityTreeModelNode(NULL, "root", Document, Document, Document);
    m_root->m_keyCopy.SetString("root");
    EnumChildren(m_root, Document, Document);
}

bool PartialMatch(wxString& a, wxString& b)
{
    size_t len = a.Length();
    if (b.Length() < len) {
        len = b.Length();
    }

    if (len == 0) {
        return false;
    }

    return (strstr(b.c_str().AsChar(), a.c_str().AsChar()) != nullptr);
}

EntityTreeModelNode* EntityTreeModelNode::Find(wxString Text)
{
    if (Text.empty() != false) {
        return this;
    }

    size_t SeparatorPosition;
    if ((SeparatorPosition = Text.find(":")) != wxString::npos) {
        wxString Token = Text.SubString(0, SeparatorPosition - 1);
        wxString Rest = Text.Remove(0, SeparatorPosition + 1);

        size_t Index = 0;
        EntityTreeModelNode *Found = FindByName(0, 1, Token, 0, Index, true, true);
        if (Found == nullptr) {
            return nullptr;
        }

        return Found->Find(Rest);

    } else {
        size_t Index = 0;
        EntityTreeModelNode* Found = FindByName(0, 1, Text, 0, Index, true, true);
        return Found;
    }

    return nullptr;
}

EntityTreeModelNode* EntityTreeModelNode::FindKey(wxString Text)
{
    size_t IndexToFind = 0;
    return FindByName(0, 1, Text, 0, IndexToFind, true, true);
}

EntityTreeModelNode* EntityTreeModelNode::FindByName(size_t Depth, size_t MaxDepth, wxString& Text, size_t IndexToFind, size_t &Index, bool Exact, bool MatchCase)
{
    wxString StrText = Text;
    wxString StrKey = m_key;
    wxString StrValue = m_value;

    if (MatchCase == false) {
        StrText = StrText.Lower();
        StrKey = StrKey.Lower();
        StrValue = StrValue.Lower();
    }

    if ((StrText == StrKey || StrText == StrValue) ||
       ((Exact == false) && (PartialMatch(StrText, StrKey) || PartialMatch(StrText, StrValue)))) {

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
        EntityTreeModelNode *Result = GetChildren().at(i)->FindByName(Depth + 1, MaxDepth, Text, IndexToFind, Index, Exact, MatchCase);
        if (Result != nullptr) {
            return Result;
        }
    }

    return nullptr;
}

EntityTreeModelNode* EntityTreeModelNode::Visit(wxString& Text, bool Exact, bool MatchCase)
{
    wxString StrText = Text;
    wxString StrKey = m_key;
    wxString StrValue = m_value;

    if (MatchCase == false) {
        StrText = StrText.Lower();
        StrKey = StrKey.Lower();
        StrValue = StrValue.Lower();
    }

    if ((StrText == StrKey || StrText == StrValue) ||
        ((Exact == false) && (PartialMatch(StrText, StrKey) || PartialMatch(StrText, StrValue)))) {

        return this;
    }

    return nullptr;
}

// EntityTreeModelNode* EntityTreeModelNode::FindFromHere(wxString& Text, eSearchDirection SearchDir, bool Exact, bool MatchCase)
// {
//     std::vector<size_t> StackIndex;
//     std::vector<EntityTreeModelNode*> EntityStack; 
//     EntityStack.push_back(this);
//     StackIndex.push_back(m_children.size());
//     while ((EntityStack.empty() == false) && (StackIndex.empty() == false)) {
//         EntityTreeModelNode *Match = EntityStack.back()->Visit(Text, Exact, MatchCase);
//         if (Match != nullptr) {
//             return Match;
//         }
// 
//         EntityStack.pop_back();
// 
//         if (StackIndex.back() == 0) {
//             StackIndex.pop_back();
//             continue;
//         }
// 
//         auto Child = m_children[m_children.size() - StackIndex.back()];
//         EntityStack.push_back(Child);
//     }
// 
//     //Parent->FindFromHere(Text, SearchDir, Exact, MatchCase);
//     return nullptr;
// }

// Fwd search
// Next
// if Found(return)
// for each Child->Find()
//    if Found (return)
// if (Parent == invoker)
// Parent->Find(Parent->Location(This)) // Skip when parent is invoker
EntityTreeModelNode* EntityTreeModelNode::FindFromHere(EntityTreeModelNode* Invoker, wxString& Text, eSearchDirection SearchDir, bool Exact, bool MatchCase)
{
    EntityTreeModelNode *Found = nullptr;
    // Skip first invocation.
    if (Invoker == m_parent) {
        Found = Visit(Text, Exact, MatchCase);
    }

    if (Found != nullptr) {
        return Found;
    }

    // Check the location of the invoker in the child list, and start from that location.
    if ((Invoker != nullptr) || (SearchDir == NEXT)) {
        size_t Start = (SearchDir == NEXT) ? 0 : (m_children.size() - 1);
        for (size_t i = 0; i < m_children.size(); i += 1) {
            if (Invoker == m_children[i]) {
                if (SearchDir == NEXT) {
                    Start = i + 1;

                } else {
                    Start = i - 1;
                }
                break;
            }
        }

        size_t Index = Start;
        size_t End = (SearchDir == NEXT) ? m_children.size() : (-1);
        int Direction = (SearchDir==NEXT)?1:(-1);
        while (Index != End) {
            auto Child = m_children[Index];
            Found = Child->FindFromHere(this, Text, SearchDir, Exact, MatchCase);
            if (Found != nullptr) {
                return Found;
            }

            Index += Direction;
        }
    }

    if ((SearchDir == PREV) && (Invoker != nullptr)) {
        Found = Visit(Text, Exact, MatchCase);
        if (Found != nullptr) {
            return Found;
        }
    }

    if ((m_parent != Invoker) && (m_parent != nullptr)) {
        Found = m_parent->FindFromHere(this, Text, SearchDir, Exact, MatchCase);
    }

    return Found;
}

wxString m_PreviousToFind;
size_t Index = -1;
wxDataViewItem EntityTreeModel::SelectText(wxString& Text, eSearchDirection SearchDir, bool Exact, bool MatchCase)
{
    //
    // TODO: Rewrite this function to have a context Node. This should optimize consecutive searching considerably.
    //

    if (SearchDir == eSearchDirection::FIRST) {
        Index = 0;
    }

    size_t PrevIndex = Index;
    if (SearchDir == eSearchDirection::NEXT) {
        Index += 1;

    } else if (SearchDir == eSearchDirection::PREV) {
        Index -= 1;
    }

    if (m_PreviousToFind != Text) {
        Index = 0;
        PrevIndex = 0;
    }

    m_PreviousToFind = Text;
    size_t Counter = 0;
    EntityTreeModelNode *Result = m_root->FindByName(0, 0, Text, Index, Counter, Exact, MatchCase);
    if (Result == nullptr) {
        Index = PrevIndex;
    }

    return wxDataViewItem(Result);
}

bool EntityTreeModel::IsArrayElement(wxDataViewItem* Item)
{
    EntityTreeModelNode *Node = (EntityTreeModelNode*)Item->GetID();
    EntityTreeModelNode *Parent = Node->GetParent();
    if (Parent == nullptr) {
        return false;
    }

    if ((Parent->m_valueRef->GetFlags() & 0x4000) != 0) {
        // Check whether Parent is a direct ancestor, in JSON too.
        // for (auto Member = Parent->m_valueRef->MemberBegin(); Member != Parent->m_valueRef->MemberEnd(); Member++) {
        //     if (&(Member->name) == Node->m_keyRef) {
        //         return true;
        //     }
        // }
        // 
        // return false;
        //if (Node->m_keyRef)
        return true;
    }

    return false;
}

size_t EntityTreeModel::CountByName(wxString& Text, bool Exact, size_t MaxDepth)
{
    size_t Counter = 0;
    EntityTreeModelNode* Result = nullptr;
    do {
        Result = m_root->FindByName(0, MaxDepth, Text, Index, Counter, Exact, true);
    } while (Result != nullptr);

    return Counter;
}

size_t EntityTreeModelNode::GetChildIndex(EntityTreeModelNode* Node)
{
    if (IsContainer() == false) {
        return -1;
    }

    size_t count = m_children.GetCount();
    for (size_t i = 0; i < count; i++)
    {
        EntityTreeModelNode* child = m_children[i];
        if (child == Node) {
            return i;
        }
    }

    return -1;
}

rapidjson::Value DuplicateJson(rapidjson::Value &val, rapidjson::Document& Document) {
    rapidjson::Value Out;
    Out.CopyFrom(val, Document.GetAllocator(), true);
    return Out;
}

EntityTreeModelNode* EntityTreeModel::Duplicate(wxDataViewItem* Item, rapidjson::Document &Document)
{
    EntityTreeModelNode* node = (EntityTreeModelNode*)Item->GetID();
    if (node == nullptr) {
        return nullptr;
    }

    EntityTreeModelNode *Parent = node->GetParent();
    size_t ChildIndex = Parent->GetChildIndex(node);
    // Copy the json tree first.
    Value valueCopy = DuplicateJson(*(node->m_valueRef), Document);
    Value keyCopy = DuplicateJson(*(node->m_keyRef), Document);
    EntityTreeModelNode* NewNode;
    if (node->m_valueCopy.IsObject() == false) {
        NewNode = new EntityTreeModelNode(Parent, wxString("0"), ValueToString(node->m_valueCopy), node->m_valueCopy, node->m_valueCopy, Document);

    } else {
        NewNode = new EntityTreeModelNode(Parent, node->m_key, keyCopy, valueCopy, Document);
    }

    EnumChildren(NewNode, valueCopy, Document);
    Parent->Insert(NewNode, ChildIndex + 1, Document, INSERT_TYPE_AUTO);
    ItemAdded(GetParent(*Item), wxDataViewItem(NewNode));
    RebuildReferences(node->GetParent(), *(node->GetParent()->m_keyRef), *(node->GetParent()->m_valueRef), 1);
    RebuildReferences(NewNode, *(NewNode->m_keyRef), *(NewNode->m_valueRef), 0);
    return NewNode;
}

wxString EntityTreeModel::GetKey( const wxDataViewItem &item ) const
{
    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    if (!node)      // happens if item.IsOk()==false
        return wxEmptyString;

    return node->m_key;
}

wxString EntityTreeModel::GetValue( const wxDataViewItem &item ) const
{
    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    if (!node)      // happens if item.IsOk()==false
        return wxEmptyString;

    return node->m_value;
}

void EntityTreeModel::RebuildReferences(EntityTreeModelNode *Node, rapidjson::Value &Key, rapidjson::Value& Value, size_t MaxDepth, size_t CurrentDepth)
{
    for (int tab = 0; tab < CurrentDepth; tab += 1) {
        OutputDebugStringA("    ");
    }

    OutputDebugStringA(Node->m_key);
    OutputDebugStringA(" ");
    OutputDebugStringA(ValueToString(Key));
    if (Node->IsContainer()) {
        OutputDebugStringA(" Container ");
    }

    if (Node->m_valueRef->IsObject()) {
        OutputDebugStringA("Object ");
    }

    OutputDebugStringA("\n");

    bool isObject = Node->m_valueRef->IsObject();
    assert(Node->IsContainer() == Node->m_valueRef->IsObject());

    isObject = Value.IsObject();
    assert(Node->IsContainer() == Value.IsObject());

    Node->m_keyRef = &Key;
    Node->m_valueRef = &Value;

    size_t ArrayItemCount = 0;
    size_t ArrayItem = 0;
    if ((MaxDepth != 0) && (MaxDepth == CurrentDepth)) {
        return;
    }

    if (Node->IsContainer() != false) {
        if (strcmp(ValueToString(*(Node->m_keyRef)), "entity") == 0) {
            size_t Index = 0;
            wxString ToFind = "entityDef ";
            EntityTreeModelNode *Found = Node->FindByName(0, 1, ToFind, 0, Index, false, true);
            if (Found != nullptr) {
                Node->m_key = Found->m_key.substr(10);
            }
        }

        auto Member = Node->GetChildren().begin();
        for (auto JsonMember = Node->m_valueRef->MemberBegin(); JsonMember != Node->m_valueRef->MemberEnd(); JsonMember++) {
            // for (int tab = 0; tab < CurrentDepth; tab += 1) {
            //     OutputDebugStringA("    ");
            // }
            // 
            // OutputDebugStringA(ValueToString(JsonMember->name));
            // OutputDebugStringA("\n");
            if (strcmp(ValueToString(JsonMember->name), "num") == 0) {
                // skip and reduce the tree for the next x items.
                //ArrayItemCount = JsonMember->value.GetInt();
                ArrayItemCount = (Node->m_valueRef->MemberEnd() - Node->m_valueRef->MemberBegin()) - 1;
                continue;
            }

            if (ArrayItemCount != 0) {
                char itemstr[MAX_PATH];
                sprintf(itemstr, "item[%i]", (int)ArrayItem);
                if (JsonMember->value.GetType() == kObjectType) {
                    auto ArrayObject = JsonMember->value.GetObject();
                    auto NextMember = Member + 1;
                    for (auto ArrayMember = ArrayObject.MemberBegin(); ArrayMember != ArrayObject.MemberEnd(); ArrayMember++) {
                        RebuildReferences(*Member, ArrayMember->name, ArrayMember->value, MaxDepth, CurrentDepth + 1);
                        Member++;
                    }

                } else {
                    RebuildReferences(*Member, JsonMember->name, JsonMember->value, MaxDepth, CurrentDepth + 1);
                    (**Member).m_key = itemstr;
                    Member++;
                }

                ArrayItem += 1;
                if (ArrayItem == ArrayItemCount) {
                    ArrayItemCount = 0;
                }
                continue;
            }

            RebuildReferences(*Member, JsonMember->name, JsonMember->value, MaxDepth, CurrentDepth + 1);
            Member++;
        }

        assert(Member == Node->GetChildren().end());
    }
}

void EntityTreeModel::Delete( const wxDataViewItem &item )
{
    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    if (!node)      // happens if item.IsOk()==false
        return;

    wxDataViewItem parent( node->GetParent() );
    if (!parent.IsOk())
    {
        wxASSERT(node == m_root);

        // don't make the control completely empty:
        wxLogError( "Cannot remove the root item!" );
        return;
    }

    // first remove the node from the parent's array of children;
    // NOTE: EntityTreeModelNodePtrArray is only an array of _pointers_
    //       thus removing the node from it doesn't result in freeing it
    node->GetParent()->GetChildren().Remove( node );

    // Delete the JSON backing items.
    bool Deleted = false;
    Value *ParentRef = node->GetParent()->m_valueRef;
    for (auto Member = ParentRef->MemberBegin(); Member != ParentRef->MemberEnd(); Member++) {
        if (&(Member->name) == (node->m_keyRef)) {
            ParentRef->EraseMember(Member, Member + 1);
            Deleted = true;
            break;
        }

        if (Member->value.IsObject() != false) {
            if (&(Member->value.MemberBegin()->name) == (node->m_keyRef)) {
                ParentRef->EraseMember(Member, Member + 1);
                Deleted = true;
                break;
            }
        }
    }

    assert(Deleted != false);
    RebuildReferences(node->GetParent(), *(node->GetParent()->m_keyRef), *(node->GetParent()->m_valueRef), 1);

    // notify control
    ItemDeleted( parent, item );
}

int EntityTreeModel::Insert(wxDataViewItem* ParentItem, size_t Index, wxDataViewItem* Item, rapidjson::Document& Document, INSERT_TYPE InsertType)
{
    EntityTreeModelNode* Node = (EntityTreeModelNode*)Item->GetID();
    if (!Node)      // happens if item.IsOk()==false
        return 0;

    EntityTreeModelNode* ParentNode = (EntityTreeModelNode*)ParentItem->GetID();
    if (!ParentNode)      // happens if item.IsOk()==false
        return 0;

    assert(Index != size_t(-1));

    // If inserting an object, the object also needs an additional item[x] object before it can be inserted.
    ValidateTree(ParentNode, *(ParentNode->m_keyRef), *(ParentNode->m_valueRef));
    ParentNode->Insert(Node, Index, Document, InsertType);
    ItemAdded(*ParentItem, wxDataViewItem(Node));
    RebuildReferences(ParentNode, *(ParentNode->m_keyRef), *(ParentNode->m_valueRef), 1);
    //ValidateTree(ParentNode, *(ParentNode->m_keyRef), *(ParentNode->m_valueRef));
    RebuildReferences(Node, *(Node->m_keyRef), *(Node->m_valueRef), 0);
    ValidateTree(Node, *(Node->m_keyRef), *(Node->m_valueRef));

    return 1;
}

int EntityTreeModel::Compare( const wxDataViewItem &item1, const wxDataViewItem &item2,
                               unsigned int column, bool ascending ) const
{
    wxASSERT(item1.IsOk() && item2.IsOk());
        // should never happen

    if (IsContainer(item1) && IsContainer(item2))
    {
        wxVariant value1, value2;
        GetValue( value1, item1, 0 );
        GetValue( value2, item2, 0 );

        wxString str1 = value1.GetString();
        wxString str2 = value2.GetString();
        int res = str1.Cmp( str2 );
        if (res) return res;

        // items must be different
        wxUIntPtr litem1 = (wxUIntPtr) item1.GetID();
        wxUIntPtr litem2 = (wxUIntPtr) item2.GetID();

        return litem1-litem2;
    }

    return wxDataViewModel::Compare( item1, item2, column, ascending );
}

void EntityTreeModel::GetValue( wxVariant &variant,
                                 const wxDataViewItem &item, unsigned int col ) const
{
    wxASSERT(item.IsOk());

    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    switch (col)
    {
    case 0:
        variant = node->m_key;
        break;
    case 1:
        if (node->HasContainerColumns()) {
            EntityTreeModelNode* EncounterSpawnNode = node->Find("args:eEncounterSpawnType_t");
            if (EncounterSpawnNode != nullptr) {
                variant = EncounterSpawnNode->m_value;
            }

        } else {
            variant = node->m_value;
        }
        break;
    default:
        wxLogError( "EntityTreeModel::GetValue: wrong column %d", col );
    }
}

void EntityTreeModel::GetJsonValue( wxVariant &variant,
                                 const wxDataViewItem &item, unsigned int col ) const
{
    wxASSERT(item.IsOk());

    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    switch (col)
    {
    case 0:
        variant = wxString(ValueToString(*(node->m_keyRef)));
        break;
    case 1:
        if (node->HasContainerColumns()) {
            EntityTreeModelNode* EncounterSpawnNode = node->Find("args:eEncounterSpawnType_t");
            if (EncounterSpawnNode != nullptr) {
                variant = wxString(ValueToString(*(EncounterSpawnNode->m_valueRef)));
            }

        } else {
            variant = wxString(ValueToString(*(node->m_valueRef)));
        }
        break;
    default:
        wxLogError( "EntityTreeModel::GetValue: wrong column %d", col );
    }
}

bool EntityTreeModel::SetValue( const wxVariant &variant,
                                 const wxDataViewItem &item, unsigned int col )
{
    wxASSERT(item.IsOk());

    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    int UpperFlags;
    switch (col)
    {
        case 0:
        {
            node->m_key = variant.GetString();
            UpperFlags = node->m_keyRef->GetFlags() & 0xE000;
            node->m_keyRef->SetString(node->m_key.c_str().AsChar(), node->m_key.Len());
            node->m_keyRef->SetFlags(true, UpperFlags);
            node->m_keyCopy.SetString(node->m_key.c_str().AsChar(), node->m_key.Len());
            node->m_keyCopy.SetFlags(true, UpperFlags);

            EntityTreeModelNode *ParentNode = node->GetParent();
            if (ParentNode != nullptr) {
                RebuildReferences(ParentNode, *(ParentNode->m_keyRef), *(ParentNode->m_valueRef), 1);
                RebuildReferences(node, *(node->m_keyRef), *(node->m_valueRef), 0);
            }

            return true;
        }
        case 1:
            assert(node->IsContainer() == false);
            node->m_value = variant.GetString();
            ToStringValue(node->m_value.c_str().AsChar(), node->m_value.Len(), *(node->m_valueRef));
            ToStringValue(node->m_value.c_str().AsChar(), node->m_value.Len(), node->m_valueCopy);
            return true;
        default:
            wxLogError( "EntityTreeModel::SetValue: wrong column" );
    }
    return false;
}

bool EntityTreeModel::IsEnabled(const wxDataViewItem& item, unsigned int col) const
{
    return true;
}

wxDataViewItem EntityTreeModel::GetParent( const wxDataViewItem &item ) const
{
    // the invisible root node has no parent
    if (!item.IsOk())
        return wxDataViewItem(0);

    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();

    if (node == m_root)
        return wxDataViewItem(0);

    return wxDataViewItem( (void*) node->GetParent() );
}

bool EntityTreeModel::HasContainerColumns(const wxDataViewItem& item) const
{
    // the invisible root node can have children
    if (!item.IsOk())
        return true;

    EntityTreeModelNode* node = (EntityTreeModelNode*)item.GetID();
    return node->HasContainerColumns();
}

bool EntityTreeModel::IsContainer( const wxDataViewItem &item ) const
{
    // the invisible root node can have children
    if (!item.IsOk())
        return true;

    EntityTreeModelNode *node = (EntityTreeModelNode*) item.GetID();
    return node->IsContainer();
}

bool EntityTreeModel::IsArrayElement(const wxDataViewItem& item) const
{
    if (!item.IsOk())
        return false;

    EntityTreeModelNode* node = (EntityTreeModelNode*)item.GetID();
    if (node->GetParent() == nullptr) {
        return false;
    }

    return node->GetParent()->IsArray();
}

unsigned int EntityTreeModel::GetChildren( const wxDataViewItem &parent,
                                            wxDataViewItemArray &array ) const
{
    EntityTreeModelNode *node = (EntityTreeModelNode*) parent.GetID();
    if (!node)
    {
        array.Add( wxDataViewItem( (void*) m_root ) );
        return 1;
    }

    if (node->GetChildCount() == 0)
    {
        return 0;
    }

    unsigned int count = node->GetChildren().GetCount();
    for (unsigned int pos = 0; pos < count; pos++)
    {
        EntityTreeModelNode *child = node->GetChildren().Item( pos );
        array.Add( wxDataViewItem( (void*) child ) );
    }

    return count;
}