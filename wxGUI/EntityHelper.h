#pragma once
#include "entityviewmodel.h"
#include "EventDescription.h"

typedef struct _Holder {
    std::string EntityName;
    std::string ModelName;
} Holder;

std::map<std::string, std::vector<Holder>> ValidEncounterMap = {
    {"ENCOUNTER_SPAWN_ARACHNOTRON", {{"cathedral_ai_heavy_arachnotron_1", "md6def/characters/monsters/arachnotron/base/arachnotron.md6"}}}
};

wxString GetModelString(EntityTreeModelNode* Root, wxString String);

EntityTreeModelNode* SearchParentNode(EntityTreeModelNode* EntityNode, wxString TextToFind)
{
    if (EntityNode == nullptr) {
        return nullptr;
    }

    // Walk up the chain untill an entitydef is present then look for layers.
    EntityTreeModelNode* FoundNode = EntityNode;
    if (FoundNode->m_key != TextToFind) {
        while ((EntityNode != nullptr) && ((FoundNode = EntityNode->Find(TextToFind)) == nullptr)) {
            EntityNode = EntityNode->GetParent();
        }
    }

    // Check EntityNode again as it may have been modified.
    if (EntityNode == nullptr) {
        return nullptr;
    }

    return FoundNode;
}

EntityTreeModelNode* GetLayerNode(EntityTreeModelNode* EntityNode)
{
    EntityTreeModelNode* Layers = SearchParentNode(EntityNode, "layers");
    if (Layers == nullptr) {
        return nullptr;
    }

    if (Layers->GetChildCount() == 0) {
        return nullptr;
    }

    return Layers->GetNthChild(0);
}

bool IsLayerSupported(EntityTreeModelNode* EntityNode, wxString LayerName)
{
    if (EntityNode == nullptr) {
        return false;
    }

    // Walk up the chain untill an entitydef is present then look for layers.
    EntityTreeModelNode *Layers = EntityNode;
    if (Layers->m_key != "layers") {
        while ((EntityNode != nullptr) && ((Layers = EntityNode->Find("layers")) == nullptr)) {
            EntityNode = EntityNode->GetParent();
        }
    }

    // Check EntityNode again as it may have been modified.
    if (EntityNode == nullptr) {
        return true;
    }

    if (Layers->GetNthChild(0)->m_value == LayerName) {
        return true;
    }

    return false;
}

std::vector<wxString> GetSpawnTokens(wxString EncounterName) {
    std::vector<wxString> SpawnEncounters;
    int EncounterCount = EncounterName.Freq(' ') + 1;
    size_t Start = 0;
    size_t End = 0;
    for (int i = 0; i < EncounterCount; i += 1) {
        End = EncounterName.find(' ', Start);
        if (End == wxString::npos) {
            End = EncounterName.Len();
        }

        wxString Token = EncounterName.SubString(Start, End);
        Start = End + 1;
        if (Token.Matches("ENCOUNTER_SPAWN*") != false) {
            SpawnEncounters.push_back(Token.Trim().Trim(false));
        }
    }

    return SpawnEncounters;
}

