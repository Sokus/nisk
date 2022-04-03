/* date = December 5th 2021 11:36 am */

#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

typedef struct SDLTextureAsset
{
    bool loaded;
    
    SDL_Texture *texture;
    int width;
    int height;
    int tile_w;
    int tile_h;
} SDLTextureAsset;

typedef struct SDLApp
{
    bool is_running;
    bool fullscreen;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDLTextureAsset textures[TextureID_COUNT];
} SDLApp;

typedef struct timespec timespec;
typedef struct LinuxGameCode
{
    void *game_code_dll;
    timespec last_dll_write_time;
    
    GameUpdateAndRenderType *game_update_and_render;
    
    bool is_valid;
} LinuxGameCode;

typedef struct LinuxState
{
    size_t total_size;
    void *game_memory_block;
    
    char exe_path[PATH_MAX];
    char *one_past_last_exe_path_slash;
} LinuxState;

typedef enum SDLSourceRectType
{
    SourceRectType_None,
    SourceRectType_Whole,
    SourceRectType_Rect,
    SourceRectType_TileIndex,
    SourceRectType_TileCoord,
} SDLSourceRectType;

typedef union SDLSourceRect
{
    struct { int x,          y,         w,         h; };
    struct { int tile_x,     tile_y,    _ignored0, _ignored1; };
    struct { int tile_index, _ignored2, _ignored3, _ignored4; };
} SDLSourceRect;

#endif //LINUX_PLATFORM_H
