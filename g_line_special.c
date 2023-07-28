//
//  g_line_special.c
//  de
//
//  Created by Thomas Foster on 7/27/23.
//

#include "g_line_special.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LineSpecial * specials;
int numSpecials;
int numCategories = 0;
SpecialCategory categories[NUM_SPECIAL_CATEGORIES];


/// Load specials from .dsp file.
void LoadSpecials(const char * path)
{
//    const char * path = "doom_dsp/linespecials.dsp";
    FILE * file = fopen(path, "r");
    if ( file == NULL ) {
        fprintf(stderr, "Error: could not find '%s'!\n", path);
        return;
    }

    fscanf(file, "numspecials: %d\n", &numSpecials);
    specials = calloc(numSpecials, sizeof(*specials));

    int longestName = 0;

    for ( int i = 0; i < numSpecials; i++ )
    {
        int id;
        fscanf(file, "%d:", &id);
        specials[i].id = id;

        fscanf(file, "%s\n", specials[i].name);

        int len = (int)strlen(specials[i].name);
        if ( len > longestName )
            longestName = len;

        // Extract the category name.
        char category[80] = { 0 };
        strncpy(category, specials[i].name, 80);
        long firstSpaceIndex = strchr(category, '_') - category;
        specials[i].shortName = &specials[i].name[firstSpaceIndex + 1];

        category[firstSpaceIndex] = '\0';

        if ( numCategories == 0 ||
            (numCategories > 0
             && strncmp(categories[numCategories - 1].name, category, 80) != 0) )
        {
            // Found a new category.
            strncpy(categories[numCategories].name, category, 80);
            categories[numCategories].startIndex = i;
            categories[numCategories].count++;
            numCategories++;
        }
        else
        {
            categories[numCategories - 1].count++;
        }

        // Make the display name more purdy.
        for ( char * c = specials[i].name; *c; c++ )
            if ( *c == '_' )
                *c = ' ';
    }

    for ( int i = 0; i < numCategories; i++ )
    {
        SpecialCategory * category = &categories[i];
        category->maxShortNameLength = 0;

        int start = category->startIndex;
        int end = start + category->count - 1;

        for ( int j = start; j <= end; j++ )
        {
            int len = (int)strlen(specials[j].shortName);
            if (  len > category->maxShortNameLength )
                category->maxShortNameLength = len;
        }

#if 0
        printf("Cateogry %d: %s; start: %d; count: %d\n",
               i,
               categories[i].name,
               categories[i].startIndex,
               categories[i].count);
#endif
    }
    printf("Longest name: %d\n", longestName);
}



LineSpecial * FindSpecial(int id)
{
    for ( int i = 0; i < numSpecials; i++ )
        if ( specials[i].id == id )
            return &specials[i];

    return NULL;
}
