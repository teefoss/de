//
//  WadFile.cc
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "wad.h"

#include "common.h"

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

        for ( int i = 0; i < wadInfo.lumpCount; i++ )
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
    printf("type: %s\n", GetWadType(wad));
    printf("lump count: %d\n", wad->directory->count);
    printf("lumps:\n");

    LumpInfo * info = wad->directory->data;
    for ( int i = 0; i < wad->directory->count; i++, info++ ) {
        char name[9] = { 0 };
        strncpy(name, info->name, 8);
        printf("%3d: %s, %d bytes\n", i, name, info->size);
    }
}

void AddLump(const Wad * wad, const char * name, void * data, u32 size)
{
    LumpInfo lump = { 0 };

    strncpy(lump.name, name, 8);
    for ( int i = 0; i < 8; i++ )
        lump.name[i] = toupper(lump.name[i]);

    WadInfo info = GetWadInfo(wad);

    if ( info.directoryOffset == 0 ) {
        // There's no directory yet, add to end of file.
        fseek(wad->stream, 0, SEEK_END);
    } else {
        // There's already a directory. The directory starts right after the
        // last lump, so add the new lump here, then rewrite the directory.
        fseek(wad->stream, info.directoryOffset, SEEK_SET);
    }

    lump.offset = (u32)ftell(wad->stream);
    lump.size = size;
    Push(wad->directory, &lump);
    fwrite(data, size, 1, wad->stream);

    WriteDirectory(wad, (u32)ftell(wad->stream)); // Rewrite.
}

void WriteDirectory(const Wad * wad, u32 offset)
{
    LumpInfo * directory = wad->directory->data;
    u32 count = wad->directory->count;

    for ( int i = 0; i < count; i++ )
    {
        directory[i].offset = SWAP32(directory[i].offset);
        directory[i].size = SWAP32(directory[i].size);
    }

    // Write the directory.
    fseek(wad->stream, offset, SEEK_SET);
    fwrite(directory, sizeof(LumpInfo), count, wad->stream);

    for ( int i = 0; i < count; i++ )
    {
        directory[i].offset = SWAP32(directory[i].offset);
        directory[i].size = SWAP32(directory[i].size);
    }

    // Write the header.
    WadInfo info;
    info.directoryOffset = SWAP32(offset);
    info.lumpCount = SWAP32(count);
    rewind(wad->stream);
    fwrite(&info, sizeof(info), 1, wad->stream);
}

int GetLumpIndex(const Wad * wad, const char * name)
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
    LumpInfo * directory = wad->directory->data;

    void * buffer = malloc(directory[index].size);
    fseek(wad->stream, directory[index].offset, SEEK_SET);
    fread(buffer, directory[index].size, 1, wad->stream);

    return buffer;
}

void * GetLumpWithName(const Wad * wad, const char * name)
{
    int index = GetLumpIndex(wad, name);
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

void RemoveLump(const Wad * wad, const char * name)
{
    int removeIndex = GetLumpIndex(wad, name);

    LumpInfo * directory = wad->directory->data;

    for ( int i = removeIndex; i < wad->directory->count - 1; i++ ) {
        LumpInfo * currentEntry = &directory[i];
        LumpInfo * nextEntry = &directory[i + 1];

        // Set position to current lump and move the next lump to it.
        fseek(wad->stream, currentEntry->offset, SEEK_SET);
        void * nextLump = GetLumpWithIndex(wad, i + 1);
        fwrite(nextLump, nextEntry->size, 1, wad->stream);
        free(nextLump);

        // Set the next entry's offset to current.
        nextEntry->offset = currentEntry->offset;
    }

    // Update the directory.
    Remove(wad->directory, removeIndex);
    u32 directoryOffset = (u32)ftell(wad->stream);
    WriteDirectory(wad, directoryOffset);

    // Update the header.
    WadInfo info = GetWadInfo(wad);
    info.lumpCount = SWAP32(wad->directory->count);
    info.directoryOffset = SWAP32(directoryOffset);
    rewind(wad->stream);
    fwrite(&info, sizeof(info), 1, wad->stream);
}
