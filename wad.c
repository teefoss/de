//
//  WadFile.cc
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "wad.h"

#include "common.h"
#include "doomdata.h"

#include <stdio.h>
#include <string.h>

void FreeWad(Wad * wad)
{
    fclose(wad->stream);
    FreeArray(wad->directory);
    free(wad);
}

Wad * CreateWad(const char * path)
{
    Wad * wad = malloc(sizeof(*wad));

    if ( wad )
    {
        wad->stream = fopen(path, "w+");

        if ( wad->stream == NULL )
            return NULL;

        WadInfo info = { 0 };
        fwrite(&info, sizeof(info), 1, wad->stream);

        wad->directory = NewArray(0, sizeof(LumpInfo), 1);

        printf("Created WAD '%s'\n", path);
    }

    return wad;
}

Wad * OpenWad(const char * path)
{
    Wad * wad = malloc(sizeof(*wad));

    if ( wad )
    {
        wad->stream = fopen(path, "r+");

        if ( wad->stream == NULL )
            return NULL;

        WadInfo wadInfo = GetWadInfo(wad);

        // Load the directory.
        fseek(wad->stream, wadInfo.directoryOffset, SEEK_SET);
        wad->directory = NewArray(wadInfo.lumpCount, sizeof(LumpInfo), 1);

        for ( u32 i = 0; i < wadInfo.lumpCount; i++ )
        {
            LumpInfo lumpInfo;
            fread(&lumpInfo, sizeof(lumpInfo), 1, wad->stream);
            lumpInfo.offset = SWAP32(lumpInfo.offset);
            lumpInfo.size = SWAP32(lumpInfo.size);

            Push(wad->directory, &lumpInfo);
        }
    }

    return wad;
}

const char * GetWadType(const Wad * wad)
{
    char string[4];
    fread(string, sizeof(char), 4, wad->stream);

    if ( strncmp(string, "IWAD", 4) )
        return "IWAD";
    else if ( strncmp(string, "PWAD", 4) )
        return "PWAD";
    else
        return "unknown WAD type";
}

WadInfo GetWadInfo(const Wad * wad)
{
    WadInfo info;
    rewind(wad->stream);
    fread(&info, sizeof(info), 1, wad->stream);
    info.lumpCount = SWAP32(info.lumpCount);
    info.directoryOffset = SWAP32(info.directoryOffset);

    return info;
}

void ListDirectory(const Wad * wad)
{
    size_t totalSize = 0;
    int count = wad->directory->count;
    printf("type: %s\n", GetWadType(wad));
    printf("lump count: %d\n", count);
    printf("lumps:\n");

    LumpInfo * info = wad->directory->data;
    for ( int i = 0; i < count; i++, info++ ) {
        char name[9] = { 0 };
        strncpy(name, info->name, 8);
        printf("%3d: %s, %d bytes\n", i, name, info->size);
        totalSize += info->size;
    }

    printf("total size: %zu bytes\n",
           totalSize + sizeof(WadInfo) + sizeof(LumpInfo) * count);
}

/// Append new lump to WAD. The directory will need to be updated.
void AddLump(const Wad * wad, const char * name, void * data, u32 size)
{
    LumpInfo lump = { 0 };

    strncpy(lump.name, name, 8);
    for ( int i = 0; i < 8; i++ )
        lump.name[i] = toupper(lump.name[i]);

    int count = wad->directory->count;

    if ( count == 0 ) // No lumps yet, position at end of file.
    {
        fseek(wad->stream, 0, SEEK_END);
    }
    else
    {
        // Find the position just after the last lump. Can't just use the
        // directory offset since it may have been invalidated!
        LumpInfo * last = Get(wad->directory, count - 1);
        fseek(wad->stream, last->offset + last->size, SEEK_SET);
    }

    lump.offset = (u32)ftell(wad->stream);
    lump.size = size;
    Push(wad->directory, &lump);
    fwrite(data, size, 1, wad->stream);
}

