//
//  progress_panel.c
//  de
//
//  Created by Thomas Foster on 6/12/23.
//

#include "p_progress_panel.h"
#include "p_panel.h"

#define MAX_STR 44

Panel progressPanel;

static char title[MAX_STR];
static char info[MAX_STR];
static float progress;

void OpenProgressPanel(const char * _title)
{
    rightPanels[++topPanel] = &progressPanel;
    progress = 0.0f;
    strncpy(title, _title, sizeof(title));
    info[0] = '\0';
}

void SetProgress(float _progress, const char * _info)
{
    progress = _progress;
    strncpy(info, _info, sizeof(title));
}

void RenderProgressPanel(void)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    progressPanel.location.x = (w - (progressPanel.width * FONT_WIDTH)) / 2;
    progressPanel.location.y = (h - (progressPanel.height * FONT_HEIGHT)) / 2;

    RenderPanelTexture(&progressPanel);

    SDL_RenderSetViewport(renderer, &progressPanel.location);

    // Title and info strings
    SetPanelRenderColor(15);
    PANEL_RENDER_STRING(2, 1, title);
    PANEL_RENDER_STRING(progressPanel.width - 2 - (int)strlen(info), 3, info);

    // Render progress bar
    SetPanelRenderColor(10);
    int barWidth = progressPanel.width - 4;
    int numSegments = barWidth * progress;

    for ( int i = 0; i < numSegments; i++ )
        PANEL_RENDER_STRING(2 + i, 2, "%c", 219);

    SDL_RenderSetViewport(renderer, NULL);
}

void LoadProgressPanel(void)
{
    LoadPanelConsole(&progressPanel, PANEL_DATA_DIRECTORY"progress.panel");
    progressPanel.render = RenderProgressPanel;
}

void CloseProgressPanel(void)
{
    --topPanel;
}
