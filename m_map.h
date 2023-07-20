//
//  Map.h
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#ifndef Map_h
#define Map_h

#include "array.h"
#include "wad.h"
#include "m_vertex.h"
#include "m_line.h"
#include "m_thing.h"

#include <stdbool.h>

#define BOUNDS_BORDER 128
#define MAP_LABEL_LENGTH 6

typedef struct
{
    char label[MAP_LABEL_LENGTH]; // "ExMx" or "MAPxx" + \0
    Array * vertices;
    Array * lines;
    Array * things;

    SDL_Rect bounds;
    bool boundsDirty;
} Map;

extern Map map;

/// Create a new, blank map.
void CreateMap(const char * label);

/// Load map from WAD lump.
bool LoadMap(const Wad * wad, const char * lumpLabel);

/// Run a diagnostic to check for problems.
void CheckMap(void);

/// Load map from .dwd file.
void LoadDWD(const char * mapName);

/// Save the currently loaded map to .dwd
void SaveDWD(void);

bool VertexOnLine(const Vertex * vertex, const Line * line);
SDL_Rect GetMapBounds(void);
void TranslateCoord(int * y, const SDL_Rect * bounds);
Line * NewLine(const Line * data, const SDL_Point * p1, const SDL_Point * p2);
void SplitLine(Line * line, const SDL_Point * gridPoint);
Thing * NewThing(const Thing * thing, const SDL_Point * point);
void FlipSelectedLines(void);
void MergeVertices(void);
bool GetClosestSide(const SDL_Point * point, Sidedef * out);

#endif /* Map_h */
