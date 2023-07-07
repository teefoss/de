//
//  edit.c
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#include "edit.h"
#include "edit_state.h"

#include "common.h"
#include "mapview.h"
#include "map.h"
#include "geometry.h"
#include "text.h"
#include "panel.h"
#include "line_panel.h"
#include "thing_panel.h"
#include "texture_panel.h"
#include "progress_panel.h"
#include "patch.h"
#include "defaults.h"
#include "sector.h"
#include "sector_panel.h"
#include "flat.h"
#include "doombsp.h"

#include <stdbool.h>
#include <limits.h>

#define SHIFT_DOWN (mods & KMOD_SHIFT)

Editor editor;

static bool running = true;

// Input

static const u8 * keys;
static SDL_Keymod mods;
static SDL_Point worldMouse;
static SDL_Point windowMouse;
u32 mouseButtons;

static SDL_Point dragStart;
static SDL_Rect selectionBox;
static int previousMouseX;
static int previousMouseY;
static SDL_Point visibleRectTarget;

static int bmapDY = 0;
static int bmapDX = 0;


SDL_Point GridPoint(const SDL_Point * worldPoint)
{
    float x = (float)worldPoint->x / (float)gridSize;
    float y = (float)worldPoint->y / (float)gridSize;
    x += worldPoint->x < 0.0f ? -0.5f : 0.5f;
    y += worldPoint->y < 0.0f ? -0.5f : 0.5f;

    SDL_Point gridPoint = { (int)x * gridSize, (int)y * gridSize };

    return gridPoint;
}

void DeselectAllObjects(void)
{
    Vertex * vertices = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++ ) {
        vertices[i].selected = false;
    }

    Line * lines = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++ ) {
        lines[i].selected = false;
    }

    Thing * things = map.things->data;
    for ( int i = 0; i < map.things->count; i++ ) {
        things[i].selected = false;
    }

//    while ( topPanel )
//        ClosePanel();
    ClosePanel();
//    topPanel = -1; // Close all panels.
}

#pragma mark - AUTOSCROLL

void AutoScrollToPoint(SDL_Point worldPoint)
{
    visibleRectTarget.x = worldPoint.x - visibleRect.w / 2;
    visibleRectTarget.y = worldPoint.y - visibleRect.h / 2;
    editorState = ES_AUTO_SCROLL;
}

void UpdateAutoscroll(void)
{
    float epsilon = 1.0f;
    float factor = 0.1f;

    visibleRect.x = LerpEpsilon(visibleRect.x,
                                visibleRectTarget.x,
                                factor,
                                epsilon);

    visibleRect.y = LerpEpsilon(visibleRect.y,
                                visibleRectTarget.y,
                                factor,
                                epsilon);

    if (   visibleRectTarget.x == visibleRect.x
        && visibleRectTarget.y == visibleRect.y )
    {
        editorState = ES_EDIT;
    }
}

#pragma mark - DRAG OBJECTS

static SDL_Point previousDragPoint;

void StartDraggingObjects(void)
{
    editorState = ES_DRAG_OBJECTS;
    previousDragPoint = GridPoint(&worldMouse);
}

void DragSelectedObjects(float dt)
{
    (void)dt;

    SDL_Point current = GridPoint(&worldMouse);

    int dx = current.x - previousDragPoint.x;
    int dy = current.y - previousDragPoint.y;

    Vertex * vertices = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++ )
    {
        if ( vertices[i].removed ) continue;

        if ( vertices[i].selected )
        {
            vertices[i].origin.x += dx;
            vertices[i].origin.y += dy;
            map.boundsDirty = true;
        }
    }

    Thing * things = map.things->data;
    for ( int i = 0; i < map.things->count; i++ )
    {
        if ( things[i].selected ) {
            things[i].origin.x += dx;
            things[i].origin.y += dy;
            map.boundsDirty = true;
        }
    }

    previousDragPoint = current;
}

void HandleDragObjectsEvent(const SDL_Event * event)
{
    if ( event->type == SDL_MOUSEBUTTONUP
        && event->button.button == SDL_BUTTON_LEFT )
    {
        editorState = ES_EDIT;
    }
}

#pragma mark - SELECTION BOX

