#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include "buffer.h"
#include "connection.h"
#include "pollfdvector.h"
#include "connectionvector.h"
#include <sys/time.h>

#define INITIAL_CAPACITY 4

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
    fprintf(stderr, "[%d] %s\n", errno, msg);
    abort();
}

static void fd_set_nb(int fd)
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

const size_t k_max_msg = 32 << 20;

static Connection *handle_accept(int fd)
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

    fd_set_nb(connfd);

    Connection *conn = malloc(sizeof(Connection));
    conn->fd = connfd;
    conn->want_read = true;

    initBuffer(&conn->incoming_buffer);
    initBuffer(&conn->outgoing_buffer);
    return conn;
}

static bool try_one_request(Connection *conn)
{
    size_t incomming_buffer_size = conn->incoming_buffer.data_end - conn->incoming_buffer.data_begin;
    if (incomming_buffer_size < 4)
    {
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, conn->incoming_buffer.data_begin, 4);
    if (len > k_max_msg)
    {
        msg("too long");
        conn->want_close = true;
        return false;
    }
    if (4 + len > incomming_buffer_size)
    {
        return false;
    }
    const uint8_t *request = &conn->incoming_buffer.data_begin[4];

    printf("client says: len:%d data:%.*s\n",
           len, len < 100 ? len : 100, request);

    appendToNewBuffer(&conn->outgoing_buffer, (const uint8_t *)&len, 4);
    appendToNewBuffer(&conn->outgoing_buffer, request, len);

    consumeNewBuffer(&conn->incoming_buffer, 4 + len);
    return true;
}

static void handle_write(Connection *conn)
{
    size_t outgoing_buffer_size = conn->outgoing_buffer.data_end - conn->outgoing_buffer.data_begin;
    if (outgoing_buffer_size == 0)
    {
        conn->want_read = true;
        conn->want_write = false;
        return;
    }
    ssize_t rv = write(conn->fd, conn->outgoing_buffer.data_begin, outgoing_buffer_size);
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

    consumeNewBuffer(&conn->outgoing_buffer, (size_t)rv);

    if (outgoing_buffer_size == 0)
    {
        conn->want_read = true;
        conn->want_write = false;
    }
}

static void handle_read(Connection *conn)
{
    struct timeval start, end;
    gettimeofday(&start, NULL);
    uint8_t buf[64 * 1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    size_t incomming_buffer_size = conn->incoming_buffer.data_end - conn->incoming_buffer.data_begin;
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
        if (incomming_buffer_size == 0)
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

    appendToNewBuffer(&conn->incoming_buffer, buf, (size_t)rv);

    while (try_one_request(conn))
    {
    }

    size_t outgoing_buffer_size = conn->outgoing_buffer.data_end - conn->outgoing_buffer.data_begin;
    if (outgoing_buffer_size > 0)
    {
        conn->want_read = false;
        conn->want_write = true;
        gettimeofday(&end, NULL);
        double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) / 1e6;


        printf("Request processed in %.6f seconds\n", time_taken);
        return handle_write(conn);
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

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
    {
        die("bind()");
    }

    fd_set_nb(fd);

    rv = listen(fd, SOMAXCONN);
    if (rv)
    {
        die("listen()");
    }

    ConnectionVector fd2conn = initConnectionVector();
    pollFdVector poll_args;
    initPollFdVector(&poll_args);
    while (true)
    {
        clearPollFdVector(&poll_args);
        struct pollfd pfd = {fd, POLLIN, 0};
        pollVectorPushBack(&poll_args, pfd);
        size_t size = fd2conn.size;
        for (int i = 0; i < size; i++)
        {
            Connection *conn = &fd2conn.array[i];
            if (!conn)
            {
                continue;
            }
            struct pollfd pfd = {conn->fd, POLLERR, 0};
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
            Connection *conn = handle_accept(fd);

            if (conn)
            {
                if (fd2conn.size <= (size_t)conn->fd)
                {
                    resizeConnectionVector(&fd2conn, conn->fd + 1, 0);
                }
                assert(&fd2conn.array[conn->fd] != NULL);

                fd2conn.array[conn->fd] = *conn;
            }
        }

        for (size_t i = 1; i < poll_args.size; ++i)
        {
            uint32_t ready = poll_args.array[i].revents;
            if (ready == 0)
            {
                continue;
            }

            Connection *conn = &fd2conn.array[poll_args.array[i].fd];
            if (ready & POLLIN)
            {
                assert(conn->want_read);
                handle_read(conn);
            }
            if (ready & POLLOUT)
            {
                assert(conn->want_write);
                handle_write(conn);
            }

            if ((ready & POLLERR) || conn->want_close)
            {
                (void)close(conn->fd);
                fd2conn.array[conn->fd] = emptyConnection();
                freeConnection(conn);
            }
        }
    }
    return 0;
}