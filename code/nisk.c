#include "base.h"
#include "nisk_math.h"

#include "networking.h"
#include "nisk.h"

#include "nisk_platform.h"

#include <stdio.h>

Position PositionOffset(Position position,
                        int offset_unit_x, int offset_unit_y,
                        float offset_rem_x, float offset_rem_y)
{
    Position result;
    float rem_x = position.rem.x + offset_rem_x;
    float rem_y = position.rem.y + offset_rem_y;
    result.unit.x = position.unit.x + offset_unit_x + ROUNDF(rem_x);
    result.unit.y = position.unit.y + offset_unit_y + ROUNDF(rem_y);
    result.rem.x = position.rem.x + offset_rem_x - ROUNDF(rem_x);
    result.rem.y = position.rem.y + offset_rem_y - ROUNDF(rem_y);
    return result;
}

Vec2 ProjectGlobalToView(Vec2i point, Position camera_p)
{
    Vec2 result;
    result.x = (float)(point.x - camera_p.unit.x) - camera_p.rem.x;
    result.y = (float)(point.y - camera_p.unit.y) - camera_p.rem.y;
    return result;
}

Vec2 ProjectViewToClip(Vec2 point, Vec2i screen_size, float units_to_pixels)
{
    Vec2 point_in_pixels = Vec2Scale(point, units_to_pixels);
    Vec2 result;
    result.x = point_in_pixels.x / (float)screen_size.x;
    result.y = point_in_pixels.y / (float)screen_size.y;
    return result;
}

Vec2 ProjectClipToScreen(Vec2 point, Vec2i screen_size)
{
    Vec2 point_01, result;
    point_01.x = (point.x / 2.0f) + 0.5f;
    point_01.y = (point.y / 2.0f) + 0.5f;
    result.x = point_01.x * (float)screen_size.x;
    result.y = (1 - point_01.y) * (float)screen_size.y;
    return result;
}

Vec2 ProjectGlobalToScreen(Vec2i point, Position camera_p, Vec2i screen_size, float units_to_pixels)
{
    Vec2 view = ProjectGlobalToView(point, camera_p);
    Vec2 clip = ProjectViewToClip(view, screen_size, units_to_pixels);
    Vec2 screen = ProjectClipToScreen(clip, screen_size);
    return screen;
}

Recti RectiMove(Recti recti, int move_x, int move_y)
{
    Recti result;
    result.x0 = recti.x0 + move_x;
    result.y0 = recti.y0 + move_y;
    result.x1 = recti.x1 + move_x;
    result.y1 = recti.y1 + move_y;
    return result;
}

Recti RectiAbs(int pos_x, int pos_y, int width, int height)
{
    Recti result = { pos_x, pos_y, pos_x + width, pos_y + height };
    return result;
}

bool RectiCheckOverlap(Recti a, Recti b)
{
    if(a.p1.x <= b.p0.x || b.p1.x <= a.p0.x)
        return false;
    
    if(a.p1.y <= b.p0.y || b.p1.y <= a.p0.y)
        return false;
    
    return true;
}

Recti ObstacleGetMeters(int pos_x, int pos_y, int width, int height, int meters_to_units)
{
    Recti result;
    result.p0.x = pos_x * meters_to_units;
    result.p0.y = pos_y * meters_to_units;
    result.p1.x = (pos_x + width) * meters_to_units;
    result.p1.y = (pos_y + height) * meters_to_units;
    return result;
}

typedef enum ObstacleLayout
{
    ObstacleLayout_Invalid,
    ObstacleLayout_Big,
    ObstacleLayout_Horizontal,
    ObstacleLayout_Vertical,
    ObstacleLayout_Dot
} ObstacleLayout;

