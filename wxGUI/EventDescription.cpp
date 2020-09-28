#include <map>
#include <string>
#include "EventDescription.h"

std::map<std::string, std::map<std::string, EntryDescription>> EventDescriptor;

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

        EventDescriptor[NameA][Value] = {NameB, Index};
        Index += 1;
    }

    fclose(EventFile);
    return true;
}
