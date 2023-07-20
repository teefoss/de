//
//  common.c
//  de
//
//  Created by Thomas Foster on 5/20/23.
//

#include "common.h"
#include "e_defaults.h"
#include <stdbool.h>

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
//    rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
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

//    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
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

float Lerp(float a, float b, float w)
{
    return (1.0f - w) * a + w * b;
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

static inline void
_dla_changebrightness(const SDL_Color * from, SDL_Color * to, float br)
{
    if ( br > 1.0 )
        br = 1.0;

    to->r = from->r;
    to->g = from->g;
    to->b = from->b;
    to->a = 255.0f * br;
}

#define plot_(X,Y,D) do{ SDL_Color f_;                \
  f_.r = r; f_.g = g; f_.b = b;            \
  _dla_plot((X), (Y), &f_, (D)) ; }while(0)

static inline void _dla_plot(int x, int y, SDL_Color * col, float br)
{
    SDL_Color oc;
    _dla_changebrightness(col, &oc, br);

    SDL_SetRenderDrawColor(renderer, oc.r, oc.g, oc.b, oc.a);
    SDL_RenderDrawPoint(renderer, x, y);
}

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((double)(X))+0.5))
#define fpart_(X) (((double)(X))-(double)ipart_(X))
#define rfpart_(X) (1.0-fpart_(X))

#define swap_(a, b) do { __typeof__(a) tmp; tmp = a; a = b; b = tmp; } while(0)

