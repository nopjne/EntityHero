

/* this ALWAYS GENERATED file contains the RPC client stubs */


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

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#include <string.h>

#include "meathook_interface.h"

#define TYPE_FORMAT_STRING_SIZE   43                                
#define PROC_FORMAT_STRING_SIZE   337                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _meathook_interface_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } meathook_interface_MIDL_TYPE_FORMAT_STRING;

typedef struct _meathook_interface_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } meathook_interface_MIDL_PROC_FORMAT_STRING;

typedef struct _meathook_interface_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } meathook_interface_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const meathook_interface_MIDL_TYPE_FORMAT_STRING meathook_interface__MIDL_TypeFormatString;
extern const meathook_interface_MIDL_PROC_FORMAT_STRING meathook_interface__MIDL_ProcFormatString;
extern const meathook_interface_MIDL_EXPR_FORMAT_STRING meathook_interface__MIDL_ExprFormatString;

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: meathook_interface, ver. 1.0,
   GUID={0x1c9ca7c8,0xd421,0x482d,{0xb8,0x5d,0x79,0xfa,0xc3,0x3b,0x26,0x58}} */



static const RPC_CLIENT_INTERFACE meathook_interface___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x1c9ca7c8,0xd421,0x482d,{0xb8,0x5d,0x79,0xfa,0xc3,0x3b,0x26,0x58}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000000
    };
RPC_IF_HANDLE meathook_interface_v1_0_c_ifspec = (RPC_IF_HANDLE)& meathook_interface___RpcClientInterface;

extern const MIDL_STUB_DESC meathook_interface_StubDesc;

static RPC_BINDING_HANDLE meathook_interface__MIDL_AutoBindHandle;


void ExecuteConsoleCommand( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ unsigned char *pszString)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[0],
                  IDL_handle,
                  pszString);
    
}


void PushEntitiesFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ unsigned char *pBuffer,
    /* [in] */ boolean Start,
    /* [in] */ int Size)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[36],
                  IDL_handle,
                  pBuffer,
                  Start,
                  Size);
    
}


void UploadData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ int Size,
    /* [in] */ int Offset,
    /* [size_is][in] */ unsigned char *pBuffer)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[84],
                  IDL_handle,
                  Size,
                  Offset,
                  pBuffer);
    
}


void GetEntitiesFile( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[132],
                  IDL_handle,
                  Size,
                  pBuffer);
    
}


void GetActiveEncounter( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[174],
                  IDL_handle,
                  Size,
                  pBuffer);
    
}


void GetCurrentCheckpoint( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[216],
                  IDL_handle,
                  Size,
                  pBuffer);
    
}


void GetSpawnInfo( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size,
    /* [size_is][out] */ unsigned char *pBuffer)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[258],
                  IDL_handle,
                  Size,
                  pBuffer);
    
}


void KeepAlive( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *Size)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&meathook_interface_StubDesc,
                  (PFORMAT_STRING) &meathook_interface__MIDL_ProcFormatString.Format[300],
                  IDL_handle,
                  Size);
    
}


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const meathook_interface_MIDL_PROC_FORMAT_STRING meathook_interface__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure ExecuteConsoleCommand */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	0x42,		/* Oi2 Flags:  clt must size, has ext, */
			0x1,		/* 1 */
/* 20 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pszString */

/* 30 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 32 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 34 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Procedure PushEntitiesFile */

/* 36 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 38 */	NdrFcLong( 0x0 ),	/* 0 */
/* 42 */	NdrFcShort( 0x1 ),	/* 1 */
/* 44 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 46 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 48 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 50 */	NdrFcShort( 0xd ),	/* 13 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	0x42,		/* Oi2 Flags:  clt must size, has ext, */
			0x3,		/* 3 */
/* 56 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pBuffer */

/* 66 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 68 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 70 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Start */

/* 72 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 74 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 76 */	0x3,		/* FC_SMALL */
			0x0,		/* 0 */

	/* Parameter Size */

/* 78 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 80 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 82 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UploadData */

/* 84 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 86 */	NdrFcLong( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x2 ),	/* 2 */
/* 92 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 94 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 96 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 98 */	NdrFcShort( 0x10 ),	/* 16 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 102 */	0x42,		/* Oi2 Flags:  clt must size, has ext, */
			0x3,		/* 3 */
