//
//  WadFile.cc
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "WadFile.h"

#include "defines.h"

#include <stdio.h>
#include <string.h>

struct WadInfo
{
    u8 identifer[4] = { 0 }; // "IWAD" or "PWAD"
    u32 lumpCount = 0;
    u32 directoryOffset = 0;
};

WadFile::~WadFile()
{
    fclose(stream);
}

bool WadFile::create(const char * path)
{
    stream = fopen(path, "w+");

    if ( stream == nullptr )
    {
        return false;
    }

    WadInfo wadInfo;
    fwrite(&wadInfo, sizeof(wadInfo), 1, stream);

    return true;
}

bool WadFile::open(const char * path)
{
    stream = fopen(path, "r+");

    if ( stream == nullptr )
    {
        return false;
    }

    // Read the header.
    WadInfo wadInfo;
    fread(&wadInfo, sizeof(wadInfo), 1, stream);
    wadInfo.lumpCount = SWAP32(wadInfo.lumpCount);
    wadInfo.directoryOffset = SWAP32(wadInfo.directoryOffset);

    // Load the directory.
    fseek(stream, wadInfo.directoryOffset, SEEK_SET);
    directory.resize(wadInfo.lumpCount);

    for ( int i = 0; i < wadInfo.lumpCount; i++ )
    {
        LumpInfo lumpInfo;
        fread(&lumpInfo, sizeof(lumpInfo), 1, stream);
        lumpInfo.offset = SWAP32(lumpInfo.offset);
        lumpInfo.size = SWAP32(lumpInfo.size);
        directory.append(lumpInfo);
    }
    
    return true;
}

const char * WadFile::getType()
{
    char string[4];
    fread(string, sizeof(char), 4, stream);

    if ( strncmp(string, "IWAD", 4) )
    {
        return "IWAD";
    }
    else if ( strncmp(string, "PWAD", 4) )
    {
        return "PWAD";
    }
    else
    {
        return "unknown WAD type";
    }
}

void WadFile::listDirectory()
{
    printf("type: %s\n", getType());
    printf("lump count: %d\n", directory.count());
    printf("lumps:\n");

    for ( int i = 0; i < directory.count(); i++ ) {
        char name9[9] = { 0 };
        strncpy(name9, directory[i].name, 8);
        printf("%3d: %s, %d bytes\n", i, name9, directory[i].size);
    }
}