std::vector<bool> SpawnTypeSupportedByGroup(EntityTreeModelNode *root, EntityTreeModelNode* SpawnGroup, EntityTreeModelNode* Encounter)
{
    // Encounter name.
    std::vector<bool> EncounterPresent;
    if ((Encounter == nullptr) || (SpawnGroup == nullptr)) {
        return EncounterPresent;
    }

    wxString EnconterName = Encounter->m_value;
    wxString EntityDefName = SpawnGroup->m_key.substr(10);

    //entity(spawnGroup):entityDef(name):edit:entityDefs[]:name -> value (entitydef of idAI2)
    wxString ToFind = wxString::Format("%s:%s:edit:entityDefs", EntityDefName, SpawnGroup->m_key);
    EntityTreeModelNode* DefsArray = root->Find(ToFind);
    if (DefsArray == nullptr) {
        ToFind = wxString::Format("%s:%s:edit:targetSpawnParent", EntityDefName, SpawnGroup->m_key);
        EntityTreeModelNode* SpawnParent = root->Find(ToFind);
        if (SpawnParent != nullptr) {
            ToFind = wxString::Format("%s:entityDef %s", SpawnParent->m_value, SpawnParent->m_value);
            EntityTreeModelNode* TargetSpawn = root->Find(ToFind);
            return SpawnTypeSupportedByGroup(root, TargetSpawn, Encounter);
        }

        // ToFind = wxString::Format("%s:%s:edit:spawners", EntityDefName, SpawnGroup->m_key);
        // EntityTreeModelNode* DefsArray = root->Find(ToFind);
        // Check spawners and spawn target parent.
        return EncounterPresent;
    }

    // Construct the encounter spawn list
    std::vector<wxString> SpawnEncounters = GetSpawnTokens(EnconterName);
    EncounterPresent.resize(SpawnEncounters.size());

    // Search for the encounters in the array
    for (size_t i = 0; i < DefsArray->GetChildren().size(); i += 1) {
        EntityTreeModelNode* Entity = DefsArray->GetNthChild(i);
        wxString Name = Entity->FindKey("name")->m_value;

        //entity(idAI2)::entityDef(name)::edit::renderModelInfo::model -> value (arachnotron.md6)
        wxString idAiName = wxString::Format("entityDef %s", Name);
        wxString ModelValue = root->Find(wxString::Format("%s:%s:edit:renderModelInfo:model", Name, idAiName))->m_value;
        for (size_t Index = 0; Index < SpawnEncounters.size(); Index += 1) {
            auto Encounter = SpawnEncounters[Index];
            if (ValidEncounterMap.find(Encounter.c_str().AsChar()) != ValidEncounterMap.end()) {
                std::vector<Holder>& Model = ValidEncounterMap[Encounter.c_str().AsChar()];
                for (size_t x = 0; x < Model.size(); x += 1) {
                    if (ModelValue == Model[x].ModelName) {
                        EncounterPresent[Index] = true;
                        break;
                    }
                }
            }
        }
    }

    return EncounterPresent;
}

int SpawnTargetIndex(wxString String)
{
    auto Descriptor = EventDescriptor[String.c_str().AsChar()];
    auto Entry = Descriptor.find("idTargetSpawnGroup*");
    if (Entry == Descriptor.end()) {
        Entry = Descriptor.find("idTarget_Spawn*");
    }

    if (Entry == Descriptor.end()) {
        return -1;
    }

    return Entry->second.Index;
}

wxString GetEntityClass(EntityTreeModelNode* Root, wxString String)
{
    wxString FindStr = wxString::Format("%s:entityDef %s:class", String, String);
    EntityTreeModelNode* Class = Root->Find(FindStr);
    if (Class == nullptr) {
        return "";
    }

    return Class->m_value;
}