void ObstacleRender(GameMemory *memory, Platform *platform, Vec2i screen_size,
                    Recti *obstacle, Position camera_p, int meters_to_units, int units_to_pixels)
{
    int width_in_tiles = (obstacle->p1.x - obstacle->p0.x) / meters_to_units;
    int height_in_tiles = (obstacle->p1.y - obstacle->p0.y) / meters_to_units;
    ASSERT(width_in_tiles > 0 && height_in_tiles > 0);
    
    bool wide = (width_in_tiles > 1);
    bool high = (height_in_tiles > 1);
    ObstacleLayout obstacle_layout = (wide && high ? ObstacleLayout_Big        :
                                      wide         ? ObstacleLayout_Horizontal :
                                      high         ? ObstacleLayout_Vertical   :
                                      ObstacleLayout_Dot);
    
#if 1
    int coord_y = 4; // blue obstacles
#else
    int coord_y = 5; // brown obstacles
#endif
    switch(obstacle_layout)
    {
        case ObstacleLayout_Big:
        {
            for(int tile_y=0; tile_y < height_in_tiles; ++tile_y)
            {
                int coord_offset_x0;
                if(tile_y == (height_in_tiles - 1)) coord_offset_x0 = 0;
                else if(tile_y > 0)                 coord_offset_x0 = 3;
                else                                coord_offset_x0 = 6;
                
                for(int tile_x=0; tile_x < width_in_tiles; ++tile_x)
                {
                    int coord_offset_x1;
                    if(tile_x == (width_in_tiles - 1)) coord_offset_x1 = 2;
                    else if(tile_x > 0)                coord_offset_x1 = 1;
                    else                               coord_offset_x1 = 0;
                    
                    Vec2i tile_p0, tile_p1;
                    tile_p0.x = obstacle->p0.x + tile_x * meters_to_units;
                    tile_p0.y = obstacle->p0.y + tile_y * meters_to_units;
                    tile_p1.x = tile_p0.x + meters_to_units;
                    tile_p1.y = tile_p0.y + meters_to_units;
                    
                    Vec2 p0 = ProjectGlobalToScreen(tile_p0, camera_p,
                                                    screen_size, units_to_pixels);
                    Vec2 p1 = ProjectGlobalToScreen(tile_p1, camera_p,
                                                    screen_size, units_to_pixels);
                    
                    int coord_x = coord_offset_x0 + coord_offset_x1;
                    platform->blit_texture_tile_coord(TextureID_Atlas, coord_x, coord_y,
                                                      p0.x, p0.y, p1.x, p1.y, false);
                }
            }
        } break;
        
        case ObstacleLayout_Horizontal:
        {
            for(int tile_x=0; tile_x < width_in_tiles; ++tile_x)
            {
                int coord_offset_x0;
                if(tile_x == (width_in_tiles - 1)) coord_offset_x0 = 11;
                else if(tile_x > 0)                coord_offset_x0 = 10;
                else                               coord_offset_x0 = 9;
                
                Vec2i tile_p0, tile_p1;
                tile_p0.x = obstacle->p0.x + tile_x * meters_to_units;
                tile_p0.y = obstacle->p0.y;
                tile_p1.x = tile_p0.x + meters_to_units;
                tile_p1.y = tile_p0.y + meters_to_units;
                
                Vec2 p0 = ProjectGlobalToScreen(tile_p0, camera_p,
                                                screen_size, units_to_pixels);
                Vec2 p1 = ProjectGlobalToScreen(tile_p1, camera_p,
                                                screen_size, units_to_pixels);
                
                int coord_x = coord_offset_x0;
                platform->blit_texture_tile_coord(TextureID_Atlas, coord_x, coord_y,
                                                  p0.x, p0.y, p1.x, p1.y, false);
            }
        } break;
        
        case ObstacleLayout_Vertical:
        {
            for(int tile_y=0; tile_y < height_in_tiles; ++tile_y)
            {
                int coord_offset_x0;
                if(tile_y == (height_in_tiles - 1)) coord_offset_x0 = 12;
                else if(tile_y > 0)                 coord_offset_x0 = 13;
                else                                coord_offset_x0 = 14;
                
                Vec2i tile_p0, tile_p1;
                tile_p0.x = obstacle->p0.x;
                tile_p0.y = obstacle->p0.y + tile_y * meters_to_units;
                tile_p1.x = tile_p0.x + meters_to_units;
                tile_p1.y = tile_p0.y + meters_to_units;
                
                Vec2 p0 = ProjectGlobalToScreen(tile_p0, camera_p,
                                                screen_size, units_to_pixels);
                Vec2 p1 = ProjectGlobalToScreen(tile_p1, camera_p,
                                                screen_size, units_to_pixels);
                
                int coord_x = coord_offset_x0;
                platform->blit_texture_tile_coord(TextureID_Atlas, coord_x, coord_y,
                                                  p0.x, p0.y, p1.x, p1.y, false);
            }
        } break;
        
        case ObstacleLayout_Dot:
        {
            Vec2i tile_p0, tile_p1;
            tile_p0.x = obstacle->p0.x;
            tile_p0.y = obstacle->p0.y;
            tile_p1.x = tile_p0.x + meters_to_units;
            tile_p1.y = tile_p0.y + meters_to_units;
            
            Vec2 p0 = ProjectGlobalToScreen(tile_p0, camera_p,
                                            screen_size, units_to_pixels);
            Vec2 p1 = ProjectGlobalToScreen(tile_p1, camera_p,
                                            screen_size, units_to_pixels);
            
            int coord_x = 15;
            platform->blit_texture_tile_coord(TextureID_Atlas, coord_x, coord_y,
                                              p0.x, p0.y, p1.x, p1.y, false);
        } break;
        
        default: break;
    }
}

