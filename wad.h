//
//  WadFile.h
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#ifndef WadFile_hpp
#define WadFile_hpp

#include "array.h"
#include "common.h"
#include <stdio.h>

typedef struct
{
    u32 numTextures;
    u32 textureOffsets;
} TextureHeader;

typedef struct
{
    u8 identifer[4]; // "IWAD" or "PWAD"
    u32 lumpCount;
    u32 directoryOffset;
} WadInfo; // aka WAD Header

typedef struct
{
    u32 offset;     // Offset within WAD file.
    u32 size;       // Size in bytes.
    char name[8];   // Lump's name (not NUL-terminated)
} LumpInfo; // aka directory (aka info table) entry

typedef struct {
    FILE * stream;
    Array * directory;
} Wad;

Wad * CreateWad(const char * path);
Wad * OpenWad(const char * path);
void FreeWad(Wad * wad);

void ListDirectory(const Wad * wad);
void WriteDirectory(const Wad * wad, u32 offset);
void AddLump(Wad * wad, const char * name, void * data, u32 size);
void RemoveLump(const Wad * wad, const char * name);
WadInfo GetWadInfo(const Wad * wad);
const char * GetWadType(const Wad * wad);
int GetLumpIndex(const Wad * wad, const char * name);
void * GetLumpWithIndex(const Wad * wad, int index);
void * GetLumpWithName(const Wad * wad, const char * name);
u32 GetLumpSize(const Wad * wad, int index);
const char * GetLumpName(const Wad * wad, int index);

#endif /* WadFile_h */
