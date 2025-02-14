#include "connection.h"

Connection initConnection()
{
    Connection conn;
    conn.fd = -1;
    conn.want_read = false;
    conn.want_write = false;
    conn.want_close = false;
    initVector(&conn.incoming);
    initVector(&conn.outgoing);

    return conn;
}

void freeConnection(Connection *connection)
{
    connection->fd = 0;
    freeVector(&connection->incoming);
    freeVector(&connection->outgoing);
}

Connection emptyConnection()
{
    Connection retConn;
    retConn.fd = -1;
    retConn.want_close = false;
    retConn.want_read = false;
    retConn.want_write = false;
    initVector(&retConn.incoming);
    initVector(&retConn.outgoing);

    return retConn;
}