wxString GetAIDefinitionString(EntityTreeModelNode* Root, wxString String, wxString Name)
{
    // Iterate through all the targets in the spawn group select the one that resembles.
    wxString FindStr = wxString::Format("%s:entityDef %s:edit:targets", String, String);
    EntityTreeModelNode* Entity = Root->Find(FindStr);
    if (Entity == nullptr) {
        FindStr = wxString::Format("%s:entityDef %s:edit:entityDefs", String, String);
        Entity = Root->Find(FindStr);
        FindStr = wxString::Format("%s:entityDef %s:edit:targetSpawnParent", String, String);
        Entity = Root->Find(FindStr);
        if (Entity != nullptr) {
            return GetAIDefinitionString(Root, Entity->m_value, Name);
        }

        if (Entity == nullptr) {
            FindStr = wxString::Format("%s:entityDef %s:edit:spawners", String, String);
            Entity = Root->Find(FindStr);
            if (Entity == nullptr) {
                return "";
            }

            EntityTreeModelNodePtrArray SpawnTargets = Entity->GetChildren();
            for (auto SpawnTarget : SpawnTargets) {
                wxString AI2NodeStr = GetAIDefinitionString(Root, SpawnTarget->m_value, Name);
                if (AI2NodeStr.empty() == false) {
                    return AI2NodeStr;
                }
            }

            FindStr = wxString::Format("%s:entityDef %s:edit:targetSpawnParent", String, String);
            Entity = Root->Find(FindStr);
            if (Entity == nullptr) {
                return "";
            }

            return GetAIDefinitionString(Root, Entity->m_value, Name);
        }
    }

    EntityTreeModelNodePtrArray SpawnTargets = Entity->GetChildren();
    for (auto SpawnTarget : SpawnTargets) {
        wxString EntityClass = GetEntityClass(Root, SpawnTarget->m_value);
        if (EntityClass == "idTarget_Spawn_Parent") {
            return GetAIDefinitionString(Root, SpawnTarget->m_value, Name);
        }

        wxString ModelString = GetModelString(Root, SpawnTarget->m_value);

        wxString NamePermutation = Name.substr(wxString("ENCOUNTER_SPAWN_").length());
        NamePermutation = NamePermutation.Lower();
        // Count underscores.
        int UnderscoreCount = NamePermutation.Freq('_');
        int PermutationCount = pow(2, UnderscoreCount);
        for (int i = 0; i < PermutationCount; i += 1) {
            int context = 0;
            int lastcontext = 0;
            wxString NameToTest;
            for (int x = 0; x < UnderscoreCount; x += 1) {
                context = NamePermutation.find('_', context);
                bool Remove = false;
                if (((i >> x) & 1) == 0) {
                    // Remove underscore.
                    context -= 1;
                    Remove = true;
                }

                NameToTest.append(NamePermutation.SubString(lastcontext, context));
                if (Remove != false) {
                    context += 2;

                } else {
                    context += 1;
                }

                lastcontext = context;
            }

            NameToTest.append(NamePermutation.SubString(lastcontext, NamePermutation.Length()));

            // Permute underscore additions.
            if (ModelString.Find(NameToTest.substr()) != wxString::npos) {
                return SpawnTarget->m_value;
            }
        }

        // Test for chunks only.
        int context = 0;
        int lastcontext = 0;
        wxString NameToTest;
        for (int x = 0; x < UnderscoreCount + 1; x += 1) {
            context = NamePermutation.find('_', context);
            NameToTest = NamePermutation.SubString(lastcontext, context - 1);
            context += 1;
            lastcontext = context;
            // Permute underscore additions.
            if (ModelString.Find(NameToTest.substr()) != wxString::npos) {
                return SpawnTarget->m_value;
            }
        }
    }

    return "";
}

wxString GetModelString(EntityTreeModelNode *Root, wxString String)
{
    wxString FindStr = wxString::Format("%s:entityDef %s:edit:renderModelInfo:model", String, String);
    EntityTreeModelNode *Entity = Root->Find(FindStr);
    if (Entity == nullptr) {
        return "";
    }

    return Entity->m_value;
}

