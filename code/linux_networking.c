bool SocketsInit() { return true; }
void SocketsShutdown() { return; }

bool SocketCreate(int *sockfd)
{
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(handle <= 0)
    {
        fprintf(stderr, "[ERROR] Create socket\n");
        INVALID_CODE_PATH;
        return false;
    }
    
    bool non_blocking = true;
    int fcntl_result = fcntl(handle,
                             F_SETFL,
                             O_NONBLOCK,
                             non_blocking);
    if(fcntl_result == -1)
    {
        fprintf(stderr, "[ERROR] Set to non-blocking\n");
        INVALID_CODE_PATH;
        return false;
    }
    
    *sockfd = handle;
    return true;
}

bool SocketBind(int sockfd, unsigned short port)
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    int bind_result = bind(sockfd,
                           (struct sockaddr *)&address,
                           sizeof(address));
    if(bind_result < 0)
    {
        fprintf(stderr, "[ERROR] Bind socket\n");
        INVALID_CODE_PATH;
        return false;
    }
    
    return true;
}

bool SocketClose(int sockfd)
{
    int rc = close(sockfd);
    bool success = (rc == 0);
    return success;
}

bool SocketSend(int sockfd, Address *destination, void *data, int size)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(destination->address);
    addr.sin_port = htons(destination->port);
    
    long int bytes_sent = sendto(sockfd,
                                 (char *)data,
                                 (size_t)size,
                                 0,
                                 (struct sockaddr *)&addr,
                                 sizeof(addr));
    
    if(bytes_sent != size)
    {
        fprintf(stderr, "[ERROR] Failed to send packet\n");
        return false;
    }
    
    return true;
}

int SocketRecieve(int sockfd, Address *sender, void *data, int size)
{
    struct sockaddr_in from;
    socklen_t from_length = sizeof(from);
    
    int bytes_recieved = (int)recvfrom(sockfd,
                                       (char *)data,
                                       (size_t)size,
                                       0,
                                       (struct sockaddr *)&from,
                                       &from_length);
    
    if(bytes_recieved == -1 && errno != EWOULDBLOCK)
    {
        fprintf(stderr, "[ERROR] SocketRecieve: %s\n", strerror(errno));
        INVALID_CODE_PATH;
    }
    
    if(sender != 0)
    {
        sender->address = ntohl(from.sin_addr.s_addr);
        sender->port = ntohs(from.sin_port);
    }
    
    return bytes_recieved;
}