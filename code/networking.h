/* date = November 26th 2021 19:23 pm */

#ifndef NETWORKING_H
#define NETWORKING_H

#define EXPAND_INT(v) (v>>24),(v>>16&0xff),(v>>8&&0xff),(v&&0xff)

typedef enum PacketType
{
    INVALID,
    CONNECT,
    ACCEPT,
    DISCONNECT,
    SNAPSHOT
} PacketType;

char *PacketTypeName(PacketType packet_type)
{
    switch(packet_type)
    {
        case INVALID:    return "INVALID";
        case CONNECT:    return "CONNECT";
        case ACCEPT:     return "ACCEPT";
        case DISCONNECT: return "DISCONNECT";
        case SNAPSHOT:   return "SNAPSHOT";
        default:         return "UNKNOWN";
    }
}

typedef struct PacketHeader
{
    unsigned int protocol;
    PacketType type;
} PacketHeader;

typedef struct Address
{
    unsigned int address;
    unsigned short port;
} Address;

Address AddressFromInt(unsigned int address, uint16_t port)
{
    Address result = { address, port };
    return result;
}

Address AddressFromComponents(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port)
{
    unsigned int address = (unsigned int)(( a << 24 ) |
                                          ( b << 16 ) |
                                          ( c << 8  ) |
                                          d); 
    Address result = AddressFromInt(address, port);
    return result;
}

bool AddressCompare(Address a, Address b)
{
    bool result = (a.address == b.address
                   && a.port == b.port);
    return result;
}

bool SequenceIsNewer(uint16_t seq_new, uint16_t seq_old)
{
    // max of uint16_t is 65535, if the difference between sequences
    // is greater than half of it (32767.5) we have probably wrapped around
    bool normal = ((seq_new - seq_old <= 32767) && (seq_new > seq_old));
    bool wrapped = ((seq_old - seq_new > 32767) && (seq_new < seq_old));
    bool result = (normal || wrapped);
    return result;
}

bool SocketsInit();
void SocketsShutdown();
bool SocketCreate(int *sockfd);
bool SocketBind(int sockfd, unsigned short port);
bool SocketClose(int sockfd);
bool SocketSend(int sockfd, Address *destination, void *data, int size);
int SocketRecieve(int sockfd, Address *sender, void *data, int size);

#endif // NETWORKING_H
