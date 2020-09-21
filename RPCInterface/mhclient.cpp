#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "meathook_interface.h" 
#include <windows.h>
#include "mhclient.h"

// struct idMat3 {
//     idVec3			mat[3];
// };
// 
// class idAngles
// {
// public:
//     float			pitch;
//     float			yaw;
//     float			roll;
//     idMat3 ToMat3() const;
// };
// 
// #define DEG2RAD(a)				( (a) * idMath::M_DEG2RAD )
// 
// idMat3 idAngles::ToMat3() const
// {
//     idMat3 mat;
//     float sr, sp, sy, cr, cp, cy;
// 
//     idMath::SinCos(DEG2RAD(yaw), sy, cy);
//     idMath::SinCos(DEG2RAD(pitch), sp, cp);
//     idMath::SinCos(DEG2RAD(roll), sr, cr);
// 
//     mat.mat[0].Set(cp * cy, cp * sy, -sp);
//     mat.mat[1].Set(sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp);
//     mat.mat[2].Set(cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp);
//     return mat;
// }

DWORD WINAPI MeathookInterface::KeepAlive(LPVOID Data)
{
    MeathookInterface *pthis = (MeathookInterface*)Data;
    while (1) {
        pthis->m_Initialized = false;
        pthis->InitializeRpcInterface();
        int result = WaitForSingleObject(pthis->m_UnInitialized, INFINITE);
        Sleep(5000);
    }
}

void MeathookInterface::StartKeepAliveThread()
{
    m_UnInitialized = CreateEvent(NULL, true, false, L"MHThreadEvent");
    CreateThread(NULL, 0, MeathookInterface::KeepAlive, this, 0, &m_ThreadId);
}

void MeathookInterface::GetSpawnInfo(unsigned char* pBuffer)
{
    RpcTryExcept
    {
        int Size = (int)sizeof(m_SpawnInfoBuffer);
        ::GetSpawnInfo(meathook_interface_v1_0_c_ifspec, &Size, (unsigned char*) m_SpawnInfoBuffer);
        strcpy_s((char*)pBuffer, 256, m_SpawnInfoBuffer);
    }
    RpcExcept(1)
    {
        int ulCode = RpcExceptionCode();
        printf("Runtime reported exception 0x%lx = %ld\n", ulCode, ulCode);
    }
    RpcEndExcept
}
void MeathookInterface::GetEntitiesFile(unsigned char* pBuffer, size_t *Size)
{
    RpcTryExcept
    {
        int TempSize = (int)*Size;
        ::GetEntitiesFile(meathook_interface_v1_0_c_ifspec, &TempSize, pBuffer);
        *Size = TempSize;
    }
    RpcExcept(1)
    {
        int ulCode = RpcExceptionCode();
        printf("Runtime reported exception 0x%lx = %ld\n", ulCode, ulCode);
    }
    RpcEndExcept
}

void MeathookInterface::PushEntitiesFile(unsigned char* pBuffer, size_t Size)
{
    RpcTryExcept
    {
        ::PushEntitiesFile(meathook_interface_v1_0_c_ifspec, (int)Size, pBuffer);
    }
    RpcExcept(1)
    {
        int ulCode = RpcExceptionCode();
        printf("Runtime reported exception 0x%lx = %ld\n", ulCode, ulCode);
    }
    RpcEndExcept
}

void MeathookInterface::ExecuteConsoleCommand(unsigned char* pszString)
{
    RpcTryExcept
    {
        ::ExecuteConsoleCommand(meathook_interface_v1_0_c_ifspec, pszString);
    }
    RpcExcept(1)
    {
        int ulCode = RpcExceptionCode();
        printf("Runtime reported exception 0x%lx = %ld\n", ulCode, ulCode);
    }
    RpcEndExcept
}

bool MeathookInterface::DestroyRpcInterface() {
    int status = RpcStringFreeA(&pszStringBinding);
    if (status != 0) {
        return status;
    }

    status = RpcBindingFree(&meathook_interface_v1_0_c_ifspec);
    if (status != 0) {
        return status;
    }

    return 0;
}

bool MeathookInterface::InitializeRpcInterface()
{
    RPC_STATUS status;
    unsigned char* pszUuid = NULL;
    const char* pszProtocolSequence = "ncacn_np";
    unsigned char* pszNetworkAddress = NULL;
    const char* pszEndpoint = "\\pipe\\meathook_interface_rpc";
    unsigned char* pszOptions = NULL;

    status = RpcStringBindingComposeA(
        pszUuid,
        (unsigned char*)pszProtocolSequence,
        pszNetworkAddress,
        (unsigned char*)pszEndpoint,
        pszOptions,
        &pszStringBinding
        );

    if (status != 0) {
        return status;
    }

    status = RpcBindingFromStringBindingA(pszStringBinding, &meathook_interface_v1_0_c_ifspec);
    if (status != 0) {
        return status;
    }

    m_Initialized = true;
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