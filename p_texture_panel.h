//
//  p_texture_panel.h
//  de
//
//  Created by Thomas Foster on 6/16/23.
//

#ifndef texture_panel_h
#define texture_panel_h

#include "p_panel.h"
#include "m_line.h"

extern Panel texturePanel;

void LoadTexturePanel(void);
void OpenTexturePanel(char * texture, LineProperty property);

#endif /* texture_panel_h */
