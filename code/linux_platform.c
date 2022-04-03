// helpers and platform independent headers
#include "base.h"
#include "nisk_math.h"

#include "networking.h"
#include "nisk_platform.h"

// libraries
#include "SDL.h"
#include "SDL_image.h"

// platform specific includes
#include <time.h>         // timespec
#include <sys/mman.h>     // mmap
#include <sys/stat.h>     // file info
#include <linux/limits.h> // MAX_PATH 
#include <dlfcn.h>        // dlopen, dlsym, dlclose

#include "linux_platform.h"

// networking
#include <netinet/in.h>   // sockaddr_in
#include <arpa/inet.h>    // inet_addr
#include <fcntl.h>        // set file/socket handle to non-blocking
#include <unistd.h>       // write(), close()
#include <errno.h>        // socket error handling

#include "linux_networking.c"

#include <stdio.h>

global SDLApp global_app;

#define MAX_CONTROLLERS 4
global SDL_GameController *global_controller_handles[MAX_CONTROLLERS];

internal bool LinuxTimespecsAreTheSame(timespec a, timespec b)
{
    bool result = false;
    
    result = (a.tv_sec == b.tv_sec
              && a.tv_nsec == b.tv_nsec);
    
    return result;
}

internal void LinuxInitEXEPath(LinuxState *state)
{
    int bytes_read = (int)readlink("/proc/self/exe", state->exe_path, PATH_MAX);
    ASSERT(bytes_read > 0);
    
    state->one_past_last_exe_path_slash = state->exe_path;
    for(int offset = 0;
        *(state->exe_path + offset) && offset < bytes_read;
        ++offset)
    {
        if(*(state->exe_path+offset) == '/')
        {
            state->one_past_last_exe_path_slash = state->exe_path + offset + 1;
        }
    }
}

internal void LinuxAppendToParentDirectoryPath(LinuxState *state, char *filename, char *dest, size_t dest_count)
{
    long int path_count = (state->one_past_last_exe_path_slash - state->exe_path);
    ASSERT(path_count > 0);
    ConcatenateStrings(state->exe_path, (size_t)path_count,
                       filename, StringLength(filename),
                       dest, dest_count);
}

internal timespec LinuxGetLastWriteTime(char *filename)
{
    struct timespec result;
    struct stat file_stat;
    if(stat(filename, &file_stat))
    {
        ASSERT("Error getting file stat" != 0);
    }
    result = file_stat.st_mtim;
    return result;
}

internal float LinuxGetSecondsElapsed(timespec start, timespec end)
{
    long int sec_diff = end.tv_sec - start.tv_sec;
    long int nsec_diff = end.tv_nsec - start.tv_nsec;
    float result = (float)sec_diff + (float)nsec_diff * (float)1e-9;
    return result;
}

internal LinuxGameCode LinuxLoadGameCode(char *source_dll_name)
{
    LinuxGameCode result = {0};
    result.last_dll_write_time = LinuxGetLastWriteTime(source_dll_name);
    result.game_code_dll = dlopen(source_dll_name, RTLD_NOW);
    if(result.game_code_dll)
    {
        result.game_update_and_render = (GameUpdateAndRenderType *)
            dlsym(result.game_code_dll, "GameUpdateAndRender");
        
        result.is_valid = !!(result.game_update_and_render);
    }
    return result;
}

internal void LinuxUnloadGameCode(LinuxGameCode *game_code)
{
    if(game_code->game_code_dll)
    {
        dlclose(game_code->game_code_dll);
        game_code->game_code_dll = 0;
    }
    game_code->game_update_and_render = 0;
    game_code->is_valid = false;
}