// Scan for all eEncounterSpawnType_t's that match the new value.
void BuildEncounterToEntityMap(EntityTreeModelNode* root, bool *Busy, size_t *Index, size_t *MaxCount)
{
    //
    ValidEncounterMap.clear();
    auto AllNodes = root->GetChildren();
    *MaxCount = AllNodes.size();
    *Index = 0;
    for (auto Node : AllNodes) {
        // Check if user cancelled this operation.
        if (*Index == *MaxCount) {
            *Busy = false;
            return;
        }

        *Index += 1;

        // Look for encounter nodes then read through all possible spawn descriptions.
        EntityTreeModelNode *ClassDescription = Node->Find(wxString::Format("entityDef %s:class", Node->m_key));
        if ((ClassDescription == nullptr) || (ClassDescription->m_value != "idEncounterManager")) {
            continue;
        }

        EntityTreeModelNode* entityEvents = Node->Find(wxString::Format("entityDef %s:edit:encounterComponent:entityEvents", Node->m_key));
        // "[]:events[]:eventCall:args[]:eEncounterSpawnType_t"
        if (entityEvents == nullptr) {
            continue;
        }

        for (auto EntityNode : entityEvents->GetChildren()) {
            EntityTreeModelNode* Events = EntityNode->Find("events");
            if (Events == nullptr) {
                continue;
            }

            for (auto Event : Events->GetChildren()) {
                wxString Type = Event->Find("eventCall:eventDef")->m_value;
                int Index = SpawnTargetIndex(Type);
                if (Index == -1) {
                    continue;
                }

                EntityTreeModelNode* Arguments = Event->Find("eventCall:args");
                EntityTreeModelNode* SpawnType = Arguments->Find("eEncounterSpawnType_t");
                EntityTreeModelNode* SpawnTarget = Arguments->GetNthChild(Index);
                if (SpawnTarget == nullptr) {
                    continue;
                }

                wxString SpawnTargetStr = SpawnTarget->m_value;
                if (SpawnType == nullptr) {
                    continue;
                }

                //std::map<std::string, std::vector<Holder>> ValidEncounterMap = {
                //{"ENCOUNTER_SPAWN_ARACHNOTRON", {{"cathedral_ai_heavy_arachnotron_1", "md6def/characters/monsters/arachnotron/base/arachnotron.md6"}}}
                //};
                wxString SpawnTypeStr = SpawnType->m_value;
                int EncounterCount = SpawnTypeStr.Freq(' ') + 1;
                size_t Start = 0;
                size_t End = 0;
                for (int EncounterIndex = 0; EncounterIndex < EncounterCount; EncounterIndex += 1) {
                    End = SpawnTypeStr.find(L' ', Start);
                    wxString SpawnToken = SpawnType->m_value.substr(Start, End);
                    Start = End;

                    if (SpawnToken == "ENCOUNTER_SPAWN_ANY") {
                        continue;
                    }

                    wxString AI2String = GetAIDefinitionString(root, SpawnTargetStr, SpawnToken);
                    wxString ModelString = GetModelString(root, AI2String);
                    if (AI2String.empty() || ModelString.empty()) {
                        wxString Error = wxString::Format("Could not resolve %s in %s\n", SpawnToken, SpawnTargetStr);
                        OutputDebugStringA(Error.c_str().AsChar());
                        continue;
                    }

                    ValidEncounterMap[SpawnToken.c_str().AsChar()].push_back({AI2String.c_str().AsChar(), ModelString.c_str().AsChar()});
                }
            }
        }
    }

    *Busy = false;
}

EntityTreeModelNode* GetExistingAI2Node(EntityTreeModelNode* root, wxString EncounterSpawn, wxString Layer)
{
    assert(ValidEncounterMap.find(EncounterSpawn.c_str().AsChar()) != ValidEncounterMap.end());
    if (ValidEncounterMap.find(EncounterSpawn.c_str().AsChar()) == ValidEncounterMap.end()) {
        return nullptr;
    }

    auto SpawnGroups = ValidEncounterMap[EncounterSpawn.c_str().AsChar()];
    for (auto Group : SpawnGroups) {
        EntityTreeModelNode *EntityNode = root->Find(Group.EntityName);
        if (Layer.empty() != false) {
            return EntityNode;
        }

        if (EntityNode == nullptr) {
            continue;
        }

        auto LayerArray = EntityNode->Find("layers")->GetChildren();
        for (int x = 0; x < LayerArray.size(); x += 1) {
            if ((LayerArray[x]->m_value == Layer) || (LayerArray[x]->m_value == "spawn_target_layer")) {
                return EntityNode;
            }
        }
    }

    return nullptr;
}

