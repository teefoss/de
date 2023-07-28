//
//  p_line_panel.h
//  de
//
//  Created by Thomas Foster on 5/30/23.
//

#ifndef line_panel_h
#define line_panel_h

#include "p_panel.h"
#include "m_map.h"
#include <stdbool.h>

extern Panel linePanel;
extern Line baseLine;

void LoadLinePanels(const char * dspPath);
void OpenLinePanel(Line * line);
void LinePanelApplyChange(LineProperty property);
void UpdateLinePanelContent(void);
void FreeLinePanels(void);

#endif /* line_panel_h */