internal void SDLHandleEvent(Input *input, SDL_Event *event)
{
    switch(event->type)
    {
        case SDL_QUIT:
        {
            global_app.is_running = false;
        } break;
        
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_Keycode key_code = event->key.keysym.sym;
            uint16_t mod = event->key.keysym.mod;
            bool is_down = (event->key.state == SDL_PRESSED);
            
            bool lshift_key_is_down = (mod & KMOD_LSHIFT);
            bool ctrl_key_is_down = (mod & KMOD_CTRL);
            bool alt_key_is_down = (mod & KMOD_ALT);
            
            if(event->key.repeat == 0)
            {
                InputDevice *keyboard = &input->input_devices[InputDevice_Keyboard];
                
#define SDL_PROCESS_KEYBOARD_MESSAGE(character, pad_key) case character: keyboard->keys_down[pad_key] = is_down; break;
                switch(key_code)
                {
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_w,      Input_DPadUp);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_a,      Input_DPadLeft);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_s,      Input_DPadDown);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_d,      Input_DPadRight);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_r,      Input_ActionUp);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_f,      Input_ActionLeft);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_e,      Input_ActionDown);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_q,      Input_ActionRight);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_TAB,    Input_Select);
                    SDL_PROCESS_KEYBOARD_MESSAGE(SDLK_ESCAPE, Input_Start);
                    default: break;
                }
                
#undef SDL_PROCESS_KEYBOARD_MESSAGE
                
                
                keyboard->keys_down[Input_BumperLeft] = ctrl_key_is_down;
                keyboard->keys_down[Input_BumperRight] = lshift_key_is_down;
            }
            
            if(event->type == SDL_KEYDOWN)
            {
                if((key_code == SDLK_RETURN && alt_key_is_down)
                   || (key_code == SDLK_F11))
                {
                    global_app.fullscreen = !global_app.fullscreen;
                    unsigned int flags = global_app.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
                    SDL_SetWindowFullscreen(global_app.window, flags);
                }
                
                if(key_code == SDLK_F4 && alt_key_is_down)
                    global_app.is_running = false;
            }
        } break;
    }
}

void SDLOpenGameControllers(void)
{
    int max_joysticks = SDL_NumJoysticks();
    int controller_idx = 0;
    for(int joystick_idx = 0;
        (joystick_idx < max_joysticks
         && joystick_idx < MAX_CONTROLLERS);
        ++joystick_idx)
    {
        if(!SDL_IsGameController(joystick_idx))
            continue;
        
        global_controller_handles[controller_idx] = SDL_GameControllerOpen(joystick_idx);
        ++controller_idx;
    }
}

void SDLCloseGameControllers(void)
{
    for(int controller_idx = 0;
        controller_idx < MAX_CONTROLLERS;
        ++controller_idx)
    {
        SDL_GameControllerClose(global_controller_handles[controller_idx]);
    }
}

float SDLProcessGameControllerAxisValue(long value, long deadzone)
{
    float result = 0;
    if(value < - deadzone)
        result = (float)(value + deadzone) / (float)(32768 - deadzone);
    else if(value > deadzone)
        result = (float)(value - deadzone) / (float)(32767 - deadzone);
    return result;
}

float SDLGetSecondsElapsed(long unsigned int start_counter, long unsigned int end_counter)
{
    long unsigned int counter_elapsed = end_counter - start_counter;
    float result = (float)counter_elapsed / (float)SDL_GetPerformanceFrequency();
    return result;
}


void SDLLoadTextureEx(TextureID texture_id, char *filename, int opt_tile_w, int opt_tile_h)
{
    ASSERT(texture_id >= 0 && texture_id < TextureID_COUNT);
    SDLTextureAsset *asset = global_app.textures + texture_id;
    asset->texture = IMG_LoadTexture(global_app.renderer, filename);
    if(asset->texture == 0)
    {
        fprintf(stderr, "ERROR: Could not load texture: %s\n", filename);
        return;
    }
    
    asset->loaded = true;
    SDL_QueryTexture(asset->texture, 0, 0, &asset->width, &asset->height);
    
    if(opt_tile_w > 0 || opt_tile_h > 0)
    {
        ASSERT(opt_tile_w > 0 && opt_tile_h > 0);
        ASSERT(asset->width % opt_tile_w == 0
               && asset->height % opt_tile_h == 0);
        
        asset->tile_w = opt_tile_w;
        asset->tile_h = opt_tile_h;
    }
}

