#include "connection.h"
#include <unistd.h>

Connection initConnection()
{
    Connection conn;
    conn.fd = -1;
    conn.want_read = false;
    conn.want_write = false;
    conn.want_close = false;

    return conn;
}

void freeConnection(Connection *conn)
{
    if (conn->fd >= 0)
    {
        close(conn->fd);
        freeBuffer(&conn->incoming_buffer);
        freeBuffer(&conn->outgoing_buffer);
        conn->fd = -1;
        conn->want_read = false;
        conn->want_write = false;
        conn->want_close = false;
    }
}

Connection emptyConnection()
{
    Connection retConn;
    retConn.fd = -1;
    retConn.want_close = false;
    retConn.want_read = false;
    retConn.want_write = false;

    return retConn;
}