void SelectObjectsInSelectionBox(void)
{
    SDL_Rect vertexCheckRect = {
        .x = selectionBox.x - VERTEX_DRAW_SIZE / 2,
        .y = selectionBox.y - VERTEX_DRAW_SIZE / 2,
        .w = selectionBox.w + VERTEX_DRAW_SIZE,
        .h = selectionBox.h + VERTEX_DRAW_SIZE
    };

    Vertex * vertex = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++, vertex++ )
    {
        if ( vertex->removed ) continue;
        if ( SDL_PointInRect(&vertex->origin, &vertexCheckRect) )
            vertex->selected = true;
    }

    Vertex * vertices = map.vertices->data;
    Line * line = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        Vertex * v1 = &vertices[line->v1];
        Vertex * v2 = &vertices[line->v2];
        if ( LineInRect(&v1->origin, &v2->origin, &selectionBox) ) {
            v1->selected = true;
            v2->selected = true;
            line->selected = true;
        }
    }

    SDL_Rect thingCheckRect = {
        .x = selectionBox.x - THING_DRAW_SIZE / 2,
        .y = selectionBox.y - THING_DRAW_SIZE / 2,
        .w = selectionBox.w + THING_DRAW_SIZE,
        .h = selectionBox.h + THING_DRAW_SIZE
    };

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
    {
        if ( SDL_PointInRect(&thing->origin, &thingCheckRect) ) {
            thing->selected = true;
        }
    }
}

void HandleDragBoxEvent(const SDL_Event * event)
{
    if ( event->type == SDL_MOUSEBUTTONUP
        && event->button.button == SDL_BUTTON_LEFT )
    {
        SelectObjectsInSelectionBox();
        editorState = ES_EDIT;
    }
}

void UpdateSelectionBox(float dt)
{
    (void)dt;

    int left    = worldMouse.x < dragStart.x ? worldMouse.x : dragStart.x;
    int top     = worldMouse.y < dragStart.y ? worldMouse.y : dragStart.y;
    int right   = worldMouse.x < dragStart.x ? dragStart.x : worldMouse.x;
    int bottom  = worldMouse.y < dragStart.y ? dragStart.y : worldMouse.y;

    selectionBox.x = left;
    selectionBox.y = top;
    selectionBox.w = (right - left) + 1;
    selectionBox.h = (bottom - top) + 1;
}

#pragma mark - DRAG VIEW

void UpdateDragView(void)
{
    visibleRect.x -= windowMouse.x - previousMouseX;
    visibleRect.y -= windowMouse.y - previousMouseY;
    previousMouseX = windowMouse.x;
    previousMouseY = windowMouse.y;
}

void HandleDragViewEvent(const SDL_Event * event)
{
    if ( event->type == SDL_MOUSEBUTTONUP
        && event->button.button == SDL_BUTTON_LEFT )
    {
        editorState = ES_EDIT;
    }

    if ( !keys[SDL_SCANCODE_SPACE] )
        editorState = ES_EDIT;
}

#pragma mark - EDIT