void SDLLoadTexture(TextureID texture_id, char *filename)
{
    SDLLoadTextureEx(texture_id, filename, 0, 0);
}

void SDLBlitTextureEx(TextureID texture_id,
                      SDLSourceRectType source_rect_type, SDLSourceRect source_rect,
                      int x0, int y0, int x1, int y1, bool flip_h)
{
    ASSERT(texture_id >= 0 && texture_id < TextureID_COUNT);
    ASSERT(x1 >= x0 && y0 >= y1);
    
    
    SDLTextureAsset *asset = global_app.textures + texture_id;
    if(asset == 0 || asset->loaded == false)
    {
        INVALID_CODE_PATH;
        return;
    }
    
    SDL_Rect sdl_srcrect;
    switch(source_rect_type)
    {
        case SourceRectType_Whole:
        {
            sdl_srcrect.x = sdl_srcrect.y = 0;
            sdl_srcrect.w = asset->width;
            sdl_srcrect.h = asset->height;
        } break;
        
        case SourceRectType_Rect:
        {
            sdl_srcrect = (SDL_Rect){ source_rect.x, source_rect.y, source_rect.w, source_rect.h };
        } break;
        
        case SourceRectType_TileIndex:
        {
            int width_in_tiles = asset->width / asset->tile_w;
            int tile_x = source_rect.tile_index % width_in_tiles;
            int tile_y = source_rect.tile_index / width_in_tiles;
            sdl_srcrect.x = tile_x * asset->tile_w;
            sdl_srcrect.y = tile_y * asset->tile_h;
            sdl_srcrect.w = asset->tile_w;
            sdl_srcrect.h = asset->tile_h;
        } break;
        
        case SourceRectType_TileCoord:
        {
            sdl_srcrect.x = source_rect.tile_x * asset->tile_w;
            sdl_srcrect.y = source_rect.tile_y * asset->tile_h;
            sdl_srcrect.w = asset->tile_w;
            sdl_srcrect.h = asset->tile_h;
        } break;
        
        default:
        {
            INVALID_CODE_PATH;
            return;
        } break;
    }
    
    ASSERT(sdl_srcrect.x >= 0 && sdl_srcrect.x + sdl_srcrect.w <= asset->width
           && sdl_srcrect.y >= 0 && sdl_srcrect.y + sdl_srcrect.h <= asset->height);
    
    SDL_Rect sdl_dstrect = { x0, y1, x1-x0, y0-y1 }; 
    SDL_RendererFlip flip = (flip_h ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    SDL_RenderCopyEx(global_app.renderer, asset->texture, &sdl_srcrect, &sdl_dstrect, 0, 0, flip);
}

void SDLBlitTexture(TextureID texture_id, int x0, int y0, int x1, int y1)
{
    SDLSourceRect srcrect = {0};
    SDLBlitTextureEx(texture_id, SourceRectType_Whole, srcrect, x0, y0, x1, y1, false);
}

void SDLBlitTextureRect(TextureID texture_id,
                        int r_x0, int r_y0, int r_x1, int r_y1,
                        int x0, int y0, int x1, int y1, bool flip_h)
{
    SDLSourceRect srcrect;
    srcrect.x = r_x0;
    srcrect.y = r_y0;
    srcrect.w = r_x1 - r_x0;
    srcrect.h = r_y1 - r_y0;
    SDLBlitTextureEx(texture_id, SourceRectType_Rect, srcrect, x0, y0, x1, y1, flip_h);
}

void SDLBlitTextureTileIndex(TextureID texture_id, int idx, int x0, int y0, int x1, int y1, bool flip_h)
{
    SDLSourceRect srcrect;
    srcrect.tile_index = idx;
    SDLBlitTextureEx(texture_id, SourceRectType_TileIndex, srcrect, x0, y0, x1, y1, flip_h);
}

void SDLBlitTextureTileCoord(TextureID texture_id, int t_x, int t_y, int x0, int y0, int x1, int y1, bool flip_h)
{
    SDLSourceRect srcrect;
    srcrect.tile_x = t_x;
    srcrect.tile_y = t_y;
    SDLBlitTextureEx(texture_id, SourceRectType_TileCoord, srcrect, x0, y0, x1, y1, flip_h);
}

void SDLRenderFillRect(int x0, int y0, int x1, int y1,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    ASSERT(x1 >= x0 && y0 >= y1);
    SDL_SetRenderDrawColor(global_app.renderer, r, g, b, a);
    SDL_Rect rect = { x0, y1, x1-x0, y0-y1 };
    SDL_RenderFillRect(global_app.renderer, &rect);
}

unsigned int LinuxInetAddrWrap(char *addr)
{
    unsigned int result = ntohl(inet_addr(addr));
    return result;
}

int main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    IMG_Init(IMG_INIT_PNG);
    
    LinuxState linux_state = {0};
    LinuxInitEXEPath(&linux_state);
    
    char source_game_code_dll_path[PATH_MAX];
    LinuxAppendToParentDirectoryPath(&linux_state, "nisk.so", source_game_code_dll_path, PATH_MAX);
    LinuxGameCode game_code = LinuxLoadGameCode(source_game_code_dll_path);
    
    unsigned int window_flags = SDL_WINDOW_RESIZABLE;
    if(global_app.fullscreen)
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    
    global_app.window = SDL_CreateWindow("MP Client",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                         960,
                                         540,
                                         window_flags);
    
    SDLOpenGameControllers();
    
    if(global_app.window)
    {
        
        uint renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
        global_app.renderer = SDL_CreateRenderer(global_app.window, -1, renderer_flags);
        
        if(global_app.renderer)
        {
            global_app.is_running = true;
            
            int game_update_hz = 60;
            float target_seconds_per_frame = 1.0f / (float)game_update_hz;
            
#if NISK_DEBUG
            // WARNING! This will fail on 32-bit
            void *base_address = (void *)TERABYTES(2);
#else
            void *base_address = 0;
#endif
            
            GameMemory game_memory = {0};
            game_memory.permanent_storage_size = MEGABYTES(64);
            game_memory.transient_storage_size = MEGABYTES(256);
            
            size_t total_storage_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            linux_state.total_size = total_storage_size;
            linux_state.game_memory_block = mmap(base_address, total_storage_size,
                                                 PROT_READ | PROT_WRITE,
                                                 MAP_ANON | MAP_PRIVATE,
                                                 -1, 0);
            ASSERT(linux_state.game_memory_block != MAP_FAILED);
            
            game_memory.permanent_storage = linux_state.game_memory_block;
            game_memory.transient_storage = ((uint8_t *)(linux_state.game_memory_block) +
                                             game_memory.permanent_storage_size); 
            
            Platform platform;
            platform.load_texture_ex         = &SDLLoadTextureEx;
            platform.load_texture            = &SDLLoadTexture;
            platform.blit_texture            = &SDLBlitTexture;
            platform.blit_texture_rect       = &SDLBlitTextureRect;
            platform.blit_texture_tile_index = &SDLBlitTextureTileIndex;
            platform.blit_texture_tile_coord = &SDLBlitTextureTileCoord;
            platform.render_fill_rect        = &SDLRenderFillRect;
            platform.socket_create           = &SocketCreate;
            platform.socket_close            = &SocketClose;
            platform.socket_send             = &SocketSend;
            platform.socket_recieve          = &SocketRecieve;
            platform.inet_addr_wrap          = &LinuxInetAddrWrap;
            
            Input input = {0};
            input.argc = argc;
            input.argv = argv;
            input.dt   = target_seconds_per_frame;
            
            InputDevice *controller = &input.input_devices[InputDevice_Controller];
            
            size_t last_dll_reload_check = SDL_GetPerformanceCounter();
            size_t last_counter = SDL_GetPerformanceCounter();
            float dll_reload_check_delay = 1.0f;
            
            while(global_app.is_running)
            {
                float time_since_reload_check = SDLGetSecondsElapsed(last_dll_reload_check, last_counter);
                if(time_since_reload_check > dll_reload_check_delay)
                {
                    timespec new_dll_write_time = LinuxGetLastWriteTime(source_game_code_dll_path);
                    
                    if(!LinuxTimespecsAreTheSame(game_code.last_dll_write_time, new_dll_write_time))
                    {
                        LinuxUnloadGameCode(&game_code);
                        game_code = LinuxLoadGameCode(source_game_code_dll_path);
                    }
                    
                    last_dll_reload_check = SDL_GetPerformanceCounter();
                }
                
                MEMORY_SET(controller->keys_down, 0, sizeof(controller->keys_down));
                controller->axis_x = 0.0f;
                controller->axis_y = 0.0f;
                
                SDL_Event event;
                while(SDL_PollEvent(&event))
                    SDLHandleEvent(&input, &event);
                SDL_GetWindowSize(global_app.window,
                                  &input.screen_size.x,
                                  &input.screen_size.y);
                
                for(int controller_idx = 0;
                    controller_idx < MAX_CONTROLLERS;
                    ++controller_idx)
                {
                    if(global_controller_handles[controller_idx] == 0
                       || !SDL_GameControllerGetAttached(global_controller_handles[controller_idx]))
                        continue;
                    
#define SDL_PROCESS_CONTROLLER_KEY(sdl_controller_key, pad_key) controller->keys_down[(pad_key)] |= SDL_GameControllerGetButton(global_controller_handles[(controller_idx)], sdl_controller_key);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_DPAD_UP,    Input_DPadUp);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_DPAD_LEFT,  Input_DPadLeft);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_DPAD_DOWN,  Input_DPadDown);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, Input_DPadRight);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_Y,          Input_ActionUp);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_X,          Input_ActionLeft);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_A,          Input_ActionDown);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_B,          Input_ActionRight);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_BACK,       Input_Select);
                    SDL_PROCESS_CONTROLLER_KEY(SDL_CONTROLLER_BUTTON_START,      Input_Start);
#undef SDL_PROCESS_CONTROLLER_KEY
                    
                    long raw_axis_x = SDL_GameControllerGetAxis(global_controller_handles[controller_idx],
                                                                SDL_CONTROLLER_AXIS_LEFTX);
                    float new_axis_x = SDLProcessGameControllerAxisValue(raw_axis_x, 7000);
                    controller->axis_x += new_axis_x;
                    
                    long raw_axis_y = SDL_GameControllerGetAxis(global_controller_handles[controller_idx],
                                                                SDL_CONTROLLER_AXIS_LEFTY);
                    float new_axis_y = -SDLProcessGameControllerAxisValue(raw_axis_y, 7000);
                    controller->axis_y += new_axis_y;
                    
                }
                
                controller->axis_x = CLAMP(-1.0f, controller->axis_x, 1.0f);
                controller->axis_y = CLAMP(-1.0f, controller->axis_y, 1.0f);
                
                SDL_SetRenderDrawColor(global_app.renderer, 0, 0, 0, 255);
                SDL_RenderClear(global_app.renderer);
                
                if(game_code.game_update_and_render)
                    game_code.game_update_and_render(&game_memory,
                                                     &platform,
                                                     &input,
                                                     &global_app.is_running);
                
                
                
                unsigned long int work_counter = SDL_GetPerformanceCounter();
                float work_in_seconds = SDLGetSecondsElapsed(last_counter, work_counter);
                
                if(work_in_seconds < target_seconds_per_frame)
                {
                    float time_to_sleep_in_seconds = target_seconds_per_frame - work_in_seconds;
                    unsigned int time_to_sleep_in_ms = (unsigned int)(time_to_sleep_in_seconds * 1000) + 1;
                    SDL_Delay(time_to_sleep_in_ms);
                }
                last_counter = SDL_GetPerformanceCounter();
                
                SDL_RenderPresent(global_app.renderer);
            }
        }
        else
        {
            fprintf(stderr, "SDL_CreateRenderer(): %s\n", SDL_GetError());
        }
    }
    else
    {
        fprintf(stderr, "SDL_CreateWindow(): %s\n", SDL_GetError());
    }
    
    LinuxUnloadGameCode(&game_code);
    SDLCloseGameControllers();
    
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}