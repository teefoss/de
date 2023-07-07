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
} DirectoryEntry;

typedef struct
{
    int offset;
    int size;
    char name[9];
    void * data;
} Lump;

typedef struct {
    char path[256];
    enum { PWAD, IWAD, UNKNOWN_WAD } type;
    Array * lumps; // Lump[]
    int position; // The index at which lumps are added with `AddLump`.
} Wad;

Wad * CreateWad(const char * path);
Wad * OpenWad(const char * path);
Wad * OpenOrCreateWad(const char * path);
void  SaveWAD(const Wad * wad);
void  FreeWad(Wad * wad);

void ListDirectory(const Wad * wad);

void AddLump(Wad * wad, const char * name, void * data, u32 size);
void RemoveLumpNumber(const Wad * wad, int index);
void RemoveLumpNamed(const Wad * wad, const char * name);
void RemoveMap(const Wad * wad, const char * mapLabel);

int GetIndexOfLumpNamed(const Wad * wad, const char * name);
const char * GetNameOfLump(const Wad * wad, int index);
Lump * GetLumpNamed(const Wad * wad, const char * name);
Lump * GetLump(const Wad * wad, int i);

#endif /* WadFile_h */
