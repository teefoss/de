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

    WadInfo info;
    memset(&info, 0, sizeof(info));
    fwrite(&info, sizeof(info), 1, stream);

    return true;
}

bool WadFile::open(const char * path)
{
    stream = fopen(path, "r+");

    if ( stream == nullptr )
        return false;

    WadInfo wadInfo = getInfo();

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

WadInfo WadFile::getInfo()
{
    WadInfo info;
    fread(&info, sizeof(info), 1, stream);
    info.lumpCount = SWAP32(info.lumpCount);
    info.directoryOffset = SWAP32(info.directoryOffset);

    return info;
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

void WadFile::addLump(const char * name, void * data, u32 size)
{
    LumpInfo lump = { 0 };

    strncpy(lump.name, name, 8);
    for ( int i = 0; i < 8; i++ ) {
        lump.name[i] = toupper(lump.name[i]);
    }

    WadInfo info = getInfo();

    if ( info.directoryOffset == 0 ) {
        // There's no directory yet, add to end of file.
        fseek(stream, 0, SEEK_END);
    } else {
        // There's already a directory. The directory starts right after the
        // last lump, so add the new lump here, then rewrite the directory.
        fseek(stream, info.directoryOffset, SEEK_SET);
    }

    lump.offset = (u32)ftell(stream);
    lump.size = size;
    directory.append(lump);
    fwrite(data, 1, size, stream);

    writeDirectory((u32)ftell(stream)); // Rewrite.
}

void WadFile::writeDirectory(u32 offset)
{
    for ( int i = 0; i < directory.count(); i++ ) {
        directory[i].offset = SWAP32(directory[i].offset);
        directory[i].size = SWAP32(directory[i].size);
    }

    // Write the directory.
    LumpInfo * directoryStart = &directory[0];
    fseek(stream, offset, SEEK_SET);
    fwrite(directoryStart, sizeof(LumpInfo), directory.count(), stream);

    for ( int i = 0; i < directory.count(); i++ )
    {
        directory[i].offset = SWAP32(directory[i].offset);
        directory[i].size = SWAP32(directory[i].size);
    }

    // Write the header.
    WadInfo info;
    info.directoryOffset = SWAP32(offset);
    info.lumpCount = SWAP32(directory.count());
    rewind(stream);
    fwrite(&info, sizeof(info), 1, stream);
}

int WadFile::getLumpIndex(const char * name)
{
    char capitalized[9] = { 0 };

    for ( int i = 0; i < 8; i++ )
        capitalized[i] = toupper(name[i]);

    for ( int i = directory.count() - 1; i >= 0; i-- )
        if ( STRNEQ(directory[i].name, capitalized, 8) )
            return i;

    return -1;
}

void * WadFile::getLump(int index)
{
    void * buffer = malloc(directory[index].size);
    fseek(stream, directory[index].offset, SEEK_SET);
    fread(buffer, directory[index].size, 1, stream);

    return buffer;
}

void * WadFile::getLump(const char * name)
{
    return getLump(getLumpIndex(name));
}

u32 WadFile::getLumpSize(int index)
{
    return directory[index].size;
}

u32 WadFile::getLumpSize(const char * name)
{
    return getLumpSize(getLumpIndex(name));
}

const char * WadFile::getLumpName(int index)
{
    return directory[index].name;
}
