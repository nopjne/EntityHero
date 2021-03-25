#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

void FindFile(std::wstring FileName)
{

}

bool BuildEntityDatabase(std::wstring Directory)
{
    // Open the directory that was specified.
    // Search for all the .resources files in the directory.
        // For every .resources open and search for .entities.
        // Decompress entities.
        // Run through all entities.
            // For each entity add the entity to a class field.
            // How to distiguish between the different values for a class?

    std::wstring tmp = Directory + L"\\*";
    WIN32_FIND_DATAW file;
    HANDLE search_handle = FindFirstFileW(tmp.c_str(), &file);
    if (search_handle != INVALID_HANDLE_VALUE)
    {
        std::vector<std::wstring> directories;

        do
        {
            if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if ((!lstrcmpW(file.cFileName, L".")) || (!lstrcmpW(file.cFileName, L"..")))
                    continue;
            }

            tmp = Directory + L"\\" + std::wstring(file.cFileName);
            if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                directories.push_back(tmp);

            // Process the files in this directory
            if (std::wstring(file.cFileName).rfind(L".resources") != std::wstring::npos) {
                // Execute load and search for the entities file.
                assert(false); // TODO: Implement me.
            }

        } while (FindNextFileW(search_handle, &file));

        FindClose(search_handle);

        for (std::vector<std::wstring>::iterator iter = directories.begin(), 
             end = directories.end(); iter != end; 
             ++iter) {


            // Push next directory goto restart.
            FindFile(*iter);
        }   
    }

    return true;
}

void LoadEntityDatabase()
{
    // Parse the clear text db.
}