void RemoveLumpNumber(const Wad * wad, int index)
{
    LumpInfo * directory = wad->directory->data;

    char name[9] = { 0 };
    strncpy(name, directory[index].name, 8);

    for ( int i = index; i < wad->directory->count - 1; i++ ) {
        LumpInfo * currentEntry = &directory[i];
        LumpInfo * nextEntry = &directory[i + 1];

        // Set position to current lump and move the next lump to it.
        fseek(wad->stream, currentEntry->offset, SEEK_SET);
        void * nextLump = GetLumpWithIndex(wad, i + 1);
        fwrite(nextLump, nextEntry->size, 1, wad->stream);
        free(nextLump);

        currentEntry->size = nextEntry->size;
        strncpy(currentEntry->name, nextEntry->name, 8);
    }

    printf("Removed lump %d (%s)\n", index, name);

//    Remove(wad->directory, index);
    wad->directory->count--; // Remove last entry, as it was moved up one.
//    WriteDirectory(wad);
}

void RemoveLumpNamed(const Wad * wad, const char * name)
{
    int index = GetLumpIndexFromName(wad, name);
    RemoveLumpNumber(wad, index);
    WriteDirectory(wad);
}

void RemoveMap(const Wad * wad, const char * mapLabel)
{
    int index = GetLumpIndexFromName(wad, mapLabel);
    int i = ML_COUNT;

    // TODO: Don't use RemoveLumpNumber, it moves all lumps up each time.
    while ( i-- )
        RemoveLumpNumber(wad, index);

    WriteDirectory(wad);

//    printf("post-RemoveMap directory:\n");
//    ListDirectory(wad);
}

/// Write the WAD's directory after the last lump and update the header.
void WriteDirectory(const Wad * wad)
{
    LumpInfo * directory = wad->directory->data;
    u32 count = wad->directory->count;

    for ( u32 i = 0; i < count; i++ )
    {
        directory[i].offset = SWAP32(directory[i].offset);
        directory[i].size = SWAP32(directory[i].size);
    }

    // Write the directory.

    LumpInfo * last = Get(wad->directory, wad->directory->count - 1);
    u32 directoryStart = last->offset + last->size;
    fseek(wad->stream, directoryStart, SEEK_SET);
    fwrite(directory, sizeof(LumpInfo), count, wad->stream);

    for ( u32 i = 0; i < count; i++ )
    {
        directory[i].offset = SWAP32(directory[i].offset);
        directory[i].size = SWAP32(directory[i].size);
    }

    // Write the header.

    WadInfo info;

    if ( wad->type == IWAD )
        strncpy(info.identifer, "IWAD", 4);
    else
        strncpy(info.identifer, "PWAD", 4);

    info.directoryOffset = SWAP32(directoryStart);
    info.lumpCount = SWAP32(count);

    rewind(wad->stream);
    fwrite(&info, sizeof(info), 1, wad->stream);
}

int GetLumpIndexFromName(const Wad * wad, const char * name)
{
    char capitalized[9] = { 0 };

    for ( int i = 0; i < 8; i++ )
        capitalized[i] = toupper(name[i]);

    LumpInfo * directory = wad->directory->data;
    for ( int i = wad->directory->count - 1; i >= 0; i-- )
        if ( STRNEQ(directory[i].name, capitalized, 8) )
            return i;

    return -1;
}

void * GetLumpWithIndex(const Wad * wad, int index)
{
    if ( index < 0 )
        return NULL;
    
    LumpInfo * directory = wad->directory->data;

    void * buffer = malloc(directory[index].size);
    fseek(wad->stream, directory[index].offset, SEEK_SET);
    fread(buffer, directory[index].size, 1, wad->stream);

    return buffer;
}

void * GetLumpWithName(const Wad * wad, const char * name)
{
    int index = GetLumpIndexFromName(wad, name);
    if ( index != -1 )
        return GetLumpWithIndex(wad, index);

    return NULL;
}

u32 GetLumpSize(const Wad * wad, int index)
{
    LumpInfo * directory = wad->directory->data;
    return directory[index].size;
}

const char * GetLumpName(const Wad * wad, int index)
{
    LumpInfo * directory = wad->directory->data;
    return directory[index].name; // TODO: check this works
}
