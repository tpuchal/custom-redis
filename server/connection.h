#ifndef CONNECTION_HEADER
#define CONNECTION_HEADER
#include "uint8tvector.h"

typedef struct
{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;
    uint8TVector incoming;
    uint8TVector outgoing;
} Connection;

Connection initConnection();

void freeConnection(Connection *connection);

Connection emptyConnection();

#endif