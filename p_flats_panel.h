//
//  p_flats_panel.h
//  de
//
//  Created by Thomas Foster on 7/24/23.
//

#ifndef p_flats_panel_h
#define p_flats_panel_h

typedef enum {
    SELECTING_FLOOR,
    SELECTING_CEILING
} FlatSelection;

void LoadFlatsPanel(void);
void OpenFlatsPanel(FlatSelection selection);

#endif /* p_flats_panel_h */
