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

static const char * wadTypeNames[] = { "PWAD", "IWAD", "ERR" };

void FreeWad(Wad * wad)
{
    Lump * lump = wad->lumps->data;
    for ( int i = 0; i < wad->lumps->count; i++, lump++ )
        free(lump->data);

    FreeArray(wad->lumps);
    free(wad);
}

Wad * OpenOrCreateWad(const char * path)
{
    Wad * wad = OpenWad(path);
    if ( wad != NULL )
        return wad;

    return CreateWad(path);
}

Wad * CreateWad(const char * path)
{
    Wad * wad = calloc(1, sizeof(*wad));

    if ( wad )
    {
        strncpy(wad->path, path, sizeof(wad->path));
        wad->type = PWAD;
        wad->lumps = NewArray(0, sizeof(Lump), 1);

        printf("Created WAD '%s'\n", path);
    }

    return wad;
}

Wad * OpenWad(const char * path)
{
    Wad * wad = calloc(1, sizeof(*wad));
    if ( wad == NULL )
    {
        printf("Error: ran out of memory trying to load WAD %s!\n", path);
        return NULL;
    }

    strncpy(wad->path, path, sizeof(wad->path));

    FILE * stream = fopen(path, "r");
    if ( stream == NULL )
    {
        // Don't error message as it's acceptible to not find a WAD.
        goto error;
    }

    // Check the header.
    WadInfo header;
    fread(&header, sizeof(header), 1, stream);

    if ( strncmp(header.identifer, "IWAD", 4) == 0 )
        wad->type = IWAD;
    else if ( strncmp(header.identifer, "PWAD", 4) == 0 )
        wad->type = PWAD;
    else
    {
        printf("Error: 'WAD %s' has unknown type '%.*s'!",
               path, 4, header.identifer);
        goto error;
    }

    wad->lumps = NewArray(header.lumpCount, sizeof(Lump), 1);

    // Load the directory.
    fseek(stream, header.directoryOffset, SEEK_SET);
    int count = header.lumpCount;
    DirectoryEntry * directory = malloc(count * sizeof(*directory));
    fread(directory, sizeof(*directory), count, stream);

    // Load all lump info and data from WAD.
    for ( int i = 0; i < count; i++ )
    {
        Lump lump = { 0 };

        strncpy(lump.name, directory[i].name, 8);
        lump.offset = SWAP32(directory[i].offset);
        lump.size = SWAP32(directory[i].size);
        if ( lump.size )
        {
            lump.data = malloc(lump.size);
            fseek(stream, lump.offset, SEEK_SET);
            fread(lump.data, lump.size, 1, stream);
        }

        Push(wad->lumps, &lump);
    }

    free(directory);
    fclose(stream);

    return wad;
error:
    if ( wad )
        free(wad);
    return NULL;
}

void ListDirectory(const Wad * wad)
{
    int count = wad->lumps->count;

    printf("type: %s\n", wadTypeNames[wad->type]);
    printf("lump count: %d\n", count);
    printf("lumps:\n");

    Lump * lump = wad->lumps->data;
    for ( int i = 0; i < count; i++, lump++ )
        printf("%3d: %s, %d bytes\n", i, lump->name, lump->size);
}

// TODO: InsertLump(index)

/// Add lump to WAD at `wad->position`.
void AddLump(Wad * wad, const char * name, void * data, u32 size)
{
    Lump lump = { 0 };

    strncpy(lump.name, name, 8);
    lump.name[8] = '\0';
    SDL_strupr(lump.name);

    lump.size = size;
    if ( data != NULL && size > 0 )
    {
        lump.data = malloc(size);
        memcpy(lump.data, data, size);
    }

    Insert(wad->lumps, &lump, wad->position);
    wad->position++;
}

/// Append new lump to WAD. Does not change the WAD position.
void AppendLump(Wad * wad, const char * name, void * data, u32 size)
{
    int oldPosition = wad->position;
    wad->position = wad->lumps->count;

    AddLump(wad, name, data, size);

    wad->position = oldPosition;
}

int GetIndexOfLumpNamed(const Wad * wad, const char * name)
{
    char capitalized[9] = { 0 };
    strncpy(capitalized, name, 8);
    SDL_strupr(capitalized);

    Lump * lump = wad->lumps->data;
    for ( int i = 0; i < wad->lumps->count; i++, lump++ )
    {
        if ( strncmp(lump->name, capitalized, 8) == 0 )
            return i;
    }

    return -1;
}

void RemoveLumpNumber(const Wad * wad, int index)
{
    Lump * lump = Get(wad->lumps, index);
    if ( lump->data) // Some lumps have no data (labels)
        free(lump->data);

    Remove(wad->lumps, index);
}

void RemoveLumpNamed(const Wad * wad, const char * name)
{
    int index = GetIndexOfLumpNamed(wad, name);

    if ( index != -1 )
        RemoveLumpNumber(wad, index);

    printf("Error: could not remove lump '%s'! (Not found)\n", name);
}

void RemoveMap(const Wad * wad, const char * mapLabel)
{
    int index = GetIndexOfLumpNamed(wad, mapLabel);
    if ( index == -1 )
    {
        printf("Error: no map '%s' in WAD '%s'!\n", mapLabel, wad->path);
        return;
    }

    int i = ML_COUNT;

    // Remove at `index` each time because each call to Remove (array member)
    // moves all members down one.
    while ( i-- )
        RemoveLumpNumber(wad, index);
}

void CopyLump(Wad * destination, const Wad * source, int lumpIndex)
{
    Lump * lump = GetLump(source, lumpIndex);
    AddLump(destination, lump->name, lump->data, lump->size);
    SaveWAD(destination);
}

void SaveWAD(const Wad * wad)
{
    FILE * output = fopen(wad->path, "w");
    if ( output == NULL )
    {
        printf("Error: WAD save failed!\n");
        return;
    }

    // Write all lump data.
    fseek(output, sizeof(WadInfo), SEEK_SET); // Leave room for the header.
    for ( int i = 0; i < wad->lumps->count; i++ )
    {
        Lump * lump = Get(wad->lumps, i);
        lump->offset = (u32)ftell(output); // Take note for later.
        fwrite(lump->data, lump->size, 1, output);
    }

    WadInfo header = { 0 };
    strncpy(header.identifer, wadTypeNames[wad->type], 4);
    header.lumpCount = wad->lumps->count;
    header.directoryOffset = (u32)ftell(output);

    // Write the directory.
    for ( int i = 0; i < wad->lumps->count; i++ )
    {
        Lump * lump = Get(wad->lumps, i);

        // Make an entry from this lump.
        DirectoryEntry entry;
        entry.offset = lump->offset;
        entry.size = lump->size;
        strncpy(entry.name, lump->name, 8);

        fwrite(&entry, sizeof(entry), 1, output);
    }

    // Write the header.
    rewind(output);
    fwrite(&header, sizeof(header), 1, output);
    fclose(output);
}

Lump * GetLumpNamed(const Wad * wad, const char * name)
{
    int index = GetIndexOfLumpNamed(wad, name);

    if ( index != -1 )
        return Get(wad->lumps, index);

    return NULL;
}

Lump * GetLump(const Wad * wad, int i)
{
    return Get(wad->lumps, i);
}

const char * GetNameOfLump(const Wad * wad, int index)
{
    Lump * lump = Get(wad->lumps, index);
    if ( lump )
        return lump->name;

    return NULL;
}
