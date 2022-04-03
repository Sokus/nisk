#include "base.h"
#include "nisk_math.h"

#include "networking.h"
#include "nisk.h"


#include <netinet/in.h>   // sockaddr_in
#include <fcntl.h>        // set file/socket handle to non-blocking
#include <unistd.h>       // write(), close()
#include <errno.h>        // socket error handling
#include <time.h>         // nanosleep()

#include <stdio.h>

#include "linux_networking.c"

typedef struct Client
{
    bool connected;
    Nickname nickname;
    Address address;
    float time_since_last_packet;
    Snapshot snapshot;
} Client;

typedef struct ServerState
{
    Client clients[128];
} ServerState;

Client *GetClient(ServerState *state, Address address)
{
    for(long unsigned offset=0; offset < ARRAY_SIZE(state->clients); ++offset)
    {
        int idx = (address.address + offset) % ARRAY_SIZE(state->clients);
        Client *client = state->clients + idx;
        if(client->connected)
        {
            if(AddressCompare(client->address, address))
                return client;
        }
        else
        {
            return client;
        }
    }
    
    return 0;
}

typedef struct timespec timespec;

typedef enum TimerEntry
{
    TimerEntry_Cycle,
    TimerEntry_Work,
    TimerEntry_Recieve,
    TimerEntry_Send,
    TimerEntry_COUNT
} TimerEntry;

char *TimerEntryName(TimerEntry timer_entry)
{
    switch(timer_entry)
    {
        case TimerEntry_Cycle:   return "Cycle";
        case TimerEntry_Work:    return "Work";
        case TimerEntry_Recieve: return "Recieve";
        case TimerEntry_Send:    return "Send";
        default:                 return "Unknown";
    }
}

typedef struct Timer
{
    timespec start;
    timespec end;
    float ms;
    unsigned int hit_count;
} Timer;

Timer global_timers[TimerEntry_COUNT];

#define RESET_TIMERS STATEMENT(for(unsigned int _timer_idx=0; _timer_idx < TimerEntry_COUNT; _timer_idx++)\
{ global_timers[_timer_idx].hit_count = 0; })

#define BEGIN_TIMER(ID) STATEMENT(clock_gettime(CLOCK_MONOTONIC, &global_timers[ID].start);)
#define END_TIMER(ID) STATEMENT(clock_gettime(CLOCK_MONOTONIC, &global_timers[ID].end);\
++global_timers[ID].hit_count;\
global_timers[ID].ms = MilisecondsElapsed(global_timers[ID].start,\
global_timers[ID].end);)

float MilisecondsElapsed(timespec start, timespec end)
{
    float result = (1000.0f*(float)(end.tv_sec - start.tv_sec)
                    + (float)(end.tv_nsec - start.tv_nsec)/1000000.0f);
    
    return result;
}

timespec TimespecFromMiliseconds(float ms)
{
    timespec result = {0};
    if(ms > 1000)
    {
        float seconds = ms / 1000.0f;
        ms -= (seconds * 1000.0f);
        result.tv_sec = (long int)seconds;
    }
    
    result.tv_nsec = (long int)(ms * 1000000.0f);
    
    return result;
}

