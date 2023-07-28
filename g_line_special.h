//
//  g_line_special.h
//  de
//
//  Created by Thomas Foster on 7/27/23.
//

#ifndef g_line_special_h
#define g_line_special_h

#define NUM_SPECIAL_CATEGORIES 7

typedef struct
{
    char name[80];
    int startIndex;
    int count;
    int maxShortNameLength;
} SpecialCategory;

typedef struct
{
    int id;
    char name[80];
    char * shortName;
} LineSpecial;


extern LineSpecial * specials;
extern int numCategories;
extern SpecialCategory categories[NUM_SPECIAL_CATEGORIES];

void LoadSpecials(const char * path);
LineSpecial * FindSpecial(int id);

#endif /* g_line_special_h */
