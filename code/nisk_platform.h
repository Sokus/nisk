/* date = December 5th 2021 11:14 am */

#ifndef NISK_PLATFORM_H
#define NISK_PLATFORM_H

typedef enum InputKey
{
    Input_DPadUp,      // W
    Input_DPadLeft,    // A
    Input_DPadDown,    // S
    Input_DPadRight,   // D
    Input_ActionUp,    // R
    Input_ActionLeft,  // F
    Input_ActionDown,  // E      // Confirm
    Input_ActionRight, // Q      // Cancel
    Input_BumperLeft,  // Ctrl
    Input_BumperRight, // Shift
    Input_Select,      // Tab
    Input_Start,       // Escape
    Input_MouseLeft,
    Input_MouseRight,
    Input_MouseMiddle,
    Input_MouseM4,
    Input_MouseM5,
    Input_COUNT
} InputKey;

char *InputKeyName(int value)
{
    ASSERT(value >= 0 && value < Input_COUNT);
    switch(value)
    {
        case Input_DPadUp:      return "Input_DPadUp";
        case Input_DPadLeft:    return "Input_DPadLeft";
        case Input_DPadDown:    return "Input_DPadDown";
        case Input_DPadRight:   return "Input_DPadRight";
        case Input_ActionUp:    return "Input_ActionUp";
        case Input_ActionLeft:  return "Input_ActionLeft";
        case Input_ActionDown:  return "Input_ActionDown";
        case Input_ActionRight: return "Input_ActionRight";
        case Input_BumperLeft:  return "Input_BumperLeft";
        case Input_BumperRight: return "Input_BumperRight";
        case Input_Select:      return "Input_Select";
        case Input_Start:       return "Input_Start";
        case Input_MouseLeft:   return "Input_MouseLeft";
        case Input_MouseRight:  return "Input_MouseRight";
        case Input_MouseMiddle: return "Input_MouseMiddle";
        case Input_MouseM4:     return "Input_MouseM4";
        case Input_MouseM5:     return "Input_MouseM5";
        default:                return "Input_Unknown";
    }
}

typedef enum TextureID
{
    TextureID_Atlas,
    
    TextureID_COUNT
} TextureID;


// platform functions used in the game layer
typedef void LoadTextureExType(TextureID texture_id, char *filename, int opt_tile_w, int opt_tile_h);
typedef void LoadTextureType(TextureID texture_id, char *filename);
typedef void BlitTextureType(TextureID texture_id, int x0, int y0, int x1, int y1);
typedef void BlitTextureRectType(TextureID texture_id,
                                 int r_x0, int r_y0, int r_x1, int r_y1,
                                 int x0, int y0, int x1, int y1, bool flip_h);
typedef void BlitTextureTileIndexType(TextureID texture_id, int idx, int x0, int y0, int x1, int y1, bool flip_h);
typedef void BlitTextureTileCoordType(TextureID texture_id, int t_x, int t_y,
                                      int x0, int y0, int x1, int y1, bool flip_h);
typedef void RenderFillRectType(int x0, int y0, int x1, int y1,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a);
typedef bool SocketCreateType(int *sockfd);
typedef bool SocketCloseType(int sockfd);
typedef bool SocketSendType(int sockfd, Address *destination, void *data, int size);
typedef int SocketRecieveType(int sockfd, Address *sender, void *data, int size);

typedef unsigned int InetAddrWrapType(char *addr);

typedef struct Platform
{
    LoadTextureExType        *load_texture_ex;
    LoadTextureType          *load_texture;
    BlitTextureType          *blit_texture;
    BlitTextureRectType      *blit_texture_rect;
    BlitTextureTileIndexType *blit_texture_tile_index;
    BlitTextureTileCoordType *blit_texture_tile_coord;
    RenderFillRectType       *render_fill_rect;
    SocketCreateType         *socket_create;
    SocketCloseType          *socket_close;
    SocketSendType           *socket_send;
    SocketRecieveType        *socket_recieve;
    InetAddrWrapType         *inet_addr_wrap;
} Platform;

typedef struct ServerInfo
{
    char *nickname;
    Address address;
} ServerInfo;

typedef enum InputDeviceType
{
    InputDevice_Keyboard,
    InputDevice_Controller,
    
    InputDevice_Count
} InputDeviceType;

typedef struct InputDevice
{
    bool is_active;
    bool is_analog;
    
    bool keys_down[Input_COUNT];
    float keys_down_duration[Input_COUNT];
    float keys_down_duration_previous[Input_COUNT];
    
    float axis_x;
    float axis_y;
} InputDevice;

typedef struct Input
{
    InputDevice input_devices[InputDevice_Count];
    
    Vec2i screen_size;
    
    int argc;
    char **argv;
    
    float dt;
} Input;

typedef struct GameMemory
{
    bool is_initialized;
    
    size_t permanent_storage_size;
    void  *permanent_storage;
    
    size_t transient_storage_size;
    void  *transient_storage;
} GameMemory;

// game functions used in the platform layer
typedef void GameUpdateAndRenderType(GameMemory *memory,
                                     Platform *platform,
                                     Input *input,
                                     bool *running);

#endif //NISK_PLATFORM_H
