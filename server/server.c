#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
// system
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 4

const size_t k_max_msg = 32 << 20;

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

static void msg_errno(const char *msg)
{
    fprintf(stderr, "[errno:%d] %s\n", errno, msg);
}

static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void fd_set_nonblocking(int fd)
{
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno)
    {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno)
    {
        die("fcntl error");
    }
}

// vector handling buffers

struct uint8TVector
{
    size_t size;
    size_t capacity;
    u_int8_t *array;
};

void freeVector(struct uint8TVector *vec)
{
    free(vec->array);
    vec->array = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

void initVector(struct uint8TVector *vector)
{
    vector->size = 0;
    vector->capacity = INITIAL_CAPACITY;
    vector->array = (uint8_t *)malloc(vector->capacity * sizeof(uint8_t));
    if (!vector->array)
    {
        die("Memory allocation failer \n");
        return;
    }
}

void resizeVector(struct uint8TVector *vector)
{
    size_t old_capacity = vector->capacity;
    uint8_t *new_array = (uint8_t *)realloc(vector->array, old_capacity * 2 * sizeof(uint8_t));
    if (!new_array)
    {
        die("Memory reallocation failed\n");
        return;
    }
}

void appendToVector(struct uint8TVector *vector, uint8_t value)
{
    if (vector->size >= vector->capacity)
    {
        resizeVector(vector);
    }
    vector->array[vector->size++] = value;
}

void eraseFromVector(struct uint8TVector *vector, size_t index)
{
    if (index >= vector->size || index < 0)
    {
        die("failed to erase from vector\n");
        return;
    }
    memmove(&vector->array[index], &vector->array[index + 1], (vector->size - index - 1) * sizeof(uint8_t));
    vector->size--;
}

// vector handling pollfds

typedef struct pollfd pollfd;

struct pollFdVector
{
    size_t size;
    size_t capacity;
    pollfd *array;
};

void freepollFdVector(struct pollFdVector *vector)
{
    free(vector->array);
    vector->array = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

void initPollFdVector(struct pollFdVector *vector)
{
    vector->size = 0;
    vector->capacity = INITIAL_CAPACITY;
    vector->array = (pollfd *)malloc(vector->capacity * sizeof(pollfd));
}

void resizePollFdVector(struct pollFdVector *vector)
{
    size_t old_capacity = vector->capacity;
    pollfd *new_array = (pollfd *)realloc(vector->array, old_capacity * 2 * sizeof(pollfd));
    if (!new_array)
    {
        die("Memory reallocation failed\n");
        return;
    }
}

void pollFdVectorClear(struct pollFdVector *vector)
{
    for (int i = 0; i < vector->size; i++)
    {
        vector->array[i] = (pollfd){};
    }
}

void pollVectorPushBack(struct pollFdVector *vector, pollfd value)
{
    if (vector->size >= vector->capacity)
    {
        resizePollFdVector(vector);
    }
    vector->array[vector->size++] = value;
}

struct Connection
{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;
    struct uint8TVector incoming;
    struct uint8TVector outgoing;
};

void freeConnection(struct Connection *connection)
{
    connection->fd = 0;
    freeVector(&connection->incoming);
    freeVector(&connection->outgoing);
}

void initConnection(struct Connection *conn)
{
    conn->fd = -1;
    conn->want_read = false;
    conn->want_write = false;
    conn->want_close = false;
}

struct ConnectionVector
{
    size_t size;
    size_t capacity;
    struct Connection *array;
};

void resizeConnectionVector(struct ConnectionVector *vector, int newSize)
{
    size_t old_capacity = vector->capacity;
    struct Connection *new_array = (struct Connection *)realloc(vector->array, old_capacity * 2 * sizeof(struct Connection));
    if (!new_array)
    {
        die("Memory reallocation failed\n");
        return;
    }
}

void freeConnectionVector(struct ConnectionVector *vector)
{
    free(vector->array);
    vector->array = NULL;
    vector->size = 0;
    vector->capacity = 0;
}
void initConnectionVector(struct ConnectionVector *vector)
{
    vector->size = 0;
    vector->capacity = INITIAL_CAPACITY;
    vector->array = (struct Connection *)malloc(vector->capacity * sizeof(struct Connection));
}

static void appendBuffer(struct uint8TVector *buffer, const uint8_t *data, size_t len)
{
    if (len > buffer->capacity - buffer->size)
    {
        for (size_t i = 0; i < len; i++)
        {
            appendToVector(buffer, data[i]);
        }
    }
}

static void consumeBuffer(struct uint8TVector *buffer, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        eraseFromVector(buffer, 0);
    }
}

static struct Connection *handleAccept(int fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);

    if (connfd < 0)
    {
        msg_errno("accept() error");
        return NULL;
    }

    uint32_t ip = client_addr.sin_addr.s_addr;

    fprintf(stderr, "new client from %u.%u.%u.%u:%u\n",
            ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
            ntohs(client_addr.sin_port));

    fd_set_nonblocking(connfd);

    struct Connection *conn = NULL;
    initConnection(conn);
    conn->fd = connfd;
    conn->want_read = true;
    return conn;
}

