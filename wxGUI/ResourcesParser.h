#pragma once

#pragma pack(push, 1)
typedef struct _IDCL_HEADER {
    // idstring "IDCL"
    long IdString;
    // get VER long  # 12
    long Version;
    // get DUMMY longlong
    long long Dummy;
    // get ZERO long
    long Zero;
    // get DUMMY long  # 1
    // get DUMMY long  #  - 1
    // get DUMMY long
    long unk[3];
    // #get DUMMY longlong
    // get FILES long
    long FileCount;
    // get DUMMY_NUM long
    // get DUMMY2_NUM long
    long unk1[2];
    // get FILES_2 long
    long IndexCount;
    // get ZERO longlong
    long long Zero2;
    // get DUMMY2 longlong
    long long unk2;
    // get NAMES_OFF longlong
    long long NamesOffset;
    // get DUMMY4_OFF longlong
    long long StringTableEnd;
    // get INFO_OFF longlong
    long long InfoOffset;
    // get DUMMY6_OFF longlong
    long long unk4;
    // get DUMMY7_OFF longlong
    long long FileOffset;
    // get DATA_OFF longlong
    long long DataOffset;
} IDCL_HEADER;

typedef struct _IDCL_FILE_DESCRIPTOR {
    // get ZERO longlong
    long long Zero;
    // get DUMMY longlong      # 1
    long long unk0;
    // get DUMMY longlong      #  - 1
    long long unk1;
    // get TYPE_ID longlong
    unsigned long long TypeId;
    // get NAME_ID longlong
    unsigned long long NameId;
    // get ZERO longlong
    long long Zero1;
    // get ZERO longlong
    long long Zero2;
    // get OFFSET longlong
    long long Offset;
    // get ZSIZE longlong
    long long CompressedSize;
    // get SIZE longlong
    long long UncompressedSize;
    // get DUMMY longlong
    long long unk2;
    // get DUMMY long
    long unk3;
    // get DUMMY long
    long unk4;
    // get DUMMY longlong
    long long unk5;
    // get DUMMY long          # ZIP related ?
    long unk6;
    // get DUMMY long          # ZIP related ?
    long unk7;
    // get ZIP_FLAGS longlong  # 0, 2, 4, 0x210002, 0x410002 and so on
    long long ZipFlags;
    // get ZERO longlong
    long long Zero3;
    // get FLAGS long          # 2
    long Flags;
    // get FLAGS2 long
    long Flags2;
    // get ZERO longlong
    long long Zero4;
} IDCL_FILE_DESCRIPTOR;

#pragma pack (pop)

#include <vector>
#include <string>

typedef struct _IDCT_FILE {
    long long Offset;
    unsigned long long Size;
    long long SizeUncompressed;
    long long NameOffset;
    long long TypeIdOffset;
    long long NameIdOffset;
    long long InfoOffset;
    IDCL_FILE_DESCRIPTOR FileInfo;
    std::string FileName;
    std::string TypeName;
} IDCT_FILE;

typedef struct _IDCT_STRING {
    long long NameOffset;
    std::string String;
} IDCT_STRING;

class IDCLReader {
    FILE* m_Handle;
    std::vector<IDCT_FILE> m_FileList;
    std::vector<IDCT_STRING> m_IdclStrings;
    bool ParseFileList();

public:
    std::string m_FileName;
    bool OpenFile(const char* FileName);
    std::vector<IDCT_FILE> GetFileList() { return m_FileList; }
    bool ReadFile(size_t Index, char* Buffer, size_t BufferSize);
    bool ReadFile(IDCT_FILE& File, char* Buffer, size_t Size);
    bool WriteFile(IDCT_FILE& File, char* Buffer, size_t BufferSize);
};