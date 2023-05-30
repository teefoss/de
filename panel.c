//
//  panel.c
//  de
//
//  Created by Thomas Foster on 5/27/23.
//

#include "panel.h"
#include "common.h"
#include "text.h"

static SDL_Color palette[16] = {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xAA },
//    { 0x04, 0x14, 0x41 }, // Get that funkly blue color!
    { 0x00, 0xAA, 0x00 },
    { 0x00, 0xAA, 0xAA },
    { 0xAA, 0x00, 0x00 },
    { 0xAA, 0x00, 0xAA },
    { 0xAA, 0x55, 0x00 },
    { 0xAA, 0xAA, 0xAA },
    { 0x55, 0x55, 0x55 },
    { 0x55, 0x55, 0xFF },
    { 0x55, 0xFF, 0x55 },
    { 0x55, 0xFF, 0xFF },
    { 0xFF, 0x55, 0x55 },
    { 0xFF, 0x55, 0xFF },
    { 0xFF, 0xFF, 0x55 },
    { 0xFF, 0xFF, 0xFF },
};


void SetPanelColor(int index)
{
    SDL_Color c = palette[index];

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
}

Panel LoadPanel(const char * path)
{
    Panel panel = { 0 };

    FILE * file = fopen(path, "rb");
    if ( file == NULL ) {
        printf("Error: '%s' does not exist\n", path);
        return panel;
    }

    u8 width;
    u8 height;
    fread(&width, sizeof(width), 1, file);
    fread(&height, sizeof(height), 1, file);

    u16 * data = malloc(sizeof(*data) * width * height);
    fread(data, sizeof(*data), width * height, file);

    fclose(file);

    SDL_Texture * texture = SDL_CreateTexture(renderer,
                                              SDL_PIXELFORMAT_RGBA8888,
                                              SDL_TEXTUREACCESS_TARGET,
                                              width * FONT_WIDTH,
                                              height * FONT_HEIGHT);

    if ( texture == NULL )
    {
        printf("Error: could not load line panel (%s)\n", SDL_GetError());
        return panel;
    }

    SDL_SetRenderTarget(renderer, texture);
    SDL_Rect bgRect = { .w = FONT_WIDTH, .h = FONT_HEIGHT };

    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            BufferCell cell = GetCell(data[y * width + x]);

            int rx = x * FONT_WIDTH;
            int ry = y * FONT_HEIGHT;
            bgRect.x = rx;
            bgRect.y = ry;

            SetPanelColor(cell.background);
            SDL_RenderFillRect(renderer, &bgRect);

            SetPanelColor(cell.foreground);
            PrintChar(rx, ry, cell.character);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);

    panel.width = width;
    panel.height = height;
    panel.texture = texture;
    panel.data = data;

    return panel;
}

void PanelRenderTexture(const Panel * panel, int x, int y)
{
    SDL_Rect dest = {
        .x = x,
        .y = y,
        .w = panel->width * FONT_WIDTH,
        .h = panel->height * FONT_HEIGHT
    };

    SDL_Rect shadow = dest;
    shadow.x += 16;
    shadow.y += 16;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(renderer, &shadow);
    SDL_RenderCopy(renderer, panel->texture, NULL, &dest);
}