void SelectObject(bool openPanel)
{
    SDL_Rect clickRect = MakeCenteredRect(&worldMouse, SELECTION_SIZE / scale);

    Vertex * vertex = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++, vertex++ )
    {
        if ( vertex->removed ) continue;

        if ( SDL_PointInRect(&vertex->origin, &clickRect) )
        {
            if ( !SHIFT_DOWN && !vertex->selected )
                DeselectAllObjects();

            if ( vertex->selected && SHIFT_DOWN )
            {
                vertex->selected = false;
            }
            else
            {
                vertex->selected = true;
                //printf("%d, %d\n", vertex->origin.x, vertex->origin.y);
                StartDraggingObjects();
            }

            return;
        }
    }

    Vertex * vertices = map.vertices->data;
    Line * line = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        if ( LineInRect(&vertices[line->v1].origin,
                        &vertices[line->v2].origin,
                        &clickRect) )
        {
            if ( !SHIFT_DOWN && !line->selected )
                DeselectAllObjects();

            if ( SHIFT_DOWN && line->selected )
            {
                line->selected = false;
            }
            else
            {
                line->selected = true;
                vertices[line->v1].selected = true;
                vertices[line->v2]. selected = true;

                printf("selected line %d:\n", i);
                printf("- v1: %d, %d\n",
                       vertices[line->v1].origin.x,
                       vertices[line->v1].origin.y);
                printf("- v2: %d, %d\n",
                       vertices[line->v2].origin.x,
                       vertices[line->v2].origin.y);

                if ( openPanel )
                {
                    OpenPanel(&linePanel, line);
                    UpdateLinePanelContent(); // TODO: panel->refresh
                }
                else
                {
                    StartDraggingObjects();
                }
            }

            return;
        }
    }

    clickRect = MakeCenteredRect(&worldMouse, THING_DRAW_SIZE);
    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
    {
        if ( SDL_PointInRect(&thing->origin, &clickRect) )
        {
            if ( !SHIFT_DOWN && !thing->selected )
                DeselectAllObjects();

            if ( SHIFT_DOWN && thing->selected )
            {
                thing->selected = false;
            }
            else
            {
                thing->selected = true;
                if ( openPanel )
                {
                    OpenPanel(&thingPanel, thing);
                }
                else
                {
                    StartDraggingObjects();
                }
            }

            return;
        }
    }

    if ( !SHIFT_DOWN )
        DeselectAllObjects();

    if ( openPanel )
    {
        if ( SelectSector(&worldMouse) )
            OpenSectorPanel();
    }
    else
    {
        dragStart = worldMouse;
        editorState = ES_DRAG_BOX;
    }
}

/// Autoscroll to the center of selected object(s)
void CenterSelectedObjects(void)
{
    Box box =
    {
        .left = INT_MAX,
        .top = INT_MAX,
        .right = INT_MIN,
        .bottom = INT_MIN
    };

    Vertex * vertex = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++, vertex++ )
        if ( vertex->selected )
            EnclosePoint(&vertex->origin, &box);

    Vertex * vertices = map.vertices->data;
    Line * line = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        if ( line->selected )
        {
            EnclosePoint(&vertices[line->v1].origin, &box);
            EnclosePoint(&vertices[line->v2].origin, &box);
        }
    }

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
        if ( thing->selected )
            EnclosePoint(&thing->origin, &box);

    SDL_Point focus =
    {
        .x = box.left + (box.right - box.left) / 2,
        .y = box.top + (box.bottom - box.top) / 2
    };

    AutoScrollToPoint(focus);
}

void ProcessEditEvent(const SDL_Event * event)
{
    switch ( event->type )
    {
        case SDL_QUIT:
            running = false;
            break;

        case SDL_WINDOWEVENT:
            switch ( event->window.event )
            {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    visibleRect.w = event->window.data1;
                    visibleRect.h = event->window.data2;
                    break;
                default:
                    break;
            }
            break;

        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_1:
                    if ( mods & KMOD_GUI )
                    {
                        DoomBSP();
                        system("./1.sh");
                    }
                    break;
                case SDLK_s:
                    if ( mods & KMOD_GUI )
                        DoomBSP();
                    break;
#ifdef DRAW_BLOCK_MAP
                case SDLK_o:
                    bmapScale /= 2.0f;
                    break;
                case SDLK_p:
                    bmapScale *= 2.0f;
                    break;
#endif
                case SDLK_EQUALS:
                    ZoomIn();
                    break;
                case SDLK_MINUS:
                    ZoomOut();
                    break;
                case SDLK_RIGHTBRACKET:
                    if ( gridSize * 2 <= 64 )
                        gridSize *= 2;
                    break;
                case SDLK_LEFTBRACKET:
                    if ( gridSize / 2 >= 1 )
                        gridSize /= 2;
                    break;
                case SDLK_F1:
//                    showBlockMap = !showBlockMap;
                    break;
                case SDLK_ESCAPE:
                    DeselectAllObjects();
                    break;
                case SDLK_c:
                    CenterSelectedObjects();
                    break;
                default:
                    break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch ( event->button.button )
            {
                case SDL_BUTTON_LEFT:
                    previousMouseX = windowMouse.x;
                    previousMouseY = windowMouse.y;

                    if ( event->button.clicks == 2 )
                        AutoScrollToPoint(worldMouse);
                    else if ( keys[SDL_SCANCODE_SPACE] )
                        editorState = ES_DRAG_VIEW;
                    else
                        SelectObject(false);
                    break;

                case SDL_BUTTON_RIGHT:
                    SelectObject(true);
                    break;

                default:
                    break;
            }
            break;

        case SDL_MOUSEWHEEL:
//            printf("precise x, y: %f, %f\n", event->wheel.preciseX, event->wheel.preciseY);
//            visibleRect.x += event->wheel.preciseX * 8.0f;// * 64.0f;
//            visibleRect.y -= event->wheel.preciseY * 8.0f;// * 64.0f;
            if ( event->wheel.y < 0 )
                ZoomIn();
            else if ( event->wheel.y > 0 )
                ZoomOut();
            break;
        default:
            break;
    }
}

