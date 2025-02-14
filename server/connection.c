#include "connection.h"

Connection initConnection()
{
    Connection conn;
    conn.fd = -1;
    conn.want_read = false;
    conn.want_write = false;
    conn.want_close = false;

    return conn;
}

void freeConnection(Connection *connection)
{
    connection->fd = 0;

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