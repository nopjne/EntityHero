#pragma once
#ifndef _EVENT_DESCRIPTION_INCLUDED
#define _EVENT_DESCRIPTION_INCLUDED

#include <map>
#include <string>
#include <vector>

struct EntryDescription {
    std::string Type;
    int Index;
};

extern std::map<std::string, std::vector<std::pair<std::string, EntryDescription>>> EventDescriptor;

struct MenuDescriptor {
    std::string Class; // idEncounterManager
    std::string Requirements; // edit:encounterComponent:entityEvents
    std::vector<std::string> StringMatch;
};

extern std::map<std::string, MenuDescriptor> MenuDescription;

bool LoadEventDescriptor(void);
bool LoadMenuDescriptor(void);

#endif // _EVENT_DESCRIPTION_INCLUDED