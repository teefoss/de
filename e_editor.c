//
//  edit.c
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#include "e_editor.h"
#include "e_state.h"

#include "common.h"
#include "e_map_view.h"
#include "m_map.h"
#include "e_geometry.h"
#include "e_undo.h"
#include "text.h"
#include "p_panel.h"
#include "p_line_panel.h"
#include "p_thing_panel.h"
#include "p_texture_panel.h"
#include "p_progress_panel.h"
#include "patch.h"
#include "e_defaults.h"
#include "e_sector.h"
#include "p_sector_panel.h"
#include "flat.h"
#include "doombsp.h"
#include "r_main.h"
#include "i_video.h"
#include "v_video.h"

#include <stdbool.h>
#include <limits.h>

#define SHIFT_DOWN (mods & KMOD_SHIFT)

#ifdef __APPLE__
#define COMMAND (mods & KMOD_GUI)
#else
#define COMMAND (mods & KMOD_CTRL)
#endif

Editor editor;

static bool running = true;

// Input

static const u8 * keys;
static SDL_Keymod mods;
static SDL_Point worldMouse;
static SDL_Point windowMouse;
u32 mouseButtons;

static SDL_Point dragStart;
static bool didDragObjects;
static SDL_Rect selectionBox;
static int previousMouseX;
static int previousMouseY;
static SDL_Point visibleRectTarget;

static int bmapDY = 0;
static int bmapDX = 0;

static SDL_Point newLine[2];

static Array * lineCopies;
static Array * thingCopies;

SectorDef defaultSectorDef = {
    .floorHeight = 0,
    .ceilingHeight = 128,
    .floorFlat = "FLOOR0_1",
    .ceilingFlat = "FLOOR0_1",
    .lightLevel = 255,
};

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

    topPanel = -1; // Close all panels.
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
    didDragObjects = false;
}

void DragSelectedObjects(float dt)
{
    (void)dt;

    SDL_Point current = GridPoint(&worldMouse);

    int dx = current.x - previousDragPoint.x;
    int dy = current.y - previousDragPoint.y;

    if ( !didDragObjects && (dx != 0 || dy != 0) )
    {
        didDragObjects = true;
        SaveUndoState();
    }

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

#pragma mark - NEW LINE

void ProcessNewLineEvent(const SDL_Event * event)
{
    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_ESCAPE:
                    editorState = ES_EDIT;
                    break;
                default:
                    break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch ( event->button.button )
            {
                case SDL_BUTTON_LEFT:
                {
                    SaveUndoState();

                    Side side = {
                        .top = "-",
                        .middle = "-",
                        .bottom = "-",
                    };

                    Side closest;
                    SectorDef sectorDef;
                    if ( GetClosestSide(&newLine[1], &closest) )
                        sectorDef = closest.sectorDef;
                    else
                        sectorDef = defaultSectorDef;
                    side.sectorDef = sectorDef;

                    Line * line = NewLine(&newLine[0], &newLine[1]);
                    line->sides[0] = side;

                    newLine[0] = newLine[1];
                    if ( event->button.clicks == 2 ) // Done.
                        editorState = ES_EDIT;
                    break;
                }
                case SDL_BUTTON_RIGHT:
                    editorState = ES_EDIT;
                    break;
            }
            break;

        default:
            break;
    }
}

void UpdateNewLine(float dt)
{
    (void)dt;
    newLine[1] = GridPoint(&worldMouse);
}

#pragma mark - EDIT

