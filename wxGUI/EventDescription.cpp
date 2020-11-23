#include <map>
#include <vector>
#include <string>
#include <wx/string.h>
#include "EventDescription.h"

std::map<std::string, std::vector<std::pair<std::string, EntryDescription>>> EventDescriptor;
std::map<std::string, MenuDescriptor> MenuDescription = {
    {"spawn", {"idEncounterManager", "edit:encounterComponent:entityEvents", {"spawn*", "*spawn", "*spawn*", "spawn"}}},
    {"wait", {"idEncounterManager", "edit:encounterComponent:entityEvents", {"wait*", "*wait", "wait", "*wait*"}}},
    {"maintain", {"idEncounterManager", "edit:encounterComponent:entityEvents", {"maintain*", "*maintain*", "*maintain", "maintain"}}},
};

bool LoadEventDescriptor(void)
{
    FILE *EventFile;
    char NameA[256];
    char NameB[256];
    char Value[256];
    fopen_s(&EventFile, "eternalevents.txt", "rt");
    if (EventFile == nullptr) {
        fopen_s(&EventFile, "../eternalevents.txt", "rt");
        if (EventFile == nullptr) {
            fopen_s(&EventFile, "../../eternalevents.txt", "rt");
            if (EventFile == nullptr) {
                return false;
            }
        }
    }

    int Index = 0;
    while (feof(EventFile) == false) {
        int resultB = fscanf_s(EventFile, "\t%s : %s\n", NameB, (unsigned int)sizeof(NameB), Value, (unsigned int)sizeof(Value));
        if (resultB == 1) {
            strcpy_s(NameA, NameB);
            NameA[strlen(NameA) - 1] = 0;
            Index = 0;
            continue;
        }

        if (resultB == 0) {
            break;
        }

        EventDescriptor[NameA].push_back({Value, {NameB, Index}});
        Index += 1;
    }

    fclose(EventFile);
    return true;
}

bool LoadMenuDescriptor(void)
{
    FILE* EventFile;
    fopen_s(&EventFile, "insert_desc.txt", "rt");
    if (EventFile == nullptr) {
        fopen_s(&EventFile, "../insert_desc.txt", "rt");
        if (EventFile == nullptr) {
            fopen_s(&EventFile, "../../insert_desc.txt", "rt");
            if (EventFile == nullptr) {
                return false;
            }
        }
    }

    MenuDescription.clear();
    int Index = 0;
    char Line[4096]; //
    char* Context;
    while (feof(EventFile) == false) {
        char* ElementsRead = fgets(Line, (int)sizeof(Line), EventFile);
        if (ElementsRead == 0) {
            return false;
        }

        if (Line[0] == '/' && Line[1] == '/') {
            continue;
        }

        MenuDescriptor Descriptor;
        std::string Name;
        Name = wxString(strtok_s(Line, ",", &Context)).Trim().Trim(false);
        Descriptor.Class = wxString(strtok_s(nullptr, ",", &Context)).Trim().Trim(false);
        Descriptor.Requirements = wxString(strtok_s(nullptr, ",", &Context)).Trim().Trim(false);
        char *MatchString = nullptr;
        do {
            MatchString = strtok_s(nullptr, ",", &Context);
            wxString Match(MatchString);
            Match = Match.Trim().Trim(false);
            Descriptor.StringMatch.push_back(Match.c_str().AsChar());
        } while (MatchString != nullptr);

        MenuDescription[Name] = Descriptor;
    }

    fclose(EventFile);
    return true;
}