int main(void)
{
    int socket;
    if(!SocketCreate(&socket))
        return -1;
    
    if(!SocketBind(socket, 54321))
        return -1;
    
    int server_update_hz = 60;
    float target_ms_per_frame = 1000.0f/(float)server_update_hz;
    // float dt = target_ms_per_frame/1000.0f;
    
    ServerState state = {0};
    
    while(true)
    {
        RESET_TIMERS;
        BEGIN_TIMER(TimerEntry_Cycle);
        BEGIN_TIMER(TimerEntry_Work);
        
        // Read all incoming packets
        BEGIN_TIMER(TimerEntry_Recieve);
        unsigned char buffer_in[2048];
        while(true)
        {
            
            Address sender;
            int bytes_read = SocketRecieve(socket, &sender, buffer_in, sizeof(buffer_in));
            
            if(bytes_read <= 0) break;
            if(bytes_read < (int)sizeof(PacketHeader)) continue;
            
            MemoryArena buffer_in_arena;
            InitializeArena(&buffer_in_arena, buffer_in, sizeof(buffer_in));
            
            PacketHeader *header_in = PUSH_STRUCT(&buffer_in_arena, PacketHeader);
#if 0
            printf("%d.%d.%d.%d:%d  prot: %d  type: %s\n",
                   EXPAND_INT(sender.address), sender.port,
                   header_in->protocol,
                   PacketTypeName(header_in->type));
#endif
            switch(header_in->type)
            {
                case CONNECT:
                {
                    Client *remote_client = GetClient(&state, sender);
                    if(remote_client)
                    {
                        if(remote_client->connected != true)
                        {
                            Nickname *remote_nickname = PUSH_STRUCT(&buffer_in_arena, Nickname);
                            if(remote_nickname != 0)
                            {
                                unsigned int max_nickname_size = ARRAY_SIZE(remote_nickname->str) - 1;
                                if(remote_nickname->size > max_nickname_size)
                                {
                                    remote_nickname->size = max_nickname_size;
                                    remote_nickname->str[max_nickname_size] = 0;
                                }
                                
                                
                                bool nickname_taken = false;
                                for(unsigned int idx = 0; idx < ARRAY_SIZE(state.clients); ++idx)
                                {
                                    Client *local_client = state.clients + idx;
                                    if(remote_client != local_client
                                       && local_client->connected)
                                    {
                                        if(StringCompare(local_client->nickname.str, local_client->nickname.size,
                                                         remote_nickname->str, remote_nickname->size))
                                        {
                                            nickname_taken = true;
                                            break;
                                        }
                                    }
                                }
                                
                                if(nickname_taken == false)
                                {
                                    remote_client->connected = true;
                                    remote_client->nickname = *remote_nickname;
                                    remote_client->address = sender;
                                    
                                    printf("%s %d.%d.%d.%d:%d connected.\n",
                                           remote_nickname->str,
                                           EXPAND_INT(sender.address),
                                           sender.port);
                                    
                                    remote_client->time_since_last_packet = 0.0f;
                                    PacketHeader header = { 0, ACCEPT };
                                    SocketSend(socket, &sender, &header, sizeof(PacketHeader));
                                }
                                else
                                {
                                    fprintf(stderr, "Nickname taken: %s\n", remote_nickname->str);
                                    // nickname taken
                                }
                            }
                        }
                    }
                    else
                    {
                        // server is full
                        PacketHeader header = {0, DISCONNECT };
                        SocketSend(socket, &sender, &header, sizeof(PacketHeader));
                    }
                } break;
                
                case DISCONNECT:
                { 
                    
                } break;
                
                case SNAPSHOT:
                {
                    Client *remote_client = GetClient(&state, sender);
                    if(remote_client->connected)
                    {
                        Snapshot *remote_snapshot = PUSH_STRUCT(&buffer_in_arena, Snapshot);
                        if(SequenceIsNewer(remote_snapshot->sequence, remote_client->snapshot.sequence))
                            MEMORY_COPY(&remote_client->snapshot, remote_snapshot, sizeof(Snapshot));
                    }
                } break;
                
                default:
                {
                    fprintf(stderr, "[ERROR] Invalid packet type!\n");
                } break;
            }
        }
        END_TIMER(TimerEntry_Recieve);
        
        
        // Deliever position info to all clients
        BEGIN_TIMER(TimerEntry_Send);
        for(long unsigned dst_idx=0; dst_idx < ARRAY_SIZE(state.clients); ++dst_idx)
        {
            Client *dst_client = state.clients + dst_idx;
            if(dst_client->connected)
            {
#if 0
                if(dst_client->time_since_last_packet > global_timeout_treshold)
                {
                    printf("%d.%d.%d.%d:%d timed out.\n",
                           EXPAND_INT(dst_client->address.address),
                           dst_client->address.port);
                    dst_client->connected = false;
                    continue;
                }
                dst_client->time_since_last_packet += dt;
#endif
                for(long unsigned src_idx=0; src_idx < ARRAY_SIZE(state.clients); ++src_idx)
                {
                    if(dst_idx == src_idx)
                        continue;
                    
                    Client *src_client = state.clients + src_idx;
                    if(src_client->connected)
                    {
                        uint8_t buffer_out[sizeof(PacketHeader) + sizeof(Snapshot)];
                        uint8_t *buffer_out_ptr = buffer_out;
                        
                        PacketHeader *header = (PacketHeader *)buffer_out_ptr;
                        header->protocol = 0;
                        header->type = SNAPSHOT;
                        buffer_out_ptr += sizeof(PacketHeader);
                        
                        Snapshot *snapshot = (Snapshot *)buffer_out_ptr;
                        MEMORY_COPY(snapshot, &src_client->snapshot, sizeof(Snapshot));
                        snapshot->idx = (uint8_t)src_idx;
                        snapshot->sequence = dst_client->snapshot.sequence;
                        
                        SocketSend(socket, &dst_client->address, buffer_out, sizeof(buffer_out));
                    }
                }
            }
        }
        END_TIMER(TimerEntry_Send);
        END_TIMER(TimerEntry_Work);
        
        float work_in_ms = global_timers[TimerEntry_Work].ms;
        if(work_in_ms < target_ms_per_frame)
        {
            timespec sleep_time = TimespecFromMiliseconds(target_ms_per_frame - work_in_ms);
            nanosleep(&sleep_time, 0);
        }
        
        END_TIMER(TimerEntry_Cycle);
    }
    
    return 0;
}