void SelectObject(bool openPanel)
{
    SDL_Rect clickRect = MakeCenteredRect(&worldMouse, SELECTION_SIZE / scale);

    if ( !openPanel ) // Don't bother checking vertices when right-clicking.
    {
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
    }

    Vertex * vertices = map.vertices->data;
    Line * line = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        if ( LineInRect(&vertices[line->v1].origin,
                        &vertices[line->v2].origin,
                        &clickRect) )
        {
            if ( !SHIFT_DOWN && !line->selected && !openPanel )
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
                    OpenLinePanel(line);
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

void CopyObjects(void)
{
    Clear(lineCopies);
    Clear(thingCopies);

    for ( int i = 0; i < map.lines->count; i++ )
    {
        Line * line = Get(map.lines, i);

        if ( line->selected )
        {
            Line copy = *line;

            // Store the line's position, in case vertices change in between
            // now and pasting. Well, we've already got an NXPoint's so just
            // use that.
            SDL_Point p1, p2;
            GetLinePoints(i, &p1, &p2);
            copy.p1 = (NXPoint){ p1.x, p1.y };
            copy.p2 = (NXPoint){ p2.x, p2.y };

            Push(lineCopies, &copy);
        }
    }

    for ( int i = 0; i < map.things->count; i++ )
    {
        Thing * thing = Get(map.things, i);

        if ( thing->selected )
            Push(thingCopies, thing);
    }
}

void PasteObjects(void)
{
    if ( lineCopies->count > 0 || thingCopies->count > 0 )
        SaveUndoState();
    
    DeselectAllObjects();

    // TODO: find the bbox of all selected objects, paste at mouse location.
    for ( int i = 0; i < lineCopies->count; i++ )
    {
        Line * line = Get(lineCopies, i);
        SDL_Point p1 = { line->p1.x + 16, line->p1.y + 16 };
        SDL_Point p2 = { line->p2.x + 16, line->p2.y + 16 };
        Line * new = NewLine(&p1, &p2);

        // Copy everything but the vertex indices.
        int v1 = new->v1; // Save
        int v2 = new->v2;
        *new = *line; // Copy
        new->v1 = v1; // Restore
        new->v2 = v2;

        Vertex * vertices = map.vertices->data;
        vertices[v1].selected = true;
        vertices[v2].selected = true;
    }

    for ( int i = 0; i < thingCopies->count; i++ )
    {
        Thing * thing = Get(thingCopies, i);
        NewThing(thing,
                 &(SDL_Point){ thing->origin.x + 32, thing->origin.y + 32 });
    }
}

/// Delete all selected objects.
void DeleteObjects(void)
{
    SaveUndoState();

    Vertex * vertices = map.vertices->data;

    for ( int i = 0; i < map.lines->count; i++ )
    {
        Line * line = Get(map.lines, i);
        Vertex * v1 = &vertices[line->v1];
        Vertex * v2 = &vertices[line->v2];

        // Remove lines with both vertices selected.
        if ( line->selected && v1->selected && v2->selected )
        {
            line->deleted = true;

            // Remove vertices if this is the last line that uses them.
            if ( --v1->referenceCount <= 0 )
                v1->removed = true;
            if ( --v2->referenceCount <= 0 )
                v2->removed = true;
        }
    }

    for ( int i = 0; i < map.things->count; i++ )
    {
        Thing * thing = Get(map.things, i);
        if ( thing->selected )
            thing->deleted = true;
    }

    DeselectAllObjects();
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

void TryRunNumericScript(int num)
{
    if ( COMMAND )
    {
        char command[8] = { 0 };

#ifdef _WIN32
        sprintf(command, "%d.bat", num);
#else
        sprintf(command, "./%d.sh", num);
#endif

        DoomBSP();
        system(command);
    }
}

void ProcessEditorEvent(const SDL_Event * event)
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
                case SDLK_1: TryRunNumericScript(1); break;
                case SDLK_2: TryRunNumericScript(2); break;
                case SDLK_3: TryRunNumericScript(3); break;
                case SDLK_4: TryRunNumericScript(4); break;
                case SDLK_5: TryRunNumericScript(5); break;
                case SDLK_6: TryRunNumericScript(6); break;
                case SDLK_7: TryRunNumericScript(7); break;
                case SDLK_8: TryRunNumericScript(8); break;
                case SDLK_9: TryRunNumericScript(9); break;
                case SDLK_0: TryRunNumericScript(0); break;

                case SDLK_i:
                {
                    size_t mapSize = sizeof(Map);
                    mapSize += map.vertices->count + sizeof(Vertex);
                    mapSize += map.lines->count + sizeof(Line);
                    mapSize += map.things->count + sizeof(Thing);
                    printf("map size: %zu bytes\n", mapSize);
                    break;
                }

                case SDLK_s:
                    if ( COMMAND )
                        DoomBSP();
                    break;

                case SDLK_f:
                    if ( COMMAND )
                        FlipSelectedLines();
                    break;

                case SDLK_c:
                    if ( COMMAND )
                        CopyObjects();
                    else
                        CenterSelectedObjects();
                    break;

                case SDLK_x:
                    if ( COMMAND )
                    {
                        CopyObjects();
                        DeleteObjects();
                    }
                    break;

                case SDLK_v:
                    if ( COMMAND )
                        PasteObjects();
                    break;

                case SDLK_z:
                    if ( COMMAND && SHIFT_DOWN )
                        Redo();
                    else if ( COMMAND )
                        Undo();

                    break;

                case SDLK_BACKSPACE:
                    DeleteObjects();
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
                    {
                        newLine[0] = GridPoint(&worldMouse);
                        editorState = ES_NEW_LINE;
                    }
                    else if ( keys[SDL_SCANCODE_SPACE] )
                    {
                        editorState = ES_DRAG_VIEW;
                    }
                    else
                    {
                        SelectObject(false);
                    }
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

    if ( keys[SDL_SCANCODE_G] )
    {
        //        liveView.angle -= 0.1f;
    }
    if ( keys[SDL_SCANCODE_J] )
    {
//        liveView.angle += 0.1f;
    }
    if ( keys[SDL_SCANCODE_Y] )
    {
//        liveView.position.x += cosf(liveView.angle);
//        liveView.position.y += sinf(liveView.angle);
    }

    if ( keys[SDL_SCANCODE_H] )
    {
//        liveView.position.x -= cosf(liveView.angle);
//        liveView.position.y -= sinf(liveView.angle);
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
    LoadProgressPanel();
    LoadAllPatches(editor.iwad);
    LoadAllTextures(editor.iwad);
    LoadTexturePanel();
    LoadThingPanel();
    LoadThingDefinitions(); // Needs thing palette to be loaded first.
    LoadFlats(editor.iwad);
    LoadSectorPanel();

    keys = SDL_GetKeyboardState(NULL);

    visibleRectTarget.x = visibleRect.x;
    visibleRectTarget.y = visibleRect.y;

    // TODO: add some test that VSYNC is working, otherwise limit framerate.
    // FIXME: get smooth scrolling and non-laggy mouse movement

    SDL_SetEventFilter(WindowResizeEventFilter, NULL);

#if 0
    V_Init();
    I_InitGraphics();
    viewPlayer.mo = calloc(1, sizeof(*viewPlayer.mo));
    // Populate node builder arrays, which the live view renderer needs.
    DoomBSP();
#endif


    lineCopies = NewArray(0, sizeof(Line), 16);
    thingCopies = NewArray(0, sizeof(Thing), 16);
}

void RenderEditor(void)
{
    SDL_Color background = DefaultColor(BACKGROUND);
    SetRenderDrawColor(&background);
    SDL_RenderClear(renderer);

    DrawMap();

    switch ( editorState )
    {
        case ES_DRAG_BOX:
            DrawSelectionBox(&selectionBox);
            break;
        case ES_NEW_LINE:
        {
            SDL_Color lineColor = DefaultColor(LINE_ONE_SIDED);
            SetRenderDrawColor(&lineColor);
            WorldDrawLine(&(SDL_FPoint){ newLine[0].x, newLine[0].y },
                          &(SDL_FPoint){ newLine[1].x, newLine[1].y });
            DrawVertex(&newLine[0]);
            DrawVertex(&newLine[1]);
            break;
        }
        default:
            break;
    }

    for ( int i = 0; i <= topPanel; i++ )
        RenderPanel(rightPanels[i]);

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

//    R_RenderPlayerView(&viewPlayer);
//    I_FinishUpdate(); // Actually render the live view.

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
