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

struct LumpInfo
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
    const char * getType();
    void listDirectory();
};

#endif /* WadFile_h */