// TODO: Move this to PollEvent, so scrolling doesn't happen on, e.g., CTRL-S
void ManualScrollView(float dt)
{
    static float scrollSpeed = 0.0f;
    bool moved = false;

    if (   keys[SDL_SCANCODE_W]
        || keys[SDL_SCANCODE_A]
        || keys[SDL_SCANCODE_S]
        || keys[SDL_SCANCODE_D] )
    {
        scrollSpeed += (60.0f / scale) * dt;
        float max = (600.0f / scale) * dt;
        if ( scrollSpeed > max )
            scrollSpeed = max;
        moved = true;
    }
    else
    {
        scrollSpeed = 0.0f;
    }

    if ( keys[SDL_SCANCODE_W] )
        visibleRect.y -= scrollSpeed;

    if ( keys[SDL_SCANCODE_S] )
        visibleRect.y += scrollSpeed;

    if ( keys[SDL_SCANCODE_A] )
        visibleRect.x -= scrollSpeed;

    if ( keys[SDL_SCANCODE_D] )
        visibleRect.x += scrollSpeed;

    if ( moved )
    {
        SDL_Rect bounds = GetMapBounds();

        // TODO: only clamp when moving toward the edge.
        // This way, a user who has reduced the world bounds the vis rect
        // don't snap back when subsequently scrolling.

        // Don't let the user scroll off into infinity!
        visibleRect.x = SDL_clamp(visibleRect.x,
                                  bounds.x,
                                  bounds.x + bounds.w - visibleRect.w);
        visibleRect.y = SDL_clamp(visibleRect.y,
                                  bounds.y,
                                  bounds.y + bounds.h - visibleRect.h);
    }
}

void UpdateEdit(float dt)
{
    ManualScrollView(dt);
}

#pragma mark -

bool isResizing;
SDL_Point oldWindowOrigin;
SDL_Point oldVisibleOrigin;

int WindowResizeEventFilter(void * data, SDL_Event * event)
{
    (void)data;
    
    if (   event->type == SDL_WINDOWEVENT
        && (event->window.event == SDL_WINDOWEVENT_RESIZED
            || event->window.event == SDL_WINDOWEVENT_RESIZED) )
    {
        SDL_Rect frame = GetWindowFrame();

        if ( !isResizing )
        {
            isResizing = true;
            oldWindowOrigin.x = frame.x;
            oldWindowOrigin.y = frame.y;
            oldVisibleOrigin.x = visibleRect.x;
            oldVisibleOrigin.y = visibleRect.y;
        }

        if ( event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED )
        {
            isResizing = false;
        }

        frame = GetWindowFrame();

        int dx = frame.x - oldWindowOrigin.x;
        int dy = frame.y - oldWindowOrigin.y;

        visibleRect.x = oldVisibleOrigin.x + dx;
        visibleRect.y = oldVisibleOrigin.y + dy;
        visibleRect.w = event->window.data1;
        visibleRect.h = event->window.data2;

        RenderEditor();
    }

    return 1;
}

