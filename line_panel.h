//
//  line_panel.h
//  de
//
//  Created by Thomas Foster on 5/30/23.
//

#ifndef line_panel_h
#define line_panel_h

#include "panel.h"
#include "map.h"
#include <stdbool.h>

typedef struct
{
    int id;
    char name[80];
    char * shortName;
} LineSpecial;

extern Panel linePanel;

void LoadLinePanels(const char * dspPath);
void UpdateLinePanelContent(void);
void FreeLinePanels(void);

#endif /* line_panel_h */
