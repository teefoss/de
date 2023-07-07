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
    char identifer[4]; // "IWAD" or "PWAD"
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
    enum { PWAD, IWAD, UNKNOWN_WAD } type;
    FILE * stream;
    Array * directory;
} Wad;

Wad * CreateWad(const char * path); // TODO: create with 'PWAD' id
Wad * OpenWad(const char * path); // TODO: set .type member
void FreeWad(Wad * wad);

void ListDirectory(const Wad * wad);
void WriteDirectory(const Wad * wad); // TODO: use .type

void AddLump(const Wad * wad, const char * name, void * data, u32 size);
void RemoveLumpNumber(const Wad * wad, int index);
void RemoveLumpNamed(const Wad * wad, const char * name);
void RemoveMap(const Wad * wad, const char * mapLabel);

WadInfo GetWadInfo(const Wad * wad);
const char * GetWadType(const Wad * wad); // TODO: remove
int GetLumpIndexFromName(const Wad * wad, const char * name);
void * GetLumpWithIndex(const Wad * wad, int index);
void * GetLumpWithName(const Wad * wad, const char * name);
u32 GetLumpSize(const Wad * wad, int index);
const char * GetLumpName(const Wad * wad, int index);

#endif /* WadFile_h */