void InitEditor(void)
{
    switch ( editor.game )
    {
        case GAME_DOOM1:   LoadLinePanels(DOOM1_PATH"linespecials.dsp"); break;
        case GAME_DOOM1SE: LoadLinePanels(DOOMSE_PATH"linespecials.dsp"); break;
        case GAME_DOOM2:   LoadLinePanels(DOOM2_PATH"linespecials.dsp"); break;
        default: break;
    }

    InitLineCross();
    InitMapView();
    printf("LoadProgressPanel (%d)\n", topPanel);
    LoadProgressPanel();
    LoadAllPatches(editor.iwad);
    LoadAllTextures(editor.iwad);
    printf("LoadTexturePanel (%d)\n", topPanel);
    LoadTexturePanel();
    printf("LoadThingPanel (%d)\n", topPanel);
    LoadThingPanel();
    LoadThingDefinitions(); // Needs thing palette to be loaded first.
    LoadFlats(editor.iwad);
    printf("LoadSectorPanel (%d)\n", topPanel);

    LoadSectorPanel();

    keys = SDL_GetKeyboardState(NULL);

    visibleRectTarget.x = visibleRect.x;
    visibleRectTarget.y = visibleRect.y;

    InitRightTray();

    // TODO: add some test that VSYNC is working, otherwise limit framerate.
    // FIXME: get smooth scrolling and non-laggy mouse movement

    SDL_SetEventFilter(WindowResizeEventFilter, NULL);
}

void RenderEditor(void)
{
    SDL_Color background = DefaultColor(BACKGROUND);
    SetRenderDrawColor(&background);
    SDL_RenderClear(renderer);

    DrawMap();

    if ( editorState == ES_DRAG_BOX )
        DrawSelectionBox(&selectionBox);

    if ( topPanel != -1 )
        RenderPanel(rightPanels[topPanel]);

    SDL_RenderPresent(renderer);
}

void EditorFrame(float dt)
{
    mods = SDL_GetModState();
    mouseButtons = SDL_GetMouseState(&windowMouse.x, &windowMouse.y);
    worldMouse = WindowToWorld(&windowMouse);

    bmapDY = 0;
    bmapDX = 0;

    SDL_Event event;
    while ( SDL_PollEvent(&event) )
    {
        for ( int i = topPanel; i >= 0; i-- )
        {
            //            if ( topPanel >= 0 )
            if ( ProcessPanelEvent(rightPanels[i], &event) )
                goto nextEvent;
        }

        StateHandleEvent(&event);

        // TODO: handle quit separately here.
    nextEvent:
        ;
    }

    UpdatePanelMouse(&windowMouse);

    // Block map texture scrolling.
#ifdef DRAW_BLOCK_MAP
    if ( keys[SDL_SCANCODE_RIGHT] )
        bmapLocation.x -= 2 * bmapScale;
    if ( keys[SDL_SCANCODE_LEFT] )
        bmapLocation.x += 2 * bmapScale;
    if ( keys[SDL_SCANCODE_UP] )
        bmapLocation.y += 2 * bmapScale;
    if ( keys[SDL_SCANCODE_DOWN] )
        bmapLocation.y -= 2 * bmapScale;
#endif

    StateUpdate(dt);
    UpdateRightTray();

    RenderEditor();

#ifdef DRAW_BLOCK_MAP
    // TODO: refactor and move to sector.c
    if ( bmapRenderer )
    {
        SDL_SetRenderTarget(bmapRenderer, NULL);
        SDL_SetRenderDrawColor(bmapRenderer, 128, 128, 128, 255);
        SDL_RenderClear(bmapRenderer);

        SDL_QueryTexture(bmapTexture,
                         NULL,
                         NULL,
                         &bmapLocation.w,
                         &bmapLocation.h);

        bmapLocation.w *= bmapScale;
        bmapLocation.h *= bmapScale;

        SDL_RenderCopy(bmapRenderer, bmapTexture, NULL, &bmapLocation);
        SDL_RenderPresent(bmapRenderer);
    }
#endif

    if ( nbRenderer )
    {
//        SDL_SetRenderTarget(nbRenderer, NULL);
//        SDL_SetRenderDrawColor(nbRenderer, 0, 0, 64, 255);
//        SDL_RenderClear(nbRenderer);
        SDL_RenderCopy(nbRenderer, nbTexture, NULL, NULL);
        SDL_RenderPresent(nbRenderer);
    }
}

void EditorLoop(void)
{
    int refreshRate = GetRefreshRate();
    printf("refresh rate: %d Hz\n", refreshRate);
    const float dt = 1.0f / refreshRate;

    while ( running )
        EditorFrame(dt);
}

void CleanupEditor(void)
{
    FreeWad(editor.pwad);
    FreeWad(editor.iwad);
    FreePanel(&texturePanel);
    FreeLinePanels();
    FreePatchesAndTextures();
}
