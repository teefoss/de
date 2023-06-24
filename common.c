//
//  common.c
//  de
//
//  Created by Thomas Foster on 5/20/23.
//

#include "common.h"

void _assert(const char * message, const char * file, int line)
{
    fflush(NULL);
    fprintf(stderr, "%s:%d Assertion '%s' Failed.\n", file, line, message);
    fflush(stderr);
}



SDL_Window * window;
SDL_Renderer * renderer;

static void CleanUpWindowAndRenderer(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void InitWindow(int width, int height)
{
    atexit(CleanUpWindowAndRenderer);

    if ( SDL_Init(SDL_INIT_VIDEO) != 0 )
    {
        fprintf(stderr, "Error: could not init SDL (%s)\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    u32 flags = 0;
    flags |= SDL_WINDOW_RESIZABLE;
//    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    window = SDL_CreateWindow("", x, y, width, height, flags);

    if ( window == NULL )
    {
        fprintf(stderr, "Error: could not create window (%s)\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    u32 rendererFlags = 0;
    rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
    rendererFlags |= SDL_RENDERER_TARGETTEXTURE;
    rendererFlags |= SDL_RENDERER_SOFTWARE;
//    rendererFlags |= SDL_RENDERER_ACCELERATED;
    renderer = SDL_CreateRenderer(window, -1, rendererFlags);

    if ( renderer == NULL )
    {
        fprintf(stderr,
                "Error: could not create renderer (%s)\n",
                SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
//    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    // Adjust for High DPI displays.
    // TODO: nix this for now. Things get weird when the window resizes...
#if 0
    SDL_Rect renderSize;
    SDL_GetRendererOutputSize(renderer, &renderSize.w, &renderSize.h);
    if ( renderSize.w != WINDOW_WIDTH && renderSize.h != WINDOW_HEIGHT ) {
        SDL_RenderSetScale(renderer,
                           renderSize.w / WINDOW_WIDTH,
                           renderSize.h / WINDOW_HEIGHT);
    }
#endif
}

void SetRenderDrawColor(const SDL_Color * c)
{
    SDL_SetRenderDrawColor(renderer, c->r, c->g, c->b, c->a);
}

int GetRefreshRate(void)
{
    int displayIndex = SDL_GetWindowDisplayIndex(window);

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(displayIndex, &displayMode);

    return displayMode.refresh_rate;
}


float LerpEpsilon(float a, float b, float w, float epsilon)
{
    float result = (1.0f - w) * a + w * b;
    if ( fabsf(result - b) < epsilon ) {
        result = b;
    }

    return result;
}

SDL_Texture * GetScreen(void)
{
    SDL_Surface * surface = SDL_GetWindowSurface(window);
    if ( surface == NULL )
        printf("%s\n", SDL_GetError());
    return SDL_CreateTextureFromSurface(renderer, surface);
}

// TODO: clean up this knarly-ass code
// https://rosettacode.org/wiki/Xiaolin_Wu's_line_algorithm#C

typedef SDL_Color * rgb_color_p;
typedef SDL_Color rgb_color;
static inline void _dla_changebrightness(rgb_color_p from,
                  rgb_color_p to, float br)
{
    if ( br > 1.0 )
        br = 1.0;

    to->r = from->r;
    to->g = from->g;
    to->b = from->b;
    to->a = 255 * br;
}

#define plot_(X,Y,D) do{ rgb_color f_;                \
  f_.r = r; f_.g = g; f_.b = b;            \
  _dla_plot((X), (Y), &f_, (D)) ; }while(0)

static inline void _dla_plot(int x, int y, rgb_color_p col, float br)
{
  rgb_color oc;
  _dla_changebrightness(col, &oc, br);
//  put_pixel_clip(img, x, y, oc.red, oc.green, oc.blue);
    SDL_SetRenderDrawColor(renderer, oc.r, oc.g, oc.b, oc.a);
    SDL_RenderDrawPoint(renderer, x, y);
}

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((double)(X))+0.5))
#define fpart_(X) (((double)(X))-(double)ipart_(X))
#define rfpart_(X) (1.0-fpart_(X))

#define swap_(a, b) do{ __typeof__(a) tmp;  tmp = a; a = b; b = tmp; }while(0)
void draw_line_antialias(int x1, int y1, int x2, int y2)
{
    if ( x1 == x2 || y1 == y2 )
    {
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        return;
    }

    u8 r, g, b;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, NULL);
  double dx = (double)x2 - (double)x1;
  double dy = (double)y2 - (double)y1;
  if ( fabs(dx) > fabs(dy) ) {
    if ( x2 < x1 ) {
      swap_(x1, x2);
      swap_(y1, y2);
    }
    double gradient = dy / dx;
    double xend = round_(x1);
    double yend = y1 + gradient*(xend - x1);
    double xgap = rfpart_(x1 + 0.5);
    int xpxl1 = xend;
    int ypxl1 = ipart_(yend);
    plot_(xpxl1, ypxl1, rfpart_(yend)*xgap);
    plot_(xpxl1, ypxl1+1, fpart_(yend)*xgap);
    double intery = yend + gradient;

    xend = round_(x2);
    yend = y2 + gradient*(xend - x2);
    xgap = fpart_(x2+0.5);
    int xpxl2 = xend;
    int ypxl2 = ipart_(yend);
    plot_(xpxl2, ypxl2, rfpart_(yend) * xgap);
    plot_(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

    int x;
    for(x=xpxl1+1; x < xpxl2; x++) {
      plot_(x, ipart_(intery), rfpart_(intery));
      plot_(x, ipart_(intery) + 1, fpart_(intery));
      intery += gradient;
    }
  } else {
    if ( y2 < y1 ) {
      swap_(x1, x2);
      swap_(y1, y2);
    }
    double gradient = dx / dy;
    double yend = round_(y1);
    double xend = x1 + gradient*(yend - y1);
    double ygap = rfpart_(y1 + 0.5);
    int ypxl1 = yend;
    int xpxl1 = ipart_(xend);
    plot_(xpxl1, ypxl1, rfpart_(xend)*ygap);
    plot_(xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
    double interx = xend + gradient;

    yend = round_(y2);
    xend = x2 + gradient*(yend - y2);
    ygap = fpart_(y2+0.5);
    int ypxl2 = yend;
    int xpxl2 = ipart_(xend);
    plot_(xpxl2, ypxl2, rfpart_(xend) * ygap);
    plot_(xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

    int y;
    for(y=ypxl1+1; y < ypxl2; y++) {
      plot_(ipart_(interx), y, rfpart_(interx));
      plot_(ipart_(interx) + 1, y, fpart_(interx));
      interx += gradient;
    }
  }
}
#undef swap_
#undef plot_
#undef ipart_
#undef fpart_
#undef round_
#undef rfpart_


void Capitalize(char * string)
{
    if ( string == NULL )
        return;

    for ( char * c = string; *c; c++ )
        *c = toupper(*c);
}

void DrawRect(const SDL_Rect * rect, int thinkness)
{
    SDL_Rect r = *rect;

    for ( int i = 0; i < thinkness; i++ )
    {
        SDL_RenderDrawRect(renderer, &r);
        r.x++;
        r.y++;
        r.w -= 2;
        r.h -= 2;

        if ( r.w <= 0 || r.h <= 0 )
            break;
    }
}

SDL_Color Int24ToSDL(int integer)
{
    SDL_Color color;

    color.r = (integer & 0xFF0000) >> 16;
    color.g = (integer & 0x00FF00) >> 8;
    color.b = integer & 0x0000FF;
    color.a = 255;

    return color;
}