PhysicsSpec PhysicsSpecDefault()
{
    PhysicsSpec result;
    result.meters_to_units = 8;
    
    result.run_speed = 12;
    result.run_accel = 128;
    
    result.fall_speed = 16;
    result.fall_accel = 96;
    result.air_inertia = 0.3f;
    
    result.jump_speed = 16;
    result.jump_time = 0.5f;
    result.jump_gravity_mult = 0.5f;
    
    result.spell_speed = 20;
    result.spell_time  = 0.4f;
    return result;
}

void PlayerRender(Platform *platform, Vec2i screen_size,
                  Entity *player, Position camera_p, int meters_to_units, int units_to_pixels)
{
    Recti texture_rect = RectiMove(player->texture_rect,
                                   player->p.unit.x,
                                   player->p.unit.y);
    
    Vec2 screen_player_p0 = ProjectGlobalToScreen(texture_rect.p0, camera_p,
                                                  screen_size, units_to_pixels);
    Vec2 screen_player_p1 = ProjectGlobalToScreen(texture_rect.p1, camera_p,
                                                  screen_size, units_to_pixels);
    
    platform->blit_texture_tile_coord(TextureID_Atlas, 0, 1,
                                      screen_player_p0.x, screen_player_p0.y,
                                      screen_player_p1.x, screen_player_p1.y,
                                      player->direction < 0);
}

