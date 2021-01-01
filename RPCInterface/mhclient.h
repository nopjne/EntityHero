#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "meathook_interface.h" 
#include <windows.h>

class MeathookInterface
{
    HANDLE m_UnInitialized;
    char m_SpawnInfoBuffer[MAX_PATH];
    void StartKeepAliveThread();
    DWORD m_ThreadId;
    unsigned char* pszStringBinding = NULL;

public:
    bool m_Initialized;
    MeathookInterface() { StartKeepAliveThread(); }
    ~MeathookInterface() {}
    bool DestroyRpcInterface();
    bool InitializeRpcInterface();

    bool ExecuteConsoleCommand(unsigned char* pszString);
    bool PushEntitiesFile(char* pFileName, char* pBuffer, int Size);
    bool GetSpawnInfo(unsigned char* pBuffer);
    bool GetEntitiesFile(unsigned char* pBuffer, size_t* Size);
    bool GetActiveEncounter(int* Size, char* pBuffer);
    bool GetCurrentCheckpoint(int* Size, char* pBuffer);

    static DWORD WINAPI KeepAlive(LPVOID Data);
};