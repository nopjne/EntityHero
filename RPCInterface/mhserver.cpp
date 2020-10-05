#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <meathook_interface.h>
#include <windows.h>
#include "cmdsystem.hpp"
#include "game_exe_interface.hpp"
#include "doomoffs.hpp"
#include "meathook.h"
#include "cmdsystem.hpp"
#include "idtypeinfo.hpp"
#include "eventdef.hpp"
#include "scanner_core.hpp"
#include "idLib.hpp"
#include "idStr.hpp"
#include "clipboard_helpers.hpp"

class RpcServer
{
    DWORD m_ThreadId;

    static DWORD WINAPI RpcListener(LPVOID Data);
public:
    RpcServer() {
        CreateThread(NULL, 0, RpcListener, this, 0, &m_ThreadId);
    }
};

DWORD WINAPI RpcServer::RpcListener(LPVOID Data)
{
    RPC_STATUS status;
    const char* pszProtocolSequence = "ncacn_np";
    unsigned char* pszSecurity = NULL;
    const char* pszEndpoint = "\\pipe\\meathook_interface_rpc";
    unsigned int    cMinCalls = 1;
    unsigned int    fDontWait = FALSE;

    status = RpcServerUseProtseqEpA((unsigned char*)pszProtocolSequence,
                                    RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                    (unsigned char*)pszEndpoint,
                                    pszSecurity);

    if (status) return 0;

    status = RpcServerRegisterIf(meathook_interface_v1_0_s_ifspec,
                                 NULL,
                                 NULL);

    if (status) return 0;

    status = RpcServerListen(cMinCalls,
                             RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                             fDontWait);

    if (status) return 0;

    return 0;
}

/******************************************************/
/*         MIDL allocate and free                     */
/******************************************************/

void __RPC_FAR* __RPC_USER midl_user_allocate(size_t len)
{
    return(malloc(len));
}

void __RPC_USER midl_user_free(void __RPC_FAR* ptr)
{
    free(ptr);
}

const static RpcServer gRpcServer;

void KeepAlive(
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ int* Size)
{
}

void ExecuteConsoleCommand(
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ unsigned char* pszString
    )
{
    idCmd::execute_command_text((char *)pszString);
}

extern std::string gActiveEncounterName;
void GetActiveEncounter(
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ int *Size,
    /* [string][in] */ unsigned char* pszString
    )
{
    idCmd::execute_command_text("mh_active_encounter");
    strcpy_s((char*)pszString, *Size, gActiveEncounterName.c_str());
    OutputDebugStringA("MH Interface Active encounter:");
    OutputDebugStringA(gActiveEncounterName.c_str());
    OutputDebugStringA("\n");
    *Size = (int)gActiveEncounterName.size();
}

void OverloadMemory(void* Memory, void* Data, size_t Size);
void WriteOverloadMemorySize(void* Memory, size_t Size);
void WriteOverloadMemory(void* Memory, void* Data, size_t Size, size_t Offset);
std::string gOverrideName;
extern std::string gLastLoadedEntities;
char gOverrideBuffer[256];
const char* gOverrideFileName = nullptr;
unsigned long long gBufferReload = 0;
void PushEntitiesFile(
    /* [in] */ handle_t IDL_handle,
    /* [size_is][in] */ unsigned char* pBuffer,
    boolean Start,
    int Size)
{
    if (Start != false) {
        strcpy_s(gOverrideBuffer, (char*)pBuffer);
        gOverrideFileName = gOverrideBuffer;
        OutputDebugStringA("Overriding the next load: ");
        OutputDebugStringA(gOverrideBuffer);
        OutputDebugStringA("\n");
        // Currently crashes when invoked from the RPC thread.
        // Works if invoked from console, but does not cause the map to be re-parsed from file.
        //gBufferReload = true;

        //idCmd::add_command("mh_force_reload");
        //idCmd::execute_command_buffer();
#if 0
        idCmd::execute_command_text("mh_force_reload"); 
#endif
    } else {

        // TODO: Cause an automatic reload.
    }
}

void UploadData(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ int Size,
    /* [in] */ int Offset,
    /* [size_is][in] */ unsigned char* pBuffer)
{
    // if (gMemoryHookData != nullptr) {
    //     char String[MAX_PATH];
    //     sprintf_s(String, "Mem overload: size:%i offset:%i\n", Size, Offset);
    //     OutputDebugStringA(String);
    //     WriteOverloadMemory(gMemoryHookData, pBuffer, Size, Offset);
    // }
}

void GetEntitiesFile(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int* Size,
    /* [size_is][out] */ unsigned char* pBuffer)
{
    //void* levelmap = call_as<void*>(descan::g_maplocal_getlevelmap, *reinterpret_cast<void**>(descan::g_gamelocal));
    //if (!levelmap) {
    //    idLib::Printf("GetLevelMap returned null.\n");
    //    return;
    //}
    gOverrideName = gLastLoadedEntities;

    char TempPath[MAX_PATH];
    char TempFile[MAX_PATH];
    GetTempPathA(sizeof(TempPath), TempPath);
    GetTempFileNameA(TempPath, "Temp", 0, TempFile);
    strcat_s(TempPath, TempFile);
    //call_as<void>(descan::g_idmapfile_write, levelmap, TempPath);
    char String[MAX_PATH];
    sprintf_s(String, "mh_dumpmap %s", TempFile);
    idCmd::execute_command_text(String);
    memcpy(pBuffer, TempFile, strlen(TempFile) + 1);
    *Size = (int)strlen(TempFile);
}

void cmd_mh_spawninfo(idCmdArgs* args);
void GetSpawnInfo(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int* Size,
    /* [size_is][out] */ unsigned char* pBuffer)
{
    idCmd::execute_command_text("getviewpos");

    if (!OpenClipboard(NULL))
        return;
    char *cbhandle = (char*)GetClipboardData(CF_TEXT);
    strcpy_s((char*)pBuffer, *Size, cbhandle);
    CloseClipboard();
}