/* 104 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 106 */	NdrFcShort( 0x0 ),	/* 0 */
/* 108 */	NdrFcShort( 0x1 ),	/* 1 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 112 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Size */

/* 114 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 116 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Offset */

/* 120 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 122 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pBuffer */

/* 126 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 128 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 130 */	NdrFcShort( 0xa ),	/* Type Offset=10 */

	/* Procedure GetEntitiesFile */

/* 132 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 134 */	NdrFcLong( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x3 ),	/* 3 */
/* 140 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 142 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 144 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 146 */	NdrFcShort( 0x1c ),	/* 28 */
/* 148 */	NdrFcShort( 0x1c ),	/* 28 */
/* 150 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x2,		/* 2 */
/* 152 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 154 */	NdrFcShort( 0x1 ),	/* 1 */
/* 156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Size */

/* 162 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 164 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pBuffer */

/* 168 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 170 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 172 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Procedure GetActiveEncounter */

/* 174 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 176 */	NdrFcLong( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x4 ),	/* 4 */
/* 182 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 184 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 186 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 188 */	NdrFcShort( 0x1c ),	/* 28 */
/* 190 */	NdrFcShort( 0x1c ),	/* 28 */
/* 192 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x2,		/* 2 */
/* 194 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 196 */	NdrFcShort( 0x1 ),	/* 1 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Size */

/* 204 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 206 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 208 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pBuffer */

/* 210 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 212 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 214 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Procedure GetCurrentCheckpoint */

/* 216 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 222 */	NdrFcShort( 0x5 ),	/* 5 */
/* 224 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 226 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 230 */	NdrFcShort( 0x1c ),	/* 28 */
/* 232 */	NdrFcShort( 0x1c ),	/* 28 */
/* 234 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x2,		/* 2 */
/* 236 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 238 */	NdrFcShort( 0x1 ),	/* 1 */
/* 240 */	NdrFcShort( 0x0 ),	/* 0 */
/* 242 */	NdrFcShort( 0x0 ),	/* 0 */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Size */

/* 246 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 248 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 250 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pBuffer */

/* 252 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 254 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 256 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Procedure GetSpawnInfo */

/* 258 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x6 ),	/* 6 */
/* 266 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 268 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 270 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 272 */	NdrFcShort( 0x1c ),	/* 28 */
/* 274 */	NdrFcShort( 0x1c ),	/* 28 */
/* 276 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x2,		/* 2 */
/* 278 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 280 */	NdrFcShort( 0x1 ),	/* 1 */
/* 282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Size */

/* 288 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 290 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 292 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pBuffer */

/* 294 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 296 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 298 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Procedure KeepAlive */

/* 300 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 306 */	NdrFcShort( 0x7 ),	/* 7 */
/* 308 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 310 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 312 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 314 */	NdrFcShort( 0x1c ),	/* 28 */
/* 316 */	NdrFcShort( 0x1c ),	/* 28 */
/* 318 */	0x40,		/* Oi2 Flags:  has ext, */
			0x1,		/* 1 */
/* 320 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Size */

/* 330 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 332 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 334 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const meathook_interface_MIDL_TYPE_FORMAT_STRING meathook_interface__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/*  4 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x0,	/* FC_RP */
/*  8 */	NdrFcShort( 0x2 ),	/* Offset= 2 (10) */
/* 10 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x1 ),	/* 1 */
/* 14 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
/* 16 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 18 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 20 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 22 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 24 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 26 */	
			0x11, 0x0,	/* FC_RP */
/* 28 */	NdrFcShort( 0x2 ),	/* Offset= 2 (30) */
/* 30 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 32 */	NdrFcShort( 0x1 ),	/* 1 */
/* 34 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
/* 36 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 38 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 40 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */

			0x0
        }
    };

static const unsigned short meathook_interface_FormatStringOffsetTable[] =
    {
    0,
    36,
    84,
    132,
    174,
    216,
    258,
    300
    };


static const MIDL_STUB_DESC meathook_interface_StubDesc = 
    {
    (void *)& meathook_interface___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &meathook_interface__MIDL_AutoBindHandle,
    0,
    0,
    0,
    0,
    meathook_interface__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x801026e, /* MIDL Version 8.1.622 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

