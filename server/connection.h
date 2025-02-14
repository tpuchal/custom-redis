#ifndef CONNECTION_HEADER
#define CONNECTION_HEADER
#include "buffer.h"

typedef struct
{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;
    Buffer incoming_buffer;
    Buffer outgoing_buffer;
} Connection;

Connection initConnection();

void freeConnection(Connection *connection);

Connection emptyConnection();

#endif