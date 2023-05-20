//
//  WadFile.h
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#ifndef WadFile_hpp
#define WadFile_hpp

#include "Storage.h"
#include "defines.h"
#include <stdio.h>

struct WadInfo // aka WAD Header
{
    u8 identifer[4] = { 0 }; // "IWAD" or "PWAD"
    u32 lumpCount = 0;
    u32 directoryOffset = 0;
};

struct LumpInfo // aka directory (aka info table) entry
{
    u32 offset;     // Offset within WAD file.
    u32 size;       // Size in bytes.
    char name[8];   // Lump's name (not NUL-terminated)
};

struct WadFile {
    FILE * stream = NULL;
    Storage<LumpInfo> directory;

    ~WadFile();

    bool create(const char * path);
    bool open(const char * path);

    void listDirectory();
    void writeDirectory(u32 offset);
    void addLump(const char * name, void * data, u32 size);
    void removeLump(const char * name);

    WadInfo         getInfo();
    const char *    getType();
    int             getLumpIndex(const char * name);
    void *          getLump(int index);
    void *          getLump(const char * name);
    u32             getLumpSize(int index);
    u32             getLumpSize(const char * name);
    const char *    getLumpName(int index);
};

#endif /* WadFile_h */
