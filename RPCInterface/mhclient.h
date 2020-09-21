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

    void ExecuteConsoleCommand(unsigned char* pszString);
    void PushEntitiesFile(unsigned char* pBuffer, size_t Size);
    void GetSpawnInfo(unsigned char* pBuffer);
    void GetEntitiesFile(unsigned char* pBuffer, size_t* Size);

    static DWORD WINAPI KeepAlive(LPVOID Data);
};