void EnumChildren(EntityTreeModelNode* Parent, rapidjson::Value& val, rapidjson::Document& Document);
EntityTreeModelNode* RotationMatrixFromAngle(rapidjson::Document &Document, float Yaw, float Pitch, float Roll)
{
    float sr, sp, sy, cr, cp, cy;
    Yaw = wxDegToRad(Yaw);
    Pitch = wxDegToRad(Pitch);
    Roll = wxDegToRad(Roll);
    sy = sin(Yaw); cy = cos(Yaw);
    sp = sin(Pitch); cp = cos(Pitch);
    sr = sin(Roll); cr = cos(Roll);

    float mat[3][3] = { {cp * cy, cp * sy, -sp},
                        {sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp},
                        {cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp}
                      };

    // spawnOrientation = {
    // mat = {
    //     mat[0] = {
    //         x = 0.526016116;
    //         y = -0.829617441;
    //         z = -0.187194914;
    //     }
    //     mat[1] = {
    //         x = 0.843141496;
    //         y = 0.479849815;
    //         z = 0.242603809;
    //     }
    //     mat[2] = {
    //         x = -0.111442924;
    //         y = -0.285445303;
    //         z = 0.951893628;
    //     }
    // }
    // }

    rapidjson::Value MatrixKey("mat", Document.GetAllocator());
    rapidjson::Value Matrix("empty", Document.GetAllocator());
    Matrix.SetObject();
    rapidjson::Value Row;
    Row.SetObject();
    Row.AddMember(rapidjson::Value("x", Document.GetAllocator()), rapidjson::Value("empty", Document.GetAllocator()), Document.GetAllocator());
    Row.AddMember(rapidjson::Value("y", Document.GetAllocator()), rapidjson::Value("empty", Document.GetAllocator()), Document.GetAllocator());
    Row.AddMember(rapidjson::Value("z", Document.GetAllocator()), rapidjson::Value("empty", Document.GetAllocator()), Document.GetAllocator());

    char String[260];
    for (int i = 0; i < 3; i += 1) {
        sprintf_s(String, "mat[%i]", i);
        Row.MemberBegin()[0].value.SetFloat(mat[i][0]);
        Row.MemberBegin()[1].value.SetFloat(mat[i][1]);
        Row.MemberBegin()[2].value.SetFloat(mat[i][2]);
        rapidjson::Value RowCopy;
        RowCopy.CopyFrom(Row, Document.GetAllocator(), true);
        Matrix.AddMember(rapidjson::Value(String, strlen(String), Document.GetAllocator()), RowCopy, Document.GetAllocator());
    }

    EntityTreeModelNode* Node = new EntityTreeModelNode(nullptr, "mat", MatrixKey, Matrix, Document);
    EnumChildren(Node, Matrix, Document);
    return Node;
}

wxString GetEntityClassName(EntityTreeModelNode* Node)
{
    if (Node == nullptr) {
        return "";
    }

    // Walk up the chain untill an entitydef is present then look for layers.
    EntityTreeModelNode* ClassNode = Node;
    if (ClassNode->m_key.Matches("class") == false) {
        while (ClassNode != nullptr) {
            EntityTreeModelNode* ClassFound = ClassNode->Find("class");
            if (ClassFound == nullptr) {
                ClassNode = ClassNode->GetParent();

            } else {
                ClassNode = ClassFound;
                break;
            }
        }
    }

    // Check EntityNode again as it may have been modified.
    if (ClassNode == nullptr) {
        return "";
    }

    return ClassNode->m_value;
}

EntityTreeModelNode* GetEntityDefNode(EntityTreeModelNode *Node)
{
    if (Node == nullptr) {
        return nullptr;
    }

    // Walk up the chain untill an entitydef is present then look for layers.
    EntityTreeModelNode* EntityNode = Node;
    if (EntityNode->m_key.Matches("entityDef *") == false) {
        while (EntityNode != nullptr) {
            bool EntityFound = EntityNode->m_key.Matches("entityDef *");
            if (EntityFound != false) {
                break;
            }

            EntityNode = EntityNode->GetParent();
        }
    }

    return EntityNode;
}