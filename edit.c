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
#include "panels.h"

#include <stdbool.h>

#define SHIFT_DOWN (mods & KMOD_SHIFT)

bool running = true;

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
static int selectedLineIndex = -1;


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

    selectedLineIndex = -1;
}

#pragma mark - AUTOSCROLL

void AutoScrollToPoint(SDL_Point worldPoint)
{
    visibleRectTarget.x = worldPoint.x - visibleRect.w / 2;
    visibleRectTarget.y = worldPoint.y - visibleRect.h / 2;
    editorState = ES_AUTO_SCROLL;
}

void UpdateAutoscroll(float dt)
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
        }
    }

    Thing * things = map.things->data;
    for ( int i = 0; i < map.things->count; i++ )
    {
        if ( things[i].selected ) {
            things[i].origin.x += dx;
            things[i].origin.y += dy;
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

void UpdateDragView(float dt)
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
    SDL_Rect clickRect = MakeCenteredRect(&worldMouse, SELECTION_SIZE);

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

                if ( openPanel )
                {
                    selectedLineIndex = i;

                    // TODO: refactor LineMidpoint()
                    SDL_Point p1 = vertices[line->v1].origin;
                    SDL_Point p2 = vertices[line->v2].origin;
                    int dx = p2.x - p1.x;
                    int dy = p2.y - p2.y;
                    SDL_Point midpoint = { p1.x + dx / 2, p1.y + dy / 2 };
                    AutoScrollToPoint(midpoint);
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
                StartDraggingObjects();
            }

            return;
        }
    }

    if ( !SHIFT_DOWN )
        DeselectAllObjects();

    dragStart = worldMouse;
    editorState = ES_DRAG_BOX;
}

bool HandlePanelEvent(const SDL_Event * event)
{
    Panel * panel = NULL;
    PanelEventHandler handler = NULL;
    void * object = NULL;
    PanelItem * item = NULL;

    if ( selectedLineIndex != -1 ) {
        panel = &line_panel;
        handler = HandleLinePanelEvent;
        object = Get(map.lines, selectedLineIndex);
    }

    // TODO: things etc.

    // Nothing is currently selected.
    if ( panel == NULL ) {
        return false;
    }

    item = &panel->items[panel->selection];

    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_UP:
                    if ( item->up == 0 )
                        panel->selection--;
                    else if ( item->up > 0 )
                        panel->selection = item->up;
                    return true;

                case SDLK_DOWN:
                    if ( item->down == 0 )
                        panel->selection++;
                    else if ( item->down > 0 )
                        panel->selection = item->down;
                    return true;

                case SDLK_LEFT:
                    if ( item->left != -1 )
                        panel->selection = item->left;
                    return true;

                case SDLK_RIGHT:
                    if ( item->right != -1 )
                        panel->selection = item->right;
                    return true;

                case SDLK_RETURN:
                    return handler(event, object);

                default:
                    return false;
            }

        default:
            return false;
    }
}

void HandleEditEvent(const SDL_Event * event)
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
                case SDLK_ESCAPE:
                    DeselectAllObjects();
                    break;
                default:
                    break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch ( event->button.button ) {

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
            if ( event->wheel.y < 0 )
                ZoomIn();
            else if ( event->wheel.y > 0 )
                ZoomOut();
            break;
        default:
            break;
    }
}

void ManualScrollView(float dt)
{
    static float scrollSpeed = 0.0f;

    if (   keys[SDL_SCANCODE_W]
        || keys[SDL_SCANCODE_A]
        || keys[SDL_SCANCODE_S]
        || keys[SDL_SCANCODE_D] )
    {
        scrollSpeed += 60.0f * dt;
        float max = (600.0f / scale) * dt;
        if ( scrollSpeed > max )
            scrollSpeed = max;
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
}

void UpdateEdit(float dt)
{
    ManualScrollView(dt);
}

#pragma mark -

static void RenderEditor(void)
{
    SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
    SDL_RenderClear(renderer);

    DrawMap();

    if ( editorState == ES_DRAG_BOX )
        DrawSelectionBox(&selectionBox);

    if ( selectedLineIndex != -1 )
    {
        Line * line = Get(map.lines, selectedLineIndex);
        RenderLinePanel(line);
    }

    PrintString(0, 0, "World Point: %d, %d", worldMouse.x, worldMouse.y);

    SDL_RenderPresent(renderer);
}

void EditorLoop(void)
{
    keys = SDL_GetKeyboardState(NULL);

    const float dt = 1.0f / GetRefreshRate();

    visibleRectTarget.x = visibleRect.x;
    visibleRectTarget.y = visibleRect.y;

    // TODO: add some test that VSYNC is working, otherwise limit framerate.

    while ( running )
    {
        mods = SDL_GetModState();

        mouseButtons = SDL_GetMouseState(&windowMouse.x, &windowMouse.y);
        worldMouse = WindowToWorld(&windowMouse);

        SDL_Event event;
        while ( SDL_PollEvent(&event) )
        {
            if ( HandlePanelEvent(&event) )
                continue;

            StateHandleEvent(&event);
        }

        StateUpdate(dt);

        RenderEditor();
    }
}