void PlayerUpdate(GameData *game_data, Entity *player, Input *input, PhysicsSpec *physics_spec, float dt)
{
    InputDevice *keyboard = &input->input_devices[InputDevice_Keyboard];
    InputDevice *controller = &input->input_devices[InputDevice_Controller];
    
    float move_x = (-keyboard->keys_down[Input_DPadLeft]
                    + keyboard->keys_down[Input_DPadRight]
                    - controller->keys_down[Input_DPadLeft]
                    + controller->keys_down[Input_DPadRight]
                    + controller->axis_x);
    move_x = CLAMP(-1.0f, move_x, 1.0f);
    
    bool jump_key_down = (keyboard->keys_down[Input_DPadUp]
                          || controller->keys_down[Input_ActionDown]);
    bool spell_key_down = (keyboard->keys_down[Input_BumperRight]
                           || controller->keys_down[Input_ActionRight]);
    
    if(move_x != 0)
        player->direction = SIGN(move_x);
    
    bool on_ground = false;
    {
        Recti hitbox = RectiMove(player->hitbox,
                                 player->p.unit.x,
                                 player->p.unit.y - 1);
        for(unsigned int idx=0; idx < game_data->obstacle_count; ++idx)
        {
            Recti *obstacle = game_data->obstacles + idx;
            if(RectiCheckOverlap(*obstacle, hitbox))
            {
                on_ground = true;;
                break;
            }
        }
    }
    
    // Strafe
    {
        float mult = (on_ground ? 1 : physics_spec->air_inertia);
        player->v.x = Apporach(player->v.x,
                               move_x * physics_spec->run_speed,
                               mult * physics_spec->run_accel * dt);
    }
    
    if(on_ground)
    {
        if(jump_key_down)
        {
            player->jump_timer = physics_spec->jump_time;
            player->v.y = physics_spec->jump_speed;
        }
    }
    else
    {
        if(player->jump_timer > 0)
        {
            if(jump_key_down)
                player->jump_timer -= dt;
            else
                player->jump_timer = 0;
        }
    }
    
    if(!on_ground)
    {
        float mult = (player->jump_timer > 0 ? physics_spec->jump_gravity_mult : 1);
        player->v.y = Apporach(player->v.y,
                               -physics_spec->fall_speed,
                               mult * physics_spec->fall_accel * dt);
    }
    
    // MOVE X
    {
        float unit_velocity_x = player->v.x * physics_spec->meters_to_units;
        player->p.rem.x += unit_velocity_x * dt;
        int move = ROUNDF(player->p.rem.x);
        
        if(move != 0)
        {
            player->p.rem.x -= move;
            int sign = SIGN(move);
            
            while(move != 0)
            {
                Recti hitbox = RectiMove(player->hitbox,
                                         player->p.unit.x + sign,
                                         player->p.unit.y);
                bool colided = false;
                for(unsigned int idx=0; idx < game_data->obstacle_count; ++idx)
                {
                    Recti *obstacle = game_data->obstacles + idx;
                    if(RectiCheckOverlap(*obstacle, hitbox))
                    {
                        colided = true;
                        break;
                    }
                }
                
                if(!colided)
                {
                    player->p.unit.x += sign;
                    move -= sign;
                }
                else
                {
                    player->v.x = 0;
                    break;
                }
            }
        }
    }
    
    // MOVE Y
    {
        float unit_velocity_y = player->v.y * physics_spec->meters_to_units;
        player->p.rem.y += unit_velocity_y * dt;
        int move = ROUNDF(player->p.rem.y);
        
        if(move != 0)
        {
            player->p.rem.y -= move;
            int sign = SIGN(move);
            
            while(move != 0)
            {
                Recti hitbox = RectiMove(player->hitbox,
                                         player->p.unit.x,
                                         player->p.unit.y + sign);
                bool colided = false;
                for(unsigned int idx=0; idx < game_data->obstacle_count; ++idx)
                {
                    Recti *obstacle = game_data->obstacles + idx;
                    if(RectiCheckOverlap(*obstacle, hitbox))
                    {
                        colided = true;
                        break;
                    }
                }
                
                if(!colided)
                {
                    player->p.unit.y += sign;
                    move -= sign;
                }
                else
                {
                    player->v.y = 0;
                    if(player->jump_timer > 0)
                        player->jump_timer = 0;
                    break;
                }
            }
        }
    }
    
    if(player->spell_timer > 0)
    {
        float spell_velocity = (player->spell_direction
                                * physics_spec->spell_speed
                                * physics_spec->meters_to_units);
        player->spell_p = PositionOffset(player->spell_p, 0, 0, spell_velocity * dt, 0);
        player->spell_timer -= dt;
    }
    else
    {
        if(spell_key_down)
        {
            player->spell_timer = physics_spec->spell_time;
            player->spell_direction = player->direction;
            player->spell_p = player->p;
        }
    }
}

