// idMapFileEditor 0.1 by proteh
// -- edited by Scorp0rX0r 09/09/2020 - Remove file operations and work with streams only.
 
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
 
typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;
typedef signed __int64 int64;
typedef signed int int32;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint;
 
typedef int WINAPI OodLZ_CompressFunc(
    int codec, uint8* src_buf, size_t src_len, uint8* dst_buf, int level,
    void* opts, size_t offs, size_t unused, void* scratch, size_t scratch_size);
 
typedef int WINAPI OodLZ_DecompressFunc(uint8* src_buf, int src_len, uint8* dst, size_t dst_size,
    int fuzz, int crc, int verbose,
    uint8* dst_base, size_t e, void* cb, void* cb_ctx, void* scratch, size_t scratch_size, int threadPhase);
 
OodLZ_CompressFunc* OodLZ_Compress;
OodLZ_DecompressFunc* OodLZ_Decompress;

int DecompressEntities(std::istream *input, char** OutDecompressedData, size_t& OutSize) {
    // read uncompressed size
    //input->seekg(input->beg);
    uint64_t uncompressedSize;
    input->read((char*)&uncompressedSize, 8);
 
    // read compressed size
    size_t compressedSize;
    input->read((char*)&compressedSize, 8);
 
    // read all the compressed data and validate the file
    byte* compressedData = new byte[compressedSize];
 
    input->read((char*)compressedData, compressedSize);
    if (input->bad()) {
        OutputDebugString(L"Bad file, can't decompress.\n");
        return 1;
    }
 
    // decompress the data
    byte* decompressedData = new byte[uncompressedSize+4096];
 
    auto oodle = LoadLibraryA("./oo2core_8_win64.dll");
    if (oodle == nullptr) {
        OutputDebugString(L"Could not find oodle binary.\n");
        return 2;
    }

    OodLZ_Decompress = (OodLZ_DecompressFunc*)GetProcAddress(oodle, "OodleLZ_Decompress");
    if (OodLZ_Decompress == nullptr) {
        OutputDebugString(L"Could not find OodleLZ_Decompress in the oodle binary.\n");
        return 4;
    }
 
    // try to decompress
    if (OodLZ_Decompress(compressedData, (int)compressedSize, decompressedData, uncompressedSize, 1, 1, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0) != uncompressedSize) {
        OutputDebugString(L"Couldn't decompress %s. Bad file?\n");
        return 3;
    }
 
    *OutDecompressedData = (char*)decompressedData;
    OutSize = uncompressedSize;
    return 0;
}
 
int CompressEntities(const char* destFilename, byte* uncompressedData, size_t size) {
    // compress the data
    auto oodle = LoadLibraryA("./oo2core_8_win64.dll");
    if (oodle == nullptr) {
        OutputDebugString(L"Could not find oodle.\n");
        return 5;
    }

    OodLZ_Compress = (OodLZ_CompressFunc*)GetProcAddress(oodle, "OodleLZ_Compress");
    if (OodLZ_Compress == nullptr) {
        OutputDebugString(L"Could not find OodleLZ_Compress in the oodle binary.\n");
        return 6;
    }
 
    byte* compressedData = new byte[size + 65536];
    *(uint64*)compressedData = size;
 
    if (!compressedData) {
        OutputDebugString(L"Couldn't allocate memory for compression.\n");
        return 2;
    }
 
    int compressedSize = OodLZ_Compress(13, uncompressedData, size, compressedData, 4, 0, 0, 0, 0, 0);
 
    if (compressedSize < 0) {
        OutputDebugString(L"Compression failed.\n");
        return 3;
    }
 
    // write the compressed file
    FILE* dest;
 
    if (fopen_s(&dest, destFilename, "wb") != 0) {
        OutputDebugString(L"Failed to open %s for writing!\n");
        return 4;
    }
 
    // write uncompressed size
    fwrite(&size, 8, 1, dest);
 
    // write compressed size
    fwrite(&compressedSize, 8, 1, dest);
 
    // write compressed data
    fwrite(compressedData, 1, compressedSize, dest);
    //printf("Sucessfullty compressed %s into: %s\n", filename, destFilename);
    fclose(dest);
    return 0;
}

int CompressEntities(byte* uncompressedData, size_t size, char **output, size_t &outputSize)
{
    // compress the data
    auto oodle = LoadLibraryA("./oo2core_8_win64.dll");
    if (oodle == nullptr) {
        OutputDebugString(L"Could not find oodle.\n");
        return 5;
    }

    OodLZ_Compress = (OodLZ_CompressFunc*)GetProcAddress(oodle, "OodleLZ_Compress");
    if (OodLZ_Compress == nullptr) {
        OutputDebugString(L"Could not find OodleLZ_Compress in the oodle binary.\n");
        return 6;
    }

    byte* compressedData = new byte[size + 65536];
    ((uint64*)compressedData)[3] = size;
    if (!compressedData) {
        OutputDebugString(L"Couldn't allocate memory for compression.\n");
        return 2;
    }

    int compressedSize = OodLZ_Compress(13, uncompressedData, size, (compressedData + 16), 4, 0, 0, 0, 0, 0);
    if (compressedSize < 0) {
        OutputDebugString(L"Compression failed.\n");
        return 3;
    }

    ((uint64*)compressedData)[0] = size;
    ((uint64*)compressedData)[1] = compressedSize;
    *output = (char*)compressedData;
    outputSize = compressedSize + 16;
    return 0;
}