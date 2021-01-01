

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Mon Jan 18 19:14:07 2038
 */
/* Compiler settings for meathook_interface.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __meathook_interface_h__
#define __meathook_interface_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __meathook_interface_INTERFACE_DEFINED__
#define __meathook_interface_INTERFACE_DEFINED__

/* interface meathook_interface */
/* [version][uuid] */ 

void ExecuteConsoleCommand( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ unsigned char *pszString);

void PushEntitiesFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ unsigned char *pBuffer,
    /* [in] */ boolean Start,
    /* [in] */ int Size);

void UploadData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ int Size,
    /* [in] */ int Offset,
    /* [size_is][in] */ unsigned char *pBuffer);

void GetEntitiesFile( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer);

void GetActiveEncounter( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer);

void GetCurrentCheckpoint( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer);

void GetSpawnInfo( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer);

void KeepAlive( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size);



extern RPC_IF_HANDLE meathook_interface_v1_0_c_ifspec;
extern RPC_IF_HANDLE meathook_interface_v1_0_s_ifspec;
#endif /* __meathook_interface_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