void draw_line_antialias(int x1, int y1, int x2, int y2)
{
    if ( x1 == x2 || y1 == y2 )
    {
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        return;
    }

    u8 r, g, b; // Used by macros... pretty sus.
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

        for( int x = xpxl1 + 1; x < xpxl2; x++ )
        {
            plot_(x, ipart_(intery), rfpart_(intery));
            plot_(x, ipart_(intery) + 1, fpart_(intery));
            intery += gradient;
        }
    } else {
        if ( y2 < y1 )
        {
            swap_(x1, x2);
            swap_(y1, y2);
        }
        double gradient = dx / dy;
        double yend = round_(y1);
        double xend = x1 + gradient * (yend - y1);
        double ygap = rfpart_(y1 + 0.5);
        int ypxl1 = yend;
        int xpxl1 = ipart_(xend);
        plot_(xpxl1, ypxl1, rfpart_(xend) * ygap);
        plot_(xpxl1 + 1, ypxl1, fpart_(xend) * ygap);
        double interx = xend + gradient;

        yend = round_(y2);
        xend = x2 + gradient * (yend - y2);
        ygap = fpart_(y2+0.5);
        int ypxl2 = yend;
        int xpxl2 = ipart_(xend);
        plot_(xpxl2, ypxl2, rfpart_(xend) * ygap);
        plot_(xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

        for( int y = ypxl1 + 1; y < ypxl2; y++)
        {
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

void DrawRect(const SDL_FRect * rect, int thinkness)
{
    SDL_FRect r = *rect;

    for ( int i = 0; i < thinkness; i++ )
    {
        SDL_RenderDrawRectF(renderer, &r);
        r.x++;
        r.y++;
        r.w -= 2.0f;
        r.h -= 2.0f;

        if ( r.w <= 0.0f || r.h <= 0.0f )
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

SDL_Rect GetWindowFrame(void)
{
    SDL_Rect frame;
    SDL_GetWindowPosition(window, &frame.x, &frame.y);
    SDL_GetWindowSize(window, &frame.w, &frame.h);

    return frame;
}


void IntensifyPixels(float x, float y, float d)
{
    SDL_Color c;
    SDL_GetRenderDrawColor(renderer, &c.r, &c.g, &c.b, &c.a);
    SDL_Color bg = DefaultColor(BACKGROUND);

    SDL_SetRenderDrawColor(renderer,
                           Lerp(c.r, bg.r, d),
                           Lerp(c.g, bg.g, d),
                           Lerp(c.b, bg.b, d),
                           c.a);
    SDL_RenderDrawPoint(renderer, x, y);

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

// This absolutely does not work at all.
void GuptaSprollDrawLine(float x1, float x2, float y1, float y2)
{
    float x = x1;
    float y = y1;
    float dx = x2 - x1;
    float dy = y2 - y1;
    float d = 2.0f * dy - dx; // discriminator

    // Euclidean distance of point (x,y) from line (signed)
    float D = 0;

    // Euclidean distance between points (x1, y1) and (x2, y2)
    float length = sqrtf(dx * dx + dy * dy);

    float sin = dx / length;
    float cos = dy / length;
    while (x <= x2) {
        IntensifyPixels(x, y - 1, D + cos);
        IntensifyPixels(x, y, D);
        IntensifyPixels(x, y + 1, D - cos);
        x = x + 1;
        if (d <= 0) {
            D = D + sin;
            d = d + 2 * dy;
        } else {
            D = D + sin - cos;
            d = d + 2 * (dy - dx);
            y = y + 1;
        }
    }
}





double round(double d)
{
    return floor(d + 0.5);
}


//double Map(double input,
//           double input_start,
//           double input_end,
//           double output_start,
//           double output_end)
//{
//    double slope = 1.0 * (output_end - output_start) / (input_end - input_start);
//    double output = output_start + round(slope * (input - input_start));
//
//    return output;
//};




void plot(double x, double y, double c)
{
    // plot the pixel at (x, y) with brightness c (where 0 ≤ c ≤ 1)
    u8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    SDL_Color bg = DefaultColor(BACKGROUND);

    SDL_SetRenderDrawColor(renderer,
                           Lerp(bg.r, r, c),
                           Lerp(bg.g, g, c),
                           Lerp(bg.b, b, c),
                           Lerp(bg.a, a, c));
    SDL_RenderDrawPoint(renderer, x, y);
}

// integer part of x
//function ipart(x) is
//    return floor(x)

//function round(x) is
//    return ipart(x + 0.5)

// fractional part of x
double fpart(double x)
{
    return x - floor(x);
}

double rfpart(double x)
{
    return 1.0 - fpart(x);
}

#define swap(a, b) do { double temp = a; a = b; b = temp;} while (0);

void WuDrawLine(double x0, double y0, double x1, double y1)
{
    bool steep = fabs(y1 - y0) > fabs(x1 - x0);

    if ( steep ) {
        swap(x0, y0)
        swap(x1, y1)
    }
    if ( x0 > x1 ) {
        swap(x0, x1)
        swap(y0, y1)
    }

    double dx = x1 - x0;
    double dy = y1 - y0;

    double gradient;
    if ( dx == 0.0 )
        gradient = 1.0;
    else
        gradient = dy / dx;

    // handle first endpoint
    double xend = round(x0);
    double yend = y0 + gradient * (xend - x0);
    double xgap = rfpart(x0 + 0.5);
    double xpxl1 = xend; // this will be used in the main loop
    double ypxl1 = floor(yend);

    if ( steep ) {
        plot(ypxl1,   xpxl1, rfpart(yend) * xgap);
        plot(ypxl1+1, xpxl1,  fpart(yend) * xgap);
    } else {
        plot(xpxl1, ypxl1  , rfpart(yend) * xgap);
        plot(xpxl1, ypxl1+1,  fpart(yend) * xgap);
    }
    double intery = yend + gradient; // first y-intersection for the main loop

    // handle second endpoint
    xend = round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart(x1 + 0.5);
    double xpxl2 = xend; //this will be used in the main loop
    double ypxl2 = floor(yend);
    if ( steep ) {
        plot(ypxl2  , xpxl2, rfpart(yend) * xgap);
        plot(ypxl2+1, xpxl2,  fpart(yend) * xgap);
    } else {
        plot(xpxl2, ypxl2,  rfpart(yend) * xgap);
        plot(xpxl2, ypxl2+1, fpart(yend) * xgap);
    }

    // main loop
    if ( steep ) {
        for ( double x = xpxl1 + 1.0; x <= xpxl2 - 1; x++ )
        {
            plot(floor(intery)  , x, rfpart(intery));
            plot(floor(intery)+1, x,  fpart(intery));
            intery = intery + gradient;
        }
    } else {
        for ( double x = xpxl1 + 1; x <= xpxl2 - 1; x++ )
        {
            plot(x, floor(intery),  rfpart(intery));
            plot(x, floor(intery)+1, fpart(intery));
            intery = intery + gradient;
        }
    }
}

void Bresenham(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;

    int error = dx + dy;
    int error2;

    int x = x1;
    int y = y1;

    while ( x != x2 || y != y2 )
    {
        SDL_RenderDrawPoint(renderer, x, y);

        error2 = error * 2;

        if ( error2 >= dy )
        {
            error += dy;
            x += sx;
        }

        if ( error2 <= dx )
        {
            error += dx;
            y += sy;
        }
    }
}

void Refresh(SDL_Renderer * _renderer, SDL_Texture * _texture)
{
    SDL_SetRenderTarget(_renderer, NULL);
    SDL_RenderClear(_renderer);
    SDL_RenderCopy(_renderer, _texture, NULL, NULL);
    SDL_RenderPresent(_renderer);
    SDL_SetRenderTarget(_renderer, _texture);
    SDL_PumpEvents();
}