void CameraUpdate(Position *camera_p, Position *target_p, int meters_to_units, float dt)
{
    ASSERT(camera_p != 0 && target_p != 0);
    if(camera_p == 0 || target_p == 0)
        return;
    
    Vec2i unit_diff = Vec2iSubtract(target_p->unit, camera_p->unit);
    Vec2 rem_diff = Vec2Subtract(target_p->rem, camera_p->rem);
    
    float diff_x = (float)unit_diff.x + rem_diff.x;
    float diff_y = (float)unit_diff.y + rem_diff.y;
    
    float rate = 5;
    
    if(ABS(diff_x) > 0.5f)
    {
        float move_x = Lerp(0, diff_x, 1-POWF(2, -dt * rate));
        float camera_rem_x = camera_p->rem.x + move_x;
        int unit_move_x = ROUNDF(camera_rem_x);
        camera_rem_x -= unit_move_x;
        camera_p->rem.x = camera_rem_x;
        camera_p->unit.x += unit_move_x;
    }
    
    if(ABS(diff_y) > 0.5f)
    {
        float move_y = Lerp(0, diff_y, 1-POWF(2, -dt * rate));
        float camera_rem_y = camera_p->rem.y + move_y;
        int unit_move_y = ROUNDF(camera_rem_y);
        camera_rem_y -= unit_move_y;
        camera_p->rem.y = camera_rem_y;
        camera_p->unit.y += unit_move_y;
    }
}

void InitMemory(GameMemory *memory, Platform *platform, Input *input, bool *is_running)
{
    GameData *game_data = (GameData *)memory->permanent_storage;
    Entity *player = &game_data->player;
    PhysicsSpec *physics_spec = &game_data->physics_spec;
    
    bool success = true;
    unsigned int address = 0;
    int port = 0;
    
    if(input->argc >= 4)
    {
        unsigned int nickname_size = StringLength(input->argv[1]);
        MEMORY_COPY(game_data->nickname.str, input->argv[1], nickname_size);
        game_data->nickname.size = nickname_size;
        address = platform->inet_addr_wrap(input->argv[2]);
        sscanf(input->argv[3], "%i", &port);
        
        if((int)address == -1)
        {
            printf("Invalid address: %s\n", input->argv[2]);
            success = false;
        }
        
        if(port <= 0 || port >= 100000)
        {
            printf("Invalid port: %s\n", input->argv[3]);
            success = false;
        }
    }
    else
    {
        printf("Not enough arguments to join a server.\n"
               "Usage:   nisk.out <nickname>   <address>  <port>\n"
               "Example: nisk.out        Bob 192.168.8.76 12345\n");
        success = false;
    }
    
    if(success)
    {
        game_data->server_address = AddressFromInt(address, port);
    }
    else
    {
        *is_running = false;
        return;
    }
    
    platform->load_texture_ex(TextureID_Atlas, "../assets/atlas.png", 8, 8);
    
    InitializeArena(&game_data->arena,
                    (uint8_t *)memory->permanent_storage + sizeof(GameData),
                    0);
    
    *physics_spec = PhysicsSpecDefault();
    
    if(!platform->socket_create(&game_data->socketfd))
        INVALID_CODE_PATH;
    
    player->p.unit = Vec2iGet(0, 30);
    
    int player_w = 8;
    int player_h = 8;
    player->direction = 1;
    player->hitbox =             RectiAbs(-player_w/2, 0, player_w, player_h - 1);
    player->texture_rect =       RectiAbs(-player_w/2, 0, player_w, player_h);
    player->spell_hitbox =       RectiAbs(-player_w/2, 0, player_w, player_h);
    player->spell_texture_rect = RectiAbs(-player_w/2, 0, player_w, player_h);
    
    Recti *obstacle = game_data->obstacles + game_data->obstacle_count;
    *obstacle++ = ObstacleGetMeters(-8, -8, 16, 2, physics_spec->meters_to_units);
    *obstacle++ = ObstacleGetMeters(-6, -5,  4, 1, physics_spec->meters_to_units);
    *obstacle++ = ObstacleGetMeters( 0, -5,  2, 1, physics_spec->meters_to_units);
    *obstacle++ = ObstacleGetMeters( 3, -5,  1, 1, physics_spec->meters_to_units);
    *obstacle++ = ObstacleGetMeters( 5, -5,  1, 2, physics_spec->meters_to_units);
    *obstacle++ = ObstacleGetMeters(-1, -1,  2, 2, physics_spec->meters_to_units);
    game_data->obstacle_count += 6;
}

