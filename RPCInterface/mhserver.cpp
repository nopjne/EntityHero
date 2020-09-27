#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "meathook_interface.h"
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

void ExecuteConsoleCommand(
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ unsigned char* pszString)
{
    idCmd::execute_command_text((char *)pszString);
}

char *gOverrideData;
const char gOver[] = "*.entities";
const char *gOverrideFileName = nullptr;
void PushEntitiesFile(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ int Size,
    /* [size_is][in] */ unsigned char* pBuffer)
{
    char *NewBuffer = new char[Size];
    memcpy(NewBuffer, pBuffer, Size);
    void* OldOverride = gOverrideData;
    gOverrideData = NewBuffer;
    delete[] OldOverride;
    gOverrideFileName = gOver;

    // TODO: Casue a reload.
}

void GetEntitiesFile(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int* Size,
    /* [size_is][out] */ unsigned char* pBuffer)
{
    void* levelmap = call_as<void*>(descan::g_maplocal_getlevelmap, *reinterpret_cast<void**>(descan::g_gamelocal));
    if (!levelmap) {
        idLib::Printf("GetLevelMap returned null.\n");
        return;
    }

    memcpy(pBuffer, levelmap, *Size);
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
}

