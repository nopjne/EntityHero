#pragma once
#ifndef _EVENT_DESCRIPTION_INCLUDED
#define _EVENT_DESCRIPTION_INCLUDED

#include <map>
#include <string>

struct EntryDescription {
    std::string Type;
    int Index;
};

extern std::map<std::string, std::map<std::string, EntryDescription>> EventDescriptor;

#endif // _EVENT_DESCRIPTION_INCLUDED