void DoConnect(GameData *game_data, Platform *platform)
{
    // SEND PACKETS
    {
        unsigned char buffer_out[sizeof(PacketHeader) + sizeof(Nickname)];
        MemoryArena buffer_out_arena;
        InitializeArena(&buffer_out_arena, buffer_out, sizeof(buffer_out));
        PacketHeader *header_out = PUSH_STRUCT(&buffer_out_arena, PacketHeader);
        header_out->protocol = 0;
        header_out->type = CONNECT;
        Nickname *nickname = PUSH_STRUCT(&buffer_out_arena, Nickname);
        MEMORY_COPY(nickname, &game_data->nickname, sizeof(Nickname));
        
        if(!platform->socket_send(game_data->socketfd, &game_data->server_address,
                                  buffer_out_arena.base, buffer_out_arena.used))
        {
            INVALID_CODE_PATH;
        }
    }
    
    unsigned char buffer_in[2048];
    while(true)
    {
        Address sender;
        int bytes_read = platform->socket_recieve(game_data->socketfd, &sender,
                                                  buffer_in, sizeof(buffer_in));
        
        if(bytes_read <= 0) break;
        if(!AddressCompare(game_data->server_address, sender)) continue;
        if(bytes_read < (int)sizeof(PacketHeader)) continue;
        
        MemoryArena buffer_in_arena;
        InitializeArena(&buffer_in_arena, buffer_in, sizeof(buffer_in));
        
        PacketHeader *header_in = PUSH_STRUCT(&buffer_in_arena, PacketHeader);
        
        switch(header_in->type)
        {
            case ACCEPT:
            {
                printf("Connected!\n");
                game_data->connected = true;
            } break;
            
            default: break;
        }
    }
}