static bool try_one_request(struct Connection *conn)
{
    if (conn->incoming.size < 4)
    {
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, conn->incoming.array, 4);
    if (len > k_max_msg)
    {
        msg("too long");
        conn->want_close = true;
        return false;
    }

    if (4 + len > conn->incoming.size)
    {
        return false;
    }

    const uint8_t *request = &conn->incoming.array[4];

    printf("client says: len:%d data:%.*s\n",
           len, len < 100 ? len : 100, request);

    appendBuffer(&conn->outgoing, (const uint8_t *)&len, 4);
    appendBuffer(&conn->outgoing, request, len);

    consumeBuffer(&conn->incoming, 4 + len);

    return true;
}

static void handleWrite(struct Connection *conn)
{
    assert(conn->outgoing.size > 0);
    ssize_t rv = write(conn->fd, &conn->outgoing.array[0], conn->outgoing.size);
    if (rv < 0 && errno == EAGAIN)
    {
        return;
    }
    if (rv < 0)
    {
        msg_errno("write() error");
        conn->want_close = true;
        return;
    }

    consumeBuffer(&conn->outgoing, (size_t)rv);

    if (conn->outgoing.size == 0)
    {
        conn->want_read = true;
        conn->want_write = false;
    }

    conn->want_write = true;
}

static void handleRead(struct Connection *conn)
{
    uint8_t buffer[64 * 1024];
    ssize_t rv = read(conn->fd, buffer, sizeof(buffer));
    if (rv < 0 && errno == EAGAIN)
    {
        return;
    }
    if (rv < 0)
    {
        msg_errno("read() error");
        conn->want_close = true;
        return;
    }
    if (rv == 0)
    {
        if (conn->incoming.size == 0)
        {
            msg("client closed");
        }
        else
        {
            msg("unexpected EOF");
        }
        conn->want_close = true;
        return;
    }

    appendBuffer(&conn->incoming, buffer, (size_t)rv);
    while (try_one_request(conn))
    {
    };

    if (conn->outgoing.size > 0)
    {
        conn->want_read = false;
        conn->want_write = true;
        return handleWrite(conn);
    }
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        die("socket()");
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
    {
        die("bind()");
    }

    // set non blocking and listen
    fd_set_nonblocking(fd);
    rv = listen(fd, SOMAXCONN);
    if (rv)
    {
        die("listen()");
    }

    struct ConnectionVector fd2conn;
    struct pollFdVector poll_args;
    initPollFdVector(&poll_args);
    initConnectionVector(&fd2conn);

    while (true)
    {
        pollFdVectorClear(&poll_args);
        pollfd pfd = {fd, POLLIN, 0};
        pollVectorPushBack(&poll_args, pfd);

        // add ConnectionVector logic
        for (int i = 0; i < fd2conn.size; i++)
        {
            struct Connection *conn = &fd2conn.array[i];
            if (!conn)
            {
                continue;
            }
            pollfd pfd = {conn->fd, POLL_ERR, 0};

            if (conn->want_read)
            {
                pfd.events |= POLLIN;
            }
            if (conn->want_write)
            {
                pfd.events |= POLLOUT;
            }
            pollVectorPushBack(&poll_args, pfd);
        }

        int rv = poll(poll_args.array, (nfds_t)poll_args.size, -1);
        if (rv < 0 && errno == EINTR)
        {
            continue;
        }
        if (rv < 0)
        {
            die("poll");
        }
        if (poll_args.array[0].revents)
        {
            struct Connection *conn = handleAccept(fd);
            if (conn)
            {
                if (fd2conn.size <= (size_t)conn->fd)
                {
                    resizeConnectionVector(&fd2conn, conn->fd + 1);
                }
                fd2conn.array[conn->fd] = *conn;
            }
        }

        for (size_t i = 1; i < poll_args.size; i++)
        {
            uint32_t ready = poll_args.array[i].revents;
            if (ready == 0)
            {
                continue;
            }
            struct Connection *conn = &fd2conn.array[poll_args.array[i].fd];
            if (ready & POLLIN)
            {
                bool isRead = conn->want_read;
                assert(isRead);
                handleRead(conn);
            }
            if (ready & POLLOUT)
            {
                bool isWrite = conn->want_write;
                assert(isWrite);
                handleWrite(conn);
            }
            bool isClose = conn->want_close;
            if ((ready & POLLERR) || isClose)
            {
                (void)close(conn->fd);
                static const struct Connection nullConnection;
                fd2conn.array[conn->fd] = nullConnection;
                freeConnection(conn);
            }
        }
    }

    freeConnectionVector(&fd2conn);
    freepollFdVector(&poll_args);
    return 0;
}