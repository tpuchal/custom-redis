#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include<stdbool.h>
#include <assert.h>

const size_t k_max_msg = 4096;

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void do_something(int connfd) {
    char readbuf[64] = {};
    ssize_t n = read(connfd, readbuf, sizeof(readbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    fprintf(stderr, "client says: %s\n", readbuf);

    char write_buffer[] = "world";
    write(connfd, write_buffer, strlen(write_buffer));
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const int32_t one_request(int connfd) {
    char read_buffer[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, read_buffer, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t length = 0;
    memcpy(&length, read_buffer, 4);  // assume little endian
    if (length > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &read_buffer[4], length);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    fprintf(stderr, "client says: %.*s\n", length, &read_buffer[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char write_buffer[4 + sizeof(reply)];
    length = (uint32_t)strlen(reply);
    memcpy(write_buffer, &length, 4);
    memcpy(&write_buffer[4], reply, length);
    return write_all(connfd, write_buffer, 4 + length);
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        while (true) {
            // here the server only serves one client connection at once
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }
        close(connfd);
    }

    return 0;
}