void DoUpdateAndRender(GameMemory *memory,
                       Platform *platform,
                       Input *input,
                       bool *is_running)
{
    GameData *game_data = (GameData *)memory->permanent_storage;
    
    Entity *player = &game_data->player;
    Position *camera_p = &game_data->camera_p;
    PhysicsSpec *physics_spec = &game_data->physics_spec;
    float units_to_pixels = 8;
    
    // RECIEVE PACKETS
    unsigned char buffer_in[2048];
    while(true)
    {
        Address sender;
        int bytes_read = platform->socket_recieve(game_data->socketfd, &sender,
                                                  buffer_in, sizeof(buffer_in));
        
        if(bytes_read <= 0) break;
        if(!AddressCompare(game_data->server_address, sender)) continue;
        if(bytes_read < (int)sizeof(PacketHeader)) continue;
        
        MemoryArena buffer_in_arena;
        InitializeArena(&buffer_in_arena, buffer_in, sizeof(buffer_in));
        
        PacketHeader *header_in = PUSH_STRUCT(&buffer_in_arena, PacketHeader);
        
        switch(header_in->type)
        {
            case SNAPSHOT:
            {
                Snapshot *remote_snapshot = PUSH_STRUCT(&buffer_in_arena, Snapshot);
                Snapshot *local_snapshot = 0;
                
                for(unsigned int snapshot_idx = 0;
                    snapshot_idx < game_data->snapshot_count;
                    ++snapshot_idx)
                {
                    Snapshot *s = game_data->snapshots + snapshot_idx;
                    if(s->idx == remote_snapshot->idx)
                    {
                        local_snapshot = s;
                        break;
                    }
                }
                
                if(local_snapshot != 0)
                {
                    if(SequenceIsNewer(remote_snapshot->sequence,
                                       local_snapshot->sequence))
                    {
                        MEMORY_COPY(local_snapshot, remote_snapshot, sizeof(Snapshot));
                        local_snapshot->time_since_last_update = 0.0f;
                    }
                }
                else
                {
                    if(game_data->snapshot_count < ARRAY_SIZE(game_data->snapshots))
                    {
                        local_snapshot = game_data->snapshots + game_data->snapshot_count;
                        game_data->snapshot_count += 1;
                        MEMORY_COPY(local_snapshot, remote_snapshot, sizeof(Snapshot));
                        local_snapshot->time_since_last_update = 0.0f;
                    }
                }
            } break;
            
            default: break;
        }
    }
    
    // GAME UPDATE
    PlayerUpdate(game_data, player, input, physics_spec, input->dt);
    CameraUpdate(camera_p, &player->p, physics_spec->meters_to_units, input->dt);
    
    // SEND PACKETS
    {
        unsigned char buffer_out[sizeof(PacketHeader) + sizeof(Snapshot)];
        MemoryArena buffer_out_arena;
        InitializeArena(&buffer_out_arena, buffer_out, sizeof(buffer_out));
        PacketHeader *header_out = PUSH_STRUCT(&buffer_out_arena, PacketHeader);
        header_out->protocol = 0;
        header_out->type = SNAPSHOT;
        Snapshot *local_snapshot = PUSH_STRUCT(&buffer_out_arena, Snapshot);
        local_snapshot->sequence = game_data->frame_idx;
        MEMORY_COPY(&local_snapshot->entity, player, sizeof(Entity));
        
        if(!platform->socket_send(game_data->socketfd, &game_data->server_address,
                                  buffer_out_arena.base, buffer_out_arena.used))
        {
            INVALID_CODE_PATH;
        }
    }
    
    // RENDER
    {
        for(unsigned int obstacle_idx = 0;
            obstacle_idx < game_data->obstacle_count;
            ++obstacle_idx)
        {
            Recti *obstacle = game_data->obstacles + obstacle_idx;
            ObstacleRender(memory, platform, input->screen_size, obstacle,
                           *camera_p, physics_spec->meters_to_units, units_to_pixels);
        }
        
        
        for(unsigned int snapshot_idx = 0;
            snapshot_idx < game_data->snapshot_count;
            ++snapshot_idx)
        {
            Snapshot *local_snapshot = game_data->snapshots + snapshot_idx;
            PlayerRender(platform, input->screen_size, &local_snapshot->entity,
                         *camera_p, physics_spec->meters_to_units, units_to_pixels);
        }
        
        if(player->spell_timer > 0.0f)
        {
            Recti texture_rect = RectiMove(player->spell_texture_rect,
                                           player->spell_p.unit.x,
                                           player->spell_p.unit.y);
            
            Vec2 screen_spell_p0 = ProjectGlobalToScreen(texture_rect.p0, *camera_p,
                                                         input->screen_size, units_to_pixels);
            Vec2 screen_spell_p1 = ProjectGlobalToScreen(texture_rect.p1, *camera_p,
                                                         input->screen_size, units_to_pixels);
            
            platform->blit_texture_tile_coord(TextureID_Atlas, 11, 0,
                                              screen_spell_p0.x, screen_spell_p0.y,
                                              screen_spell_p1.x, screen_spell_p1.y,
                                              player->spell_direction < 0);
        }
        
        PlayerRender(platform, input->screen_size, player, *camera_p,
                     physics_spec->meters_to_units, units_to_pixels);
    }
}

void GameUpdateAndRender(GameMemory *memory,
                         Platform *platform,
                         Input *input,
                         bool *is_running)
{
    GameData *game_data = (GameData *)memory->permanent_storage;
    
    if(memory->is_initialized == false)
    {
        InitMemory(memory, platform, input, is_running);
        memory->is_initialized = true;
    }
    
    if(game_data->connected == false)
        DoConnect(game_data, platform);
    
    if(game_data->connected == true)
        DoUpdateAndRender(memory, platform, input, is_running);
    
    ++game_data->frame_idx;
}