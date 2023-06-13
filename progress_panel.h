//
//  progress_panel.h
//  de
//
//  Created by Thomas Foster on 6/12/23.
//

#ifndef progress_panel_h
#define progress_panel_h

void OpenProgressPanel(const char * _title);
void SetProgress(float _progress, const char * _info);
void LoadProgressPanel(void);
void CloseProgressPanel(void);
void RenderProgressPanel(void);

#endif /* progress_panel_h */
