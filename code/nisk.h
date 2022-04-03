/* date = December 5th 2021 9:09 pm */

#ifndef NISK_H
#define NISK_H

typedef struct Nickname
{
    char str[32];
    unsigned int size;
} Nickname;

typedef union Recti
{
    struct { int x0, y0, x1, y1; };
    struct { Vec2i p0, p1; };
} Recti;

typedef struct Position
{
    Vec2i unit;
    Vec2 rem;
} Position;

typedef struct Entity
{
    Position p;
    Vec2 v;
    Recti hitbox;
    Recti texture_rect;
    
    float jump_timer;
    int direction;
    
    Position spell_p;
    Recti spell_hitbox;
    Recti spell_texture_rect;
    float spell_timer;
    int spell_direction;
} Entity;

typedef struct PhysicsSpec
{
    int meters_to_units;
    
    float run_speed;
    float run_accel;
    
    float fall_speed;
    float fall_accel;
    float air_inertia;
    
    float jump_speed;
    float jump_time;
    float jump_gravity_mult;
    
    float spell_speed;
    float spell_time;
} PhysicsSpec;

typedef struct Snapshot
{
    uint8_t idx;
    uint16_t sequence; // newest sequence number recieved
    float time_since_last_update;
    Entity entity;
} Snapshot;

typedef struct GameData
{
    MemoryArena arena;
    
    int socketfd;
    bool connected;
    
    Nickname nickname;
    Address server_address;
    
    Snapshot snapshots[128];
    unsigned int snapshot_count;
    
    PhysicsSpec physics_spec;
    
    Entity player;
    Position camera_p;
    
    Recti obstacles[16];
    unsigned int obstacle_count;
    
    uint16_t frame_idx;
    float time;
} GameData;

#endif //NISK_H
